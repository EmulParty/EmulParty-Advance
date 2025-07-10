// src/main.cpp
#include "mode_selector.hpp"
#include <iostream>

/**
 * @brief CHIP-8 Dual Mode Emulator
 * 
 * 8비트 모드(.ch8/.c8)와 32비트 확장 모드(.ch32/.c32)를 지원하는
 * 통합 CHIP-8 에뮬레이터입니다.
 */
int main(int argc, char* argv[]) {
    std::cout << "CHIP-8 Dual Mode Emulator v1.0" << std::endl;
    std::cout << "Supports both 8-bit and 32-bit extended modes" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    // 명령줄 인자 검증
    if (argc != 2) {
        std::cerr << "\n[ERROR] Invalid number of arguments" << std::endl;
        std::cerr << "\nUsage: " << argv[0] << " <ROM_file>" << std::endl;
        std::cerr << "\nSupported file extensions:" << std::endl;
        std::cerr << "  .ch8, .c8   - 8-bit CHIP-8 mode (original)" << std::endl;
        std::cerr << "  .ch32, .c32 - 32-bit CHIP-8 extended mode" << std::endl;
        std::cerr << "\nExamples:" << std::endl;
        std::cerr << "  " << argv[0] << " roms/tetris.ch8" << std::endl;
        std::cerr << "  " << argv[0] << " roms/pong.c8" << std::endl;
        std::cerr << "  " << argv[0] << " roms/advanced_demo.ch32" << std::endl;
        std::cerr << "  " << argv[0] << " roms/extended_game.c32" << std::endl;
        return 1;
    }
    
    // ModeSelector를 통해 적절한 에뮬레이터 실행
    int result = ModeSelector::select_and_run(argv[1]);
    
    if (result == 0) {
        std::cout << "\n[INFO] Emulator terminated successfully" << std::endl;
    } else {
        std::cout << "\n[ERROR] Emulator terminated with error code: " << result << std::endl;
    }
    
    return result;
}