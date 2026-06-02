#include "headless_runner.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace VulkanHelpers {

void HeadlessRunner::run(IGame &game) {
    game.initLogic();
    std::cout << "[Server] Headless simulation running at " << tickHz_ << " Hz\n";

    // A fake extent is passed each tick — the server never uses it for rendering,
    // but IGame::update() requires one for its interface contract.
    constexpr vk::Extent2D FAKE_EXTENT{800, 600};

    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    const float tick = 1.0f / tickHz_;
    const Duration tickDur = std::chrono::duration_cast<Duration>(std::chrono::duration<float>(tick));

    auto nextTick = Clock::now();

    while (!game.wantsClose()) {
        game.update(tick, FAKE_EXTENT);
        nextTick += tickDur;
        std::this_thread::sleep_until(nextTick);
    }

    std::cout << "[Server] Shutting down\n";
}

} // namespace VulkanHelpers
