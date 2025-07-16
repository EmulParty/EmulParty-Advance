#include "../../include/core/mode_selector.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " [--debug] <rom_file>\n";
        std::cout << "Options:\n";
        std::cout << "  --debug    Enable interactive debugger\n";
        std::cout << "\nExamples:\n";
        std::cout << "  " << argv[0] << " roms/pong.ch8\n";
        std::cout << "  " << argv[0] << " --debug roms/pong.ch8\n";
        return 1;
    }
    
    bool debug_mode = false;
    const char* rom_path = nullptr;
    
    // 명령행 인수 파싱
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
        } else {
            rom_path = argv[i];
        }
    }
    
    if (!rom_path) {
        std::cerr << "Error: No ROM file specified\n";
        return 1;
    }
    
    // 디버그 모드 설정
    ModeSelector::set_debug_mode(debug_mode);
    
    // 실행
    return ModeSelector::select_and_run(rom_path);
}