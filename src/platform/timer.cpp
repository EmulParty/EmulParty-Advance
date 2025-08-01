#include "timer.hpp"
#include <SDL2/SDL.h>

namespace timer {
    uint32_t get_ticks() {
        return SDL_GetTicks();
    }

    void delay(uint32_t ms) {
        SDL_Delay(ms);
    }
}
