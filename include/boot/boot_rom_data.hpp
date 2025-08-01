// boot_rom_data.hpp - 자동 크기 조정 (더 안전한 방법)
#pragma once
#include <cstdint>
#include <cstddef>

extern const uint32_t BOOT_ROM[];  // 크기 제거 - 자동으로 맞춤
extern const size_t BOOT_ROM_SIZE;  // 크기 상수 추가