#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "camera_system.h"
#include "combat_system.h"
#include "components.h"
#include "death_system.h"
#include "fog_of_war.h"
#include "fog_system.h"
#include "hud_system.h"
#include "input_system.h"
#include "lava_system.h"
#include "minimap_system.h"
#include "movement_system.h"
#include "network_messages.h"
#include "order_system.h"
#include "projectile_system.h"
#include "render_system.h"
#include "rts_game.h"
#include "selection_ring.h"
#include "selection_system.h"

namespace Game {

// Spawn positions for server-managed player slots (index 0 = first client, etc.)
static constexpr std::array<glm::vec3, 4> SPAWN_POSITIONS = {{
    {-3.0f, 0.0f, 0.0f},
    {3.0f, 0.0f, 0.0f},
    {-3.0f, 3.0f, 0.0f},
    {3.0f, 3.0f, 0.0f},
}};

static constexpr std::array<Components::FactionId, 2> SLOT_FACTIONS = {{
    Components::FactionId::Player,
    Components::FactionId::Enemy,
}};

// ---- Feature toggles -------------------------------------------------------

void RtsGame::enablePathfinding(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize) { pathfinder.emplace(worldMin, worldMax, cellSize); }
void RtsGame::enableFogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius) { fogOfWar.emplace(worldMin, worldMax, cellSize, sightRadius); }
void RtsGame::enableMinimap() { minimapEnabled = true; }
void RtsGame::enableCombat() { combatEnabled = true; }

// ---- Network setup ---------------------------------------------------------

void RtsGame::setupAsServer(uint16_t port) {
    networkRole_ = NetworkRole::Server;
    networkManager_ = std::make_unique<Network::NetworkManager>();
    networkManager_->startServer(port);
}

void RtsGame::setupAsClient(std::string host, uint16_t port) {
    networkRole_ = NetworkRole::Client;
    networkManager_ = std::make_unique<Network::NetworkManager>();
    networkManager_->connectToServer(host, port);
}

// ---- IGame: initLogic ------------------------------------------------------

void RtsGame::initLogic() {
    // Standalone mode pre-spawns a fixed scene. Server and Client get their
    // entities from client connections and network snapshots respectively.
    if (networkRole_ == NetworkRole::Standalone) {
        auto player = spawnUnit({-3.0f, 0.0f, 0.0f}, Components::FactionId::Player);
        registry.emplace<Components::AlwaysSelected>(player);
        registry.emplace<Components::Selected>(player);
        spawnUnit({3.0f, 0.0f, 0.0f}, Components::FactionId::Enemy);
    }
}

// ---- IGame: initGraphics ---------------------------------------------------

void RtsGame::initGraphics(const VulkanHelpers::ResourceContext &ctx) {
    window = &ctx.window;

    unitModel = std::make_shared<VulkanHelpers::Model>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout, "models/goblin.glb");
    terrain = std::make_shared<VulkanHelpers::Terrain>(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    projectileModel = Systems::createProjectileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    lavaTileModel = Systems::createLavaTileModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    hudResources = VulkanHelpers::createHudResources(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    selectionRingModel = VulkanHelpers::createSelectionRingModel(ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout);
    menuSystem = std::make_shared<VulkanHelpers::MenuSystem>(
        ctx.window, ctx.instance, ctx.physicalDevice, ctx.device, ctx.graphicsQueueFamilyIndex, ctx.graphicsQueue, ctx.swapChain, ctx.depthFormat
    );

    // Camera entity — local to this viewer, not networked
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

    // Terrain entity — rendered locally, not synced over network
    auto terrainEnt = registry.create();
    registry.emplace<Components::Transform>(terrainEnt);
    registry.emplace<Components::RenderMesh>(terrainEnt, Components::RenderMesh{terrain->getModelPtr()});

    // Backfill RenderMesh onto any units already spawned by initLogic()
    for (auto e : registry.view<Components::Health, Components::Transform>()) {
        if (!registry.all_of<Components::RenderMesh>(e))
            registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{unitModel});
    }
}

// ---- IGame: update ---------------------------------------------------------

VulkanHelpers::FrameOutput RtsGame::update(float dt, vk::Extent2D extent) {
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
            [this](const uint8_t *data, std::size_t size, ENetPeer *peer) {
                if (size == 0)
                    return;
                auto type = static_cast<MessageType>(data[0]);
                if (networkRole_ == NetworkRole::Server) {
                    if (type == MessageType::Input)
                        serverHandleInput(data, size, peer);
                } else {
                    if (type == MessageType::Snapshot)
                        clientApplySnapshot(data, size);
                    if (type == MessageType::PlayerAssignment)
                        clientHandleAssignment(data, size);
                }
            },
            [this](ENetPeer *peer) { onClientConnect(peer); }, [this](ENetPeer *peer) { onClientDisconnect(peer); }
        );
    }

    ++tick_;

    // ---- Per-role update paths ---------------------------------------------
    if (networkRole_ == NetworkRole::Client) {
        if (window && (!menuSystem || !menuSystem->wantsKeyboard()))
            Systems::updateCameraInput(registry, *window, dt);
        if (window && (!menuSystem || !menuSystem->wantsMouse()))
            clientCaptureAndSendInput(extent);
        Systems::updateCamera(registry, extent);

    } else {
        // Server or Standalone: full simulation
        if (window && (!menuSystem || !menuSystem->wantsKeyboard()))
            Systems::updateCameraInput(registry, *window, dt);

        lavaZone.update(dt);
        Systems::applyLavaDamage(lavaZone, registry, dt);
        if (combatEnabled)
            Systems::processCombat(registry, dt);
        Systems::processDeath(registry);
        Systems::tickAbilities(registry, dt);
        Systems::updateProjectiles(registry, dt);
        Systems::processOrders(registry, dt, pathfinder ? &*pathfinder : nullptr);
        Systems::applyKnockback(registry, dt);
        Systems::applySeparation(registry);
        Systems::clampToBounds(registry, {-20.0f, -20.0f}, {20.0f, 20.0f});

        if (networkRole_ != NetworkRole::Server)
            Systems::updateCamera(registry, extent);

        if (window && (!menuSystem || !menuSystem->wantsMouse())) {
            Systems::updateSelection(registry, *window, extent);
            Systems::processAbilityInput(registry, *window, extent, projectileModel);
        }

        if (networkRole_ == NetworkRole::Server) {
            snapshotTimer_ += dt;
            if (snapshotTimer_ >= SNAPSHOT_INTERVAL) {
                snapshotTimer_ -= SNAPSHOT_INTERVAL;
                serverSendSnapshot();
            }
        }
    }

    spatialGrid.update(registry);

    // ---- Rendering (skipped on dedicated server) ---------------------------
    if (networkRole_ != NetworkRole::Server) {
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
    }

    return out;
}

void RtsGame::renderImGui(vk::CommandBuffer cmd) {
    if (menuSystem)
        menuSystem->render(cmd);
}

void RtsGame::onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) {
    if (menuSystem)
        menuSystem->onSwapChainRecreated(swapChain);
}

bool RtsGame::wantsMouse() const { return menuSystem && menuSystem->wantsMouse(); }
bool RtsGame::wantsKeyboard() const { return menuSystem && menuSystem->wantsKeyboard(); }
bool RtsGame::wantsClose() const { return closeRequested; }

// ---- Unit spawning ---------------------------------------------------------

entt::entity RtsGame::respawnUnit(Components::FactionId faction, glm::vec3 position) {
    auto e = spawnUnit(position, faction);
    if (faction == Components::FactionId::Player) {
        registry.emplace<Components::AlwaysSelected>(e);
        registry.emplace<Components::Selected>(e);
    }
    return e;
}

entt::entity RtsGame::spawnUnit(glm::vec3 position, Components::FactionId faction) {
    auto e = registry.create();
    registry.emplace<Components::Transform>(
        e, Components::Transform{
               .position = position,
               .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
               .scale = {1.0f, 1.0f, 1.0f},
           }
    );
    // RenderMesh only if graphics have been initialised (null on dedicated server)
    if (unitModel)
        registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{unitModel});
    registry.emplace<Components::Selectable>(e);
    registry.emplace<Components::MovementSpeed>(e);
    registry.emplace<Components::OrderQueue>(e);
    registry.emplace<Components::Faction>(e, Components::Faction{faction});
    registry.emplace<Components::Health>(e);
    registry.emplace<Components::Ability>(e);
    if (combatEnabled)
        registry.emplace<Components::Combat>(e);
    registry.emplace<Components::NetworkId>(e, Components::NetworkId{nextNetworkId_++});
    return e;
}

// ---- Network: server -------------------------------------------------------

void RtsGame::onClientConnect(ENetPeer *peer) {
    if (networkRole_ != NetworkRole::Server)
        return;

    std::size_t slot = peerToNetId_.size();
    if (slot >= SPAWN_POSITIONS.size()) {
        std::cout << "[Server] Full — rejecting client\n";
        enet_peer_disconnect(peer, 0);
        return;
    }

    auto faction = (slot < SLOT_FACTIONS.size()) ? SLOT_FACTIONS[slot] : Components::FactionId::Enemy;
    auto unit = spawnUnit(SPAWN_POSITIONS[slot], faction);
    auto netId = registry.get<Components::NetworkId>(unit).id;
    peerToNetId_[peer] = netId;

    PlayerAssignmentPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::PlayerAssignment);
    pkt.yourNetworkId = netId;
    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    networkManager_->sendReliableTo(peer, {raw, raw + sizeof(pkt)});

    std::cout << "[Server] Client connected → slot " << slot << ", NetworkId " << netId << "\n";
}

void RtsGame::onClientDisconnect(ENetPeer *peer) {
    auto it = peerToNetId_.find(peer);
    if (it == peerToNetId_.end())
        return;

    uint32_t netId = it->second;
    for (auto e : registry.view<Components::NetworkId>()) {
        if (registry.get<Components::NetworkId>(e).id == netId) {
            registry.destroy(e);
            break;
        }
    }
    peerToNetId_.erase(it);
    std::cout << "[Server] Client disconnected, destroyed unit " << netId << "\n";
}

void RtsGame::serverSendSnapshot() {
    BufWriter w;
    SnapshotHeader hdr{};
    hdr.msgType = static_cast<uint8_t>(MessageType::Snapshot);
    hdr.tick = tick_;
    hdr.lavaRadius = lavaZone.getSafeRadius();

    std::vector<EntitySnapshot> entities;
    std::vector<ProjectileSnapshot> projectiles;

    for (auto e : registry.view<Components::Transform, Components::Faction, Components::Health, Components::NetworkId>()) {
        const auto &t = registry.get<Components::Transform>(e);
        const auto &f = registry.get<Components::Faction>(e);
        const auto &h = registry.get<Components::Health>(e);
        const auto &n = registry.get<Components::NetworkId>(e);
        EntitySnapshot es{};
        es.netId = n.id;
        es.faction = static_cast<uint8_t>(f.id);
        es.x = t.position.x;
        es.y = t.position.y;
        es.z = t.position.z;
        es.health = h.current;
        es.maxHealth = h.max;
        entities.push_back(es);
    }

    for (auto e : registry.view<Components::Transform, Components::Projectile, Components::NetworkId>()) {
        const auto &t = registry.get<Components::Transform>(e);
        const auto &p = registry.get<Components::Projectile>(e);
        const auto &n = registry.get<Components::NetworkId>(e);
        ProjectileSnapshot ps{};
        ps.netId = n.id;
        ps.faction = static_cast<uint8_t>(p.ownerFaction);
        ps.x = t.position.x;
        ps.y = t.position.y;
        ps.z = t.position.z;
        ps.vx = p.velocity.x;
        ps.vy = p.velocity.y;
        ps.vz = p.velocity.z;
        projectiles.push_back(ps);
    }

    hdr.entityCount = static_cast<uint8_t>(std::min(entities.size(), std::size_t{255}));
    hdr.projectileCount = static_cast<uint8_t>(std::min(projectiles.size(), std::size_t{255}));
    w.write(hdr);
    for (auto i = 0u; i < hdr.entityCount; ++i)
        w.write(entities[i]);
    for (auto i = 0u; i < hdr.projectileCount; ++i)
        w.write(projectiles[i]);

    networkManager_->broadcastUnreliable(w.buf());
}

void RtsGame::serverHandleInput(const uint8_t *data, std::size_t size, ENetPeer *peer) {
    BufReader r(data, size);
    InputPacket pkt{};
    if (!r.read(pkt))
        return;

    auto peerIt = peerToNetId_.find(peer);
    if (peerIt == peerToNetId_.end())
        return;

    entt::entity clientEntity = entt::null;
    for (auto e : registry.view<Components::NetworkId>()) {
        if (registry.get<Components::NetworkId>(e).id == peerIt->second) {
            clientEntity = e;
            break;
        }
    }
    if (clientEntity == entt::null)
        return;

    if ((pkt.flags & 0x01) && registry.all_of<Components::OrderQueue>(clientEntity)) {
        registry.get<Components::OrderQueue>(clientEntity).enqueueImmediate(Orders::MoveOrder{.destination = {pkt.moveX, pkt.moveY, pkt.moveZ}, .path = {}, .pathIndex = 0});
    }

    if ((pkt.flags & 0x02) && registry.all_of<Components::Ability, Components::Transform, Components::Faction>(clientEntity)) {
        auto &ab = registry.get<Components::Ability>(clientEntity);
        if (ab.timer >= ab.cooldown) {
            const auto &t = registry.get<Components::Transform>(clientEntity);
            const auto &f = registry.get<Components::Faction>(clientEntity);
            glm::vec3 delta = glm::vec3{pkt.abilityX, pkt.abilityY, pkt.abilityZ} - t.position;
            delta.z = 0.0f;
            float len = glm::length(delta);
            if (len > 0.001f) {
                auto proj = Systems::spawnProjectile(registry, projectileModel, t.position, (delta / len) * ab.projectileSpeed, f.id, ab.knockbackForce);
                registry.emplace<Components::NetworkId>(proj, Components::NetworkId{nextNetworkId_++});
                ab.timer = 0.0f;
            }
        }
    }
}

// ---- Network: client -------------------------------------------------------

void RtsGame::clientApplySnapshot(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    SnapshotHeader hdr{};
    if (!r.read(hdr))
        return;

    lavaZone.setSafeRadius(hdr.lavaRadius);

    // ---- Units -------------------------------------------------------------
    std::unordered_map<uint32_t, entt::entity> knownUnits;
    for (auto e : registry.view<Components::NetworkId, Components::Faction>())
        knownUnits[registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenUnits;
    for (uint8_t i = 0; i < hdr.entityCount; ++i) {
        EntitySnapshot es{};
        if (!r.read(es))
            break;
        seenUnits.insert(es.netId);

        auto it = knownUnits.find(es.netId);
        if (it == knownUnits.end()) {
            auto faction = static_cast<Components::FactionId>(es.faction);
            auto e = registry.create();
            registry.emplace<Components::NetworkId>(e, Components::NetworkId{es.netId});
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
            auto &t = registry.get<Components::Transform>(it->second);
            t.position = {es.x, es.y, es.z};
            auto &h = registry.get<Components::Health>(it->second);
            h.current = es.health;
            h.max = es.maxHealth;
        }
    }
    for (auto &[nid, e] : knownUnits)
        if (!seenUnits.count(nid) && registry.valid(e))
            registry.destroy(e);

    // ---- Projectiles -------------------------------------------------------
    std::unordered_map<uint32_t, entt::entity> knownProj;
    for (auto e : registry.view<Components::NetworkId, Components::Projectile>())
        knownProj[registry.get<Components::NetworkId>(e).id] = e;

    std::unordered_set<uint32_t> seenProj;
    for (uint8_t i = 0; i < hdr.projectileCount; ++i) {
        ProjectileSnapshot ps{};
        if (!r.read(ps))
            break;
        seenProj.insert(ps.netId);

        auto it = knownProj.find(ps.netId);
        if (it == knownProj.end()) {
            auto e = registry.create();
            registry.emplace<Components::NetworkId>(e, Components::NetworkId{ps.netId});
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
    for (auto &[nid, e] : knownProj)
        if (!seenProj.count(nid) && registry.valid(e))
            registry.destroy(e);
}

void RtsGame::clientHandleAssignment(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    PlayerAssignmentPacket pkt{};
    if (!r.read(pkt))
        return;
    myNetworkId_ = pkt.yourNetworkId;
    std::cout << "[Client] Assigned NetworkId " << myNetworkId_ << "\n";

    // Retroactively mark the entity if it already exists from a snapshot.
    for (auto e : registry.view<Components::NetworkId>()) {
        if (registry.get<Components::NetworkId>(e).id != myNetworkId_)
            continue;
        if (!registry.all_of<Components::AlwaysSelected>(e))
            registry.emplace<Components::AlwaysSelected>(e);
        if (!registry.all_of<Components::Selected>(e))
            registry.emplace<Components::Selected>(e);
        if (registry.all_of<Components::Faction>(e))
            myFaction_ = registry.get<Components::Faction>(e).id;
        break;
    }
}

void RtsGame::clientCaptureAndSendInput(vk::Extent2D extent) {
    if (!networkManager_ || !networkManager_->isConnected())
        return;

    static bool prevRight = false;
    static bool prevQ = false;
    bool rightDown = window->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);
    bool qDown = window->isKeyPressed(GLFW_KEY_Q);
    bool rightJust = rightDown && !prevRight;
    bool qJust = qDown && !prevQ;
    prevRight = rightDown;
    prevQ = qDown;

    if (!rightJust && !qJust)
        return;

    const Components::Camera *cam = nullptr;
    for (auto e : registry.view<Components::Camera>()) {
        cam = &registry.get<Components::Camera>(e);
        break;
    }
    if (!cam)
        return;

    auto [mx, my] = window->getMousePosition();
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

    if (rightJust) {
        pendingInput_.hasMoveOrder = true;
        pendingInput_.moveTarget = ground;
    }
    if (qJust) {
        pendingInput_.fireAbility = true;
        pendingInput_.abilityTarget = ground;
    }

    InputPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::Input);
    pkt.tick = tick_;
    pkt.flags = (pendingInput_.hasMoveOrder ? 0x01 : 0) | (pendingInput_.fireAbility ? 0x02 : 0);
    pkt.moveX = pendingInput_.moveTarget.x;
    pkt.moveY = pendingInput_.moveTarget.y;
    pkt.moveZ = pendingInput_.moveTarget.z;
    pkt.abilityX = pendingInput_.abilityTarget.x;
    pkt.abilityY = pendingInput_.abilityTarget.y;
    pkt.abilityZ = pendingInput_.abilityTarget.z;

    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    networkManager_->sendToServerUnreliable({raw, raw + sizeof(pkt)});
    pendingInput_ = {};
}

} // namespace Game
