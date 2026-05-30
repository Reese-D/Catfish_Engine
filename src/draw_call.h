#ifndef DRAW_CALL_H
#define DRAW_CALL_H

#include <glm/glm.hpp>

namespace VulkanHelpers {

class Model;

struct DrawCall {
    const Model *model;
    glm::mat4 transform;
};

} // namespace VulkanHelpers

#endif // DRAW_CALL_H
