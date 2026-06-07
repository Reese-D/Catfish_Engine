#include "fog_system.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <imgui.h>

#include "components.h"

namespace Systems {

namespace {

constexpr float kSentinel = -9999.0f;

ImVec2 worldToScreen(glm::vec3 worldPos, const Components::Camera &cam, vk::Extent2D extent) {
    glm::vec4 clip = cam.proj * cam.view * glm::vec4(worldPos, 1.0f);
    if (clip.w <= 0.0f)
        return {kSentinel, kSentinel};
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    // Skip near-horizon cells whose NDC coords blow up (perspective singularity).
    if (std::abs(ndc.x) > 8.0f || std::abs(ndc.y) > 8.0f)
        return {kSentinel, kSentinel};
    return {
        (ndc.x + 1.0f) * 0.5f * static_cast<float>(extent.width),
        (ndc.y + 1.0f) * 0.5f * static_cast<float>(extent.height),
    };
}

} // namespace

void drawFogOverlay(const FogOfWar &fog, entt::registry &registry, vk::Extent2D extent) {
    const Components::Camera *cam = nullptr;
    for (auto entity : registry.view<Components::Camera>()) {
        cam = &registry.get<Components::Camera>(entity);
        break;
    }
    if (!cam)
        return;

    const float fw = static_cast<float>(extent.width);
    const float fh = static_cast<float>(extent.height);
    const ImU32 hiddenColor = IM_COL32(0, 0, 0, 255);
    const ImU32 foggedColor = IM_COL32(0, 0, 0, 180);

    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    auto dims = fog.dims();

    for (int y = 0; y < dims.y; ++y) {
        for (int x = 0; x < dims.x; ++x) {
            glm::ivec2 cell{x, y};
            FogState state = fog.stateAt(cell);
            if (state == FogState::Visible)
                continue;

            glm::vec2 wMin = fog.cellWorldMin(cell);
            glm::vec2 wMax = fog.cellWorldMax(cell);

            ImVec2 s0 = worldToScreen({wMin.x, wMin.y, 0.0f}, *cam, extent);
            ImVec2 s1 = worldToScreen({wMax.x, wMin.y, 0.0f}, *cam, extent);
            ImVec2 s2 = worldToScreen({wMax.x, wMax.y, 0.0f}, *cam, extent);
            ImVec2 s3 = worldToScreen({wMin.x, wMax.y, 0.0f}, *cam, extent);

            // Skip if any corner hit a projection singularity (near-horizon cell).
            if (s0.x == kSentinel || s1.x == kSentinel || s2.x == kSentinel || s3.x == kSentinel)
                continue;

            float minX = std::min({s0.x, s1.x, s2.x, s3.x});
            float maxX = std::max({s0.x, s1.x, s2.x, s3.x});
            float minY = std::min({s0.y, s1.y, s2.y, s3.y});
            float maxY = std::max({s0.y, s1.y, s2.y, s3.y});
            if (maxX < 0.0f || minX > fw || maxY < 0.0f || minY > fh)
                continue;

            ImU32 color = (state == FogState::Hidden) ? hiddenColor : foggedColor;
            dl->AddQuadFilled(s0, s1, s2, s3, color);
        }
    }
}

} // namespace Systems
