#include "fog_of_war.h"

#include <cmath>

#include "components.h"

namespace Systems {

FogOfWar::FogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius)
    : cellSize_(cellSize), sightRadius_(sightRadius), worldMin_(worldMin) {
    dims_ = {
        static_cast<int>(std::ceil((worldMax.x - worldMin.x) / cellSize)),
        static_cast<int>(std::ceil((worldMax.y - worldMin.y) / cellSize)),
    };
    grid_.assign(static_cast<std::size_t>(dims_.x * dims_.y), FogState::Hidden);
}

glm::ivec2 FogOfWar::toCell(glm::vec2 worldPos) const {
    return {
        static_cast<int>((worldPos.x - worldMin_.x) / cellSize_),
        static_cast<int>((worldPos.y - worldMin_.y) / cellSize_),
    };
}

bool FogOfWar::inBounds(glm::ivec2 cell) const {
    return cell.x >= 0 && cell.x < dims_.x && cell.y >= 0 && cell.y < dims_.y;
}

std::size_t FogOfWar::cellIndex(glm::ivec2 cell) const {
    return static_cast<std::size_t>(cell.y * dims_.x + cell.x);
}

glm::vec2 FogOfWar::cellWorldMin(glm::ivec2 cell) const {
    return {
        worldMin_.x + static_cast<float>(cell.x) * cellSize_,
        worldMin_.y + static_cast<float>(cell.y) * cellSize_,
    };
}

glm::vec2 FogOfWar::cellWorldMax(glm::ivec2 cell) const {
    return {
        worldMin_.x + static_cast<float>(cell.x + 1) * cellSize_,
        worldMin_.y + static_cast<float>(cell.y + 1) * cellSize_,
    };
}

FogState FogOfWar::stateAt(glm::ivec2 cell) const {
    if (!inBounds(cell)) return FogState::Hidden;
    return grid_[cellIndex(cell)];
}

FogState FogOfWar::stateAt(glm::vec2 worldPos) const {
    return stateAt(toCell(worldPos));
}

bool FogOfWar::isVisible(glm::vec2 worldPos) const {
    return stateAt(worldPos) == FogState::Visible;
}

void FogOfWar::update(entt::registry &registry) {
    for (auto &cell : grid_) {
        if (cell == FogState::Visible)
            cell = FogState::Fogged;
    }

    int   cellRadius = static_cast<int>(std::ceil(sightRadius_ / cellSize_));
    float radiusSq   = sightRadius_ * sightRadius_;

    for (auto entity : registry.view<Components::Transform, Components::Faction>()) {
        if (registry.get<Components::Faction>(entity).id != Components::FactionId::Player)
            continue;

        const auto &pos    = registry.get<Components::Transform>(entity).position;
        auto        center = toCell({pos.x, pos.y});

        for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
            for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
                glm::ivec2 cell{center.x + dx, center.y + dy};
                if (!inBounds(cell)) continue;

                glm::vec2 cellCenter{
                    worldMin_.x + (static_cast<float>(cell.x) + 0.5f) * cellSize_,
                    worldMin_.y + (static_cast<float>(cell.y) + 0.5f) * cellSize_,
                };
                float dx2  = cellCenter.x - pos.x;
                float dy2  = cellCenter.y - pos.y;
                if (dx2 * dx2 + dy2 * dy2 <= radiusSq)
                    grid_[cellIndex(cell)] = FogState::Visible;
            }
        }
    }
}

} // namespace Systems
