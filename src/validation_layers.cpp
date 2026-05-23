#include "validation_layers.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace VulkanHelpers {

ValidationLayers::ValidationLayers() {
    // Initialize with the standard validation layer
    validationLayers = {"VK_LAYER_KHRONOS_validation"};
}

std::vector<const char *> ValidationLayers::getRequiredLayers() const { return validationLayers; }

bool ValidationLayers::areValidationLayersSupported(const std::vector<const char *> &validationLayers, const vk::raii::Context &context) const {

    // Get the available layer properties
    auto layerProperties = context.enumerateInstanceLayerProperties();

    // Check if all required layers are supported
    for (const auto &requiredLayer : validationLayers) {
        auto layerIt = std::ranges::find_if(layerProperties, [requiredLayer](const auto &layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });

        if (layerIt == layerProperties.end()) {
            return false;
        }
    }

    return true;
}

bool ValidationLayers::areRequiredExtensionsSupported(const std::vector<const char *> &requiredExtensions, const vk::raii::Context &context) const {

    // Get the available extension properties
    auto extensionProperties = context.enumerateInstanceExtensionProperties();

    // Check if all required extensions are supported
    for (const auto &requiredExtension : requiredExtensions) {
        auto extensionIt =
            std::ranges::find_if(extensionProperties, [requiredExtension](const auto &extensionProperty) { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; });

        if (extensionIt == extensionProperties.end()) {
            return false;
        }
    }

    return true;
}

} // namespace VulkanHelpers