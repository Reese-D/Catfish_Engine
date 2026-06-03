#include <cstdlib>
#include <iostream>
#include <string_view>

#include "headless_runner.h"
#include "rts_game_server.h"

int main(int argc, char *argv[]) {
    try {
        uint16_t port = 1234;

        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--port" && i + 1 < argc)
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }

        Game::RtsGameServer game;
        game.enableCombat();
        game.setupAsServer(port);

        VulkanHelpers::HeadlessRunner runner;
        runner.run(game);

    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
