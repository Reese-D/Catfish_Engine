#ifndef RTS_GAME_SERVER_H
#define RTS_GAME_SERVER_H

#include "headless_runner.h"
#include "rts_game_base.h"

namespace Game {

class RtsGameServer : public RtsGameBase, public VulkanHelpers::IServerGame {
  public:
    void initLogic() override;
    void update(float dt) override;
    bool wantsClose() const override { return closeRequested; }
};

} // namespace Game

#endif // RTS_GAME_SERVER_H
