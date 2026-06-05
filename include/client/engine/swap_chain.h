#ifndef SWAP_CHAIN_H
#define SWAP_CHAIN_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "window.h"

namespace VulkanHelpers {

class SwapChain {
  public:
    SwapChain(
        const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
        const Window &window
    );
    ~SwapChain() = default;

    SwapChain(const SwapChain &) = delete;
    SwapChain &operator=(const SwapChain &) = delete;

    std::shared_ptr<vk::raii::SwapchainKHR> getSwapChain() const { return m_swapChain; }
    const std::vector<vk::Image> &getImages() const { return m_images; }
    const std::vector<vk::raii::ImageView> &getImageViews() const { return m_imageViews; }
    vk::Format getFormat() const { return m_format; }
    vk::Extent2D getExtent() const { return m_extent; }

    void recreate(
        const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
        const Window &window
    );

  private:
    void create(
        const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface, uint32_t graphicsFamily, uint32_t presentFamily,
        const Window &window
    );
    void createImageViews(const vk::raii::Device &device);

    static vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats);
    static vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &modes);
    static vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR &capabilities, const Window &window);

    std::shared_ptr<vk::raii::SwapchainKHR> m_swapChain;
    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;
    vk::Format m_format{};
    vk::Extent2D m_extent{};
};

} // namespace VulkanHelpers

#endif // SWAP_CHAIN_H
