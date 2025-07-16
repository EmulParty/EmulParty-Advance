#include "chip8_32.hpp"
#include "opcode_table_32.hpp"
#include <cstring> // memset, memcpy
#include <random>  // for CXNN
#include <fstream>
#include <vector>
#include <iostream>

// 기본 폰트셋 (각 숫자는 4x5 픽셀로 구성됨)
// 0x000 ~ 0x050 주소에 로드됨
static const uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 생성자 : reset() 호출로 초기화
Chip8_32::Chip8_32() {
    loaded_rom_size = 0;
    last_timer_update = 0;
    reset();
}

void Chip8_32::reset() {
    pc = 0x000;  // 부트로더부터 시작!
    opcode = 0;
    I = 0;
    sp = 0;

    memory.fill(0);
    R.fill(0);
    video.fill(0);
    stack.fill(0);
    keypad.fill(0);

    delay_timer = 0;
    sound_timer = 0;

    // 부트로더 로드 (0x000 ~ 0x04F)
    LoadBuiltinFileManager();

    std::memcpy(memory.data() + 0x50, chip8_fontset, sizeof(chip8_fontset));
    draw_flag = false;

    std::cout << "32-bit CHIP-8 system reset complete" << std::endl;
}

void Chip8_32::LoadBuiltinFileManager() {
    // FILE_MANAGER 부트로더 코드 (80바이트, 20개 명령어)
    static const uint32_t boot_rom[20] = {
        0x0A000000, 0x12000050, 0x0A000001, 0x12000060, 0x0A000002, 0x12000070,
        0x0A000003, 0x12000080, 0x0A000004, 0x12000090, 0x0A000005, 0x120000A0,
        0x0A000006, 0x120000B0, 0x0A000007, 0x120000C0, 0x0A000008, 0x120000D0,
        0x0A000009, 0x120000E0
    };

    std::cout << "[BOOT] Loading built-in FILE_MANAGER..." << std::endl;
    std::cout << "[BOOT] FILE_MANAGER loaded (80 bytes at 0x000-0x04F)" << std::endl;
    std::cout << "[BOOT] Prompt message at 0xA0: \"Enter filename: \"" << std::endl;
    std::cout << "[BOOT] Input buffer at 0xC0 (64 bytes)" << std::endl;
    std::cout << "[BOOT] Font data at 0x050-0x09F" << std::endl;
    std::cout << "[BOOT] Game ROM area: 0x200-0xFFFF" << std::endl;
    std::cout << "[BOOT] Ready to boot from 0x000!" << std::endl;

    // 안전하게 20개 명령어 로드
    for(int i = 0; i < 20; ++i) {
        memory[0x000 + i*4 + 0] = (boot_rom[i] >> 24) & 0xFF;
        memory[0x000 + i*4 + 1] = (boot_rom[i] >> 16) & 0xFF;
        memory[0x000 + i*4 + 2] = (boot_rom[i] >> 8) & 0xFF;
        memory[0x000 + i*4 + 3] = boot_rom[i] & 0xFF;
    }
}

bool Chip8_32::load_rom(const char* filename) {
    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom.is_open()) {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
        return false;
    }

    std::streamsize size = rom.tellg();
    loaded_rom_size = static_cast<size_t>(size);
    rom.seekg(0, std::ios::beg);

    if (size <= 0 || size > MEMORY_SIZE_32 - 0x200) {
        std::cerr << "ROM size invalid or too large: " << size << std::endl;
        return false;
    }

    std::vector<char> buffer(size);
    if (!rom.read(buffer.data(), size)) {
        std::cerr << "Failed to read ROM file: " << filename << std::endl;
        return false;
    }

    for (size_t i = 0; i < static_cast<size_t>(size); ++i) {
        memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
    }

    std::cout << "Loaded ROM: " << filename << " (" << size << " bytes)" << std::endl;
    return true;
}

void Chip8_32::cycle() {
    if (pc >= MEMORY_SIZE_32 - 3) {
        std::cerr << "PC out of bounds: " << pc << std::endl;
        return;
    }

    // 1. Fetch : 현재 pc 위치에서 4바이트 명령어를 읽음
    opcode = (memory[pc] << 24) | (memory[pc + 1] << 16) |
             (memory[pc + 2] << 8) | memory[pc + 3];

    // 2. Decode & Execute : opcode 테이블을 통해 명령어 실행
    OpcodeTable_32::Execute(*this, opcode);

    uint32_t current_time = timer::get_ticks();
    if (current_time - last_timer_update >= 16) {
        if (delay_timer > 0) --delay_timer;
        if (sound_timer > 0) --sound_timer;
        last_timer_update = current_time;
    }
}

bool Chip8_32::needs_redraw() const { return draw_flag; }
void Chip8_32::clear_draw_flag() { draw_flag = false; }
const uint8_t* Chip8_32::get_video_buffer() const { return video.data(); }
uint8_t* Chip8_32::get_keypad() { return keypad.data(); }
uint32_t& Chip8_32::stack_at(uint8_t index) { return stack[index]; }
std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& Chip8_32::get_video() { return video; }
