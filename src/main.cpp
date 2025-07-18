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
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [--debug]\n";
            std::cout << "BootROM will handle file selection automatically.\n";
            return 0;
        }
    }

    print_banner();
    
    if (debug_mode) {
        std::cout << "🐛 Debug mode enabled\n";
    }

    ModeSelector::set_debug_mode(debug_mode);
    
    std::cout << "🚀 Starting BootROM-driven emulator...\n";
    int result = ModeSelector::select_and_run();

    std::cout << "\n✨ Emulator terminated " << (result == 0 ? "successfully" : "with errors") << "\n";
    return result;
}