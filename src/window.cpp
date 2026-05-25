#include "window.h"
#include <iostream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
namespace VulkanHelpers {
Window::Window(uint32_t width, uint32_t height, const char *title) : width(width), height(height) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

std::pair<int, int> Window::getFramebufferSize() const {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return {width, height};
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

std::vector<const char *> Window::getRequiredInstanceExtensions() const {
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    auto extension_vec = std::vector<const char *>(extensions, extensions + extensionCount);
    extension_vec.push_back(vk::EXTDebugUtilsExtensionName);

    return extension_vec;
}
} // namespace VulkanHelpers
