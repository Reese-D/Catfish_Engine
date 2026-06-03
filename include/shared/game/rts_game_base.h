#ifndef RTS_GAME_BASE_H
#define RTS_GAME_BASE_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "components.h"
#include "fog_of_war.h"
#include "lava_zone.h"
#include "network_manager.h"
#include "network_messages.h"
#include "pathfinder.h"
#include "spatial_grid.h"

namespace Game {

class RtsGameBase {
  public:
    virtual ~RtsGameBase() = default;

    // Feature toggles — call before run.
    void enablePathfinding(glm::vec2 worldMin = {-20, -20}, glm::vec2 worldMax = {20, 20}, float cellSize = 0.25f);
    void enableFogOfWar(glm::vec2 worldMin = {-20, -20}, glm::vec2 worldMax = {20, 20}, float cellSize = 1.0f, float sightRadius = 5.0f);
    void enableMinimap();
    void enableCombat();

    // Network setup — call before run.
    void setupAsServer(uint16_t port = 1234);
    void setupAsClient(std::string host, uint16_t port = 1234);

    // Accessible for round system.
    entt::entity respawnUnit(Components::FactionId faction, glm::vec3 position);

  protected:
    entt::entity spawnUnit(glm::vec3 position, Components::FactionId faction = Components::FactionId::Player);

    // Server-side network event handlers
    void onClientConnect(ENetPeer *peer);
    void onClientDisconnect(ENetPeer *peer);
    void serverSendSnapshot();
    void serverHandleInput(const uint8_t *data, std::size_t size, ENetPeer *peer);
    void serverHandleHello(const uint8_t *data, std::size_t size, ENetPeer *peer);
    void serverSendConnectionRejected(ENetPeer *peer, RejectionReason reason);
    void serverSendDisconnect(ENetPeer *peer, DisconnectReason reason);

    // Client-side network event handlers (no GPU resources)
    void clientHandleAssignment(const uint8_t *data, std::size_t size);
    void clientHandleDisconnect(const uint8_t *data, std::size_t size);
    void clientHandleConnectionRejected(const uint8_t *data, std::size_t size);

    // ECS
    entt::registry registry;
    VulkanHelpers::SpatialGrid spatialGrid{2.0f, {WorldBounds::kMin, WorldBounds::kMin}, {WorldBounds::kMax, WorldBounds::kMax}};

    // Optional features
    std::optional<Systems::Pathfinder> pathfinder;
    std::optional<Systems::FogOfWar> fogOfWar;
    bool minimapEnabled{false};
    bool combatEnabled{false};

    Systems::LavaZone lavaZone{19.0f, 15.0f, 1.5f, 15.0f};

    // Networking
    std::unique_ptr<Network::NetworkManager> networkManager_;
    uint32_t nextNetworkId_{1};
    uint32_t tick_{0};
    float snapshotTimer_{0.0f};
    static constexpr float SNAPSHOT_INTERVAL = 0.05f;

    // Server state
    std::unordered_set<ENetPeer *> pendingPeers_;
    std::unordered_map<ENetPeer *, uint32_t> peerToNetId_;

    // Shared reverse lookup
    std::unordered_map<uint32_t, entt::entity> netIdToEntity_;

    // Client state (set by clientHandleAssignment)
    std::string networkStatusMessage_;
    uint32_t myNetworkId_{0};
    Components::FactionId myFaction_{Components::FactionId::Player};

    bool closeRequested{false};
};

} // namespace Game

#endif // RTS_GAME_BASE_H
