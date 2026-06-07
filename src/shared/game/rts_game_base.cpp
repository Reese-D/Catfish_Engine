#include "rts_game_base.h"

#include <array>
#include <iostream>

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "combat_system.h"
#include "components.h"
#include "death_system.h"
#include "lava_sim.h"
#include "movement_system.h"
#include "network_messages.h"
#include "projectile_sim.h"

namespace Game {

static constexpr std::array<glm::vec3, 4> kSpawnPositions = {{
    {-3.0f, 0.0f, 0.0f},
    {3.0f, 0.0f, 0.0f},
    {-3.0f, 3.0f, 0.0f},
    {3.0f, 3.0f, 0.0f},
}};

static constexpr std::array<Components::FactionId, 2> kSlotFactions = {{
    Components::FactionId::Player,
    Components::FactionId::Enemy,
}};

// ---- Feature toggles -------------------------------------------------------

void RtsGameBase::enablePathfinding(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize) { m_pathfinder.emplace(worldMin, worldMax, cellSize); }
void RtsGameBase::enableFogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius) { m_fogOfWar.emplace(worldMin, worldMax, cellSize, sightRadius); }
void RtsGameBase::enableMinimap() { m_minimapEnabled = true; }
void RtsGameBase::enableCombat() { m_combatEnabled = true; }

// ---- Network setup ---------------------------------------------------------

void RtsGameBase::setupAsServer(uint16_t port) {
    m_networkManager = std::make_unique<Network::NetworkManager>();
    m_networkManager->startServer(port);
}

void RtsGameBase::setupAsClient(std::string host, uint16_t port) {
    m_networkManager = std::make_unique<Network::NetworkManager>();
    m_networkManager->connectToServer(host, port);
}

// ---- Unit spawning ---------------------------------------------------------

entt::entity RtsGameBase::respawnUnit(Components::FactionId faction, glm::vec3 position) {
    auto e = spawnUnit(position, faction);
    if (faction == Components::FactionId::Player) {
        m_registry.emplace<Components::AlwaysSelected>(e);
        m_registry.emplace<Components::Selected>(e);
    }
    return e;
}

entt::entity RtsGameBase::spawnUnit(glm::vec3 position, Components::FactionId faction) {
    auto e = m_registry.create();
    m_registry.emplace<Components::Transform>(
        e, Components::Transform{
               .position = position,
               .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
               .scale = {1.0f, 1.0f, 1.0f},
           }
    );
    m_registry.emplace<Components::Selectable>(e);
    m_registry.emplace<Components::MovementSpeed>(e);
    m_registry.emplace<Components::Velocity>(e);
    m_registry.emplace<Components::ThrustDirection>(e);
    m_registry.emplace<Components::OrderQueue>(e);
    m_registry.emplace<Components::Faction>(e, Components::Faction{faction});
    m_registry.emplace<Components::Health>(e);
    m_registry.emplace<Components::AbilitySet>(
        e, Components::AbilitySet{
               .slots = {
                   Components::AbilitySlot{Components::AbilityId::Projectile, 1.5f, 1.5f, 8.0f, 12.0f},
                   Components::AbilitySlot{Components::AbilityId::GravityWell, 4.0f, 4.0f, 2.0f, 0.0f, 15.0f, 5.0f, 0.5f},
                   // Lightning: cooldown=3s, projectileSpeed unused, knockback=6, activationDelay=0.2s charge
                   Components::AbilitySlot{Components::AbilityId::Lightning, 3.0f, 3.0f, 0.0f, 6.0f, 0.0f, 0.0f, 0.2f},
               }
           }
    );
    if (m_combatEnabled)
        m_registry.emplace<Components::Combat>(e);
    auto netId = m_nextNetworkId++;
    m_registry.emplace<Components::NetworkId>(e, Components::NetworkId{netId});
    m_netIdToEntity[netId] = e;
    return e;
}

entt::entity RtsGameBase::spawnRock(glm::vec3 position) {
    auto e = m_registry.create();
    m_registry.emplace<Components::Transform>(
        e, Components::Transform{
               .position = position,
               .rotation = glm::quat{1.0f, 0.0f, 0.0f, 0.0f},
               .scale = {1.5f, 1.5f, 1.5f},
           }
    );
    m_registry.emplace<Components::Velocity>(e);
    m_registry.emplace<Components::Mass>(e, Components::Mass{8.0f});
    m_registry.emplace<Components::Friction>(e, Components::Friction{4.0f});
    m_registry.emplace<Components::Rock>(e);
    // Collider derived from boulder.glb bounds × scale 1.5:
    // model X half ≈ 0.542, model-Z centre = −0.470 → world-Y offset after Rx(90°)
    m_registry.emplace<Components::Collider>(e, Components::Collider{.radius = 0.81f, .offset = {0.0f, 0.70f}});
    auto netId = m_nextNetworkId++;
    m_registry.emplace<Components::NetworkId>(e, Components::NetworkId{netId});
    m_netIdToEntity[netId] = e;
    return e;
}

// ---- Network: server -------------------------------------------------------

void RtsGameBase::onClientConnect(ENetPeer *peer) {
    if (m_peerToNetId.size() + m_pendingPeers.size() >= kSpawnPositions.size()) {
        std::cout << "[Server] Full — rejecting incoming connection\n";
        serverSendConnectionRejected(peer, RejectionReason::ServerFull);
        return;
    }
    m_pendingPeers.insert(peer);
    std::cout << "[Server] Peer connected, awaiting Hello\n";
}

void RtsGameBase::onClientDisconnect(ENetPeer *peer) {
    m_pendingPeers.erase(peer);

    auto it = m_peerToNetId.find(peer);
    if (it == m_peerToNetId.end())
        return;

    uint32_t netId = it->second;
    auto entIt = m_netIdToEntity.find(netId);
    if (entIt != m_netIdToEntity.end()) {
        if (m_registry.valid(entIt->second))
            m_registry.destroy(entIt->second);
        m_netIdToEntity.erase(entIt);
    }
    m_peerToNetId.erase(it);
    std::cout << "[Server] Client disconnected, destroyed unit " << netId << "\n";
}

void RtsGameBase::serverSendConnectionRejected(ENetPeer *peer, RejectionReason reason) {
    ConnectionRejectedPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::ConnectionRejected);
    pkt.reason = static_cast<uint8_t>(reason);
    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    m_networkManager->sendReliableTo(peer, {raw, raw + sizeof(pkt)});
    enet_peer_disconnect_later(peer, 0);
}

void RtsGameBase::serverSendDisconnect(ENetPeer *peer, DisconnectReason reason) {
    DisconnectPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::Disconnect);
    pkt.reason = static_cast<uint8_t>(reason);
    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    m_networkManager->sendReliableTo(peer, {raw, raw + sizeof(pkt)});
    enet_peer_disconnect_later(peer, 0);
}

void RtsGameBase::serverHandleHello(const uint8_t *data, std::size_t size, ENetPeer *peer) {
    if (size < sizeof(HelloPacket))
        return;

    const auto *pkt = reinterpret_cast<const HelloPacket *>(data);
    if (pkt->protocolVersion != kProtocolVersion) {
        std::cout << "[Server] Version mismatch (client=" << pkt->protocolVersion << " server=" << kProtocolVersion << ") — rejecting\n";
        serverSendConnectionRejected(peer, RejectionReason::VersionMismatch);
        return;
    }

    m_pendingPeers.erase(peer);
    std::size_t slot = m_peerToNetId.size();

    auto faction = (slot < kSlotFactions.size()) ? kSlotFactions[slot] : Components::FactionId::Enemy;
    auto unit = spawnUnit(kSpawnPositions[slot], faction);
    auto netId = m_registry.get<Components::NetworkId>(unit).id;
    m_peerToNetId[peer] = netId;

    PlayerAssignmentPacket assignPkt{};
    assignPkt.msgType = static_cast<uint8_t>(MessageType::PlayerAssignment);
    assignPkt.yourNetworkId = netId;
    const auto *raw = reinterpret_cast<const uint8_t *>(&assignPkt);
    m_networkManager->sendReliableTo(peer, {raw, raw + sizeof(assignPkt)});

    std::cout << "[Server] Client accepted → slot " << slot << ", NetworkId " << netId << "\n";
}

void RtsGameBase::serverSendSnapshot() {
    BufWriter w;
    SnapshotHeader hdr{};
    hdr.msgType = static_cast<uint8_t>(MessageType::Snapshot);
    hdr.tick = m_tick;
    hdr.lavaRadius = m_lavaZone.getSafeRadius();

    std::vector<EntitySnapshot> entities;
    std::vector<ProjectileSnapshot> projectiles;
    std::vector<RockSnapshot> rocks;

    for (auto e : m_registry.view<Components::Transform, Components::Faction, Components::Health, Components::NetworkId>()) {
        const auto &t = m_registry.get<Components::Transform>(e);
        const auto &f = m_registry.get<Components::Faction>(e);
        const auto &h = m_registry.get<Components::Health>(e);
        const auto &n = m_registry.get<Components::NetworkId>(e);
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

    for (auto e : m_registry.view<Components::Transform, Components::Projectile, Components::NetworkId>()) {
        const auto &t = m_registry.get<Components::Transform>(e);
        const auto &p = m_registry.get<Components::Projectile>(e);
        const auto &n = m_registry.get<Components::NetworkId>(e);
        ProjectileSnapshot ps{};
        ps.netId = n.id;
        ps.faction = static_cast<uint8_t>(p.ownerFaction);
        ps.type = m_registry.all_of<Components::GravityWell>(e) ? 1u : m_registry.all_of<Components::LightningBolt>(e) ? 2u : 0u;
        ps.x = t.position.x;
        ps.y = t.position.y;
        ps.z = t.position.z;
        ps.vx = p.velocity.x;
        ps.vy = p.velocity.y;
        ps.vz = p.velocity.z;
        // Encode the Z-axis yaw from the quaternion (used to orient the lightning mesh on the client)
        ps.yaw = 2.0f * std::atan2(t.rotation.z, t.rotation.w);
        projectiles.push_back(ps);
    }

    for (auto e : m_registry.view<Components::Transform, Components::Rock, Components::NetworkId>()) {
        const auto &t = m_registry.get<Components::Transform>(e);
        const auto &n = m_registry.get<Components::NetworkId>(e);
        RockSnapshot rs{};
        rs.netId = n.id;
        rs.x = t.position.x;
        rs.y = t.position.y;
        rs.z = t.position.z;
        rocks.push_back(rs);
    }

    hdr.entityCount = static_cast<uint8_t>(std::min(entities.size(), std::size_t{255}));
    hdr.projectileCount = static_cast<uint8_t>(std::min(projectiles.size(), std::size_t{255}));
    hdr.rockCount = static_cast<uint8_t>(std::min(rocks.size(), std::size_t{255}));
    w.write(hdr);
    for (auto i = 0u; i < hdr.entityCount; ++i)
        w.write(entities[i]);
    for (auto i = 0u; i < hdr.projectileCount; ++i)
        w.write(projectiles[i]);
    for (auto i = 0u; i < hdr.rockCount; ++i)
        w.write(rocks[i]);

    m_networkManager->broadcastUnreliable(w.buf());
}

void RtsGameBase::serverHandleInput(const uint8_t *data, std::size_t size, ENetPeer *peer) {
    BufReader r(data, size);
    InputPacket pkt{};
    if (!r.read(pkt))
        return;

    auto peerIt = m_peerToNetId.find(peer);
    if (peerIt == m_peerToNetId.end())
        return;

    auto entIt = m_netIdToEntity.find(peerIt->second);
    if (entIt == m_netIdToEntity.end() || !m_registry.valid(entIt->second))
        return;
    entt::entity clientEntity = entIt->second;

    if ((pkt.flags & InputFlags::kMoveOrder) && m_registry.all_of<Components::ThrustDirection, Components::Transform>(clientEntity)) {
        const auto &t = m_registry.get<Components::Transform>(clientEntity);
        glm::vec2 delta{pkt.moveX - t.position.x, pkt.moveY - t.position.y};
        if (glm::length(delta) > 0.001f)
            m_registry.get<Components::ThrustDirection>(clientEntity).dir = glm::normalize(delta);
    }

    if ((pkt.flags & InputFlags::kFireAbility) && m_registry.all_of<Components::AbilitySet, Components::Transform, Components::Faction>(clientEntity)) {
        auto &abilitySet = m_registry.get<Components::AbilitySet>(clientEntity);
        if (pkt.abilitySlot >= abilitySet.slots.size())
            return;

        auto &slot = abilitySet.slots[pkt.abilitySlot];
        if (slot.timer < slot.cooldown)
            return;

        const auto &t = m_registry.get<Components::Transform>(clientEntity);
        const auto &f = m_registry.get<Components::Faction>(clientEntity);
        glm::vec3 delta = glm::vec3{pkt.abilityX, pkt.abilityY, pkt.abilityZ} - t.position;
        delta.z = 0.0f;
        float len = glm::length(delta);
        if (len < 0.001f)
            return;

        glm::vec3 dir = delta / len;

        entt::entity proj;
        if (slot.id == Components::AbilityId::Lightning) {
            constexpr float kMaxRange = 8.0f;
            constexpr float kSpeed = 28.0f;
            glm::vec3 boltDir = dir;
            // Cap the effective range via lifetime; charge delay is slot.activationDelay
            float lifetime = slot.activationDelay + kMaxRange / kSpeed;
            proj = Systems::spawnProjectile(m_registry, t.position, boltDir, f.id, slot.knockbackForce, 0.12f, lifetime);
            // Velocity starts at zero during the charge phase
            m_registry.get<Components::Projectile>(proj).velocity = {0.0f, 0.0f, 0.0f};
            // Orient quad X-axis (long axis) toward target: atan2(y, x) is standard angle from +X
            float angle = std::atan2(boltDir.y, boltDir.x);
            m_registry.get<Components::Transform>(proj).rotation = glm::angleAxis(angle, glm::vec3{0.0f, 0.0f, 1.0f});
            m_registry.emplace<Components::LightningBolt>(
                proj, Components::LightningBolt{
                          .chargeDelay = slot.activationDelay,
                          .chargeTimer = 0.0f,
                          .boltDir = boltDir,
                          .speed = kSpeed,
                      }
            );
        } else {
            glm::vec3 velocity = dir * slot.projectileSpeed;
            proj = Systems::spawnProjectile(m_registry, t.position, velocity, f.id, slot.knockbackForce);

            if (slot.id == Components::AbilityId::GravityWell) {
                m_registry.emplace<Components::GravityWell>(
                    proj, Components::GravityWell{
                              .pullStrength = slot.pullStrength,
                              .pullRadius = slot.pullRadius,
                              .activationDelay = slot.activationDelay,
                          }
                );
                m_registry.get<Components::Projectile>(proj).lifetime = 5.0f;
                m_registry.get<Components::Projectile>(proj).hitRadius = 0.6f;
            }
        }

        auto netId = m_nextNetworkId++;
        m_registry.emplace<Components::NetworkId>(proj, Components::NetworkId{netId});
        m_netIdToEntity[netId] = proj;
        slot.timer = 0.0f;
    }
}

// ---- Network: client (no GPU resources) ------------------------------------

void RtsGameBase::clientHandleAssignment(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    PlayerAssignmentPacket pkt{};
    if (!r.read(pkt))
        return;
    m_myNetworkId = pkt.yourNetworkId;
    std::cout << "[Client] Assigned NetworkId " << m_myNetworkId << "\n";

    for (auto e : m_registry.view<Components::NetworkId>()) {
        if (m_registry.get<Components::NetworkId>(e).id != m_myNetworkId)
            continue;
        if (!m_registry.all_of<Components::AlwaysSelected>(e))
            m_registry.emplace<Components::AlwaysSelected>(e);
        if (!m_registry.all_of<Components::Selected>(e))
            m_registry.emplace<Components::Selected>(e);
        if (m_registry.all_of<Components::Faction>(e))
            m_myFaction = m_registry.get<Components::Faction>(e).id;
        break;
    }
}

void RtsGameBase::clientHandleDisconnect(const uint8_t *data, std::size_t size) {
    if (size < sizeof(DisconnectPacket))
        return;
    const auto *pkt = reinterpret_cast<const DisconnectPacket *>(data);
    switch (static_cast<DisconnectReason>(pkt->reason)) {
    case DisconnectReason::ServerShuttingDown:
        m_networkStatusMessage = "Server shut down.";
        break;
    case DisconnectReason::Kicked:
        m_networkStatusMessage = "You were kicked.";
        break;
    case DisconnectReason::GameOver:
        m_networkStatusMessage = "Game over.";
        break;
    default:
        m_networkStatusMessage = "Disconnected.";
        break;
    }
    std::cout << "[Client] Disconnected: " << m_networkStatusMessage << "\n";
}

void RtsGameBase::clientHandleConnectionRejected(const uint8_t *data, std::size_t size) {
    if (size < sizeof(ConnectionRejectedPacket))
        return;
    const auto *pkt = reinterpret_cast<const ConnectionRejectedPacket *>(data);
    switch (static_cast<RejectionReason>(pkt->reason)) {
    case RejectionReason::ServerFull:
        m_networkStatusMessage = "Server is full.";
        break;
    case RejectionReason::VersionMismatch:
        m_networkStatusMessage = "Version mismatch.";
        break;
    default:
        m_networkStatusMessage = "Connection rejected.";
        break;
    }
    std::cout << "[Client] Connection rejected: " << m_networkStatusMessage << "\n";
}

} // namespace Game
