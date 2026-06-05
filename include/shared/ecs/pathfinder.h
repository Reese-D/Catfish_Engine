#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <glm/glm.hpp>
#include <vector>

namespace Systems {

class Pathfinder {
  public:
    Pathfinder(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize);

    // Returns world-space waypoints (z=0) from start to end. Empty if unreachable.
    std::vector<glm::vec3> findPath(glm::vec2 start, glm::vec2 end) const;

    void setBlocked(glm::vec2 worldPos, bool blocked);

  private:
    glm::ivec2 toCell(glm::vec2 worldPos) const;
    glm::vec2 toWorld(glm::ivec2 cell) const;
    bool inBounds(glm::ivec2 cell) const;
    bool isWalkable(glm::ivec2 cell) const;
    std::size_t cellIndex(glm::ivec2 cell) const;

    bool hasLineOfSight(glm::vec2 a, glm::vec2 b) const;
    std::vector<glm::vec3> smooth(const std::vector<glm::vec3> &raw) const;

    float m_cellSize;
    glm::vec2 m_worldMin;
    glm::ivec2 m_dims;
    std::vector<bool> m_blocked;
};

} // namespace Systems

#endif // PATHFINDER_H
