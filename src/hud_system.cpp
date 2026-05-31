#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "components.h"
#include "fog_of_war.h"
#include "hud_system.h"
#include "texture_image.h"
#include "vertex_buffer.h"

namespace {

// Flat quad of given world-space dimensions, centered at origin, in XY plane.
// CCW winding (from +Z) so it's visible from above.
std::shared_ptr<VulkanHelpers::Model> makeBarModel(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool,
    const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout,
    float width, float height,
    uint8_t r, uint8_t g, uint8_t b
) {
    std::array<unsigned char, 4> px = {r, g, b, 255};
    auto tex = std::make_shared<VulkanHelpers::TextureImage>(
        device, physicalDevice, commandPool, graphicsQueue,
        px.data(), 1, 1
    );

    float hw = width  * 0.5f;
    float hh = height * 0.5f;

    std::vector<VulkanHelpers::Vertex> verts(4);
    // bottom-left
    verts[0].pos[0] = -hw; verts[0].pos[1] = -hh; verts[0].pos[2] = 0.0f;
    verts[0].texCoord[0] = 0.0f; verts[0].texCoord[1] = 1.0f;
    // bottom-right
    verts[1].pos[0] =  hw; verts[1].pos[1] = -hh; verts[1].pos[2] = 0.0f;
    verts[1].texCoord[0] = 1.0f; verts[1].texCoord[1] = 1.0f;
    // top-right
    verts[2].pos[0] =  hw; verts[2].pos[1] =  hh; verts[2].pos[2] = 0.0f;
    verts[2].texCoord[0] = 1.0f; verts[2].texCoord[1] = 0.0f;
    // top-left
    verts[3].pos[0] = -hw; verts[3].pos[1] =  hh; verts[3].pos[2] = 0.0f;
    verts[3].texCoord[0] = 0.0f; verts[3].texCoord[1] = 0.0f;

    for (auto &v : verts) {
        v.color[0] = v.color[1] = v.color[2] = 1.0f;
    }

    // CCW from +Z: bl, br, tr, bl, tr, tl
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};

    return std::make_shared<VulkanHelpers::Model>(
        device, physicalDevice, commandPool, graphicsQueue,
        textureLayout, verts, indices, std::move(tex)
    );
}

} // namespace

namespace VulkanHelpers {

HudResources createHudResources(
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice,
    const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
    const vk::raii::DescriptorSetLayout &textureLayout
) {
    constexpr float W  = 0.80f; // bar width
    constexpr float BH = 0.14f; // background height
    constexpr float FH = 0.09f; // foreground height (inset)

    return HudResources{
        .barBackground = makeBarModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, W, BH,  50,  50,  50),
        .barGreen      = makeBarModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, W, FH,  60, 200,  60),
        .barYellow     = makeBarModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, W, FH, 220, 200,  40),
        .barRed        = makeBarModel(device, physicalDevice, commandPool, graphicsQueue, textureLayout, W, FH, 220,  50,  50),
    };
}

} // namespace VulkanHelpers

namespace Systems {

void appendHealthBars(
    entt::registry &registry,
    std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::HudResources &hud,
    const FogOfWar *fog
) {
    constexpr float barWidth = 0.80f;
    constexpr float barZ     = 1.60f; // world units above ground

    for (auto entity : registry.view<Components::Health, Components::Transform>()) {
        if (fog) {
            const auto *faction = registry.try_get<Components::Faction>(entity);
            if (faction && faction->id != Components::FactionId::Player) {
                const auto &t = registry.get<Components::Transform>(entity);
                if (!fog->isVisible({t.position.x, t.position.y})) continue;
            }
        }

        const auto &t      = registry.get<Components::Transform>(entity);
        const auto &health = registry.get<Components::Health>(entity);

        float ratio = (health.max > 0.0f)
                          ? glm::clamp(health.current / health.max, 0.0f, 1.0f)
                          : 0.0f;

        glm::vec3 centre{t.position.x, t.position.y, barZ};

        // Background — full width, slightly below foreground
        auto bgMat = glm::translate(glm::mat4(1.0f), centre);
        draws.push_back({hud.barBackground.get(), hud.barBackground->getMaterial().getDescriptorSet(), bgMat});

        if (ratio <= 0.0f) continue;

        // Foreground — left-anchored, scaled in X by health ratio
        float xOffset = barWidth * (ratio - 1.0f) * 0.5f; // shift left so bar fills from left edge
        auto fgPos    = glm::vec3(centre.x + xOffset, centre.y, barZ + 0.01f);
        auto fgMat    = glm::translate(glm::mat4(1.0f), fgPos) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(ratio, 1.0f, 1.0f));

        const VulkanHelpers::Model *fgModel =
            (ratio > 0.66f) ? hud.barGreen.get()
            : (ratio > 0.33f) ? hud.barYellow.get()
                              : hud.barRed.get();
        draws.push_back({fgModel, fgModel->getMaterial().getDescriptorSet(), fgMat});
    }
}

} // namespace Systems
