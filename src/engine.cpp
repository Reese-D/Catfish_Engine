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
#include "validation_layers.h"
#include "vulkan_instance.h"
#include "window.h"

class HelloTriangleApplication {
  public:
    void run() {

        initWindow();
        initVulkan();
        enableValidationLayers(true);
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
        }

        // Check if the required layers are supported by the Vulkan
        // implementation.
        auto context = vulkanInstance->getContext();
        if (enableValidationLayers) {
            if (!validationLayers.areValidationLayersSupported(requiredLayers, *context)) {
                throw std::runtime_error("Required layer not supported");
            }
        }

        // Get the required extensions.
        auto requiredExtensions = window->getRequiredInstanceExtensions();

        // Check if the required extensions are supported by the Vulkan implementation.
        if (enableValidationLayers) {
            if (!validationLayers.areRequiredExtensionsSupported(requiredExtensions, *context)) {
                throw std::runtime_error("Required extension not supported");
            }
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
