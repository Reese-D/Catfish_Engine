#ifndef RTS_GAME_H
#define RTS_GAME_H

#include <memory>
#include <optional>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "components.h"
#include "engine.h"
#include "fog_of_war.h"
#include "hud_system.h"
#include "menu_system.h"
#include "model.h"
#include "pathfinder.h"
#include "selection_ring.h"
#include "spatial_grid.h"
#include "terrain.h"

namespace Game {

class RtsGame : public VulkanHelpers::IGame {
  public:
    // Call any of these before init() to opt features in.
    void enablePathfinding(glm::vec2 worldMin = {-20,-20}, glm::vec2 worldMax = {20,20}, float cellSize = 0.5f);
    void enableFogOfWar(glm::vec2 worldMin = {-20,-20}, glm::vec2 worldMax = {20,20},
                        float cellSize = 1.0f, float sightRadius = 5.0f);
    void enableMinimap();   // works without fog too
    void enableCombat();    // re-enables melee combat system; off by default

    // IGame interface
    void        init(const VulkanHelpers::ResourceContext &ctx) override;
    VulkanHelpers::FrameOutput update(float dt, vk::Extent2D extent) override;
    void        renderImGui(vk::CommandBuffer cmd) override;
    void        onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) override;
    bool        wantsMouse()    const override;
    bool        wantsKeyboard() const override;
    bool        wantsClose()    const override;

  private:
    void        initScene();
    entt::entity spawnUnit(glm::vec3 position, Components::FactionId faction = Components::FactionId::Player);

    // Engine window reference (non-owning, valid for Engine lifetime)
    VulkanHelpers::Window *window{nullptr};

    // Vulkan resources owned by the game
    std::shared_ptr<VulkanHelpers::Model>      unitModel;
    std::shared_ptr<VulkanHelpers::Terrain>    terrain;
    std::shared_ptr<VulkanHelpers::Model>      selectionRingModel;
    VulkanHelpers::HudResources                hudResources;
    std::shared_ptr<VulkanHelpers::MenuSystem> menuSystem;

    // ECS
    entt::registry             registry;
    VulkanHelpers::SpatialGrid spatialGrid{2.0f, {-20.0f, -20.0f}, {20.0f, 20.0f}};

    // Projectile shared model (created in init)
    std::shared_ptr<VulkanHelpers::Model> projectileModel;

    // Optional features
    std::optional<Systems::Pathfinder> pathfinder;
    std::optional<Systems::FogOfWar>   fogOfWar;
    bool                               minimapEnabled{false};
    bool                               combatEnabled{false};

    bool closeRequested{false};
};

} // namespace Game

#endif // RTS_GAME_H
