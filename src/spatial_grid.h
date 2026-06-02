#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <glm/glm.hpp>
#include <vector>

#include <entt/entt.hpp>

namespace VulkanHelpers {

// 2D uniform grid for fast spatial queries on Selectable entities.
// Updated each frame from current Transform positions.
class SpatialGrid {
  public:
    SpatialGrid(float cellSize, glm::vec2 min, glm::vec2 max);

    // Repopulate from all entities with Transform + Selectable components.
    void update(entt::registry &registry);

    // Returns entities within radius of center (XY plane, Z ignored).
    std::vector<entt::entity> queryRadius(glm::vec2 center, float radius) const;

    // Returns entities whose XY position falls inside [min, max].
    std::vector<entt::entity> queryRect(glm::vec2 min, glm::vec2 max) const;

  private:
    glm::ivec2 worldToCell(glm::vec2 pos) const;
    bool inBounds(glm::ivec2 cell) const;
    std::size_t cellIndex(glm::ivec2 cell) const;

    float cellSize;
    glm::vec2 gridMin;
    glm::ivec2 gridDims;

    std::vector<std::vector<entt::entity>> cells;
};

} // namespace VulkanHelpers

#endif // SPATIAL_GRID_H
