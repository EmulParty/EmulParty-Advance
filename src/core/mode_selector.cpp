#include "../../include/core/mode_selector.hpp"
#include "../../include/core/chip8.hpp"
#include "../../include/core/chip8_32.hpp"
#include "../../include/core/opcode_table.hpp"
#include "../../include/core/opcode_table_32.hpp"
#include "../../include/platform/platform.hpp"
#include "../../include/platform/timer.hpp"
#include "../../include/debugger/debugger.hpp"
#include <iostream>
#include <algorithm>

// 전역 변수로 디버그 모드 플래그
static bool g_debug_mode = false;

void ModeSelector::set_debug_mode(bool enable) {
    g_debug_mode = enable;
}

int ModeSelector::select_and_run(const char* rom_path) {
    std::string extension = get_file_extension(rom_path);
    
    if (extension == ".ch8" || extension == ".c8") {
        std::cout << "[INFO] Detected 8-bit CHIP-8 ROM: " << rom_path << std::endl;
        return run_8bit_mode(rom_path);
    }
    else if (extension == ".ch32" || extension == ".c32") {
        std::cout << "[INFO] Detected 32-bit CHIP-8 ROM: " << rom_path << std::endl;
        return run_32bit_mode(rom_path);
    }
    else {
        std::cerr << "[ERROR] Unsupported file extension '" << extension << "'" << std::endl;
        std::cerr << "Supported extensions:" << std::endl;
        std::cerr << "  .ch8, .c8   - 8-bit CHIP-8 mode" << std::endl;
        std::cerr << "  .ch32, .c32 - 32-bit CHIP-8 extended mode" << std::endl;
        return 1;
    }
}

int ModeSelector::run_8bit_mode(const char* rom_path) {
    std::cout << "\n=== Starting 8-bit CHIP-8 Emulator ===" << std::endl;
    
    // 8비트 전용 초기화
    OpcodeTable::Initialize();
    Chip8 chip8;
    
    // 디버거 생성
    chip8emu::Debugger8 debugger(chip8);
    if (g_debug_mode) {
        debugger.enable(true);
        debugger.setStepMode(true);
        std::cout << "🐛 Debug mode enabled for 8-bit CHIP-8\n";
    }
    
    // 플랫폼 초기화
    Platform platform("CHIP-8 Emulator (8-bit Mode)", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }
    
    // ROM 로드
    if (!chip8.load_rom(rom_path)) {
        std::cerr << "[ERROR] Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }
    
    // 시스템 정보 출력
    std::cout << "[INFO] 8-bit CHIP-8 System Ready" << std::endl;
    std::cout << "  Memory: 4KB (4096 bytes)" << std::endl;
    std::cout << "  Registers: 16 x 8-bit (V0-VF)" << std::endl;
    std::cout << "  Stack: 16 levels" << std::endl;
    std::cout << "  Instruction Size: 2 bytes" << std::endl;
    std::cout << "  Controls: 1234/QWER/ASDF/ZXCV" << std::endl;
    
    if (g_debug_mode) {
        std::cout << "  🐛 Debug Mode: ON (Type 'help' for commands)" << std::endl;
    }
    
    // 메인 루프 - 수정된 부분
    bool quit = false;
    bool debugger_active = true;  // 디버거 상태를 별도로 관리
    
    while (!quit && debugger_active) {
        // 입력 처리
        quit = platform.ProcessInput(chip8.keypad);
        
        // 디버그 정보 출력 (실행 전) - 디버그 모드일 때만
        if (debugger.isEnabled()) {
            uint32_t current_opcode = chip8.getCurrentOpcode();
            debugger.printState(current_opcode);
            
            // 디버거에서 quit 명령을 받았는지 확인
            if (!debugger.isEnabled()) {
                debugger_active = false;
                break;
            }
        }
        
        // CPU 사이클 실행
        chip8.cycle();
        
        // 타이머 업데이트 (60Hz)
        if (chip8.delay_timer > 0) chip8.delay_timer--;
        if (chip8.sound_timer > 0) chip8.sound_timer--;
        
        // 화면 업데이트
        if (chip8.needs_redraw()) {
            platform.Update(chip8.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8.clear_draw_flag();
        }
        
        timer::delay(g_debug_mode ? 100 : 16); // 디버그 모드에서는 느리게
    }
    
    std::cout << "[INFO] 8-bit CHIP-8 emulator terminated" << std::endl;
    return 0;
}

int ModeSelector::run_32bit_mode(const char* rom_path) {
    std::cout << "\n=== Starting 32-bit CHIP-8 Extended Emulator ===" << std::endl;
    
    // 32비트 전용 초기화
    OpcodeTable_32::Initialize();
    Chip8_32 chip8_32;
    
    // 디버거 생성
    chip8emu::Debugger32 debugger(chip8_32);
    if (g_debug_mode) {
        debugger.enable(true);
        debugger.setStepMode(true);
        std::cout << "🐛 Debug mode enabled for 32-bit CHIP-8\n";
    }
    
    // 플랫폼 초기화
    Platform platform("CHIP-8 Emulator (32-bit Extended Mode)", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }
    
    // ROM 로드
    if (!chip8_32.load_rom(rom_path)) {
        std::cerr << "[ERROR] Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }
    
    // 시스템 정보 출력
    std::cout << "[INFO] 32-bit CHIP-8 Extended System Ready" << std::endl;
    std::cout << "  Memory: 64KB (65536 bytes)" << std::endl;
    std::cout << "  Registers: 32 x 32-bit (R0-R31)" << std::endl;
    std::cout << "  Stack: 32 levels" << std::endl;
    std::cout << "  Instruction Size: 4 bytes" << std::endl;
    std::cout << "  Controls: 1234/QWER/ASDF/ZXCV" << std::endl;
    
    if (g_debug_mode) {
        std::cout << "  🐛 Debug Mode: ON (Type 'help' for commands)" << std::endl;
    }
    
    // 메인 루프 - 수정된 부분
    bool quit = false;
    bool debugger_active = true;  // 디버거 상태를 별도로 관리
    
    while (!quit && debugger_active) {
        // 입력 처리
        quit = platform.ProcessInput(chip8_32.keypad);
        
        // 디버그 정보 출력 (실행 전) - 디버그 모드일 때만
        if (debugger.isEnabled()) {
            uint32_t current_opcode = chip8_32.getCurrentOpcode();
            debugger.printState(current_opcode);
            
            // 디버거에서 quit 명령을 받았는지 확인
            if (!debugger.isEnabled()) {
                debugger_active = false;
                break;
            }
        }
        
        // CPU 사이클 실행
        chip8_32.cycle();
        
        // 화면 업데이트
        if (chip8_32.needs_redraw()) {
            platform.Update(chip8_32.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8_32.clear_draw_flag();
        }
        
        timer::delay(g_debug_mode ? 50 : 2); // 디버그 모드에서는 느리게
    }
    
    std::cout << "[INFO] 32-bit CHIP-8 extended emulator terminated" << std::endl;
    return 0;
}

std::string ModeSelector::get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    
    std::string extension = filename.substr(dot_pos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}