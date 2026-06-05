#include "headless_runner.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace VulkanHelpers {

void HeadlessRunner::run(IServerGame &game) {
    game.initLogic();
    std::cout << "[Server] Headless simulation running at " << m_tickHz << " Hz\n";

    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    const float tick = 1.0f / m_tickHz;
    const Duration tickDur = std::chrono::duration_cast<Duration>(std::chrono::duration<float>(tick));

    auto nextTick = Clock::now();

    while (!game.wantsClose()) {
        game.update(tick);
        nextTick += tickDur;
        std::this_thread::sleep_until(nextTick);
    }

    std::cout << "[Server] Shutting down\n";
}

} // namespace VulkanHelpers
