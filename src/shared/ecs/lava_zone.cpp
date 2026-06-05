#include "lava_zone.h"

#include <algorithm>

namespace Systems {

LavaZone::LavaZone(float initialRadius, float shrinkInterval, float shrinkAmount, float damagePerSecond)
    : m_safeRadius(initialRadius), m_shrinkInterval(shrinkInterval), m_shrinkAmount(shrinkAmount), m_damagePerSecond(damagePerSecond) {}

void LavaZone::update(float dt) {
    m_shrinkTimer += dt;
    if (m_shrinkTimer >= m_shrinkInterval) {
        m_shrinkTimer -= m_shrinkInterval;
        m_safeRadius = std::max(0.0f, m_safeRadius - m_shrinkAmount);
    }
}

bool LavaZone::isLava(glm::vec2 worldPos) const {
    float dx = worldPos.x;
    float dy = worldPos.y;
    return (dx * dx + dy * dy) > (m_safeRadius * m_safeRadius);
}

} // namespace Systems
