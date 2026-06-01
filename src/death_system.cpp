#include "death_system.h"

#include <iostream>

namespace Systems {

std::vector<Components::FactionId> processDeath(entt::registry &registry) {
    std::vector<entt::entity>         toDestroy;
    std::vector<Components::FactionId> deadFactions;

    for (auto entity : registry.view<Components::Health, Components::Faction>()) {
        if (registry.get<Components::Health>(entity).current > 0.0f) continue;
        toDestroy.push_back(entity);
        deadFactions.push_back(registry.get<Components::Faction>(entity).id);
    }

    for (auto e : toDestroy) {
        if (!registry.valid(e)) continue;
        auto faction = registry.get<Components::Faction>(e).id;
        std::cout << (faction == Components::FactionId::Player ? "Player" : "Enemy")
                  << " unit died\n";
        registry.destroy(e);
    }

    return deadFactions;
}

} // namespace Systems
