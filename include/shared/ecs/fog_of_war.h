#ifndef FOG_OF_WAR_H
#define FOG_OF_WAR_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "components.h"

namespace Systems {

enum class FogState : uint8_t { Hidden, Fogged, Visible };

class FogOfWar {
  public:
    FogOfWar(glm::vec2 worldMin, glm::vec2 worldMax, float cellSize, float sightRadius);

    // Resets current Visible→Fogged, then illuminates around units of viewerFaction.
    // Pass the local player's faction so each client gets their own perspective.
    void update(entt::registry &registry, Components::FactionId viewerFaction = Components::FactionId::Player);

    FogState stateAt(glm::vec2 worldPos) const;
    FogState stateAt(glm::ivec2 cell) const;
    bool isVisible(glm::vec2 worldPos) const;

    // Grid accessors for the renderer
    glm::ivec2 dims() const { return m_dims; }
    glm::vec2 cellWorldMin(glm::ivec2 cell) const;
    glm::vec2 cellWorldMax(glm::ivec2 cell) const;

  private:
    glm::ivec2 toCell(glm::vec2 worldPos) const;
    bool inBounds(glm::ivec2 cell) const;
    std::size_t cellIndex(glm::ivec2 cell) const;

    float m_cellSize;
    float m_sightRadius;
    glm::vec2 m_worldMin;
    glm::ivec2 m_dims;
    std::vector<FogState> m_grid;
};

} // namespace Systems

#endif // FOG_OF_WAR_H
