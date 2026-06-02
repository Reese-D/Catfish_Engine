#ifndef RTS_GAME_H
#define RTS_GAME_H

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "components.h"
#include "engine.h"
#include "fog_of_war.h"
#include "hud_system.h"
#include "lava_zone.h"
#include "menu_system.h"
#include "model.h"
#include "network_manager.h"
#include "network_messages.h"
#include "pathfinder.h"
#include "spatial_grid.h"
#include "terrain.h"

namespace Game {

class RtsGame : public VulkanHelpers::IGame {
  public:
    // Feature toggles — call before run.
    void enablePathfinding(glm::vec2 worldMin = {-20, -20}, glm::vec2 worldMax = {20, 20}, float cellSize = 0.25f);
    void enableFogOfWar(glm::vec2 worldMin = {-20, -20}, glm::vec2 worldMax = {20, 20}, float cellSize = 1.0f, float sightRadius = 5.0f);
    void enableMinimap();
    void enableCombat();

    // Network setup — call before run.
    void setupAsServer(uint16_t port = 1234);
    void setupAsClient(std::string host, uint16_t port = 1234);

    // IGame interface
    void initLogic() override;
    void initGraphics(const VulkanHelpers::ResourceContext &ctx) override;
    VulkanHelpers::FrameOutput update(float dt, vk::Extent2D extent) override;
    void renderImGui(vk::CommandBuffer cmd) override;
    void onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) override;
    bool wantsMouse() const override;
    bool wantsKeyboard() const override;
    bool wantsClose() const override;

    // Reserved for the round system — not called during normal gameplay.
    entt::entity respawnUnit(Components::FactionId faction, glm::vec3 position);

  private:
    entt::entity spawnUnit(glm::vec3 position, Components::FactionId faction = Components::FactionId::Player);

    // Network event handlers
    void onClientConnect(ENetPeer *peer);
    void onClientDisconnect(ENetPeer *peer);
    void serverSendSnapshot();
    void serverHandleInput(const uint8_t *data, std::size_t size, ENetPeer *peer);
    void serverHandleHello(const uint8_t *data, std::size_t size, ENetPeer *peer);
    void serverSendConnectionRejected(ENetPeer *peer, RejectionReason reason);
    void serverSendDisconnect(ENetPeer *peer, DisconnectReason reason);
    void clientApplySnapshot(const uint8_t *data, std::size_t size);
    void clientHandleAssignment(const uint8_t *data, std::size_t size);
    void clientHandleDisconnect(const uint8_t *data, std::size_t size);
    void clientHandleConnectionRejected(const uint8_t *data, std::size_t size);
    void clientCaptureAndSendInput(vk::Extent2D extent);

    // Engine window reference (null for dedicated server)
    VulkanHelpers::Window *window{nullptr};

    // Vulkan resources — null until initGraphics() is called
    std::shared_ptr<VulkanHelpers::Model> unitModel;
    std::shared_ptr<VulkanHelpers::Terrain> terrain;
    std::shared_ptr<VulkanHelpers::Model> selectionRingModel;
    std::shared_ptr<VulkanHelpers::Model> projectileModel;
    std::shared_ptr<VulkanHelpers::Model> lavaTileModel;
    VulkanHelpers::HudResources hudResources;
    std::shared_ptr<VulkanHelpers::MenuSystem> menuSystem;

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
    enum class NetworkRole { Standalone, Server, Client };
    NetworkRole networkRole_{NetworkRole::Standalone};
    std::unique_ptr<Network::NetworkManager> networkManager_;
    uint32_t nextNetworkId_{1};
    uint32_t tick_{0};
    float snapshotTimer_{0.0f};
    static constexpr float SNAPSHOT_INTERVAL = 0.05f; // 20 Hz

    // Server: peers that have connected but not yet sent a valid Hello.
    std::unordered_set<ENetPeer *> pendingPeers_;
    // Server: maps each connected peer to the NetworkId of the unit they own.
    std::unordered_map<ENetPeer *, uint32_t>  peerToNetId_;
    // Shared: reverse lookup NetworkId → entity (kept in sync by spawnUnit/destroy).
    std::unordered_map<uint32_t, entt::entity> netIdToEntity_;

    // Client: status message set when the server rejects or disconnects us.
    std::string networkStatusMessage_;

    // Client: which entity this peer controls + input edge-detection state
    uint32_t              myNetworkId_{0};
    Components::FactionId myFaction_{Components::FactionId::Player};
    bool                  prevMouseRight_{false};
    bool                  prevKeyQ_{false};

    bool closeRequested{false};
};

} // namespace Game

#endif // RTS_GAME_H
