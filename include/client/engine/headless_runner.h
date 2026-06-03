#ifndef HEADLESS_RUNNER_H
#define HEADLESS_RUNNER_H

// Runs an IGame without any Vulkan context — suitable for a dedicated server.
// Calls initLogic() but not initGraphics(), then ticks the game at a fixed rate.

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp> // needed only for vk::Extent2D in IGame::update

#include "engine.h"

namespace VulkanHelpers {

class HeadlessRunner {
  public:
    // Tick rate the server loop targets (default 60 Hz).
    explicit HeadlessRunner(float tickHz = 60.0f) : tickHz_(tickHz) {}

    void run(IGame &game);

  private:
    float tickHz_;
};

} // namespace VulkanHelpers

#endif // HEADLESS_RUNNER_H
