#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

#include "surface.h"

namespace VulkanHelpers {

Surface::Surface(const vk::raii::Instance &instance, const Window &window) {
    VkSurfaceKHR rawSurface{};
    if (glfwCreateWindowSurface(*instance, window.getWindow(), nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window m_surface");
    }
    m_surface = std::make_shared<vk::raii::SurfaceKHR>(instance, rawSurface);
}

} // namespace VulkanHelpers
