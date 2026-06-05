#ifndef HEADLESS_RUNNER_H
#define HEADLESS_RUNNER_H

#include "i_server_game.h"

namespace VulkanHelpers {

// Runs an IServerGame without any Vulkan context — suitable for a dedicated server.
// Calls initLogic() once, then ticks the game at a fixed rate.
class HeadlessRunner {
  public:
    explicit HeadlessRunner(float tickHz = 60.0f) : m_tickHz(tickHz) {}
    void run(IServerGame &game);

  private:
    float m_tickHz;
};

} // namespace VulkanHelpers

#endif // HEADLESS_RUNNER_H
