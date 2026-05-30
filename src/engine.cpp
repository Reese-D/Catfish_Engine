// Vulkan
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // allows for designated initializers introduced in C++20
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// Standard library
#include <cstdlib>
#include <iostream>
#include <memory> //smart pointers
#include <stdexcept>

// Local
#include "graphics_pipeline.h"
#include "index_buffer.h"
#include "logical_device.h"
#include "physical_device.h"
#include "renderer.h"
#include "surface.h"
#include "swap_chain.h"
#include "texture_image.h"
#include "uniform_buffer.h"
#include "validation_layers.h"
#include "vertex_buffer.h"
#include "vulkan_instance.h"
#include "window.h"

const std::vector<VulkanHelpers::Vertex> VERTICES = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
};

const std::vector<uint32_t> INDICES = {0, 1, 2, 2, 3, 0};

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
        initGraphicsPipeline();
        initRenderer();
        initTextureImage();
        initVertexBuffer();
        initIndexBuffer();
        initUniformBuffer();
        mainLoop();
    }

  private:
    void enableValidationLayers(bool enableValidationLayers) {
        std::cout << "Enabling validation layers..." << std::endl;
        VulkanHelpers::ValidationLayers validationLayers;

        std::vector<char const *> requiredLayers;
        if (enableValidationLayers) {
            requiredLayers = validationLayers.getRequiredLayers();

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
        window = std::make_shared<VulkanHelpers::Window>(800, 600, "Vulkan");
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
            *physicalDevice->getPhysicalDevice(), *logicalDevice->getDevice(), *surface->getSurface(), physicalDevice->getGraphicsQueueFamilyIndex(),
            physicalDevice->getPresentQueueFamilyIndex(), *window
        );
    }
    void initGraphicsPipeline() {
        std::cout << "creating graphics pipeline..." << std::endl;
        graphicsPipeline = std::make_shared<VulkanHelpers::GraphicsPipeline>(*logicalDevice->getDevice(), swapChain->getFormat());
    }
    void initRenderer() {
        std::cout << "creating renderer..." << std::endl;
        renderer = std::make_shared<VulkanHelpers::Renderer>(*logicalDevice->getDevice(), physicalDevice->getGraphicsQueueFamilyIndex());
    }
    void initVertexBuffer() {
        std::cout << "creating vertex buffer..." << std::endl;
        vertexBuffer = std::make_shared<VulkanHelpers::VertexBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(), VERTICES
        );
    }
    void initIndexBuffer() {
        std::cout << "creating index buffer..." << std::endl;
        indexBuffer = std::make_shared<VulkanHelpers::IndexBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(), INDICES
        );
    }
    void initTextureImage() {
        std::cout << "creating texture image..." << std::endl;
        textureImage = std::make_shared<VulkanHelpers::TextureImage>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            renderer->getCommandPool(), *logicalDevice->getGraphicsQueue(),
            "Images/652234-statue-1275469_1920.jpg"
        );
    }
    void initUniformBuffer() {
        std::cout << "creating uniform buffer..." << std::endl;
        uniformBuffer = std::make_shared<VulkanHelpers::UniformBuffer>(
            *logicalDevice->getDevice(), *physicalDevice->getPhysicalDevice(),
            *graphicsPipeline->getDescriptorSetLayout(),
            textureImage->getImageView(), textureImage->getSampler()
        );
    }

    void recreateSwapChain() {
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
            if (renderer->drawFrame(
                    *logicalDevice->getDevice(), *swapChain, *graphicsPipeline, *logicalDevice->getGraphicsQueue(), *logicalDevice->getPresentQueue(),
                    *vertexBuffer, *indexBuffer, *uniformBuffer
                )) {
                recreateSwapChain();
                std::cout << "Recreating swapchain" << std::endl;
            }
        }
        logicalDevice->getDevice()->waitIdle();
    }

    std::shared_ptr<VulkanHelpers::Window> window;
    std::shared_ptr<VulkanHelpers::Instance> vulkanInstance;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;
    std::shared_ptr<VulkanHelpers::Surface> surface;
    std::shared_ptr<VulkanHelpers::PhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanHelpers::LogicalDevice> logicalDevice;
    std::shared_ptr<VulkanHelpers::SwapChain> swapChain;
    std::shared_ptr<VulkanHelpers::GraphicsPipeline> graphicsPipeline;
    std::shared_ptr<VulkanHelpers::Renderer> renderer;
    std::shared_ptr<VulkanHelpers::TextureImage> textureImage;
    std::shared_ptr<VulkanHelpers::VertexBuffer> vertexBuffer;
    std::shared_ptr<VulkanHelpers::IndexBuffer> indexBuffer;
    std::shared_ptr<VulkanHelpers::UniformBuffer> uniformBuffer;
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
