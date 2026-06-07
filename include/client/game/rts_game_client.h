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
    bool wantsClose() const override { return m_closeRequested; }

  private:
    void clientApplySnapshot(const uint8_t *data, std::size_t size);
    void clientCaptureAndSendInput(vk::Extent2D extent);

    VulkanHelpers::Window *m_window{nullptr};

    std::shared_ptr<VulkanHelpers::Model> m_unitModel;
    std::shared_ptr<VulkanHelpers::Model> m_rockModel;
    std::shared_ptr<VulkanHelpers::Terrain> m_terrain;
    std::shared_ptr<VulkanHelpers::Model> m_selectionRingModel;
    std::shared_ptr<VulkanHelpers::Model> m_projectileModel;
    std::shared_ptr<VulkanHelpers::Model> m_gravityWellModel;
    std::shared_ptr<VulkanHelpers::Model> m_lightningModel;
    std::shared_ptr<VulkanHelpers::Model> m_lavaTileModel;

    float m_elapsedTime{0.0f};
    VulkanHelpers::HudResources m_hudResources;
    std::shared_ptr<VulkanHelpers::MenuSystem> m_menuSystem;

    bool m_prevMouseRight{false};
    bool m_prevKeyQ{false};
    bool m_prevKeyE{false};
    bool m_prevKeyR{false};
    bool m_prevKeyF{false};
};

} // namespace Game

#endif // RTS_GAME_CLIENT_H
