#include "lava_zone.h"

#include <algorithm>

namespace Systems {

LavaZone::LavaZone(float initialRadius, float shrinkInterval, float shrinkAmount, float damagePerSecond)
    : safeRadius_(initialRadius)
    , shrinkInterval_(shrinkInterval)
    , shrinkAmount_(shrinkAmount)
    , damagePerSecond_(damagePerSecond)
{}

void LavaZone::update(float dt) {
    shrinkTimer_ += dt;
    if (shrinkTimer_ >= shrinkInterval_) {
        shrinkTimer_ -= shrinkInterval_;
        safeRadius_ = std::max(0.0f, safeRadius_ - shrinkAmount_);
    }
}

bool LavaZone::isLava(glm::vec2 worldPos) const {
    float dx = worldPos.x;
    float dy = worldPos.y;
    return (dx * dx + dy * dy) > (safeRadius_ * safeRadius_);
}

} // namespace Systems
