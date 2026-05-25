#ifndef SURFACE_H
#define SURFACE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "window.h"

namespace VulkanHelpers {

class Surface {
  public:
    Surface(const vk::raii::Instance &instance, const Window &window);
    ~Surface() = default;

    Surface(const Surface &) = delete;
    Surface &operator=(const Surface &) = delete;

    std::shared_ptr<vk::raii::SurfaceKHR> getSurface() const { return surface; }

  private:
    std::shared_ptr<vk::raii::SurfaceKHR> surface;
};

} // namespace VulkanHelpers

#endif // SURFACE_H
