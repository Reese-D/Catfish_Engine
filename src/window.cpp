#include "window.h"
#include <stdexcept>
#include <iostream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
namespace Window{
    Window::Window(uint32_t width, uint32_t height, const char* title) 
	: width(width), height(height) {
    
	if (!glfwInit()) {
	    throw std::runtime_error("Failed to initialize GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    
	if (!window) {
	    glfwTerminate();
	    throw std::runtime_error("Failed to create GLFW window");
	}
    }

    Window::~Window() {
	if (window) {
	    glfwDestroyWindow(window);
	}
	glfwTerminate();
    }

    bool Window::shouldClose() const {
	return glfwWindowShouldClose(window);
    }

    void Window::pollEvents() const {
	glfwPollEvents();
    }

    void Window::waitEvents() const {
	glfwWaitEvents();
    }

    std::vector<const char*> Window::getRequiredInstanceExtensions() const {
	uint32_t extensionCount = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
	return std::vector<const char*>(extensions, extensions + extensionCount);
    }
}
