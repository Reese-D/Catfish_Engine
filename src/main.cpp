#include <cstdlib>
#include <iostream>

#include "engine.h"
#include "rts_game.h"

int main() {
    try {
        Game::RtsGame game;
        game.enablePathfinding();
        game.enableFogOfWar();
        game.enableMinimap();

        VulkanHelpers::Engine engine;
        engine.run(game);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Exiting cleanly\n";
    return EXIT_SUCCESS;
}
