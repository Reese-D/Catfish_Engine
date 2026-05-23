// Vulkan
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS //allows for designated initializers introduced in C++20
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

//Standard library
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory> //smart pointers
#include <ranges>

//Local
#include "window.h"

class HelloTriangleApplication {
public:
    void run() {
	initWindow();    
        initVulkan();
        mainLoop();
    }

private:
    void initWindow() {
	window = std::make_unique<Window::Window>(800, 600, "Vulkan");
    }    
    void initVulkan() {
	createInstance();
    }

    void mainLoop() {
	while (!window->shouldClose()) {
	    window->pollEvents();
	}
    }
    
    void createInstance()
    {
	constexpr vk::ApplicationInfo appInfo{
	    .pApplicationName = "Hello Triangle",
	    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
	    .pEngineName = "No Engine",
	    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
	    .apiVersion = vk::ApiVersion14};

	// Get the required instance extensions from GLFW.
	uint32_t glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Check if the required GLFW extensions are supported by the Vulkan implementation.
	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < glfwExtensionCount; ++i)
	    {
		if (std::ranges::none_of(extensionProperties,
					 [glfwExtension = glfwExtensions[i]](auto const& extensionProperty)
					 { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; }))
		    {
			throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
		    }
	    }

	vk::InstanceCreateInfo createInfo{
	    .pApplicationInfo = &appInfo,
	    .enabledExtensionCount = glfwExtensionCount,
	    .ppEnabledExtensionNames = glfwExtensions};
	instance = vk::raii::Instance(context, createInfo);
    }    

    std::unique_ptr<Window::Window> window;
    vk::raii::Context  context;
    vk::raii::Instance instance = nullptr;
    
};

int main()
{
    try
	{
	    HelloTriangleApplication app;
	    app.run();
	}
    catch (const std::exception& e)
	{
	    std::cerr << e.what() << std::endl;
	    return EXIT_FAILURE;
	}
    std::cout << "Exiting cleanly" << std::endl;
    return EXIT_SUCCESS;
}
