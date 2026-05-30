// Vulkan
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// Standard library
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

// ECS
#include <entt/entt.hpp>

// Local
#include "camera_system.h"
#include "components.h"
#include "depth_buffer.h"
#include "graphics_pipeline.h"
#include "logical_device.h"
#include "model.h"
#include "physical_device.h"
#include "render_system.h"
#include "renderer.h"
#include "surface.h"
#include "swap_chain.h"
#include "uniform_buffer.h"
#include "validation_layers.h"
#include "vulkan_instance.h"
#include "window.h"

class HelloTriangleApplication {
  public:
    void run() {
        initWindow();
        initVulkan();
        enableValidationLayers(true);
        initSurface();
        initPhysicalDevice();
        initLogicalDevice();
        initSwapChain();
        initDepthBuffer();
        initGraphicsPipeline();
        initRenderer();
        initModel();
        initUniformBuffer();
        initScene();
        mainLoop();
    }

  private:
    void enableValidationLayers(bool enable) {
        std::cout << "Enabling validation layers..." << std::endl;
        VulkanHelpers::ValidationLayers validationLayers;

        if (enable) {
            auto requiredLayers = validationLayers.getRequiredLayers();
            auto context = vulkanInstance->getContext();

            if (!validationLayers.areValidationLayersSupported(requiredLayers, *context)) {
                throw std::runtime_error("Required layer not supported");
            }
            auto requiredExtensions = window->getRequiredInstanceExtensions();
            if (!validationLayers.areRequiredExtensionsSupported(requiredExtensions, *context)) {
                throw std::runtime_error("Required extension not supported");
            }
            debugMessenger = validationLayers.createDebugMessenger(*vulkanInstance->getInstance(), &debugCallback);
        }
    }
    void initWindow() {
        std::cout << "creating window..." << std::endl;
        window = std::make_shared<VulkanHelpers::Window>(800, 600, "Catfish Engine");
    }
    void initVulkan() {
        std::cout << "creating vulkan instance..." << std::endl;
        vulkanInstance = std::make_shared<VulkanHelpers::Instance>(window);
    }
    void initSurface() {
        std::cout << "creating window surface..." << std::endl;
        surface = std::make_shared<VulkanHelpers::Surface>(*vulkanInstance->getInstance(), *window);
    }
    void initPhysicalDevice() {
        std::cout << "selecting physical device..." << std::endl;
        physicalDevice = std::make_shared<VulkanHelpers::PhysicalDevice>(*vulkanInstance->getInstance(), *surface->getSurface());
    }
    void initLogicalDevice() {
        std::cout << "creating logical device..." << std::endl;
        logicalDevice = std::make_shared<VulkanHelpers::LogicalDevice>(
            *physicalDevice->getPhysicalDevice(), physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex()
        );
    }
    void initSwapChain() {
        std::cout << "creating swap chain..." << std::endl;
        swapChain = std::make_shared<VulkanHelpers::SwapChain>(
            *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(),
            physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex(), *window
        );
    }
    void initDepthBuffer() {
        std::cout << "creating depth buffer..." << std::endl;
        depthBuffer = std::make_shared<VulkanHelpers::DepthBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent()
        );
    }
    void initGraphicsPipeline() {
        std::cout << "creating graphics pipeline..." << std::endl;
        graphicsPipeline = std::make_shared<VulkanHelpers::GraphicsPipeline>(
            *logicalDevice->getDevice(), swapChain->getFormat(), depthBuffer->getFormat()
        );
    }
    void initRenderer() {
        std::cout << "creating renderer..." << std::endl;
        renderer = std::make_shared<VulkanHelpers::Renderer>(*logicalDevice->getDevice(), physicalDevice->getGraphicsQueueFamilyIndex());
    }
    void initModel() {
        std::cout << "loading model..." << std::endl;
        model = std::make_shared<VulkanHelpers::Model>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(),
            "models/goblin.glb"
        );
    }
    void initUniformBuffer() {
        std::cout << "creating uniform buffer..." << std::endl;
        uniformBuffer = std::make_shared<VulkanHelpers::UniformBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            *graphicsPipeline->getDescriptorSetLayout(),
            model->getTextureImage().getImageView(), model->getTextureImage().getSampler()
        );
    }
    void initScene() {
        std::cout << "initialising scene..." << std::endl;

        auto camEntity = registry.create();
        registry.emplace<Components::Camera>(camEntity, Components::Camera{
            .position = {2.0f, 2.0f, 2.0f},
            .target   = {0.0f, 0.0f, 0.0f},
            .fov      = 45.0f,
            .near_    = 0.1f,
            .far_     = 100.0f,
        });

        auto goblin = registry.create();
        registry.emplace<Components::Transform>(goblin);
        registry.emplace<Components::RenderMesh>(goblin, Components::RenderMesh{model});
        registry.emplace<Components::Selectable>(goblin);
    }

    void recreateSwapChain() {
        auto [width, height] = window->getFramebufferSize();
        while (width == 0 || height == 0) {
            window->waitEvents();
            std::tie(width, height) = window->getFramebufferSize();
        }
        logicalDevice->getDevice()->waitIdle();
        swapChain->recreate(
            *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(),
            physicalDevice->getGraphicsQueueFamilyIndex(), physicalDevice->getPresentQueueFamilyIndex(), *window
        );
        depthBuffer->recreate(*logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(), swapChain->getExtent());
    }

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL
    debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
        if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        }
        return vk::False;
    }

    void mainLoop() {
        while (!window->shouldClose()) {
            window->pollEvents();
            Systems::updateCamera(registry, *uniformBuffer, swapChain->getExtent());
            auto draws = Systems::collectDrawCalls(registry);
            if (renderer->drawFrame(
                    *logicalDevice->getDevice(), *swapChain, *graphicsPipeline,
                    *logicalDevice->getGraphicsQueue(), *logicalDevice->getPresentQueue(),
                    draws, *uniformBuffer, *depthBuffer
                )) {
                recreateSwapChain();
                std::cout << "Recreating swapchain" << std::endl;
            }
        }
        logicalDevice->getDevice()->waitIdle();
    }

    // Vulkan
    std::shared_ptr<VulkanHelpers::Window> window;
    std::shared_ptr<VulkanHelpers::Instance> vulkanInstance;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;
    std::shared_ptr<VulkanHelpers::Surface> surface;
    std::shared_ptr<VulkanHelpers::PhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanHelpers::LogicalDevice> logicalDevice;
    std::shared_ptr<VulkanHelpers::SwapChain> swapChain;
    std::shared_ptr<VulkanHelpers::GraphicsPipeline> graphicsPipeline;
    std::shared_ptr<VulkanHelpers::Renderer> renderer;
    std::shared_ptr<VulkanHelpers::DepthBuffer> depthBuffer;
    std::shared_ptr<VulkanHelpers::Model> model;
    std::shared_ptr<VulkanHelpers::UniformBuffer> uniformBuffer;

    // ECS
    entt::registry registry;
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Exiting cleanly" << std::endl;
    return EXIT_SUCCESS;
}
