#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "components.h"
#include "fog_of_war.h"
#include "render_system.h"

namespace Systems {

namespace {

bool isEnemyInFog(entt::registry &registry, entt::entity entity, const FogOfWar *fog) {
    if (!fog) return false;
    const auto *faction = registry.try_get<Components::Faction>(entity);
    if (!faction || faction->id == Components::FactionId::Player) return false;
    const auto &t = registry.get<Components::Transform>(entity);
    return !fog->isVisible({t.position.x, t.position.y});
}

} // namespace

std::vector<VulkanHelpers::DrawCall> collectDrawCalls(
    entt::registry &registry,
    const FogOfWar *fog
) {
    std::vector<VulkanHelpers::DrawCall> result;

    auto view = registry.view<Components::Transform, Components::RenderMesh>();
    for (auto entity : view) {
        if (isEnemyInFog(registry, entity, fog)) continue;

        const auto &transform = view.get<Components::Transform>(entity);
        const auto &mesh      = view.get<Components::RenderMesh>(entity);

        auto modelMatrix = glm::translate(glm::mat4(1.0f), transform.position);
        modelMatrix      = modelMatrix * glm::mat4_cast(transform.rotation);
        modelMatrix      = glm::scale(modelMatrix, transform.scale);

        result.push_back(VulkanHelpers::DrawCall{
            mesh.model.get(),
            mesh.model->getMaterial().getDescriptorSet(),
            modelMatrix,
        });
    }

    return result;
}

void appendSelectionRings(
    entt::registry &registry,
    std::vector<VulkanHelpers::DrawCall> &draws,
    const VulkanHelpers::Model &ringModel,
    const FogOfWar *fog
) {
    vk::DescriptorSet matSet = ringModel.getMaterial().getDescriptorSet();

    for (auto entity : registry.view<Components::Selected, Components::Transform>()) {
        if (isEnemyInFog(registry, entity, fog)) continue;

        const auto &t = registry.get<Components::Transform>(entity);
        auto m = glm::translate(glm::mat4(1.0f), glm::vec3(t.position.x, t.position.y, 0.02f));
        draws.push_back({&ringModel, matSet, m});
    }
}

} // namespace Systems
