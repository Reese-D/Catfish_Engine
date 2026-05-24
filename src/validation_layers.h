#ifndef VALIDATION_LAYERS_H
#define VALIDATION_LAYERS_H

#include <vector>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace VulkanHelpers {

class ValidationLayers {
  public:
    ValidationLayers();
    ~ValidationLayers() = default;

    // Check if validation layers are supported
    bool areValidationLayersSupported(const std::vector<const char *> &validationLayers, const vk::raii::Context &context) const;

    // Check if required extensions are supported
    bool areRequiredExtensionsSupported(const std::vector<const char *> &requiredExtensions, const vk::raii::Context &context) const;

    // Get the required layers
    std::vector<const char *> getRequiredLayers() const;

    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> createDebugMessenger(const vk::raii::Instance &instance,  vk::PFN_DebugUtilsMessengerCallbackEXT debugCallback);
  private:
    std::vector<const char *> validationLayers;
    std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> debugMessenger;
};

} // namespace VulkanHelpers

#endif // VALIDATION_LAYERS_H
