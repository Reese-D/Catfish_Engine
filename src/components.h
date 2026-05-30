#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>

#include "model.h"

namespace Components {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

struct RenderMesh {
    std::shared_ptr<VulkanHelpers::Model> model;
};

struct Camera {
    glm::vec3 position{2.0f, 2.0f, 2.0f};
    glm::vec3 target{0.0f, 0.0f, 0.0f};
    float fov{45.0f};
    float near_{0.1f};
    float far_{100.0f};
};

struct Selectable {};
struct Selected {};

} // namespace Components

#endif // COMPONENTS_H
