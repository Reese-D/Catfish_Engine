#ifndef LAVA_ZONE_H
#define LAVA_ZONE_H

#include <glm/glm.hpp>

namespace Systems {

class LavaZone {
  public:
    LavaZone(
        float initialRadius,
        float shrinkInterval, // seconds between each shrink step
        float shrinkAmount,   // world units removed per step
        float damagePerSecond // HP/s dealt to units standing in lava
    );

    // Advances the shrink timer; shrinks the safe radius when the interval elapses.
    void update(float dt);

    bool isLava(glm::vec2 worldPos) const;
    float getSafeRadius() const { return m_safeRadius; }
    float getDamagePerSecond() const { return m_damagePerSecond; }
    void setSafeRadius(float r) { m_safeRadius = r; }

  private:
    float m_safeRadius;
    float m_shrinkInterval;
    float m_shrinkAmount;
    float m_damagePerSecond;
    float m_shrinkTimer{0.0f};
};

} // namespace Systems

#endif // LAVA_ZONE_H
