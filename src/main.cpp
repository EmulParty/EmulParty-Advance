// src/main.cpp - BootROM 중심 실행 (간소화)
#include "mode_selector.hpp"
#include <cstdlib>
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
    bool headless_mode = false;
    std::string startup_rom;
    int max_frames = -1;
    
    // 간단한 인수 파싱
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
        } else if (arg == "--headless") {
            headless_mode = true;
        } else if (arg == "--rom") {
            if (i + 1 >= argc) {
                std::cerr << "--rom requires a ROM filename\n";
                return 1;
            }
            startup_rom = argv[++i];
        } else if (arg == "--max-frames") {
            if (i + 1 >= argc) {
                std::cerr << "--max-frames requires a number\n";
                return 1;
            }
            max_frames = std::stoi(argv[++i]);
        } else if (arg == "--mode") {
            // --mode 옵션은 호환성을 위해 인식하지만 무시
            // (항상 32비트 BootROM으로 시작하여 자동 모드 선택)
            if (i + 1 < argc) {
                i++; // 다음 인수 건너뛰기
            }
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--debug] [--mode 32] [--rom FILE] [--headless] [--max-frames N]\n";
            std::cout << "  --debug, -d     Enable debug mode with step-by-step execution\n";
            std::cout << "  --mode 32       Legacy option (always starts in 32-bit mode)\n";
            std::cout << "  --rom FILE      Queue a ROM filename for BootROM loading\n";
            std::cout << "  --headless      Use SDL's dummy video driver for CI/server runs\n";
            std::cout << "  --max-frames N  Exit after N frames; useful with --headless\n";
            std::cout << "\nBootROM will handle file selection and mode switching automatically.\n";
            std::cout << "Use 'sf' command in debug mode for stack frame visualization.\n";
            return 0;
        }
    }

    if (headless_mode) {
        if (startup_rom.empty()) {
            std::cerr << "--headless requires --rom so BootROM input does not wait forever\n";
            return 1;
        }
        if (max_frames < 0) {
            max_frames = 300;
        }
        setenv("SDL_VIDEODRIVER", "dummy", 0);
    }

    print_banner();
    
    if (debug_mode) {
        std::cout << "Debug mode enabled\n";
    }

    ModeSelector::set_debug_mode(debug_mode);
    ModeSelector::set_startup_rom(startup_rom);
    ModeSelector::set_max_frames(max_frames);
    
    std::cout << "Starting BootROM-driven emulator...\n";
    int result = ModeSelector::select_and_run();

    std::cout << "\nEmulator terminated " << (result == 0 ? "successfully" : "with errors") << "\n";
    return result;
}
