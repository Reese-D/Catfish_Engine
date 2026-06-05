#include "window.h"
#include <iostream>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>
namespace VulkanHelpers {
Window::Window(uint32_t mWidth, uint32_t mHeight, const char *title) : m_width(mWidth), m_height(mHeight) {

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(mWidth, mHeight, title, nullptr, nullptr);

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW m_window");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetWindowSizeCallback(m_window, windowSizeCallback);
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

std::pair<int, int> Window::getFramebufferSize() const {
    int mWidth, mHeight;
    glfwGetFramebufferSize(m_window, &mWidth, &mHeight);
    return {mWidth, mHeight};
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }

void Window::requestClose() const { glfwSetWindowShouldClose(m_window, GLFW_TRUE); }

void Window::pollEvents() const { glfwPollEvents(); }

void Window::waitEvents() const { glfwWaitEvents(); }

bool Window::isKeyPressed(int key) const { return glfwGetKey(m_window, key) == GLFW_PRESS; }

bool Window::isMouseButtonPressed(int button) const { return glfwGetMouseButton(m_window, button) == GLFW_PRESS; }

std::pair<double, double> Window::getMousePosition() const {
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    return {x, y};
}

float Window::consumeScrollDelta() {
    float delta = m_scrollDelta;
    m_scrollDelta = 0.0f;
    return delta;
}

void Window::scrollCallback(GLFWwindow *win, double /*xoffset*/, double yoffset) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(win));
    self->m_scrollDelta += static_cast<float>(yoffset);
}

void Window::windowSizeCallback(GLFWwindow *win, int w, int h) {
    auto *self = static_cast<Window *>(glfwGetWindowUserPointer(win));
    self->m_width = static_cast<uint32_t>(w);
    self->m_height = static_cast<uint32_t>(h);
}

std::vector<const char *> Window::getRequiredInstanceExtensions() const {
    uint32_t extensionCount = 0;
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
    auto extensionVec = std::vector<const char *>(extensions, extensions + extensionCount);
    extensionVec.push_back(vk::EXTDebugUtilsExtensionName);

    return extensionVec;
}
} // namespace VulkanHelpers
