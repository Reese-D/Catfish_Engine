#ifndef ORDERS_H
#define ORDERS_H

#include <glm/glm.hpp>
#include <variant>
#include <vector>

#include <entt/entt.hpp>

namespace Orders {

struct MoveOrder {
    glm::vec3              destination;
    std::vector<glm::vec3> path;       // empty until computed on first tick
    std::size_t            pathIndex{0};
};

struct AttackOrder {
    entt::entity target;
};

struct HoldOrder {};

using Order = std::variant<MoveOrder, AttackOrder, HoldOrder>;

// Standard visitor helper for std::visit
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

} // namespace Orders

#endif // ORDERS_H
