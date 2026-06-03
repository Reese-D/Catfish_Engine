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
    if (networkManager_) {
        networkManager_->poll(
            [this](const uint8_t *data, std::size_t size, ENetPeer *peer) {
                if (size == 0) return;
                auto type = static_cast<MessageType>(data[0]);
                if (type == MessageType::Input) serverHandleInput(data, size, peer);
                if (type == MessageType::Hello)  serverHandleHello(data, size, peer);
            },
            [this](ENetPeer *peer) { onClientConnect(peer); },
            [this](ENetPeer *peer) { onClientDisconnect(peer); }
        );
    }

    ++tick_;

    lavaZone.update(dt);
    Systems::applyLavaDamage(lavaZone, registry, dt);
    if (combatEnabled)
        Systems::processCombat(registry, dt);
    Systems::processDeath(registry);
    Systems::tickAbilities(registry, dt);
    Systems::updateProjectiles(registry, dt);
    Systems::applyThrust(registry, dt);
    Systems::applyVelocity(registry, dt);
    Systems::applySeparation(registry);
    Systems::clampToBounds(registry, {WorldBounds::kMin, WorldBounds::kMin}, {WorldBounds::kMax, WorldBounds::kMax});

    spatialGrid.update(registry);

    snapshotTimer_ += dt;
    if (snapshotTimer_ >= SNAPSHOT_INTERVAL) {
        snapshotTimer_ -= SNAPSHOT_INTERVAL;
        serverSendSnapshot();
    }
}

} // namespace Game
