#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

#include "engine.h"
#include "rts_game_client.h"

int main(int argc, char *argv[]) {
    try {
        std::string host;
        uint16_t port = 1234;
        bool modeSet = false;

        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
                modeSet = true;
            } else if (arg == "--port" && i + 1 < argc) {
                port = static_cast<uint16_t>(std::atoi(argv[++i]));
            }
        }

        if (!modeSet) {
            std::cerr << "Usage: " << argv[0] << " --host <server> [--port <port>]\n";
            return EXIT_FAILURE;
        }

        Game::RtsGameClient game;
        game.enableFogOfWar();
        game.enableMinimap();
        game.setupAsClient(std::move(host), port);

        VulkanHelpers::Engine engine;
        engine.run(game);

    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
