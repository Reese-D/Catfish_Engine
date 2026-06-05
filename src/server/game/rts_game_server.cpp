#include "rts_game_server.h"

#include <iostream>

#include "combat_system.h"
#include "death_system.h"
#include "lava_sim.h"
#include "movement_system.h"
#include "network_messages.h"
#include "projectile_sim.h"

namespace Game {

void RtsGameServer::initLogic() {}

void RtsGameServer::update(float dt) {
    if (m_networkManager) {
        m_networkManager->poll(
            [this](const uint8_t *data, std::size_t size, ENetPeer *peer) {
                if (size == 0)
                    return;
                auto type = static_cast<MessageType>(data[0]);
                if (type == MessageType::Input)
                    serverHandleInput(data, size, peer);
                if (type == MessageType::Hello)
                    serverHandleHello(data, size, peer);
            },
            [this](ENetPeer *peer) { onClientConnect(peer); }, [this](ENetPeer *peer) { onClientDisconnect(peer); }
        );
    }

    ++m_tick;

    m_lavaZone.update(dt);
    Systems::applyLavaDamage(m_lavaZone, m_registry, dt);
    if (m_combatEnabled)
        Systems::processCombat(m_registry, dt);
    Systems::processDeath(m_registry);
    Systems::tickAbilities(m_registry, dt);
    Systems::updateProjectiles(m_registry, dt);
    Systems::applyThrust(m_registry, dt);
    Systems::applyVelocity(m_registry, dt);
    Systems::applySeparation(m_registry);
    Systems::clampToBounds(m_registry, {WorldBounds::kMin, WorldBounds::kMin}, {WorldBounds::kMax, WorldBounds::kMax});

    m_spatialGrid.update(m_registry);

    m_snapshotTimer += dt;
    if (m_snapshotTimer >= kSnapshotInterval) {
        m_snapshotTimer -= kSnapshotInterval;
        serverSendSnapshot();
    }
}

} // namespace Game
