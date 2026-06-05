#ifndef MENU_SYSTEM_H
#define MENU_SYSTEM_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "swap_chain.h"
#include "window.h"

namespace VulkanHelpers {

enum class MenuAction {
    None,
    Play,
    Exit,
};

class MenuSystem {
  public:
    MenuSystem(
        const Window &window, const vk::raii::Instance &instance, const vk::raii::PhysicalDevice &physicalDevice, const vk::raii::Device &device, uint32_t graphicsQueueFamilyIndex,
        const vk::raii::Queue &graphicsQueue, const SwapChain &swapChain, vk::Format depthFormat
    );
    ~MenuSystem();

    MenuSystem(const MenuSystem &) = delete;
    MenuSystem &operator=(const MenuSystem &) = delete;

    void beginFrame();
    MenuAction drawMainMenu(vk::Extent2D extent);
    void drawOverlay(float deltaTime);
    void render(vk::CommandBuffer commandBuffer);
    void onSwapChainRecreated(const SwapChain &swapChain);

    bool isGameplayStarted() const { return m_gameplayStarted; }
    bool wantsMouse() const;
    bool wantsKeyboard() const;

  private:
    bool m_gameplayStarted{false};
    bool m_showOverlay{true};
};

} // namespace VulkanHelpers

#endif // MENU_SYSTEM_H
