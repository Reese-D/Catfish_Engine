#ifndef PROJECTILE_PIPELINE_H
#define PROJECTILE_PIPELINE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

// Alpha-blended pipeline for procedural projectile effects (fireball / gravity
// well).  Reuses the UBO and texture descriptor set layouts from the main
// GraphicsPipeline so their descriptor sets are directly compatible.
class ProjectilePipeline {
  public:
    ProjectilePipeline(
        const vk::raii::Device &device, vk::Format swapChainFormat, vk::Format depthFormat, vk::DescriptorSetLayout uboLayout, vk::DescriptorSetLayout textureLayout
    );
    ~ProjectilePipeline() = default;

    ProjectilePipeline(const ProjectilePipeline &) = delete;
    ProjectilePipeline &operator=(const ProjectilePipeline &) = delete;

    std::shared_ptr<vk::raii::Pipeline> getPipeline() const { return m_pipeline; }
    std::shared_ptr<vk::raii::PipelineLayout> getPipelineLayout() const { return m_pipelineLayout; }

  private:
    static std::vector<char> readShaderFile(const std::string &filename);
    static vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, const std::vector<char> &code);

    std::shared_ptr<vk::raii::PipelineLayout> m_pipelineLayout;
    std::shared_ptr<vk::raii::Pipeline> m_pipeline;
};

} // namespace VulkanHelpers

#endif // PROJECTILE_PIPELINE_H
