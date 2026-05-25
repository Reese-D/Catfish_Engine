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
    GraphicsPipeline(const vk::raii::Device &device, vk::Format swapChainFormat);
    ~GraphicsPipeline() = default;

    GraphicsPipeline(const GraphicsPipeline &) = delete;
    GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;

    std::shared_ptr<vk::raii::Pipeline> getPipeline() const { return pipeline; }
    std::shared_ptr<vk::raii::PipelineLayout> getPipelineLayout() const { return pipelineLayout; }

  private:
    static std::vector<char> readShaderFile(const std::string &filename);
    static vk::raii::ShaderModule createShaderModule(const vk::raii::Device &device, const std::vector<char> &code);

    std::shared_ptr<vk::raii::PipelineLayout> pipelineLayout;
    std::shared_ptr<vk::raii::Pipeline> pipeline;
};

} // namespace VulkanHelpers

#endif // GRAPHICS_PIPELINE_H
