#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class GraphicsPipeline {
  public:
    GraphicsPipeline(const vk::raii::Device &device, vk::Format swapChainFormat, vk::Format depthFormat);
    ~GraphicsPipeline() = default;

    GraphicsPipeline(const GraphicsPipeline &) = delete;
    GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

    std::shared_ptr<vk::raii::Pipeline> getPipeline() const { return m_pipeline; }
    std::shared_ptr<vk::raii::PipelineLayout> getPipelineLayout() const { return m_pipelineLayout; }
    std::shared_ptr<vk::raii::DescriptorSetLayout> getUboLayout() const { return m_uboLayout; }
    std::shared_ptr<vk::raii::DescriptorSetLayout> getTextureLayout() const { return m_textureLayout; }

  private:
    static std::vector<char> readShaderFile(const std::string &filename);
    static vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, const std::vector<char> &code);

    std::shared_ptr<vk::raii::DescriptorSetLayout> m_uboLayout;
    std::shared_ptr<vk::raii::DescriptorSetLayout> m_textureLayout;
    std::shared_ptr<vk::raii::PipelineLayout> m_pipelineLayout;
    std::shared_ptr<vk::raii::Pipeline> m_pipeline;
};

} // namespace VulkanHelpers

#endif // GRAPHICS_PIPELINE_H
