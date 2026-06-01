#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "camera_system.h"
#include "combat_system.h"
#include "components.h"
#include "fog_of_war.h"
#include "fog_system.h"
#include "hud_system.h"
#include "input_system.h"
#include "minimap_system.h"
#include "movement_system.h"
#include "death_system.h"
#include "lava_system.h"
#include "order_system.h"
#include "projectile_system.h"
#include "render_system.h"
#include "rts_game.h"
#include "selection_system.h"

namespace Game {

// --- Feature toggles (call before init) ---

void RtsGame::enablePathfinding(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize) {
    pathfinder.emplace(worldMin, worldMax, cellSize);
}

void RtsGame::enableFogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius) {
    fogOfWar.emplace(worldMin, worldMax, cellSize, sightRadius);
}

void RtsGame::enableMinimap() {
    minimapEnabled = true;
}

void RtsGame::enableCombat() {
    combatEnabled = true;
}

// --- IGame interface ---

void RtsGame::init(const VulkanHelpers::ResourceContext &ctx) {
    window = &ctx.window;

    unitModel = std::make_shared<VulkanHelpers::Model>(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue,
        ctx.textureLayout, "models/goblin.glb"
    );

    terrain = std::make_shared<VulkanHelpers::Terrain>(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout
    );

    hudResources = VulkanHelpers::createHudResources(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout
    );

    selectionRingModel = VulkanHelpers::createSelectionRingModel(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout
    );

    projectileModel = Systems::createProjectileModel(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout
    );

    lavaTileModel = Systems::createLavaTileModel(
        ctx.device, ctx.physicalDevice, ctx.commandPool, ctx.graphicsQueue, ctx.textureLayout
    );

    menuSystem = std::make_shared<VulkanHelpers::MenuSystem>(
        ctx.window, ctx.instance, ctx.physicalDevice, ctx.device,
        ctx.graphicsQueueFamilyIndex, ctx.graphicsQueue, ctx.swapChain, ctx.depthFormat
    );

    initScene();
}

VulkanHelpers::FrameOutput RtsGame::update(float dt, vk::Extent2D extent) {
    menuSystem->beginFrame();

    VulkanHelpers::FrameOutput out;

    if (menuSystem->isGameplayStarted()) {
        if (!menuSystem->wantsKeyboard())
            Systems::updateCameraInput(registry, *window, dt);

        lavaZone.update(dt);
        Systems::applyLavaDamage(lavaZone, registry, dt);
        if (combatEnabled) Systems::processCombat(registry, dt);
        Systems::processDeath(registry);
        Systems::tickAbilities(registry, dt);
        Systems::updateProjectiles(registry, dt);
        Systems::processOrders(registry, dt, pathfinder ? &*pathfinder : nullptr);
        Systems::applyKnockback(registry, dt);
        Systems::applySeparation(registry);
        Systems::clampToBounds(registry, {-20.0f, -20.0f}, {20.0f, 20.0f});
        Systems::updateCamera(registry, extent);

        if (!menuSystem->wantsMouse()) {
            Systems::updateSelection(registry, *window, extent);
            Systems::processAbilityInput(registry, *window, extent, projectileModel);
        }

        spatialGrid.update(registry);

        const Systems::FogOfWar *fog = fogOfWar ? &*fogOfWar : nullptr;
        if (fogOfWar) fogOfWar->update(registry);

        out.draws = Systems::collectDrawCalls(registry, fog);
        Systems::appendLavaDrawCalls(lavaZone, out.draws, *lavaTileModel);
        Systems::appendSelectionRings(registry, out.draws, *selectionRingModel, fog);
        Systems::appendHealthBars(registry, out.draws, hudResources, fog);

        menuSystem->drawOverlay(dt);
        if (fogOfWar)  Systems::drawFogOverlay(*fogOfWar, registry, extent);
        if (minimapEnabled) Systems::drawMinimap(fog, registry, extent);

    } else if (menuSystem->drawMainMenu(extent) == VulkanHelpers::MenuAction::Exit) {
        closeRequested = true;
    }

    // Read camera matrices from the Camera component (set by updateCamera above).
    for (auto entity : registry.view<Components::Camera>()) {
        const auto &cam = registry.get<Components::Camera>(entity);
        out.view = cam.view;
        out.proj = cam.proj;
        break;
    }

    return out;
}

void RtsGame::renderImGui(vk::CommandBuffer cmd) {
    menuSystem->render(cmd);
}

void RtsGame::onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) {
    menuSystem->onSwapChainRecreated(swapChain);
}

bool RtsGame::wantsMouse()    const { return menuSystem && menuSystem->wantsMouse(); }
bool RtsGame::wantsKeyboard() const { return menuSystem && menuSystem->wantsKeyboard(); }
bool RtsGame::wantsClose()    const { return closeRequested; }

// --- Private helpers ---

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
    registry.emplace<Components::Transform>(e, Components::Transform{
        .position = position,
        .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        .scale    = {1.0f, 1.0f, 1.0f},
    });
    registry.emplace<Components::RenderMesh>(e, Components::RenderMesh{unitModel});
    registry.emplace<Components::Selectable>(e);
    registry.emplace<Components::MovementSpeed>(e);
    registry.emplace<Components::OrderQueue>(e);
    registry.emplace<Components::Faction>(e, Components::Faction{faction});
    registry.emplace<Components::Health>(e);
    registry.emplace<Components::Ability>(e);
    if (combatEnabled)
        registry.emplace<Components::Combat>(e);
    return e;
}

void RtsGame::initScene() {
    auto camEntity = registry.create();
    registry.emplace<Components::Camera>(camEntity, Components::Camera{
        .position = {0.0f, -12.0f, 14.0f},
        .target   = {0.0f,   0.0f,  0.0f},
        .fov      = 50.0f,
        .near_    = 0.1f,
        .far_     = 200.0f,
    });

    auto terrainEntity = registry.create();
    registry.emplace<Components::Transform>(terrainEntity);
    registry.emplace<Components::RenderMesh>(terrainEntity, Components::RenderMesh{terrain->getModelPtr()});

    auto player = spawnUnit({-3.0f, 0.0f, 0.0f}, Components::FactionId::Player);
    registry.emplace<Components::AlwaysSelected>(player);
    registry.emplace<Components::Selected>(player);

    spawnUnit({ 3.0f, 0.0f, 0.0f}, Components::FactionId::Enemy);
}

} // namespace Game
