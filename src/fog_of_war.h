#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace Systems {

enum class FogState : uint8_t { Hidden, Fogged, Visible };

class FogOfWar {
  public:
    FogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius);

    // Resets current Visible→Fogged, then illuminates around all Player-faction units.
    void update(entt::registry &registry);

    FogState stateAt(glm::vec2 worldPos) const;
    FogState stateAt(glm::ivec2 cell) const;
    bool     isVisible(glm::vec2 worldPos) const;

    // Grid accessors for the renderer
    glm::ivec2 dims() const { return dims_; }
    glm::vec2  cellWorldMin(glm::ivec2 cell) const;
    glm::vec2  cellWorldMax(glm::ivec2 cell) const;

  private:
    glm::ivec2  toCell(glm::vec2 worldPos) const;
    bool        inBounds(glm::ivec2 cell) const;
    std::size_t cellIndex(glm::ivec2 cell) const;

    float      cellSize_;
    float      sightRadius_;
    glm::vec2  worldMin_;
    glm::ivec2 dims_;
    std::vector<FogState> grid_;
};

} // namespace Systems

#endif // FOG_OF_WAR_H
