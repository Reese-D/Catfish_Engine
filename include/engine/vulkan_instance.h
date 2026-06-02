#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "window.h"

namespace VulkanHelpers {

class Instance {
  public:
    Instance(const std::shared_ptr<Window> &window);
    ~Instance();

    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;

    std::shared_ptr<vk::raii::Instance> getInstance() const { return instance; }
    std::vector<const char *> getRequiredInstanceExtensions();
    std::shared_ptr<vk::raii::Context> getContext() const { return context; }

  private:
    void createInstance(const std::shared_ptr<Window> &window);

    std::shared_ptr<vk::raii::Context> context;
    std::shared_ptr<vk::raii::Instance> instance;
};

} // namespace VulkanHelpers

#endif // VULKAN_INSTANCE_H
