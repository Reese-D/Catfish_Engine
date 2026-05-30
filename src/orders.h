#ifndef ORDERS_H
#define ORDERS_H

#include <glm/glm.hpp>
#include <variant>

#include <entt/entt.hpp>

namespace Orders {

struct MoveOrder {
    glm::vec3 destination;
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
