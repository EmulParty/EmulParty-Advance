// mode_selector.hpp - 완전 통합 BootROM 아키텍처 (deprecated 함수 제거)
#pragma once
#include <string>
#include "common/constants.hpp"

// 전방 선언
class Platform;
class Chip8_32;

class ModeSelector {
public:
    // **메인 진입점 - 통합 BootROM 시스템**
    static int select_and_run();
    
    // **디버그 모드 설정**
    static void set_debug_mode(bool enable);
    
    // **SYSCALL에서 호출할 함수들**
    static bool load_and_switch_mode(Chip8_32& chip8_32, const std::string& filename);
    static std::string get_file_extension(const std::string& filename);

private:
    // **내부 구현 함수들**
    static int run_unified_bootrom_mode();
    static int run_8bit_mode_after_bootrom(Platform& platform);
    
    // **Deprecated 함수들 완전 제거됨**
};