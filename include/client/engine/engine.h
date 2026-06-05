#ifndef ENGINE_H
#define ENGINE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "depth_buffer.h"
#include "draw_call.h"
#include "graphics_pipeline.h"
#include "logical_device.h"
#include "physical_device.h"
#include "renderer.h"
#include "surface.h"
#include "swap_chain.h"
#include "uniform_buffer.h"
#include "vulkan_instance.h"
#include "window.h"

namespace VulkanHelpers {

// References to engine-owned Vulkan resources a game needs during init.
// All references remain valid for the lifetime of the Engine.
struct ResourceContext {
    vk::raii::Instance &instance;
    vk::raii::Device &device;
    vk::raii::PhysicalDevice &physicalDevice;
    const vk::raii::CommandPool &commandPool;
    vk::raii::Queue &graphicsQueue;
    vk::raii::DescriptorSetLayout &textureLayout;
    vk::raii::DescriptorSetLayout &uboLayout;
    Window &window;
    const SwapChain &swapChain;
    vk::Format depthFormat;
    uint32_t graphicsQueueFamilyIndex;
};

// Per-frame data returned by the game to the engine.
struct FrameOutput {
    std::vector<DrawCall> draws;
    glm::mat4 view{1.0f};
    glm::mat4 proj{1.0f};
};

class IGame {
  public:
    virtual ~IGame() = default;

    // initLogic — called by both Engine and HeadlessRunner.
    // Set up ECS state, networking. Must not touch Vulkan.
    virtual void initLogic() = 0;

    // initGraphics — called by Engine only (not HeadlessRunner).
    // Load GPU resources, create camera/terrain entities, set up UI.
    virtual void initGraphics(const ResourceContext &ctx) = 0;

    virtual FrameOutput update(float dt, vk::Extent2D extent) = 0;
    virtual void renderImGui(vk::CommandBuffer cmd) = 0;
    virtual void onSwapChainRecreated(const SwapChain &swapChain) = 0;
    virtual bool wantsMouse() const = 0;
    virtual bool wantsKeyboard() const = 0;
    virtual bool wantsClose() const = 0;
};

class Engine {
  public:
    void run(IGame &game);

  private:
    void initAll();
    ResourceContext makeResourceContext();
    void recreateSwapChain(IGame &game);

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT, vk::DebugUtilsMessageTypeFlagsEXT, const vk::DebugUtilsMessengerCallbackDataEXT *, void *);

    std::shared_ptr<Window> m_window;
    std::shared_ptr<Instance> m_vulkanInstance;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> m_debugMessenger;
    std::shared_ptr<Surface> m_surface;
    std::shared_ptr<PhysicalDevice> m_physicalDevice;
    std::shared_ptr<LogicalDevice> m_logicalDevice;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<DepthBuffer> m_depthBuffer;
    std::shared_ptr<GraphicsPipeline> m_graphicsPipeline;
    std::shared_ptr<Renderer> m_renderer;
    std::shared_ptr<UniformBuffer> m_uniformBuffer;
};

} // namespace VulkanHelpers

#endif // ENGINE_H
