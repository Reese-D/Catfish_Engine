#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <deque>
#include <memory>

#include "model.h"
#include "orders.h"

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

struct MovementSpeed {
    float speed{3.0f}; // units per second
};

struct OrderQueue {
    std::deque<Orders::Order> orders;

    void enqueue(Orders::Order order) { orders.push_back(std::move(order)); }

    // Clears pending orders and issues a new one immediately
    void enqueueImmediate(Orders::Order order) {
        orders.clear();
        orders.push_back(std::move(order));
    }

    bool empty() const { return orders.empty(); }
};

} // namespace Components

#endif // COMPONENTS_H
