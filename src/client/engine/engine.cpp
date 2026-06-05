#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <chrono>
#include <iostream>

#include "engine.h"
#include "validation_layers.h"

namespace VulkanHelpers {

void Engine::run(IGame &game) {
    initAll();
    game.initLogic();
    game.initGraphics(makeResourceContext());

    auto lastTime = std::chrono::steady_clock::now();
    while (!m_window->shouldClose() && !game.wantsClose()) {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        m_window->pollEvents();

        auto output = game.update(deltaTime, m_swapChain->getExtent());
        m_uniformBuffer->update(output.view, output.proj);

        if (m_renderer->drawFrame(
                *m_logicalDevice->getDevice(), *m_swapChain, *m_graphicsPipeline, *m_logicalDevice->getGraphicsQueue(), *m_logicalDevice->getPresentQueue(), output.draws, *m_uniformBuffer,
                *m_depthBuffer, [&](vk::CommandBuffer cmd) { game.renderImGui(cmd); }
            )) {
            recreateSwapChain(game);
        }
    }
    m_logicalDevice->getDevice()->waitIdle();
}

void Engine::initAll() {
    std::cout << "creating m_window...\n";
    m_window = std::make_shared<Window>(800, 600, "Catfish Engine");

    std::cout << "creating vulkan instance...\n";
    m_vulkanInstance = std::make_shared<Instance>(m_window);

    ValidationLayers vl;
    auto ctx = m_vulkanInstance->getContext();
    if (vl.areValidationLayersSupported(vl.getRequiredLayers(), *ctx) && vl.areRequiredExtensionsSupported(m_window->getRequiredInstanceExtensions(), *ctx)) {
        m_debugMessenger = vl.createDebugMessenger(*m_vulkanInstance->getInstance(), &debugCallback);
    }

    std::cout << "creating m_surface...\n";
    m_surface = std::make_shared<Surface>(*m_vulkanInstance->getInstance(), *m_window);

    std::cout << "selecting physical device...\n";
    m_physicalDevice = std::make_shared<PhysicalDevice>(*m_vulkanInstance->getInstance(), *m_surface->getSurface());

    std::cout << "creating logical device...\n";
    m_logicalDevice = std::make_shared<LogicalDevice>(*m_physicalDevice->getPhysicalDevice(), m_physicalDevice->getGraphicsQueueFamilyIndex(), m_physicalDevice->getPresentQueueFamilyIndex());

    std::cout << "creating swap chain...\n";
    m_swapChain = std::make_shared<SwapChain>(
        *m_physicalDevice->getPhysicalDevice(), *m_logicalDevice->getDevice(), *m_surface->getSurface(), m_physicalDevice->getGraphicsQueueFamilyIndex(),
        m_physicalDevice->getPresentQueueFamilyIndex(), *m_window
    );

    std::cout << "creating depth buffer...\n";
    m_depthBuffer = std::make_shared<DepthBuffer>(*m_logicalDevice->getDevice(), *m_physicalDevice->getPhysicalDevice(), m_swapChain->getExtent());

    std::cout << "creating graphics pipeline...\n";
    m_graphicsPipeline = std::make_shared<GraphicsPipeline>(*m_logicalDevice->getDevice(), m_swapChain->getFormat(), m_depthBuffer->getFormat());

    std::cout << "creating m_renderer...\n";
    m_renderer = std::make_shared<Renderer>(*m_logicalDevice->getDevice(), m_physicalDevice->getGraphicsQueueFamilyIndex());

    std::cout << "creating uniform buffer...\n";
    m_uniformBuffer = std::make_shared<UniformBuffer>(*m_logicalDevice->getDevice(), *m_physicalDevice->getPhysicalDevice(), *m_graphicsPipeline->getUboLayout());
}

ResourceContext Engine::makeResourceContext() {
    return {
        .instance = *m_vulkanInstance->getInstance(),
        .device = *m_logicalDevice->getDevice(),
        .physicalDevice = *m_physicalDevice->getPhysicalDevice(),
        .commandPool = m_renderer->getCommandPool(),
        .graphicsQueue = *m_logicalDevice->getGraphicsQueue(),
        .textureLayout = *m_graphicsPipeline->getTextureLayout(),
        .uboLayout = *m_graphicsPipeline->getUboLayout(),
        .window = *m_window,
        .swapChain = *m_swapChain,
        .depthFormat = m_depthBuffer->getFormat(),
        .graphicsQueueFamilyIndex = m_physicalDevice->getGraphicsQueueFamilyIndex(),
    };
}

void Engine::recreateSwapChain(IGame &game) {
    auto [width, height] = m_window->getFramebufferSize();
    while (width == 0 || height == 0) {
        m_window->waitEvents();
        std::tie(width, height) = m_window->getFramebufferSize();
    }
    m_logicalDevice->getDevice()->waitIdle();
    m_swapChain->recreate(
        *m_physicalDevice->getPhysicalDevice(), *m_logicalDevice->getDevice(), *m_surface->getSurface(), m_physicalDevice->getGraphicsQueueFamilyIndex(),
        m_physicalDevice->getPresentQueueFamilyIndex(), *m_window
    );
    m_depthBuffer->recreate(*m_logicalDevice->getDevice(), *m_physicalDevice->getPhysicalDevice(), m_swapChain->getExtent());
    game.onSwapChainRecreated(*m_swapChain);
}

vk::Bool32
Engine::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    using Sev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    if (severity == Sev::eError || severity == Sev::eWarning)
        std::cerr << "validation: " << to_string(type) << ": " << pCallbackData->pMessage << "\n";
    return vk::False;
}

} // namespace VulkanHelpers
