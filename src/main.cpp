// src/main.cpp - BootROM 중심 실행 (간소화)
#include "mode_selector.hpp"
#include <iostream>
#include <string>

void print_banner() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                CHIP-8 Extended Emulator v2.0                     ║\n";
    std::cout << "║                   BootROM-Driven Architecture                    ║\n";
    std::cout << "╠══════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Features: BootROM • SYSCALL • I/O Redirection • Auto-Detection  ║\n";
    std::cout << "║  Modes: 8-bit CHIP-8 (compatibility) + 32-bit Extended          ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    bool debug_mode = false;
    
    // 간단한 인수 파싱
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
        } else if (arg == "--mode") {
            // --mode 옵션은 호환성을 위해 인식하지만 무시
            // (항상 32비트 BootROM으로 시작하여 자동 모드 선택)
            if (i + 1 < argc) {
                i++; // 다음 인수 건너뛰기
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--debug] [--mode 32]\n";
            std::cout << "  --debug, -d     Enable debug mode with step-by-step execution\n";
            std::cout << "  --mode 32       Legacy option (always starts in 32-bit mode)\n";
            std::cout << "\nBootROM will handle file selection and mode switching automatically.\n";
            std::cout << "Use 'sf' command in debug mode for stack frame visualization.\n";
            return 0;
        }
    }

    print_banner();
    
    if (debug_mode) {
        std::cout << "Debug mode enabled\n";
    }

    ModeSelector::set_debug_mode(debug_mode);
    
    std::cout << "Starting BootROM-driven emulator...\n";
    int result = ModeSelector::select_and_run();

    std::cout << "\nEmulator terminated " << (result == 0 ? "successfully" : "with errors") << "\n";
    return result;
}