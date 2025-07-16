// mode_selector.cpp - 60Hz 고정 속도 제어
#include "mode_selector.hpp"
#include "chip8.hpp"
#include "chip8_32.hpp"
#include "opcode_table.hpp"
#include "opcode_table_32.hpp"
#include "platform.hpp"
#include "timer.hpp"
#include "debugger/debugger.hpp"
#include <iostream>
#include <algorithm>

// 전역 변수로 디버그 모드 플래그
static bool g_debug_mode = false;

void ModeSelector::set_debug_mode(bool enable) {
    g_debug_mode = enable;
}

int ModeSelector::select_and_run() {
    std::cout << "[INFO] Starting Enhanced CHIP-8 Emulator\n";
    std::cout << "[INFO] Supported formats: .ch8 (8-bit), .ch32 (32-bit)\n";
    
    // 플랫폼 초기화 (파일 선택용)
    Platform platform("CHIP-8 File Selector", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }
    
    // 파일 선택
    std::cout << "[INFO] Please select ROM file..." << std::endl;
    while (!platform.IsFileSelected()) {
        static std::array<uint8_t, 16> dummy_keypad = {0}; // <- 이 부분 수정
        if (platform.ProcessInput(dummy_keypad)) return 1;
        platform.UpdateFileInput();
        timer::delay(16);
    }

    std::string filename = platform.GetSelectedFile();
    std::string extension = get_file_extension(filename);
    
    std::cout << "[INFO] Selected file: " << filename << std::endl;
    std::cout << "[INFO] Extension: " << extension << std::endl;
    
    // 확장자 기반 모드 선택
    if (extension == ".ch8" || extension == ".c8") {
        std::cout << "[INFO] Detected 8-bit CHIP-8 ROM" << std::endl;
        return run_8bit_mode_with_file(filename);
    }
    else if (extension == ".ch32" || extension == ".c32") {
        std::cout << "[INFO] Detected 32-bit CHIP-8 ROM" << std::endl;
        return run_32bit_mode_with_file(filename);
    }
    else {
        std::cout << "[WARNING] Unknown extension, trying 8-bit mode..." << std::endl;
        return run_8bit_mode_with_file(filename);
    }
}

// 8비트 모드 (파일명 직접 전달)
int ModeSelector::run_8bit_mode_with_file(const std::string& filename) {
    std::cout << "\n=== Starting 8-bit CHIP-8 Emulator ===" << std::endl;
    
    OpcodeTable::Initialize();
    Chip8 chip8;
    
    chip8emu::Debugger8 debugger(chip8);
    if (g_debug_mode) {
        debugger.enable(true);
        debugger.setStepMode(true);
        std::cout << "🐛 Debug mode enabled for 8-bit CHIP-8\n";
    }
    
    Platform platform("CHIP-8 Emulator (8-bit Mode)", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }

    // 파일 로드
    std::string full_path = "../roms/" + filename;
    if (!chip8.load_rom(full_path.c_str())) {
        std::cerr << "[ERROR] Failed to load ROM: " << full_path << std::endl;
        return 1;
    }

    std::cout << "[INFO] 8-bit CHIP-8 System Ready (60Hz)" << std::endl;
    std::cout << "  Memory: 4KB" << std::endl;
    std::cout << "  Registers: 16 x 8-bit (V0-VF)" << std::endl;
    std::cout << "  ROM: " << filename << std::endl;
    
    if (g_debug_mode) {
        std::cout << "  🐛 Debug Mode: ON" << std::endl;
        std::cout << "  Debug Commands: step, continue, quit, help" << std::endl;
    }
    
    bool quit = false;
    bool debugger_active = true;
    
    while (!quit && debugger_active) {
        uint32_t frame_start = timer::get_ticks();
        
        quit = platform.ProcessInput(chip8.keypad);
        
        // 디버그 모드 처리
        if (debugger.isEnabled()) {
            uint32_t current_opcode = chip8.getCurrentOpcode();
            debugger.printState(current_opcode);
            
            if (!debugger.isEnabled()) {
                debugger_active = false;
                break;
            }
        }
        
        chip8.cycle();
        
        // 타이머 업데이트
        static uint32_t last_timer_update = 0;
        uint32_t current_time = timer::get_ticks();
        if (current_time - last_timer_update >= 16) {
            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) chip8.sound_timer--;
            last_timer_update = current_time;
        }
        
        if (chip8.needs_redraw()) {
            platform.Update(chip8.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8.clear_draw_flag();
        }
        
        // 60Hz 속도 제어
        uint32_t frame_end = timer::get_ticks();
        uint32_t elapsed = frame_end - frame_start;
        if (elapsed < 16) {
            timer::delay(16 - elapsed);
        }
    }
    
    std::cout << "[INFO] 8-bit CHIP-8 emulator terminated" << std::endl;
    return 0;
}

// 32비트 모드 (파일명 직접 전달)
int ModeSelector::run_32bit_mode_with_file(const std::string& filename) {
    std::cout << "\n=== Starting 32-bit CHIP-8 Extended Emulator ===" << std::endl;
    
    OpcodeTable_32::Initialize();
    Chip8_32 chip8_32;
    
    chip8emu::Debugger32 debugger(chip8_32);
    if (g_debug_mode) {
        debugger.enable(true);
        debugger.setStepMode(true);
        std::cout << "🐛 Debug mode enabled for 32-bit CHIP-8\n";
    }
    
    Platform platform("CHIP-8 Emulator (32-bit Extended Mode)", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }

    // 파일 로드
    std::string full_path = "../roms/" + filename;
    if (!chip8_32.load_rom(full_path.c_str())) {
        std::cerr << "[ERROR] Failed to load ROM: " << full_path << std::endl;
        return 1;
    }

    std::cout << "[INFO] 32-bit CHIP-8 Extended System Ready (60Hz)" << std::endl;
    std::cout << "  Memory: 64KB" << std::endl;
    std::cout << "  Registers: 32 x 32-bit (R0-R31)" << std::endl;
    std::cout << "  ROM: " << filename << std::endl;
    std::cout << "  I/O: READ/WRITE syscalls supported" << std::endl;
    
    if (g_debug_mode) {
        std::cout << "  🐛 Debug Mode: ON" << std::endl;
        std::cout << "  Debug Commands: step, continue, quit, help" << std::endl;
    }
    
    bool quit = false;
    bool debugger_active = true;
    
    while (!quit && debugger_active) {
        uint32_t frame_start = timer::get_ticks();
        
        quit = platform.ProcessInput(chip8_32.keypad);
        
        // 디버그 모드 처리 - 개선된 버전
        if (debugger.isEnabled()) {
            uint32_t current_opcode = chip8_32.getCurrentOpcode();
            debugger.printState(current_opcode);
            
            if (!debugger.isEnabled()) {
                debugger_active = false;
                break;
            }
        }
        
        chip8_32.cycle();
        
        // 타이머 업데이트
        static uint32_t last_timer_update = 0;
        uint32_t current_time = timer::get_ticks();
        if (current_time - last_timer_update >= 16) {
            if (chip8_32.delay_timer > 0) chip8_32.delay_timer--;
            if (chip8_32.sound_timer > 0) chip8_32.sound_timer--;
            last_timer_update = current_time;
        }
        
        if (chip8_32.needs_redraw()) {
            platform.Update(chip8_32.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8_32.clear_draw_flag();
        }
        
        // 60Hz 속도 제어
        uint32_t frame_end = timer::get_ticks();
        uint32_t elapsed = frame_end - frame_start;
        if (elapsed < 16) {
            timer::delay(16 - elapsed);
        }
    }
    
    std::cout << "[INFO] 32-bit CHIP-8 emulator terminated" << std::endl;
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