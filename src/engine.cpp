#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>


class HelloTriangleApplication {
public:
    void run() {
	initWindow();    
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    void initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);      
    }    
    void initVulkan() {
	
    }

    void mainLoop() {
	while (!glfwWindowShouldClose(window)) {
	    glfwPollEvents();
	}      
	// constexpr vk::ApplicationInfo appInfo{.pApplicationName   = "Hello Triangle",
	// 							   .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
	// 							   .pEngineName        = "No Engine",
	// 							   .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
	// 							   .apiVersion         = vk::ApiVersion14};
	// 	vk::InstanceCreateInfo createInfo{
	// 	    .pApplicationInfo = &appInfo
	// 	};
	// 	vk::raii::Context context;
	// 	auto instance = vk::raii::Instance(context, createInfo);
    }

    void cleanup() {
	glfwDestroyWindow(window);

	glfwTerminate();
      }

    GLFWwindow* window;    
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
