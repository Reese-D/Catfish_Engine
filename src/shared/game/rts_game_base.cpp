#include "rts_game_base.h"

#include <array>
#include <iostream>

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

static constexpr std::array<glm::vec3, 4> SPAWN_POSITIONS = {{
    {-3.0f, 0.0f, 0.0f},
    { 3.0f, 0.0f, 0.0f},
    {-3.0f, 3.0f, 0.0f},
    { 3.0f, 3.0f, 0.0f},
}};

static constexpr std::array<Components::FactionId, 2> SLOT_FACTIONS = {{
    Components::FactionId::Player,
    Components::FactionId::Enemy,
}};

// ---- Feature toggles -------------------------------------------------------

void RtsGameBase::enablePathfinding(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize) { pathfinder.emplace(worldMin, worldMax, cellSize); }
void RtsGameBase::enableFogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius) { fogOfWar.emplace(worldMin, worldMax, cellSize, sightRadius); }
void RtsGameBase::enableMinimap() { minimapEnabled = true; }
void RtsGameBase::enableCombat() { combatEnabled = true; }

// ---- Network setup ---------------------------------------------------------

void RtsGameBase::setupAsServer(uint16_t port) {
    networkManager_ = std::make_unique<Network::NetworkManager>();
    networkManager_->startServer(port);
}

void RtsGameBase::setupAsClient(std::string host, uint16_t port) {
    networkManager_ = std::make_unique<Network::NetworkManager>();
    networkManager_->connectToServer(host, port);
}

// ---- Unit spawning ---------------------------------------------------------

entt::entity RtsGameBase::respawnUnit(Components::FactionId faction, glm::vec3 position) {
    auto e = spawnUnit(position, faction);
    if (faction == Components::FactionId::Player) {
        registry.emplace<Components::AlwaysSelected>(e);
        registry.emplace<Components::Selected>(e);
    }
    return e;
}

entt::entity RtsGameBase::spawnUnit(glm::vec3 position, Components::FactionId faction) {
    auto e = registry.create();
    registry.emplace<Components::Transform>(
        e, Components::Transform{
               .position = position,
               .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
               .scale = {1.0f, 1.0f, 1.0f},
           }
    );
    registry.emplace<Components::Selectable>(e);
    registry.emplace<Components::MovementSpeed>(e);
    registry.emplace<Components::Velocity>(e);
    registry.emplace<Components::ThrustDirection>(e);
    registry.emplace<Components::OrderQueue>(e);
    registry.emplace<Components::Faction>(e, Components::Faction{faction});
    registry.emplace<Components::Health>(e);
    registry.emplace<Components::AbilitySet>(e, Components::AbilitySet{
        .slots = {
            Components::AbilitySlot{Components::AbilityId::Projectile,  1.5f, 1.5f, 8.0f,  12.0f},
            Components::AbilitySlot{Components::AbilityId::GravityWell, 4.0f, 4.0f, 2.0f,  0.0f,  15.0f, 5.0f, 0.5f},
        }
    });
    if (combatEnabled)
        registry.emplace<Components::Combat>(e);
    auto netId = nextNetworkId_++;
    registry.emplace<Components::NetworkId>(e, Components::NetworkId{netId});
    netIdToEntity_[netId] = e;
    return e;
}

// ---- Network: server -------------------------------------------------------

void RtsGameBase::onClientConnect(ENetPeer *peer) {
    if (peerToNetId_.size() + pendingPeers_.size() >= SPAWN_POSITIONS.size()) {
        std::cout << "[Server] Full — rejecting incoming connection\n";
        serverSendConnectionRejected(peer, RejectionReason::ServerFull);
        return;
    }
    pendingPeers_.insert(peer);
    std::cout << "[Server] Peer connected, awaiting Hello\n";
}

void RtsGameBase::onClientDisconnect(ENetPeer *peer) {
    pendingPeers_.erase(peer);

    auto it = peerToNetId_.find(peer);
    if (it == peerToNetId_.end())
        return;

    uint32_t netId = it->second;
    auto entIt = netIdToEntity_.find(netId);
    if (entIt != netIdToEntity_.end()) {
        if (registry.valid(entIt->second))
            registry.destroy(entIt->second);
        netIdToEntity_.erase(entIt);
    }
    peerToNetId_.erase(it);
    std::cout << "[Server] Client disconnected, destroyed unit " << netId << "\n";
}

void RtsGameBase::serverSendConnectionRejected(ENetPeer *peer, RejectionReason reason) {
    ConnectionRejectedPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::ConnectionRejected);
    pkt.reason  = static_cast<uint8_t>(reason);
    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    networkManager_->sendReliableTo(peer, {raw, raw + sizeof(pkt)});
    enet_peer_disconnect_later(peer, 0);
}

void RtsGameBase::serverSendDisconnect(ENetPeer *peer, DisconnectReason reason) {
    DisconnectPacket pkt{};
    pkt.msgType = static_cast<uint8_t>(MessageType::Disconnect);
    pkt.reason  = static_cast<uint8_t>(reason);
    const auto *raw = reinterpret_cast<const uint8_t *>(&pkt);
    networkManager_->sendReliableTo(peer, {raw, raw + sizeof(pkt)});
    enet_peer_disconnect_later(peer, 0);
}

void RtsGameBase::serverHandleHello(const uint8_t *data, std::size_t size, ENetPeer *peer) {
    if (size < sizeof(HelloPacket))
        return;

    const auto *pkt = reinterpret_cast<const HelloPacket *>(data);
    if (pkt->protocolVersion != PROTOCOL_VERSION) {
        std::cout << "[Server] Version mismatch (client=" << pkt->protocolVersion
                  << " server=" << PROTOCOL_VERSION << ") — rejecting\n";
        serverSendConnectionRejected(peer, RejectionReason::VersionMismatch);
        return;
    }

    pendingPeers_.erase(peer);
    std::size_t slot = peerToNetId_.size();

    auto faction = (slot < SLOT_FACTIONS.size()) ? SLOT_FACTIONS[slot] : Components::FactionId::Enemy;
    auto unit = spawnUnit(SPAWN_POSITIONS[slot], faction);
    auto netId = registry.get<Components::NetworkId>(unit).id;
    peerToNetId_[peer] = netId;

    PlayerAssignmentPacket assignPkt{};
    assignPkt.msgType = static_cast<uint8_t>(MessageType::PlayerAssignment);
    assignPkt.yourNetworkId = netId;
    const auto *raw = reinterpret_cast<const uint8_t *>(&assignPkt);
    networkManager_->sendReliableTo(peer, {raw, raw + sizeof(assignPkt)});

    std::cout << "[Server] Client accepted → slot " << slot << ", NetworkId " << netId << "\n";
}

void RtsGameBase::serverSendSnapshot() {
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
        es.x = t.position.x; es.y = t.position.y; es.z = t.position.z;
        es.health = h.current; es.maxHealth = h.max;
        entities.push_back(es);
    }

    for (auto e : registry.view<Components::Transform, Components::Projectile, Components::NetworkId>()) {
        const auto &t = registry.get<Components::Transform>(e);
        const auto &p = registry.get<Components::Projectile>(e);
        const auto &n = registry.get<Components::NetworkId>(e);
        ProjectileSnapshot ps{};
        ps.netId = n.id;
        ps.faction = static_cast<uint8_t>(p.ownerFaction);
        ps.x = t.position.x; ps.y = t.position.y; ps.z = t.position.z;
        ps.vx = p.velocity.x; ps.vy = p.velocity.y; ps.vz = p.velocity.z;
        projectiles.push_back(ps);
    }

    hdr.entityCount = static_cast<uint8_t>(std::min(entities.size(), std::size_t{255}));
    hdr.projectileCount = static_cast<uint8_t>(std::min(projectiles.size(), std::size_t{255}));
    w.write(hdr);
    for (auto i = 0u; i < hdr.entityCount; ++i) w.write(entities[i]);
    for (auto i = 0u; i < hdr.projectileCount; ++i) w.write(projectiles[i]);

    networkManager_->broadcastUnreliable(w.buf());
}

void RtsGameBase::serverHandleInput(const uint8_t *data, std::size_t size, ENetPeer *peer) {
    BufReader r(data, size);
    InputPacket pkt{};
    if (!r.read(pkt))
        return;

    auto peerIt = peerToNetId_.find(peer);
    if (peerIt == peerToNetId_.end())
        return;

    auto entIt = netIdToEntity_.find(peerIt->second);
    if (entIt == netIdToEntity_.end() || !registry.valid(entIt->second))
        return;
    entt::entity clientEntity = entIt->second;

    if ((pkt.flags & InputFlags::MoveOrder) && registry.all_of<Components::ThrustDirection, Components::Transform>(clientEntity)) {
        const auto &t = registry.get<Components::Transform>(clientEntity);
        glm::vec2 delta{pkt.moveX - t.position.x, pkt.moveY - t.position.y};
        if (glm::length(delta) > 0.001f)
            registry.get<Components::ThrustDirection>(clientEntity).dir = glm::normalize(delta);
    }

    if ((pkt.flags & InputFlags::FireAbility) && registry.all_of<Components::AbilitySet, Components::Transform, Components::Faction>(clientEntity)) {
        auto &abilitySet = registry.get<Components::AbilitySet>(clientEntity);
        if (pkt.abilitySlot >= abilitySet.slots.size())
            return;

        auto &slot = abilitySet.slots[pkt.abilitySlot];
        if (slot.timer < slot.cooldown)
            return;

        const auto &t = registry.get<Components::Transform>(clientEntity);
        const auto &f = registry.get<Components::Faction>(clientEntity);
        glm::vec3 delta = glm::vec3{pkt.abilityX, pkt.abilityY, pkt.abilityZ} - t.position;
        delta.z = 0.0f;
        float len = glm::length(delta);
        if (len < 0.001f)
            return;

        glm::vec3 velocity = (delta / len) * slot.projectileSpeed;
        auto proj = Systems::spawnProjectile(registry, t.position, velocity, f.id, slot.knockbackForce);

        if (slot.id == Components::AbilityId::GravityWell) {
            registry.emplace<Components::GravityWell>(proj, Components::GravityWell{
                .pullStrength    = slot.pullStrength,
                .pullRadius      = slot.pullRadius,
                .activationDelay = slot.activationDelay,
            });
            registry.get<Components::Projectile>(proj).lifetime  = 5.0f;
            registry.get<Components::Projectile>(proj).hitRadius = 0.6f;
        }

        auto netId = nextNetworkId_++;
        registry.emplace<Components::NetworkId>(proj, Components::NetworkId{netId});
        netIdToEntity_[netId] = proj;
        slot.timer = 0.0f;
    }
}

// ---- Network: client (no GPU resources) ------------------------------------

void RtsGameBase::clientHandleAssignment(const uint8_t *data, std::size_t size) {
    BufReader r(data, size);
    PlayerAssignmentPacket pkt{};
    if (!r.read(pkt))
        return;
    myNetworkId_ = pkt.yourNetworkId;
    std::cout << "[Client] Assigned NetworkId " << myNetworkId_ << "\n";

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

void RtsGameBase::clientHandleDisconnect(const uint8_t *data, std::size_t size) {
    if (size < sizeof(DisconnectPacket))
        return;
    const auto *pkt = reinterpret_cast<const DisconnectPacket *>(data);
    switch (static_cast<DisconnectReason>(pkt->reason)) {
    case DisconnectReason::ServerShuttingDown: networkStatusMessage_ = "Server shut down."; break;
    case DisconnectReason::Kicked:             networkStatusMessage_ = "You were kicked.";  break;
    case DisconnectReason::GameOver:           networkStatusMessage_ = "Game over.";        break;
    default:                                   networkStatusMessage_ = "Disconnected.";     break;
    }
    std::cout << "[Client] Disconnected: " << networkStatusMessage_ << "\n";
}

void RtsGameBase::clientHandleConnectionRejected(const uint8_t *data, std::size_t size) {
    if (size < sizeof(ConnectionRejectedPacket))
        return;
    const auto *pkt = reinterpret_cast<const ConnectionRejectedPacket *>(data);
    switch (static_cast<RejectionReason>(pkt->reason)) {
    case RejectionReason::ServerFull:      networkStatusMessage_ = "Server is full.";      break;
    case RejectionReason::VersionMismatch: networkStatusMessage_ = "Version mismatch.";    break;
    default:                               networkStatusMessage_ = "Connection rejected."; break;
    }
    std::cout << "[Client] Connection rejected: " << networkStatusMessage_ << "\n";
}

} // namespace Game
