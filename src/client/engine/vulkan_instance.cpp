#include "vulkan_instance.h"
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

Instance::Instance(const std::shared_ptr<Window> &window) { createInstance(window); }

Instance::~Instance() = default;

void Instance::createInstance(const std::shared_ptr<Window> &window) {
    m_context = std::make_shared<vk::raii::Context>();
    constexpr vk::ApplicationInfo kAppInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    // Get the required m_instance extensions from the window.
    auto glfwExtensions = window->getRequiredInstanceExtensions();
    uint32_t glfwExtensionCount = static_cast<uint32_t>(glfwExtensions.size());

    // Check if the required GLFW extensions are supported by the Vulkan implementation.
    auto extensionProperties = m_context->enumerateInstanceExtensionProperties();

    //[TODO] Temporarily list extensions, this should help in testing when we later support other platforms
    std::cout << "available extensions:\n";
    for (const auto &extension : extensionProperties) {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        if (std::ranges::none_of(extensionProperties, [glfwExtension = glfwExtensions[i]](auto const &extensionProperty) {
                return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
            })) {
            throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
        }
    }

    vk::InstanceCreateInfo createInfo{.pApplicationInfo = &kAppInfo, .enabledExtensionCount = glfwExtensionCount, .ppEnabledExtensionNames = glfwExtensions.data()};
    m_instance = std::make_shared<vk::raii::Instance>(*m_context, createInfo);
}

std::vector<const char *> Instance::getRequiredInstanceExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    return extensions;
}

} // namespace VulkanHelpers
