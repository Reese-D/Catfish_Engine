#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <deque>
#include <memory>
#include <vector>

#include <entt/entt.hpp>

#include "orders.h"

namespace WorldBounds {
inline constexpr float kMin = -20.0f;
inline constexpr float kMax = 20.0f;
} // namespace WorldBounds

namespace Components {

struct Transform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

// RenderMesh is client-only — defined in include/client/ecs/render_components.h

struct Camera {
    glm::vec3 position{2.0f, 2.0f, 2.0f};
    glm::vec3 target{0.0f, 0.0f, 0.0f};
    float fov{45.0f};
    float near_{0.1f};
    float far_{100.0f};
    bool followPlayer{false};
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

enum class AbilityId : uint8_t {
    Projectile = 0,
    GravityWell = 1,
    Lightning = 2,
    Chain = 3,
};

struct AbilitySlot {
    AbilityId id{AbilityId::Projectile};
    float cooldown{1.5f};
    float timer{1.5f}; // ready when timer >= cooldown

    // Projectile params
    float projectileSpeed{8.0f};
    float knockbackForce{12.0f};

    // GravityWell params
    float pullStrength{15.0f};
    float pullRadius{5.0f};
    float activationDelay{0.5f};
};

struct AbilitySet {
    std::vector<AbilitySlot> slots;
};

// Component placed on lightning-bolt projectile entities.
// During charge (chargeTimer < chargeDelay) the velocity is zero; once the
// delay elapses the projectile is released at full speed toward boltDir.
struct LightningBolt {
    float chargeDelay{0.20f};
    float chargeTimer{0.0f};
    glm::vec3 boltDir{0.0f, 1.0f, 0.0f}; // unit vector toward target
    float speed{28.0f};
};

// Tag component placed on gravity-well projectile entities.
struct GravityWell {
    float pullStrength{15.0f};
    float pullRadius{5.0f};
    float activationDelay{0.5f};
    float activationTimer{0.0f}; // counts up; pull starts when >= activationDelay
};

struct Projectile {
    FactionId ownerFaction;
    glm::vec3 velocity{0.0f}; // world units per second
    float knockbackForce{12.0f};
    float hitRadius{0.4f};
    float lifetime{6.0f};
};

struct Velocity {
    glm::vec2 vel{0.0f};
};

struct Mass {
    float value{1.0f}; // kg-equivalent; higher = harder to accelerate and knock back
};

struct Friction {
    float coefficient{0.0f}; // velocity decay per second; 0 = frictionless
};

// Tag placed on a chain projectile while it is in flight (before latching).
// Stores the caster so we don't accidentally latch onto them.
struct ChainProjectile {
    entt::entity casterEntity{entt::null};
    float pullStrength{8.0f};
    float pullDuration{3.0f};
};

// Placed on a chain entity once it has latched onto a target.
// anchorEntity/hitEntity are valid on the server; entt::null on the client.
// anchorNetId/hitNetId are valid everywhere.
struct ChainLink {
    entt::entity anchorEntity{entt::null};
    entt::entity hitEntity{entt::null};
    uint32_t anchorNetId{0};
    uint32_t hitNetId{0};
    float pullStrength{8.0f};
    float duration{3.0f};
    float timer{0.0f};
};

struct Rock {}; // tag: static terrain piece with physics

// Explicit collision circle for entities whose mesh origin is not at the visual centre.
// offset is in world-space units, applied to Transform.position to find the physics centre.
struct Collider {
    float radius{0.4f};
    glm::vec2 offset{0.0f, 0.0f};
};

struct ThrustDirection {
    glm::vec2 dir{0.0f}; // unit vector; zero means no thrust
    float force{3.0f};   // acceleration in units/sec²
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
