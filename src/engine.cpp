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
    while (!window->shouldClose() && !game.wantsClose()) {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        window->pollEvents();

        auto output = game.update(deltaTime, swapChain->getExtent());
        uniformBuffer->update(output.view, output.proj);

        if (renderer->drawFrame(
                *logicalDevice->getDevice(), *swapChain, *graphicsPipeline, *logicalDevice->getGraphicsQueue(), *logicalDevice->getPresentQueue(), output.draws, *uniformBuffer,
                *depthBuffer, [&](vk::CommandBuffer cmd) { game.renderImGui(cmd); }
            )) {
            recreateSwapChain(game);
        }
    }
    logicalDevice->getDevice()->waitIdle();
}

void Engine::initAll() {
    std::cout << "creating window...\n";
    window = std::make_shared<Window>(800, 600, "Catfish Engine");

    std::cout << "creating vulkan instance...\n";
    vulkanInstance = std::make_shared<Instance>(window);

    ValidationLayers vl;
    auto ctx = vulkanInstance->getContext();
    if (vl.areValidationLayersSupported(vl.getRequiredLayers(), *ctx) && vl.areRequiredExtensionsSupported(window->getRequiredInstanceExtensions(), *ctx)) {
        debugMessenger = vl.createDebugMessenger(*vulkanInstance->getInstance(), &debugCallback);
    }

    std::cout << "creating surface...\n";
    surface = std::make_shared<Surface>(*vulkanInstance->getInstance(), *window);

    std::cout << "selecting physical device...\n";
    physicalDevice = std::make_shared<PhysicalDevice>(*vulkanInstance->getInstance(), *surface->getSurface());

    std::cout << "creating logical device...\n";
    logicalDevice = std::make_shared<LogicalDevice>(*physicalDevice->getPhysicalDevice(), physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex());

    std::cout << "creating swap chain...\n";
    swapChain = std::make_shared<SwapChain>(
        *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(), physicalDevice->getGraphicsQueueFamilyIndex(),
        physicalDevice->getPresentQueueFamilyIndex(), *window
    );

    std::cout << "creating depth buffer...\n";
    depthBuffer = std::make_shared<DepthBuffer>(*logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent());

    std::cout << "creating graphics pipeline...\n";
    graphicsPipeline = std::make_shared<GraphicsPipeline>(*logicalDevice->getDevice(), swapChain->getFormat(), depthBuffer->getFormat());

    std::cout << "creating renderer...\n";
    renderer = std::make_shared<Renderer>(*logicalDevice->getDevice(), physicalDevice->getGraphicsQueueFamilyIndex());

    std::cout << "creating uniform buffer...\n";
    uniformBuffer = std::make_shared<UniformBuffer>(*logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), *graphicsPipeline->getUboLayout());
}

ResourceContext Engine::makeResourceContext() {
    return {
        .instance = *vulkanInstance->getInstance(),
        .device = *logicalDevice->getDevice(),
        .physicalDevice = *physicalDevice->getPhysicalDevice(),
        .commandPool = renderer->getCommandPool(),
        .graphicsQueue = *logicalDevice->getGraphicsQueue(),
        .textureLayout = *graphicsPipeline->getTextureLayout(),
        .uboLayout = *graphicsPipeline->getUboLayout(),
        .window = *window,
        .swapChain = *swapChain,
        .depthFormat = depthBuffer->getFormat(),
        .graphicsQueueFamilyIndex = physicalDevice->getGraphicsQueueFamilyIndex(),
    };
}

void Engine::recreateSwapChain(IGame &game) {
    auto [width, height] = window->getFramebufferSize();
    while (width == 0 || height == 0) {
        window->waitEvents();
        std::tie(width, height) = window->getFramebufferSize();
    }
    logicalDevice->getDevice()->waitIdle();
    swapChain->recreate(
        *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(), physicalDevice->getGraphicsQueueFamilyIndex(),
        physicalDevice->getPresentQueueFamilyIndex(), *window
    );
    depthBuffer->recreate(*logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent());
    game.onSwapChainRecreated(*swapChain);
}

vk::Bool32
Engine::debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    using Sev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    if (severity == Sev::eError || severity == Sev::eWarning)
        std::cerr << "validation: " << to_string(type) << ": " << pCallbackData->pMessage << "\n";
    return vk::False;
}

} // namespace VulkanHelpers
