#include "validation_layers.h"
#include <algorithm>
#include <cstring>

namespace VulkanHelpers {

ValidationLayers::ValidationLayers() {
    // Initialize with the standard validation layer
    m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
}

std::vector<const char *> ValidationLayers::getRequiredLayers() const { return m_validationLayers; }

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

std::shared_ptr<vk::raii::DebugUtilsMessengerEXT> ValidationLayers::createDebugMessenger(const vk::raii::Instance &instance, vk::PFN_DebugUtilsMessengerCallbackEXT debugCallback) {
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{.messageSeverity = severityFlags, .messageType = messageTypeFlags, .pfnUserCallback = debugCallback};

    m_debugMessenger = std::make_shared<vk::raii::DebugUtilsMessengerEXT>(instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT));

    return m_debugMessenger;
}

} // namespace VulkanHelpers
