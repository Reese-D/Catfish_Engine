#include "vulkan_instance.h"
#include <stdexcept>
#include <ranges>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

    Instance::Instance(const std::shared_ptr<Window>& window) {
	createInstance(window);
    }

    Instance::~Instance() = default;

    void Instance::createInstance(const std::shared_ptr<Window> &window) {
	context = std::make_shared<vk::raii::Context>();
	constexpr vk::ApplicationInfo appInfo{
	    .pApplicationName = "Hello Triangle",
	    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
	    .pEngineName = "No Engine",
	    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
	    .apiVersion = vk::ApiVersion14};

	// Get the required instance extensions from the window.
        auto glfwExtensions = window->getRequiredInstanceExtensions();
	uint32_t glfwExtensionCount = static_cast<uint32_t>(glfwExtensions.size());

	// Check if the required GLFW extensions are supported by the Vulkan implementation.
	auto extensionProperties = context->enumerateInstanceExtensionProperties();

	//[TODO] Temporarily list extensions, this should help in testing when we later support other platforms
	std::cout << "available extensions:\n";
	for (const auto& extension : extensionProperties) {
	    std::cout << '\t' << extension.extensionName << '\n';
	}
    
	for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
	    if (std::ranges::none_of(extensionProperties,
				     [glfwExtension = glfwExtensions[i]](auto const& extensionProperty) {
					 return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
				     })) {
		throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
	    }
	}

	vk::InstanceCreateInfo createInfo{
	    .pApplicationInfo = &appInfo,
	    .enabledExtensionCount = glfwExtensionCount,
	    .ppEnabledExtensionNames = glfwExtensions.data()};
	instance = std::make_shared<vk::raii::Instance>(*context, createInfo);
    }

    std::vector<const char*> Instance::getRequiredInstanceExtensions()
    {
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	return extensions;
    }

} // namespace VulkanHelpers
