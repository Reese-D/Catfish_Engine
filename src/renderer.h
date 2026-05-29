#ifndef RENDERER_H
#define RENDERER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <chrono>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "graphics_pipeline.h"
#include "index_buffer.h"
#include "swap_chain.h"
#include "uniform_buffer.h"
#include "vertex_buffer.h"

namespace VulkanHelpers {

class Renderer {
  public:
    Renderer(const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex);
    ~Renderer() = default;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    const vk::raii::CommandPool &getCommandPool() const { return *commandPool; }

    bool drawFrame(
        const vk::raii::Device &device, const SwapChain &swapChain, const GraphicsPipeline &pipeline,
        const vk::raii::Queue &graphicsQueue, const vk::raii::Queue &presentQueue,
        const VertexBuffer &vertexBuffer, const IndexBuffer &indexBuffer, UniformBuffer &uniformBuffer
    );

  private:
    void recordCommandBuffer(
        uint32_t imageIndex, const SwapChain &swapChain, const GraphicsPipeline &pipeline,
        const VertexBuffer &vertexBuffer, const IndexBuffer &indexBuffer, vk::DescriptorSet descriptorSet
    );

    std::shared_ptr<vk::raii::CommandPool> commandPool;
    std::shared_ptr<vk::raii::CommandBuffer> commandBuffer;
    std::shared_ptr<vk::raii::Semaphore> imageAvailableSemaphore;
    std::shared_ptr<vk::raii::Semaphore> renderFinishedSemaphore;
    std::shared_ptr<vk::raii::Fence> inFlightFence;
    std::chrono::steady_clock::time_point startTime{std::chrono::steady_clock::now()};
};

} // namespace VulkanHelpers

#endif // RENDERER_H
