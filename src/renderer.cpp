#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "depth_buffer.h"
#include "model.h"
#include "renderer.h"

namespace VulkanHelpers {

Renderer::Renderer(const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex) {
    commandPool = std::make_shared<vk::raii::CommandPool>(
        device, vk::CommandPoolCreateInfo{
                    .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                    .queueFamilyIndex = graphicsQueueFamilyIndex,
                }
    );

    auto buffers = device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo{
            .commandPool = **commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        }
    );
    commandBuffer = std::make_shared<vk::raii::CommandBuffer>(std::move(buffers[0]));

    imageAvailableSemaphore = std::make_shared<vk::raii::Semaphore>(device, vk::SemaphoreCreateInfo{});
    renderFinishedSemaphore = std::make_shared<vk::raii::Semaphore>(device, vk::SemaphoreCreateInfo{});
    inFlightFence = std::make_shared<vk::raii::Fence>(
        device, vk::FenceCreateInfo{
                    .flags = vk::FenceCreateFlagBits::eSignaled,
                }
    );
}

bool Renderer::drawFrame(
    const vk::raii::Device &device, const SwapChain &swapChain, const GraphicsPipeline &pipeline,
    const vk::raii::Queue &graphicsQueue, const vk::raii::Queue &presentQueue,
    const std::vector<DrawCall> &drawCalls, const UniformBuffer &uniformBuffer,
    const DepthBuffer &depthBuffer
) {
    (void)device.waitForFences(**inFlightFence, vk::True, UINT64_MAX);

    uint32_t imageIndex;
    bool needsRecreate = false;
    try {
        auto [result, idx] = swapChain.getSwapChain()->acquireNextImage(UINT64_MAX, **imageAvailableSemaphore);
        imageIndex = idx;
        if (result == vk::Result::eSuboptimalKHR) {
            needsRecreate = true;
        }
    } catch (const vk::OutOfDateKHRError &) {
        return true;
    }

    device.resetFences(**inFlightFence);

    commandBuffer->reset();
    recordCommandBuffer(imageIndex, swapChain, pipeline, drawCalls, uniformBuffer.getDescriptorSet(), depthBuffer);

    vk::Semaphore waitSem = **imageAvailableSemaphore;
    vk::Semaphore signalSem = **renderFinishedSemaphore;
    vk::CommandBuffer cmdBuf = **commandBuffer;
    vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    auto submitInfo = vk::SubmitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSem,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &signalSem,
    };
    graphicsQueue.submit(submitInfo, **inFlightFence);

    vk::SwapchainKHR sc = **swapChain.getSwapChain();
    auto presentInfo = vk::PresentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &signalSem,
        .swapchainCount = 1,
        .pSwapchains = &sc,
        .pImageIndices = &imageIndex,
    };

    try {
        auto presentResult = presentQueue.presentKHR(presentInfo);
        return needsRecreate || presentResult == vk::Result::eSuboptimalKHR;
    } catch (const vk::OutOfDateKHRError &) {
        return true;
    }
}

void Renderer::recordCommandBuffer(
    uint32_t imageIndex, const SwapChain &swapChain, const GraphicsPipeline &pipeline,
    const std::vector<DrawCall> &drawCalls, vk::DescriptorSet descriptorSet,
    const DepthBuffer &depthBuffer
) {
    commandBuffer->begin(vk::CommandBufferBeginInfo{});

    auto image = swapChain.getImages()[imageIndex];

    auto toColorAttachment = vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .srcAccessMask = {},
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .image = image,
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    auto toDepthAttachment = vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .image = depthBuffer.getImage(),
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    std::array<vk::ImageMemoryBarrier2, 2> startBarriers = {toColorAttachment, toDepthAttachment};
    commandBuffer->pipelineBarrier2(
        vk::DependencyInfo{
            .imageMemoryBarrierCount = static_cast<uint32_t>(startBarriers.size()),
            .pImageMemoryBarriers = startBarriers.data(),
        }
    );

    auto clearColor = vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}}};
    auto colorAttachment = vk::RenderingAttachmentInfo{
        .imageView = *swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    auto clearDepth = vk::ClearValue{vk::ClearDepthStencilValue{.depth = 1.0f, .stencil = 0}};
    auto depthAttachment = vk::RenderingAttachmentInfo{
        .imageView = depthBuffer.getImageView(),
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth,
    };
    commandBuffer->beginRendering(
        vk::RenderingInfo{
            .renderArea = vk::Rect2D{.offset = {0, 0}, .extent = swapChain.getExtent()},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment,
            .pDepthAttachment = &depthAttachment,
        }
    );

    commandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline.getPipeline());

    auto ex = swapChain.getExtent();
    commandBuffer->setViewport(
        0, vk::Viewport{
               .x = 0.0f,
               .y = 0.0f,
               .width = static_cast<float>(ex.width),
               .height = static_cast<float>(ex.height),
               .minDepth = 0.0f,
               .maxDepth = 1.0f,
           }
    );
    commandBuffer->setScissor(0, vk::Rect2D{.offset = {0, 0}, .extent = ex});
    commandBuffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, **pipeline.getPipelineLayout(), 0, {descriptorSet}, {});

    for (const auto &draw : drawCalls) {
        commandBuffer->pushConstants(
            **pipeline.getPipelineLayout(),
            vk::ShaderStageFlagBits::eVertex,
            0,
            sizeof(glm::mat4),
            &draw.transform
        );
        commandBuffer->bindVertexBuffers(0, {**draw.model->getVertexBuffer().getBuffer()}, {vk::DeviceSize{0}});
        commandBuffer->bindIndexBuffer(**draw.model->getIndexBuffer().getBuffer(), 0, vk::IndexType::eUint32);
        commandBuffer->drawIndexed(draw.model->getIndexBuffer().getIndexCount(), 1, 0, 0, 0);
    }

    commandBuffer->endRendering();

    auto toPresent = vk::ImageMemoryBarrier2{
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .dstAccessMask = {},
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .image = image,
        .subresourceRange = vk::ImageSubresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    commandBuffer->pipelineBarrier2(
        vk::DependencyInfo{
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &toPresent,
        }
    );

    commandBuffer->end();
}

} // namespace VulkanHelpers
