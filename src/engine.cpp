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
#include "vulkan_instance.h"
#include "window.h"

class HelloTriangleApplication {
  public:
    void run() {

        initWindow();
        initVulkan();
        // enableValidationLayers(true);
        mainLoop();
    }

  private:
    void enableValidationLayers(bool enableValidationLayers) {
        // Get the required layers
        std::vector<char const *> requiredLayers;
        if (enableValidationLayers) {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        // Check if the required layers are supported by the Vulkan
        // implementation.
        auto context = vulkanInstance->getContext();
        auto layerProperties = context->enumerateInstanceLayerProperties();
        auto unsupportedLayerIt = std::ranges::find_if(requiredLayers, [&layerProperties](auto const &requiredLayer) {
            return std::ranges::none_of(layerProperties, [requiredLayer](auto const &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
        });
        if (unsupportedLayerIt != requiredLayers.end()) {
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
        }

        // Get the required extensions.
        auto requiredExtensions = window->getRequiredInstanceExtensions();

        // Check if the required extensions are supported by the Vulkan implementation.
        auto extensionProperties = context->enumerateInstanceExtensionProperties();
        auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions, [&extensionProperties](auto const &requiredExtension) {
            return std::ranges::none_of(extensionProperties, [requiredExtension](auto const &extensionProperty) {
                return strcmp(extensionProperty.extensionName, requiredExtension) == 0;
            });
        });
        if (unsupportedPropertyIt != requiredExtensions.end()) {
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));
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

    void mainLoop() {
        while (!window->shouldClose()) {
            window->pollEvents();
        }
    }
    std::shared_ptr<VulkanHelpers::Window> window;
    std::shared_ptr<VulkanHelpers::Instance> vulkanInstance;
    const std::vector<char const *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
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
