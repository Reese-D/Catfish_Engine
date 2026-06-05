#include <cmath>

#include <glm/glm.hpp>

#include "components.h"
#include "spatial_grid.h"

namespace VulkanHelpers {

SpatialGrid::SpatialGrid(float m_cellSize, glm::vec2 min, glm::vec2 max) : m_cellSize(m_cellSize), m_gridMin(min) {
    m_gridDims = {
        static_cast<int>(std::ceil((max.x - min.x) / m_cellSize)),
        static_cast<int>(std::ceil((max.y - min.y) / m_cellSize)),
    };
    m_cells.resize(static_cast<std::size_t>(m_gridDims.x) * m_gridDims.y);
}

void SpatialGrid::update(entt::registry &registry) {
    for (auto &cell : m_cells)
        cell.clear();

    for (auto entity : registry.view<Components::Transform, Components::Selectable>()) {
        const auto &t = registry.get<Components::Transform>(entity);
        auto cell = worldToCell({t.position.x, t.position.y});
        if (inBounds(cell)) {
            m_cells[cellIndex(cell)].push_back(entity);
        }
    }
}

std::vector<entt::entity> SpatialGrid::queryRadius(glm::vec2 center, float radius) const {
    std::vector<entt::entity> result;

    glm::ivec2 lo = worldToCell(center - glm::vec2(radius));
    glm::ivec2 hi = worldToCell(center + glm::vec2(radius));
    lo = glm::max(lo, glm::ivec2(0));
    hi = glm::min(hi, m_gridDims - 1);

    for (int cy = lo.y; cy <= hi.y; ++cy) {
        for (int cx = lo.x; cx <= hi.x; ++cx) {
            for (auto entity : m_cells[cellIndex({cx, cy})]) {
                result.push_back(entity);
            }
        }
    }

    // Returns all candidates in the overlapping m_cells.
    // For exact distance filtering, the caller checks registry.get<Transform>().
    return result;
}

std::vector<entt::entity> SpatialGrid::queryRect(glm::vec2 min, glm::vec2 max) const {
    std::vector<entt::entity> result;

    glm::ivec2 lo = glm::max(worldToCell(min), glm::ivec2(0));
    glm::ivec2 hi = glm::min(worldToCell(max), m_gridDims - 1);

    for (int cy = lo.y; cy <= hi.y; ++cy) {
        for (int cx = lo.x; cx <= hi.x; ++cx) {
            for (auto entity : m_cells[cellIndex({cx, cy})]) {
                result.push_back(entity);
            }
        }
    }

    return result;
}

glm::ivec2 SpatialGrid::worldToCell(glm::vec2 pos) const {
    return {
        static_cast<int>((pos.x - m_gridMin.x) / m_cellSize),
        static_cast<int>((pos.y - m_gridMin.y) / m_cellSize),
    };
}

bool SpatialGrid::inBounds(glm::ivec2 cell) const { return cell.x >= 0 && cell.y >= 0 && cell.x < m_gridDims.x && cell.y < m_gridDims.y; }

std::size_t SpatialGrid::cellIndex(glm::ivec2 cell) const { return static_cast<std::size_t>(cell.y) * m_gridDims.x + cell.x; }

} // namespace VulkanHelpers
