#ifndef RTS_GAME_CLIENT_H
#define RTS_GAME_CLIENT_H

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include "engine.h"
#include "hud_system.h"
#include "menu_system.h"
#include "model.h"
#include "rts_game_base.h"
#include "terrain.h"

namespace Game {

class RtsGameClient : public RtsGameBase, public VulkanHelpers::IGame {
  public:
    void initLogic() override;
    void initGraphics(const VulkanHelpers::ResourceContext &ctx) override;
    VulkanHelpers::FrameOutput update(float dt, vk::Extent2D extent) override;
    void renderImGui(vk::CommandBuffer cmd) override;
    void onSwapChainRecreated(const VulkanHelpers::SwapChain &swapChain) override;
    bool wantsMouse() const override;
    bool wantsKeyboard() const override;
    bool wantsClose() const override { return closeRequested; }

  private:
    void clientApplySnapshot(const uint8_t *data, std::size_t size);
    void clientCaptureAndSendInput(vk::Extent2D extent);

    VulkanHelpers::Window *window{nullptr};

    std::shared_ptr<VulkanHelpers::Model> unitModel;
    std::shared_ptr<VulkanHelpers::Terrain> terrain;
    std::shared_ptr<VulkanHelpers::Model> selectionRingModel;
    std::shared_ptr<VulkanHelpers::Model> projectileModel;
    std::shared_ptr<VulkanHelpers::Model> lavaTileModel;
    VulkanHelpers::HudResources hudResources;
    std::shared_ptr<VulkanHelpers::MenuSystem> menuSystem;

    bool prevMouseRight_{false};
    bool prevKeyQ_{false};
    bool prevKeyE_{false};
};

} // namespace Game

#endif // RTS_GAME_CLIENT_H
