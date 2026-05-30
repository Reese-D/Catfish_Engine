#include <cmath>

#include <glm/glm.hpp>

#include "components.h"
#include "spatial_grid.h"

namespace VulkanHelpers {

SpatialGrid::SpatialGrid(float cellSize, glm::vec2 min, glm::vec2 max)
    : cellSize(cellSize), gridMin(min) {
    gridDims = {
        static_cast<int>(std::ceil((max.x - min.x) / cellSize)),
        static_cast<int>(std::ceil((max.y - min.y) / cellSize)),
    };
    cells.resize(static_cast<std::size_t>(gridDims.x) * gridDims.y);
}

void SpatialGrid::update(entt::registry &registry) {
    for (auto &cell : cells) cell.clear();

    for (auto entity : registry.view<Components::Transform, Components::Selectable>()) {
        const auto &t    = registry.get<Components::Transform>(entity);
        auto         cell = worldToCell({t.position.x, t.position.y});
        if (inBounds(cell)) {
            cells[cellIndex(cell)].push_back(entity);
        }
    }
}

std::vector<entt::entity> SpatialGrid::queryRadius(glm::vec2 center, float radius) const {
    std::vector<entt::entity> result;

    glm::ivec2 lo = worldToCell(center - glm::vec2(radius));
    glm::ivec2 hi = worldToCell(center + glm::vec2(radius));
    lo = glm::max(lo, glm::ivec2(0));
    hi = glm::min(hi, gridDims - 1);

    for (int cy = lo.y; cy <= hi.y; ++cy) {
        for (int cx = lo.x; cx <= hi.x; ++cx) {
            for (auto entity : cells[cellIndex({cx, cy})]) {
                result.push_back(entity);
            }
        }
    }

    // Returns all candidates in the overlapping cells.
    // For exact distance filtering, the caller checks registry.get<Transform>().
    return result;
}

std::vector<entt::entity> SpatialGrid::queryRect(glm::vec2 min, glm::vec2 max) const {
    std::vector<entt::entity> result;

    glm::ivec2 lo = glm::max(worldToCell(min), glm::ivec2(0));
    glm::ivec2 hi = glm::min(worldToCell(max), gridDims - 1);

    for (int cy = lo.y; cy <= hi.y; ++cy) {
        for (int cx = lo.x; cx <= hi.x; ++cx) {
            for (auto entity : cells[cellIndex({cx, cy})]) {
                result.push_back(entity);
            }
        }
    }

    return result;
}

glm::ivec2 SpatialGrid::worldToCell(glm::vec2 pos) const {
    return {
        static_cast<int>((pos.x - gridMin.x) / cellSize),
        static_cast<int>((pos.y - gridMin.y) / cellSize),
    };
}

bool SpatialGrid::inBounds(glm::ivec2 cell) const {
    return cell.x >= 0 && cell.y >= 0 && cell.x < gridDims.x && cell.y < gridDims.y;
}

std::size_t SpatialGrid::cellIndex(glm::ivec2 cell) const {
    return static_cast<std::size_t>(cell.y) * gridDims.x + cell.x;
}

} // namespace VulkanHelpers
