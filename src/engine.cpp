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
#include "vulkan_instance.h"

class HelloTriangleApplication {
public:
    void run() {
	initWindow();    
        initVulkan();
        mainLoop();
    }

private:
    void initWindow() {
	window = std::make_shared<Window::Window>(800, 600, "Vulkan");
    }    
    void initVulkan() {
	vulkanInstance = std::make_shared<Vulkan::Instance>(window);
    }

    void mainLoop() {
	while (!window->shouldClose()) {
	    window->pollEvents();
	}
    }
    std::shared_ptr<Window::Window> window;
    std::shared_ptr<Vulkan::Instance> vulkanInstance;
    
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
