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

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetWindowSizeCallback(window, windowSizeCallback);
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

void Window::requestClose() const { glfwSetWindowShouldClose(window, GLFW_TRUE); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

bool Window::isKeyPressed(int key) const { return glfwGetKey(window, key) == GLFW_PRESS; }

bool Window::isMouseButtonPressed(int button) const { return glfwGetMouseButton(window, button) == GLFW_PRESS; }

std::pair<double, double> Window::getMousePosition() const {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return {x, y};
}

float Window::consumeScrollDelta() {
    float delta = scrollDelta;
    scrollDelta = 0.0f;
    return delta;
}

void Window::scrollCallback(GLFWwindow *win, double /*xoffset*/, double yoffset) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(win));
    self->scrollDelta += static_cast<float>(yoffset);
}

void Window::windowSizeCallback(GLFWwindow *win, int w, int h) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(win));
    self->width = static_cast<uint32_t>(w);
    self->height = static_cast<uint32_t>(h);
}

std::vector<const char *> Window::getRequiredInstanceExtensions() const {
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    auto extension_vec = std::vector<const char *>(extensions, extensions + extensionCount);
    extension_vec.push_back(vk::EXTDebugUtilsExtensionName);

    return extension_vec;
}
} // namespace VulkanHelpers
