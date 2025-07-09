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
    pc = 0x200;  // 프로그램 시작 주소
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

    std::memcpy(memory.data() + 0x50, chip8_fontset, sizeof(chip8_fontset));
    draw_flag = false;

    std::cout << "32-bit CHIP-8 system reset complete" << std::endl;
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

    opcode = (memory[pc] << 24) | (memory[pc + 1] << 16) |
             (memory[pc + 2] << 8) | memory[pc + 3];

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
