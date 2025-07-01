#pragma once

#include <cstdint>

/**
 * @file timer.hpp
 * @brief Timer 유틸리티 함수 선언 (60Hz 타이머 처리를 위한 wrapper)
 */

namespace timer {

    /**
     * @brief 현재 시간을 밀리초 단위로 반환
     *
     * SDL_GetTicks() 또는 std::chrono 기반으로 구현됨
     * @return uint32_t 현재 시간(ms)
     */
    uint32_t get_ticks();

    /**
     * @brief 주어진 밀리초(ms) 동안 대기
     *
     * SDL_Delay() 또는 std::this_thread::sleep_for 사용
     * @param ms 대기할 시간 (ms)
     */
    void delay(uint32_t ms);

} // namespace timer
