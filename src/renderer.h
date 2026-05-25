#ifndef RENDERER_H
#define RENDERER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "graphics_pipeline.h"
#include "swap_chain.h"

namespace VulkanHelpers {

class Renderer {
  public:
    Renderer(const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex);
    ~Renderer() = default;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    bool
    drawFrame(const vk::raii::Device &device, const SwapChain &swapChain, const GraphicsPipeline &pipeline, const vk::raii::Queue &graphicsQueue, const vk::raii::Queue &presentQueue);

  private:
    void recordCommandBuffer(uint32_t imageIndex, const SwapChain &swapChain, const GraphicsPipeline &pipeline);

    std::shared_ptr<vk::raii::CommandPool> commandPool;
    std::shared_ptr<vk::raii::CommandBuffer> commandBuffer;
    std::shared_ptr<vk::raii::Semaphore> imageAvailableSemaphore;
    std::shared_ptr<vk::raii::Semaphore> renderFinishedSemaphore;
    std::shared_ptr<vk::raii::Fence> inFlightFence;
};

} // namespace VulkanHelpers

#endif // RENDERER_H
