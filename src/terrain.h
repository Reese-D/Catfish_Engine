#ifndef TERRAIN_H
#define TERRAIN_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "model.h"

namespace VulkanHelpers {

// Builds a flat grid terrain mesh with a procedurally generated grass texture.
class Terrain {
  public:
    // extent: half-size in world units (terrain spans [-extent, +extent] in XY)
    // cells:  number of grid cells per axis
    Terrain(
        const vk::raii::Device &device, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::CommandPool &commandPool, const vk::raii::Queue &graphicsQueue,
        const vk::raii::DescriptorSetLayout &textureLayout, float extent = 20.0f, int cells = 40
    );
    ~Terrain() = default;

    Terrain(const Terrain &) = delete;
    Terrain &operator=(const Terrain &) = delete;

    const Model &getModel() const { return *model; }
    std::shared_ptr<Model> getModelPtr() const { return model; }

  private:
    std::shared_ptr<Model> model;
};

} // namespace VulkanHelpers

#endif // TERRAIN_H
