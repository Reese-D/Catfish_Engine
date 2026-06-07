#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "rts_game_client.h"

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui.h>

#include "camera_system.h"
#include "fog_of_war.h"
#include "fog_system.h"
#include "hud_system.h"
#include "input_system.h"
#include "lava_render.h"
#include "minimap_system.h"
#include "network_messages.h"
#include "components.h"
#include "projectile_render.h"
#include "render_components.h"
#include "render_system.h"
#include "selection_ring.h"
#include "selection_system.h"

namespace Game {

void RtsGameClient::initLogic() {}

void RtsGameClient::initGraphics(const VulkanHelpers::ResourceContext &ctx) {
    m_window = &ctx.window;

    m_unitModel = std::make_shared<VulkanHelpers::Model>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout, "models/goblin.glb");
    m_terrain = std::make_shared<VulkanHelpers::Terrain>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_projectileModel = Systems::createProjectileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_gravityWellModel = Systems::createGravityWellModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_lightningModel = Systems::createLightningModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_lavaTileModel = Systems::createLavaTileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_hudResources = VulkanHelpers::createHudResources(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_selectionRingModel = VulkanHelpers::createSelectionRingModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    m_menuSystem = std::make_shared<VulkanHelpers::MenuSystem>(
        ctx.window, ctx.instance, ctx.physicalDevice, ctx.device, ctx.graphicsQueueFamilyIndex, ctx.graphicsQueue, ctx.swapChain, ctx.depthFormat
    );

    auto camEntity = m_registry.create();
    m_registry.emplace<Components::Camera>(
        camEntity, Components::Camera{
                       .position = {0.0f, -12.0f, 14.0f},
                       .target = {0.0f, 0.0f, 0.0f},
                       .fov = 50.0f,
                       .near_ = 0.1f,
                       .far_ = 200.0f,
                   }
    );

    auto terrainEnt = m_registry.create();
    m_registry.emplace<Components::Transform>(terrainEnt);
    m_registry.emplace<Components::RenderMesh>(terrainEnt, Components::RenderMesh{m_terrain->getModelPtr()});
}

VulkanHelpers::FrameOutput RtsGameClient::update(float dt, vk::Extent2D extent) {
    if (m_menuSystem)
        m_menuSystem->beginFrame();

    VulkanHelpers::FrameOutput out;
    bool gameplayActive = !m_menuSystem || m_menuSystem->isGameplayStarted();

    if (!gameplayActive) {
        if (m_menuSystem && m_menuSystem->drawMainMenu(extent) == VulkanHelpers::MenuAction::Exit)
            m_closeRequested = true;
        for (auto e : m_registry.view<Components::Camera>()) {
            const auto &cam = m_registry.get<Components::Camera>(e);
            out.view = cam.view;
            out.proj = cam.proj;
            break;
        }
        return out;
    }

    // ---- Network poll -------------------------------------------------------
    if (m_networkManager) {
        m_networkManager->poll(
            [this](const uint8_t *data, std::size_t size, ENetPeer *) {
                if (size == 0)
                    return;
                auto type = static_cast<MessageType>(data[0]);
                if (type == MessageType::Snapshot)
                    clientApplySnapshot(data, size);
                if (type == MessageType::PlayerAssignment)
                    clientHandleAssignment(data, size);
                if (type == MessageType::Disconnect)
                    clientHandleDisconnect(data, size);
                if (type == MessageType::ConnectionRejected)
                    clientHandleConnectionRejected(data, size);
            },
            [this](ENetPeer *) {
                HelloPacket pkt{};
                pkt.msgType = static_cast<uint8_t>(MessageType::Hello);
                pkt.protocolVersion = kProtocolVersion;
                const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
                m_networkManager->sendToServerReliable({raw, raw + sizeof(pkt)});
            },
            nullptr
        );
    }

    ++m_tick;
    m_elapsedTime += dt;

    // Follow-camera toggle: F key or button click.
    bool fDown = m_window && m_window->isKeyPressed(GLFW_KEY_F);
    bool toggleFollow = fDown && !m_prevKeyF;
    m_prevKeyF = fDown;

    // Draw follow-camera button (top-left corner).
    bool followActive = false;
    for (auto e : m_registry.view<Components::Camera>()) {
        followActive = m_registry.get<Components::Camera>(e).followPlayer;
        break;
    }
    ImGui::SetNextWindowPos({10.0f, 50.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##followcam", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
    if (followActive)
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    if (ImGui::Button(followActive ? "Follow: ON  [F]" : "Follow: OFF [F]"))
        toggleFollow = true;
    if (followActive)
        ImGui::PopStyleColor();
    ImGui::End();

    if (m_window && (!m_menuSystem || !m_menuSystem->wantsKeyboard()))
        Systems::updateCameraInput(m_registry, *m_window, dt, toggleFollow);
    if (m_window && (!m_menuSystem || !m_menuSystem->wantsMouse()))
        clientCaptureAndSendInput(extent);
    Systems::updateCamera(m_registry, extent);
    m_spatialGrid.update(m_registry);

    // ---- Rendering ---------------------------------------------------------
    const Systems::FogOfWar *fog = m_fogOfWar ? &*m_fogOfWar : nullptr;
    if (m_fogOfWar)
        m_fogOfWar->update(m_registry, m_myFaction);

    out.draws = Systems::collectDrawCalls(m_registry, fog);
    Systems::appendLavaDrawCalls(m_lavaZone, out.draws, *m_lavaTileModel);
    Systems::appendSelectionRings(m_registry, out.draws, *m_selectionRingModel, fog);
    Systems::appendHealthBars(m_registry, out.draws, m_hudResources, fog);
    if (m_projectileModel && m_gravityWellModel && m_lightningModel)
        Systems::appendProjectileDrawCalls(m_registry, out.projectileDraws, *m_projectileModel, *m_gravityWellModel, *m_lightningModel, m_elapsedTime);

    if (m_menuSystem)
        m_menuSystem->drawOverlay(dt);
    if (m_fogOfWar)
        Systems::drawFogOverlay(*m_fogOfWar, m_registry, extent);
    if (m_minimapEnabled)
        Systems::drawMinimap(fog, m_registry, extent);

    for (auto e : m_registry.view<Components::Camera>()) {
        const auto &cam = m_registry.get<Components::Camera>(e);
        out.view = cam.view;
        out.proj = cam.proj;
        break;
    }

    return out;
}

void RtsGameClient::renderImGui(vk::CommandBuffer cmd) {
    if (m_menuSystem)
        m_menuSystem->render(cmd);
}

void RtsGameClient::onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) {
    if (m_menuSystem)
        m_menuSystem->onSwapChainRecreated(swapChain);
}

bool RtsGameClient::wantsMouse() const { return m_menuSystem && m_menuSystem->wantsMouse(); }
bool RtsGameClient::wantsKeyboard() const { return m_menuSystem && m_menuSystem->wantsKeyboard(); }

// ---- Client network handlers (GPU-aware) -----------------------------------

void RtsGameClient::clientApplySnapshot(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    SnapshotHeader hdr{};
    if (!r.read(hdr))
        return;

    m_lavaZone.setSafeRadius(hdr.lavaRadius);

    std::unordered_map<uint32_t, entt::entity> knownUnits;
    for (auto e : m_registry.view<Components::NetworkId, Components::Faction>())
        knownUnits[m_registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenUnits;
    for (uint8_t i = 0; i < hdr.entityCount; ++i) {
        EntitySnapshot es{};
        if (!r.read(es))
            break;
        seenUnits.insert(es.netId);

        auto it = knownUnits.find(es.netId);
        if (it == knownUnits.end()) {
            auto faction = static_cast<Components::FactionId>(es.faction);
            auto e = m_registry.create();
            m_registry.emplace<Components::NetworkId>(e, Components::NetworkId{es.netId});
            m_netIdToEntity[es.netId] = e;
            m_registry.emplace<Components::Transform>(
                e, Components::Transform{
                       .position = {es.x, es.y, es.z},
                       .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0)),
                       .scale = {1, 1, 1},
                   }
            );
            if (m_unitModel)
                m_registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{m_unitModel});
            m_registry.emplace<Components::Faction>(e, Components::Faction{faction});
            m_registry.emplace<Components::Health>(e, Components::Health{es.health, es.maxHealth});
            m_registry.emplace<Components::Selectable>(e);
            if (es.netId == m_myNetworkId) {
                m_myFaction = faction;
                m_registry.emplace<Components::AlwaysSelected>(e);
                m_registry.emplace<Components::Selected>(e);
            }
        } else {
            m_registry.get<Components::Transform>(it->second).position = {es.x, es.y, es.z};
            auto &h = m_registry.get<Components::Health>(it->second);
            h.current = es.health;
            h.max = es.maxHealth;
        }
    }
    for (auto &[nid, e] : knownUnits) {
        if (!seenUnits.count(nid) && m_registry.valid(e)) {
            m_registry.destroy(e);
            m_netIdToEntity.erase(nid);
        }
    }

    std::unordered_map<uint32_t, entt::entity> knownProj;
    for (auto e : m_registry.view<Components::NetworkId, Components::Projectile>())
        knownProj[m_registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenProj;
    for (uint8_t i = 0; i < hdr.projectileCount; ++i) {
        ProjectileSnapshot ps{};
        if (!r.read(ps))
            break;
        seenProj.insert(ps.netId);

        auto it = knownProj.find(ps.netId);
        if (it == knownProj.end()) {
            auto e = m_registry.create();
            m_registry.emplace<Components::NetworkId>(e, Components::NetworkId{ps.netId});
            m_netIdToEntity[ps.netId] = e;
            glm::quat rot = ps.type == 2
                                ? glm::angleAxis(ps.yaw, glm::vec3{0.0f, 0.0f, 1.0f})
                                : glm::quat{1, 0, 0, 0};
            m_registry.emplace<Components::Transform>(
                e, Components::Transform{
                       .position = {ps.x, ps.y, ps.z},
                       .rotation = rot,
                       .scale = {1, 1, 1},
                   }
            );
            m_registry.emplace<Components::Projectile>(
                e, Components::Projectile{
                       .ownerFaction = static_cast<Components::FactionId>(ps.faction),
                       .velocity = {ps.vx, ps.vy, ps.vz},
                   }
            );
            if (ps.type == 1)
                m_registry.emplace<Components::GravityWell>(e);
            else if (ps.type == 2)
                m_registry.emplace<Components::LightningBolt>(e);
        } else {
            auto &tr = m_registry.get<Components::Transform>(it->second);
            tr.position = {ps.x, ps.y, ps.z};
            if (ps.type == 2)
                tr.rotation = glm::angleAxis(ps.yaw, glm::vec3{0.0f, 0.0f, 1.0f});
            m_registry.get<Components::Projectile>(it->second).velocity = {ps.vx, ps.vy, ps.vz};
        }
    }
    for (auto &[nid, e] : knownProj) {
        if (!seenProj.count(nid) && m_registry.valid(e)) {
            m_registry.destroy(e);
            m_netIdToEntity.erase(nid);
        }
    }
}

void RtsGameClient::clientCaptureAndSendInput(vk::Extent2D extent) {
    if (!m_networkManager || !m_networkManager->isConnected())
        return;

    bool rightDown = m_window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool qDown = m_window->isKeyPressed(GLFW_KEY_Q);
    bool eDown = m_window->isKeyPressed(GLFW_KEY_E);
    bool rDown = m_window->isKeyPressed(GLFW_KEY_R);
    bool rightJust = rightDown && !m_prevMouseRight;
    bool qJust = qDown && !m_prevKeyQ;
    bool eJust = eDown && !m_prevKeyE;
    bool rJust = rDown && !m_prevKeyR;
    m_prevMouseRight = rightDown;
    m_prevKeyQ = qDown;
    m_prevKeyE = eDown;
    m_prevKeyR = rDown;

    if (!rightJust && !qJust && !eJust && !rJust)
        return;

    const Components::Camera *cam = nullptr;
    for (auto e : m_registry.view<Components::Camera>()) {
        cam = &m_registry.get<Components::Camera>(e);
        break;
    }
    if (!cam)
        return;

    auto [mx, my] = m_window->getMousePosition();
    float ndcX = (2.0f * static_cast<float>(mx)) / static_cast<float>(extent.width) - 1.0f;
    float ndcY = (2.0f * static_cast<float>(my)) / static_cast<float>(extent.height) - 1.0f;
    glm::mat4 invVP = glm::inverse(cam->proj * cam->view);
    glm::vec4 nearW = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farW = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearW /= nearW.w;
    farW /= farW.w;
    glm::vec3 dir = glm::normalize(glm::vec3(farW) - glm::vec3(nearW));

    if (std::abs(dir.z) < 1e-6f)
        return;
    float t = -glm::vec3(nearW).z / dir.z;
    if (t < 0.0f)
        return;
    glm::vec3 ground = glm::vec3(nearW) + t * dir;

    bool hasMoveOrder = rightJust;
    bool fireAbility = qJust || eJust || rJust;
    uint8_t abilitySlot = rJust ? 2 : eJust ? 1 : 0;
    glm::vec3 moveTarget = rightJust ? ground : glm::vec3{};
    glm::vec3 abilityTarget = fireAbility ? ground : glm::vec3{};

    InputPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::Input);
    pkt.tick = m_tick;
    pkt.flags = (hasMoveOrder ? InputFlags::kMoveOrder : 0) | (fireAbility ? InputFlags::kFireAbility : 0);
    pkt.abilitySlot = abilitySlot;
    pkt.moveX = moveTarget.x;
    pkt.moveY = moveTarget.y;
    pkt.moveZ = moveTarget.z;
    pkt.abilityX = abilityTarget.x;
    pkt.abilityY = abilityTarget.y;
    pkt.abilityZ = abilityTarget.z;

    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    m_networkManager->sendToServerUnreliable({raw, raw + sizeof(pkt)});
}

} // namespace Game
