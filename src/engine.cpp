// Vulkan
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

//Standard library
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <memory> //smart pointers
                  // 
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
	
    }

    void mainLoop() {
	while (!window->shouldClose()) {
	    window->pollEvents();
	}      
    }

    std::unique_ptr<Window::Window> window;    
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
