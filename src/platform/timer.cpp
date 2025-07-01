#include "timer.hpp"

#include <SDL2/SDL.h>
#include <thread>
#include <chrono>

namespace timer {

    // SDL_GetTicks()를 사용해 현재 시간(ms) 반환
    uint32_t get_ticks() {
        return SDL_GetTicks();
    }

    // std::this_thread::sleep_for를 이용한 지연 함수
    void delay(uint32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

} // namespace timer
