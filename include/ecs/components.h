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
    // Populated by CameraSystem each frame — read by SelectionSystem for ray casting
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
};

struct Selectable {};
struct Selected {};
struct AlwaysSelected {}; // entity keeps Selected every frame regardless of clicks

struct NetworkId {
    uint32_t id{0};
};

enum class FactionId : uint8_t { Player = 0, Enemy = 1 };

struct Faction {
    FactionId id{FactionId::Player};
};

struct Health {
    float current{100.0f};
    float max{100.0f};
};

struct Combat {
    float damage{10.0f};
    float range{1.5f};
    float cooldown{1.0f};
    float timer{1.0f}; // start ready to attack
};

struct MovementSpeed {
    float speed{3.0f}; // units per second
};

struct Ability {
    float cooldown{1.5f};
    float timer{1.5f}; // starts ready (timer >= cooldown)
    float projectileSpeed{8.0f};
    float knockbackForce{12.0f};
};

struct Projectile {
    FactionId ownerFaction;
    glm::vec3 velocity{0.0f}; // world units per second
    float knockbackForce{12.0f};
    float hitRadius{0.4f};
    float lifetime{6.0f};
};

struct Knockback {
    glm::vec2 force{0.0f};
    float decay{5.0f}; // force lost per second (exponential)
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
