#include "fog_of_war.h"

#include <cmath>

#include "components.h"

namespace Systems {

FogOfWar::FogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius) : m_cellSize(cellSize), m_sightRadius(sightRadius), m_worldMin(worldMin) {
    m_dims = {
        static_cast<int>(std::ceil((worldMax.x - worldMin.x) / cellSize)),
        static_cast<int>(std::ceil((worldMax.y - worldMin.y) / cellSize)),
    };
    m_grid.assign(static_cast<std::size_t>(m_dims.x * m_dims.y), FogState::Hidden);
}

glm::ivec2 FogOfWar::toCell(glm::vec2 worldPos) const {
    return {
        static_cast<int>((worldPos.x - m_worldMin.x) / m_cellSize),
        static_cast<int>((worldPos.y - m_worldMin.y) / m_cellSize),
    };
}

bool FogOfWar::inBounds(glm::ivec2 cell) const { return cell.x >= 0 && cell.x < m_dims.x && cell.y >= 0 && cell.y < m_dims.y; }

std::size_t FogOfWar::cellIndex(glm::ivec2 cell) const { return static_cast<std::size_t>(cell.y * m_dims.x + cell.x); }

glm::vec2 FogOfWar::cellWorldMin(glm::ivec2 cell) const {
    return {
        m_worldMin.x + static_cast<float>(cell.x) * m_cellSize,
        m_worldMin.y + static_cast<float>(cell.y) * m_cellSize,
    };
}

glm::vec2 FogOfWar::cellWorldMax(glm::ivec2 cell) const {
    return {
        m_worldMin.x + static_cast<float>(cell.x + 1) * m_cellSize,
        m_worldMin.y + static_cast<float>(cell.y + 1) * m_cellSize,
    };
}

FogState FogOfWar::stateAt(glm::ivec2 cell) const {
    if (!inBounds(cell))
        return FogState::Hidden;
    return m_grid[cellIndex(cell)];
}

FogState FogOfWar::stateAt(glm::vec2 worldPos) const { return stateAt(toCell(worldPos)); }

bool FogOfWar::isVisible(glm::vec2 worldPos) const { return stateAt(worldPos) == FogState::Visible; }

void FogOfWar::update(entt::registry &registry, Components::FactionId viewerFaction) {
    for (auto &cell : m_grid) {
        if (cell == FogState::Visible)
            cell = FogState::Fogged;
    }

    int cellRadius = static_cast<int>(std::ceil(m_sightRadius / m_cellSize));
    float radiusSq = m_sightRadius * m_sightRadius;

    for (auto entity : registry.view<Components::Transform, Components::Faction>()) {
        if (registry.get<Components::Faction>(entity).id != viewerFaction)
            continue;

        const auto &pos = registry.get<Components::Transform>(entity).position;
        auto center = toCell({pos.x, pos.y});

        for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
            for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
                glm::ivec2 cell{center.x + dx, center.y + dy};
                if (!inBounds(cell))
                    continue;

                glm::vec2 cellCenter{
                    m_worldMin.x + (static_cast<float>(cell.x) + 0.5f) * m_cellSize,
                    m_worldMin.y + (static_cast<float>(cell.y) + 0.5f) * m_cellSize,
                };
                float dx2 = cellCenter.x - pos.x;
                float dy2 = cellCenter.y - pos.y;
                if (dx2 * dx2 + dy2 * dy2 <= radiusSq)
                    m_grid[cellIndex(cell)] = FogState::Visible;
            }
        }
    }
}

} // namespace Systems
