// main.cpp - 올바른 main 함수만 포함
#include "mode_selector.hpp"
#include <iostream>
#include <string>
#include <cstdlib>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n";
    std::cout << "32-bit CHIP-8 Extended Emulator\n\n";
    std::cout << "Options:\n";
    std::cout << "  -d, --debug     Enable debug mode\n";
    std::cout << "  -h, --help      Show this help\n";
    std::cout << "  -v, --version   Show version\n";
}

void print_version() {
    std::cout << "CHIP-8 Extended Emulator v2.0\n";
    std::cout << "32-bit Enhanced Mode\n";
}

void print_banner() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                CHIP-8 Extended Emulator v2.0                 ║\n";
    std::cout << "║                   32-bit Enhanced Mode                       ║\n";
    std::cout << "╠═══════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Features: Stack Frames • Debugger • SDL2 Interface         ║\n";
    std::cout << "║  ROM Files: Place files in ../roms/ directory               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    bool debug_mode = false;
    bool show_help = false;
    bool show_version = false;

    // 명령행 인수 파싱
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
        }
        else if (arg == "--help" || arg == "-h") {
            show_help = true;
        }
        else if (arg == "--version" || arg == "-v") {
            show_version = true;
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            return 1;
        }
    }

    if (show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (show_version) {
        print_version();
        return 0;
    }

    print_banner();

    std::cout << "CHIP-8 Emulator with Enhanced File Input\n";
    std::cout << "Enter ROM filename in the SDL window\n";
    std::cout << "ROM files should be in ../roms/ directory\n\n";

    if (debug_mode) {
        std::cout << "🐛 Debug mode enabled\n";
    }

    ModeSelector::set_debug_mode(debug_mode);

    std::cout << "Initializing emulator...\n";
    int result = ModeSelector::select_and_run();

    if (result == 0) {
        std::cout << "\nEmulator terminated successfully.\n";
    } else {
        std::cout << "\nEmulator terminated with error: " << result << std::endl;
    }

    return result;
}