#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "rts_game_client.h"

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "render_components.h"
#include "camera_system.h"
#include "fog_of_war.h"
#include "fog_system.h"
#include "hud_system.h"
#include "input_system.h"
#include "lava_render.h"
#include "minimap_system.h"
#include "network_messages.h"
#include "projectile_render.h"
#include "render_system.h"
#include "selection_ring.h"
#include "selection_system.h"

namespace Game {

void RtsGameClient::initLogic() {}

void RtsGameClient::initGraphics(const VulkanHelpers::ResourceContext &ctx) {
    window = &ctx.window;

    unitModel        = std::make_shared<VulkanHelpers::Model>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout, "models/goblin.glb");
    terrain          = std::make_shared<VulkanHelpers::Terrain>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    projectileModel  = Systems::createProjectileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    lavaTileModel    = Systems::createLavaTileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    hudResources     = VulkanHelpers::createHudResources(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    selectionRingModel = VulkanHelpers::createSelectionRingModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    menuSystem = std::make_shared<VulkanHelpers::MenuSystem>(
        ctx.window, ctx.instance, ctx.physicalDevice, ctx.device,
        ctx.graphicsQueueFamilyIndex, ctx.graphicsQueue, ctx.swapChain, ctx.depthFormat
    );

    auto camEntity = registry.create();
    registry.emplace<Components::Camera>(
        camEntity, Components::Camera{
                       .position = {0.0f, -12.0f, 14.0f},
                       .target = {0.0f, 0.0f, 0.0f},
                       .fov = 50.0f,
                       .near_ = 0.1f,
                       .far_ = 200.0f,
                   }
    );

    auto terrainEnt = registry.create();
    registry.emplace<Components::Transform>(terrainEnt);
    registry.emplace<Components::RenderMesh>(terrainEnt, Components::RenderMesh{terrain->getModelPtr()});
}

VulkanHelpers::FrameOutput RtsGameClient::update(float dt, vk::Extent2D extent) {
    if (menuSystem)
        menuSystem->beginFrame();

    VulkanHelpers::FrameOutput out;
    bool gameplayActive = !menuSystem || menuSystem->isGameplayStarted();

    if (!gameplayActive) {
        if (menuSystem && menuSystem->drawMainMenu(extent) == VulkanHelpers::MenuAction::Exit)
            closeRequested = true;
        for (auto e : registry.view<Components::Camera>()) {
            const auto &cam = registry.get<Components::Camera>(e);
            out.view = cam.view;
            out.proj = cam.proj;
            break;
        }
        return out;
    }

    // ---- Network poll -------------------------------------------------------
    if (networkManager_) {
        networkManager_->poll(
            [this](const uint8_t *data, std::size_t size, ENetPeer *) {
                if (size == 0) return;
                auto type = static_cast<MessageType>(data[0]);
                if (type == MessageType::Snapshot)           clientApplySnapshot(data, size);
                if (type == MessageType::PlayerAssignment)   clientHandleAssignment(data, size);
                if (type == MessageType::Disconnect)         clientHandleDisconnect(data, size);
                if (type == MessageType::ConnectionRejected) clientHandleConnectionRejected(data, size);
            },
            [this](ENetPeer *) {
                HelloPacket pkt{};
                pkt.msgType = static_cast<uint8_t>(MessageType::Hello);
                pkt.protocolVersion = PROTOCOL_VERSION;
                const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
                networkManager_->sendToServerReliable({raw, raw + sizeof(pkt)});
            },
            nullptr
        );
    }

    ++tick_;

    if (window && (!menuSystem || !menuSystem->wantsKeyboard()))
        Systems::updateCameraInput(registry, *window, dt);
    if (window && (!menuSystem || !menuSystem->wantsMouse()))
        clientCaptureAndSendInput(extent);
    Systems::updateCamera(registry, extent);
    spatialGrid.update(registry);

    // ---- Rendering ---------------------------------------------------------
    const Systems::FogOfWar *fog = fogOfWar ? &*fogOfWar : nullptr;
    if (fogOfWar)
        fogOfWar->update(registry, myFaction_);

    out.draws = Systems::collectDrawCalls(registry, fog);
    Systems::appendLavaDrawCalls(lavaZone, out.draws, *lavaTileModel);
    Systems::appendSelectionRings(registry, out.draws, *selectionRingModel, fog);
    Systems::appendHealthBars(registry, out.draws, hudResources, fog);

    if (menuSystem)
        menuSystem->drawOverlay(dt);
    if (fogOfWar)
        Systems::drawFogOverlay(*fogOfWar, registry, extent);
    if (minimapEnabled)
        Systems::drawMinimap(fog, registry, extent);

    for (auto e : registry.view<Components::Camera>()) {
        const auto &cam = registry.get<Components::Camera>(e);
        out.view = cam.view;
        out.proj = cam.proj;
        break;
    }

    return out;
}

void RtsGameClient::renderImGui(vk::CommandBuffer cmd) {
    if (menuSystem)
        menuSystem->render(cmd);
}

void RtsGameClient::onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) {
    if (menuSystem)
        menuSystem->onSwapChainRecreated(swapChain);
}

bool RtsGameClient::wantsMouse()    const { return menuSystem && menuSystem->wantsMouse(); }
bool RtsGameClient::wantsKeyboard() const { return menuSystem && menuSystem->wantsKeyboard(); }

// ---- Client network handlers (GPU-aware) -----------------------------------

void RtsGameClient::clientApplySnapshot(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    SnapshotHeader hdr{};
    if (!r.read(hdr))
        return;

    lavaZone.setSafeRadius(hdr.lavaRadius);

    std::unordered_map<uint32_t, entt::entity> knownUnits;
    for (auto e : registry.view<Components::NetworkId, Components::Faction>())
        knownUnits[registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenUnits;
    for (uint8_t i = 0; i < hdr.entityCount; ++i) {
        EntitySnapshot es{};
        if (!r.read(es)) break;
        seenUnits.insert(es.netId);

        auto it = knownUnits.find(es.netId);
        if (it == knownUnits.end()) {
            auto faction = static_cast<Components::FactionId>(es.faction);
            auto e = registry.create();
            registry.emplace<Components::NetworkId>(e, Components::NetworkId{es.netId});
            netIdToEntity_[es.netId] = e;
            registry.emplace<Components::Transform>(
                e, Components::Transform{
                       .position = {es.x, es.y, es.z},
                       .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0)),
                       .scale = {1, 1, 1},
                   }
            );
            if (unitModel)
                registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{unitModel});
            registry.emplace<Components::Faction>(e, Components::Faction{faction});
            registry.emplace<Components::Health>(e, Components::Health{es.health, es.maxHealth});
            registry.emplace<Components::Selectable>(e);
            if (es.netId == myNetworkId_) {
                myFaction_ = faction;
                registry.emplace<Components::AlwaysSelected>(e);
                registry.emplace<Components::Selected>(e);
            }
        } else {
            registry.get<Components::Transform>(it->second).position = {es.x, es.y, es.z};
            auto &h = registry.get<Components::Health>(it->second);
            h.current = es.health; h.max = es.maxHealth;
        }
    }
    for (auto &[nid, e] : knownUnits) {
        if (!seenUnits.count(nid) && registry.valid(e)) {
            registry.destroy(e);
            netIdToEntity_.erase(nid);
        }
    }

    std::unordered_map<uint32_t, entt::entity> knownProj;
    for (auto e : registry.view<Components::NetworkId, Components::Projectile>())
        knownProj[registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenProj;
    for (uint8_t i = 0; i < hdr.projectileCount; ++i) {
        ProjectileSnapshot ps{};
        if (!r.read(ps)) break;
        seenProj.insert(ps.netId);

        auto it = knownProj.find(ps.netId);
        if (it == knownProj.end()) {
            auto e = registry.create();
            registry.emplace<Components::NetworkId>(e, Components::NetworkId{ps.netId});
            netIdToEntity_[ps.netId] = e;
            registry.emplace<Components::Transform>(
                e, Components::Transform{
                       .position = {ps.x, ps.y, ps.z},
                       .rotation = glm::quat{1, 0, 0, 0},
                       .scale = {1, 1, 1},
                   }
            );
            if (projectileModel)
                registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{projectileModel});
            registry.emplace<Components::Projectile>(
                e, Components::Projectile{
                       .ownerFaction = static_cast<Components::FactionId>(ps.faction),
                       .velocity = {ps.vx, ps.vy, ps.vz},
                   }
            );
        } else {
            registry.get<Components::Transform>(it->second).position = {ps.x, ps.y, ps.z};
            registry.get<Components::Projectile>(it->second).velocity = {ps.vx, ps.vy, ps.vz};
        }
    }
    for (auto &[nid, e] : knownProj) {
        if (!seenProj.count(nid) && registry.valid(e)) {
            registry.destroy(e);
            netIdToEntity_.erase(nid);
        }
    }
}

void RtsGameClient::clientCaptureAndSendInput(vk::Extent2D extent) {
    if (!networkManager_ || !networkManager_->isConnected())
        return;

    bool rightDown = window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool qDown     = window->isKeyPressed(GLFW_KEY_Q);
    bool eDown     = window->isKeyPressed(GLFW_KEY_E);
    bool rightJust = rightDown && !prevMouseRight_;
    bool qJust     = qDown     && !prevKeyQ_;
    bool eJust     = eDown     && !prevKeyE_;
    prevMouseRight_ = rightDown;
    prevKeyQ_       = qDown;
    prevKeyE_       = eDown;

    if (!rightJust && !qJust && !eJust)
        return;

    const Components::Camera *cam = nullptr;
    for (auto e : registry.view<Components::Camera>()) {
        cam = &registry.get<Components::Camera>(e);
        break;
    }
    if (!cam)
        return;

    auto [mx, my] = window->getMousePosition();
    float ndcX = (2.0f * static_cast<float>(mx)) / static_cast<float>(extent.width)  - 1.0f;
    float ndcY = (2.0f * static_cast<float>(my)) / static_cast<float>(extent.height) - 1.0f;
    glm::mat4 invVP = glm::inverse(cam->proj * cam->view);
    glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farW  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearW /= nearW.w;
    farW  /= farW.w;
    glm::vec3 dir = glm::normalize(glm::vec3(farW) - glm::vec3(nearW));

    if (std::abs(dir.z) < 1e-6f) return;
    float t = -glm::vec3(nearW).z / dir.z;
    if (t < 0.0f) return;
    glm::vec3 ground = glm::vec3(nearW) + t * dir;

    bool      hasMoveOrder  = rightJust;
    bool      fireAbility   = qJust || eJust;
    uint8_t   abilitySlot   = eJust ? 1 : 0;
    glm::vec3 moveTarget    = rightJust   ? ground : glm::vec3{};
    glm::vec3 abilityTarget = fireAbility ? ground : glm::vec3{};

    InputPacket pkt{};
    pkt.msgType     = static_cast<uint8_t>(MessageType::Input);
    pkt.tick        = tick_;
    pkt.flags       = (hasMoveOrder ? InputFlags::MoveOrder : 0) | (fireAbility ? InputFlags::FireAbility : 0);
    pkt.abilitySlot = abilitySlot;
    pkt.moveX = moveTarget.x;    pkt.moveY = moveTarget.y;    pkt.moveZ = moveTarget.z;
    pkt.abilityX = abilityTarget.x; pkt.abilityY = abilityTarget.y; pkt.abilityZ = abilityTarget.z;

    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    networkManager_->sendToServerUnreliable({raw, raw + sizeof(pkt)});
}

} // namespace Game
