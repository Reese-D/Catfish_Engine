#include <cstdlib>
#include <iostream>
#include <string_view>

#include "engine.h"
#include "headless_runner.h"
#include "rts_game.h"

int main(int argc, char *argv[]) {
    try {
        Game::RtsGame game;
        game.enablePathfinding();
        game.enableFogOfWar();
        game.enableMinimap();

        bool isServer = false;

        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--server") {
                isServer = true;
                uint16_t port = (i + 1 < argc && argv[i+1][0] != '-')
                                    ? static_cast<uint16_t>(std::atoi(argv[++i])) : 1234;
                game.setupAsServer(port);
            } else if (arg == "--client") {
                if (i + 1 >= argc) { std::cerr << "Usage: --client <host> [port]\n"; return EXIT_FAILURE; }
                std::string host = argv[++i];
                uint16_t port    = (i + 1 < argc && argv[i+1][0] != '-')
                                       ? static_cast<uint16_t>(std::atoi(argv[++i])) : 1234;
                game.setupAsClient(std::move(host), port);
            }
        }

        if (isServer) {
            VulkanHelpers::HeadlessRunner runner;
            runner.run(game);
        } else {
            VulkanHelpers::Engine engine;
            engine.run(game);
        }

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Exiting cleanly\n";
    return EXIT_SUCCESS;
}
