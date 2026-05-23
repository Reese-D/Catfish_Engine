#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <vector>
#include <memory>

#include "window.h"

namespace Vulkan {

class Instance {
public:
    Instance(const std::shared_ptr<Window::Window>& window);
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

    std::shared_ptr<vk::raii::Instance> getInstance() const { return instance; }

private:
    void createInstance(const std::shared_ptr<Window::Window>& window);

    vk::raii::Context context;
    std::shared_ptr<vk::raii::Instance> instance;
};

} // namespace Vulkan

#endif // VULKAN_INSTANCE_H
