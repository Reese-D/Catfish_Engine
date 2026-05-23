#ifndef VALIDATION_LAYERS_H
#define VALIDATION_LAYERS_H

#include <string>
#include <vector>
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

  private:
    std::vector<const char *> validationLayers;
};

} // namespace VulkanHelpers

#endif // VALIDATION_LAYERS_H