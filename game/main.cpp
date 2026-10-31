#include "engine/core/Log.h"
#include "game/app/Game.h"

#include <SDL3/SDL_main.h>

#include <exception>

int main(int argc, char** argv) {
    try {
        const auto options = br::game::LaunchOptions::parse(argc, argv);
        br::game::Game game;
        return game.run(options);
    } catch (const std::exception& e) {
        BR_LOG_FATAL("main", "Unhandled exception: {}", e.what());
        return 1;
    }
}
