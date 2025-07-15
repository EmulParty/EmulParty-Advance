#pragma once

#include <cstdint>

namespace timer {
    uint32_t get_ticks();
    void delay(uint32_t ms);
}
