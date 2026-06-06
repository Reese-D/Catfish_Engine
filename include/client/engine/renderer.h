#ifndef RENDERER_H
#define RENDERER_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "depth_buffer.h"
#include "draw_call.h"
#include "graphics_pipeline.h"
#include "projectile_pipeline.h"
#include "swap_chain.h"
#include "uniform_buffer.h"

namespace VulkanHelpers {

class Renderer {
  public:
    Renderer(const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex);
    ~Renderer() = default;

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    const vk::raii::CommandPool &getCommandPool() const { return *m_commandPool; }

    bool drawFrame(
        const vk::raii::Device &device, const SwapChain &swapChain, const GraphicsPipeline &pipeline, const vk::raii::Queue &graphicsQueue, const vk::raii::Queue &presentQueue,
        const std::vector<DrawCall> &drawCalls, const UniformBuffer &uniformBuffer, const DepthBuffer &depthBuffer, const ProjectilePipeline *projectilePipeline,
        const std::vector<ProjectileDrawCall> &projectileDrawCalls, const std::function<void(vk::CommandBuffer)> &drawUi = {}
    );

  private:
    void recordCommandBuffer(
        uint32_t imageIndex, const SwapChain &swapChain, const GraphicsPipeline &pipeline, const std::vector<DrawCall> &drawCalls, vk::DescriptorSet descriptorSet,
        const DepthBuffer &depthBuffer, const ProjectilePipeline *projectilePipeline, const std::vector<ProjectileDrawCall> &projectileDrawCalls,
        const std::function<void(vk::CommandBuffer)> &drawUi
    );

    std::shared_ptr<vk::raii::CommandPool> m_commandPool;
    std::shared_ptr<vk::raii::CommandBuffer> m_commandBuffer;
    std::shared_ptr<vk::raii::Semaphore> m_imageAvailableSemaphore;
    std::shared_ptr<vk::raii::Semaphore> m_renderFinishedSemaphore;
    std::shared_ptr<vk::raii::Fence> m_inFlightFence;
};

} // namespace VulkanHelpers

#endif // RENDERER_H
