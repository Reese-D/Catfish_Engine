#include "minimap_system.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "components.h"

namespace Systems {

namespace {

constexpr float WORLD_MIN = -20.0f;
constexpr float WORLD_MAX = 20.0f;
constexpr float WORLD_SIZE = 40.0f;
constexpr float MM_SIZE = 180.0f;
constexpr float MM_PADDING = 10.0f;

ImVec2 toMinimap(glm::vec2 worldPos, ImVec2 origin) {
    return {
        origin.x + (worldPos.x - WORLD_MIN) / WORLD_SIZE * MM_SIZE,
        origin.y + (WORLD_MAX - worldPos.y) / WORLD_SIZE * MM_SIZE,
    };
}

glm::vec2 fromMinimap(ImVec2 pixelPos, ImVec2 origin) {
    return {
        WORLD_MIN + (pixelPos.x - origin.x) / MM_SIZE * WORLD_SIZE,
        WORLD_MAX - (pixelPos.y - origin.y) / MM_SIZE * WORLD_SIZE,
    };
}

std::optional<glm::vec2> unprojectToGround(glm::vec2 ndc, const Components::Camera &cam) {
    glm::mat4 invVP = glm::inverse(cam.proj * cam.view);
    glm::vec4 nearW = invVP * glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
    glm::vec4 farW = invVP * glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);
    nearW /= nearW.w;
    farW /= farW.w;

    glm::vec3 origin = glm::vec3(nearW);
    glm::vec3 dir = glm::normalize(glm::vec3(farW) - origin);
    if (std::abs(dir.z) < 1e-6f)
        return std::nullopt;
    float t = -origin.z / dir.z;
    if (t < 0.0f)
        return std::nullopt;
    glm::vec3 hit = origin + t * dir;
    return glm::vec2{hit.x, hit.y};
}

} // namespace

void drawMinimap(const FogOfWar *fog, entt::registry &registry, vk::Extent2D extent) {
    const Components::Camera *cam = nullptr;
    entt::entity camEntity = entt::null;
    for (auto entity : registry.view<Components::Camera>()) {
        cam = &registry.get<Components::Camera>(entity);
        camEntity = entity;
        break;
    }
    if (!cam)
        return;

    const float fw = static_cast<float>(extent.width);
    const float fh = static_cast<float>(extent.height);
    ImGui::SetNextWindowPos({fw - MM_SIZE - MM_PADDING * 2.0f, fh - MM_SIZE - MM_PADDING * 2.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({MM_SIZE + MM_PADDING * 2.0f, MM_SIZE + MM_PADDING * 2.0f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    ImGui::Begin("##minimap", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 origin = {winPos.x + MM_PADDING, winPos.y + MM_PADDING};
    ImDrawList *dl = ImGui::GetWindowDrawList();

    // Terrain background
    dl->AddRectFilled(origin, {origin.x + MM_SIZE, origin.y + MM_SIZE}, IM_COL32(45, 65, 35, 255));

    // Fog cells (skipped when fog is disabled — everything shows as visible)
    if (fog) {
        auto dims = fog->dims();
        float cellPixels = MM_SIZE / static_cast<float>(dims.x);
        for (int y = 0; y < dims.y; ++y) {
            for (int x = 0; x < dims.x; ++x) {
                FogState state = fog->stateAt(glm::ivec2{x, y});
                if (state == FogState::Visible)
                    continue;

                ImU32 col = (state == FogState::Hidden) ? IM_COL32(0, 0, 0, 255) : IM_COL32(0, 0, 0, 160);

                float px = origin.x + static_cast<float>(x) * cellPixels;
                float py = origin.y + static_cast<float>(dims.y - 1 - y) * cellPixels;
                dl->AddRectFilled({px, py}, {px + cellPixels, py + cellPixels}, col);
            }
        }
    }

    // Border
    dl->AddRect(origin, {origin.x + MM_SIZE, origin.y + MM_SIZE}, IM_COL32(160, 160, 160, 220), 0.0f, 0, 1.5f);

    // Camera frustum
    const glm::vec2 ndcCorners[4] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    ImVec2 frustum[4];
    bool frustumValid = true;
    for (int i = 0; i < 4; ++i) {
        auto hit = unprojectToGround(ndcCorners[i], *cam);
        if (!hit) {
            frustumValid = false;
            break;
        }
        frustum[i] = toMinimap(*hit, origin);
    }
    if (frustumValid)
        dl->AddPolyline(frustum, 4, IM_COL32(255, 230, 80, 200), ImDrawFlags_Closed, 1.5f);

    // Units
    for (auto entity : registry.view<Components::Transform, Components::Faction>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        const auto &faction = registry.get<Components::Faction>(entity);

        bool isPlayer = (faction.id == Components::FactionId::Player);
        if (!isPlayer && fog && !fog->isVisible({t.position.x, t.position.y}))
            continue;

        ImU32 color = isPlayer ? IM_COL32(80, 200, 80, 255) : IM_COL32(220, 60, 60, 255);
        ImVec2 dot = toMinimap({t.position.x, t.position.y}, origin);
        dl->AddCircleFilled(dot, 3.0f, color);
    }

    // Click to pan camera
    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        glm::vec2 worldXY = fromMinimap(ImGui::GetMousePos(), origin);
        worldXY.x = glm::clamp(worldXY.x, WORLD_MIN, WORLD_MAX);
        worldXY.y = glm::clamp(worldXY.y, WORLD_MIN, WORLD_MAX);

        auto &mutableCam = registry.get<Components::Camera>(camEntity);
        glm::vec3 offset = mutableCam.position - mutableCam.target;
        mutableCam.target = {worldXY.x, worldXY.y, 0.0f};
        mutableCam.position = mutableCam.target + offset;
    }

    ImGui::End();
}

} // namespace Systems
