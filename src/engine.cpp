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
#include "logical_device.h"
#include "physical_device.h"
#include "validation_layers.h"
#include "vulkan_instance.h"
#include "window.h"

class HelloTriangleApplication {
  public:
    void run() {

        initWindow();
        initVulkan();
        enableValidationLayers(true);
        initPhysicalDevice();
        initLogicalDevice();
        mainLoop();
    }

  private:
    void enableValidationLayers(bool enableValidationLayers) {
        std::cout << "Enabling validation layers..." << std::endl;
        // Create validation layers object
        VulkanHelpers::ValidationLayers validationLayers;

        // Get the required layers
        std::vector<char const *> requiredLayers;
        if (enableValidationLayers) {
            requiredLayers = validationLayers.getRequiredLayers();

            // Check if the required layers are supported by the Vulkan
            // implementation.
            auto context = vulkanInstance->getContext();

            if (!validationLayers.areValidationLayersSupported(requiredLayers, *context)) {
                throw std::runtime_error("Required layer not supported");
            }

            // Get the required extensions.
            auto requiredExtensions = window->getRequiredInstanceExtensions();

            // Check if the required extensions are supported by the Vulkan implementation.
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
    void initPhysicalDevice() {
        std::cout << "selecting physical device..." << std::endl;
        physicalDevice = std::make_shared<VulkanHelpers::PhysicalDevice>(*vulkanInstance->getInstance());
    }
    void initLogicalDevice() {
        std::cout << "creating logical device..." << std::endl;
        logicalDevice = std::make_shared<VulkanHelpers::LogicalDevice>(*physicalDevice->getPhysicalDevice(), physicalDevice->getGraphicsQueueFamilyIndex());
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
        }
    }
    std::shared_ptr<VulkanHelpers::Window> window;
    std::shared_ptr<VulkanHelpers::Instance> vulkanInstance;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;
    std::shared_ptr<VulkanHelpers::PhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanHelpers::LogicalDevice> logicalDevice;
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
