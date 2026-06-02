#ifndef LAVA_ZONE_H
#define LAVA_ZONE_H

#include <glm/glm.hpp>

namespace Systems {

class LavaZone {
  public:
    LavaZone(
        float initialRadius,
        float shrinkInterval,   // seconds between each shrink step
        float shrinkAmount,     // world units removed per step
        float damagePerSecond   // HP/s dealt to units standing in lava
    );

    // Advances the shrink timer; shrinks the safe radius when the interval elapses.
    void update(float dt);

    bool  isLava(glm::vec2 worldPos) const;
    float getSafeRadius()      const { return safeRadius_; }
    float getDamagePerSecond() const { return damagePerSecond_; }
    void  setSafeRadius(float r)    { safeRadius_ = r; }

  private:
    float safeRadius_;
    float shrinkInterval_;
    float shrinkAmount_;
    float damagePerSecond_;
    float shrinkTimer_{0.0f};
};

} // namespace Systems

#endif // LAVA_ZONE_H
