// chip8_32.cpp - BootROM::load_into_memory() 제대로 사용하는 버전

#include "chip8_32.hpp"
#include "opcode_table_32.hpp"
#include "boot_rom.hpp"  // BootROM 헤더 포함
#include "io_manager.hpp"
#include "sdl_console_io.hpp"
#include "stack_frame.hpp"
#include <cstring>
#include <random>
#include <fstream>
#include <vector>
#include <iostream>

// 기본 폰트셋 (각 숫자는 4x5 픽셀로 구성됨)
static const uint8_t chip8_fontset_32[80] = {
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

// BootROM 시작 주소 정의
constexpr uint32_t BOOT_ROM_START = 0x0000;

// 생성자: reset() 호출로 초기화
Chip8_32::Chip8_32() {
    loaded_rom_size = 0;
    last_timer_update = 0;
    platform_ptr_ = nullptr; // 초기화
    reset();
}

void Chip8_32::reset() {
    pc = BOOT_ROM_START;  // 부트로더부터 시작!
    opcode = 0;
    I = 0;
    sp = 0;

    // 메모리, 레지스터, 화면, 키보드 초기화
    std::memset(memory.data(), 0, MEMORY_SIZE_32);
    std::memset(R.data(), 0, sizeof(R));
    std::memset(video.data(), 0, sizeof(video));
    std::memset(stack.data(), 0, sizeof(stack));
    std::memset(keypad.data(), 0, sizeof(keypad));

    delay_timer = 0;
    sound_timer = 0;
    loaded_rom_size = 0;
    draw_flag = false;
    last_timer_update = 0;

    // 폰트셋 로드 (0x050~0x09F)
    std::memcpy(&memory[0x050], chip8_fontset_32, sizeof(chip8_fontset_32));

    // 스택 프레임 시스템 초기화
    StackFrame::initialize(*this);
    // BootROM 로드
    load_boot_rom();
    

    draw_flag = false;
    std::cout << "32-bit CHIP-8 system reset complete" << std::endl;
}



// Platform 설정 함수 추가
void Chip8_32::setPlatform(Platform* platform) {
    platform_ptr_ = platform;
    std::cout << "[Chip8_32] Platform pointer set: " << platform << std::endl;
    
    // IOManager 재설정
    setup_io_devices();
}

// 수정된 setup_io_devices 함수
void Chip8_32::setup_io_devices() {
    std::cout << "[Chip8_32] Setting up I/O devices..." << std::endl;
    
    // 기존 장치들 정리
    io_manager_.clear();
    
    // SDL 콘솔 I/O 장치 생성 (Platform 포인터 사용)
    console_io_ = std::make_shared<SDLConsoleIO>(platform_ptr_);
    
    // 표준 파일 디스크립터에 등록
    io_manager_.registerDevice(0, console_io_);  // stdin
    io_manager_.registerDevice(1, console_io_);  // stdout  
    io_manager_.registerDevice(2, console_io_);  // stderr
    
    // 등록된 장치 목록 출력
    io_manager_.printDevices();
    
    std::cout << "[INFO] IOManager initialized with SDL console I/O" << std::endl;
}

// IOManager 접근 함수
IOManager& Chip8_32::get_io_manager() {
    return io_manager_;
}

// SDLConsoleIO 접근 함수 추가
std::shared_ptr<SDLConsoleIO> Chip8_32::get_console_io() {
    return console_io_;
}

// ★ 핵심: 기존 boot_rom.cpp의 함수를 사용!
void Chip8_32::load_boot_rom() {
    std::cout << "[Chip8_32] Loading Boot ROM using BootROM::load_into_memory()..." << std::endl;
    
    // boot_rom.cpp에 정의된 함수 사용
    BootROM::load_into_memory(*this);
    
    std::cout << "[Chip8_32] Boot ROM loaded successfully" << std::endl;
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

/**
 * @brief 스택 프레임 정보 출력 (디버깅용)
 */
void Chip8_32::print_stack_info() const {
    StackFrame::print_stack_frame(*this);
}

/**
 * @brief 스택 메모리 덤프 (디버깅용)
 * @param range 덤프할 범위 (기본값: 현재 스택 프레임 주변)
 */
void Chip8_32::dump_stack(uint32_t range) const {
    uint32_t rsp = get_R(StackFrame::RSP_INDEX);
    uint32_t rbp = get_R(StackFrame::RBP_INDEX);
    
    uint32_t start = std::max(rsp - range, static_cast<uint32_t>(StackFrame::STACK_END));
    uint32_t end = std::min(rbp + range, static_cast<uint32_t>(StackFrame::STACK_START));
    
    StackFrame::dump_stack_memory(*this, start, end);
}

/**
 * @brief RBP 레지스터 접근자
 */
uint32_t Chip8_32::get_RBP() const {
    return get_R(StackFrame::RBP_INDEX);
}

void Chip8_32::set_RBP(uint32_t value) {
    set_R(StackFrame::RBP_INDEX, value);
}

/**
 * @brief RSP 레지스터 접근자  
 */
uint32_t Chip8_32::get_RSP() const {
    return get_R(StackFrame::RSP_INDEX);
}

void Chip8_32::set_RSP(uint32_t value) {
    set_R(StackFrame::RSP_INDEX, value);
}

/**
 * @brief RIP 레지스터 접근자 (pc와 동기화)
 */
uint32_t Chip8_32::get_RIP() const {
    return get_R(StackFrame::RIP_INDEX);
}

void Chip8_32::set_RIP(uint32_t value) {
    set_R(StackFrame::RIP_INDEX, value);
    // pc와 동기화
    pc = value;
}

bool Chip8_32::needs_redraw() const { return draw_flag; }
void Chip8_32::clear_draw_flag() { draw_flag = false; }
const uint8_t* Chip8_32::get_video_buffer() const { return video.data(); }
uint8_t* Chip8_32::get_keypad() { return keypad.data(); }
uint32_t& Chip8_32::stack_at(uint8_t index) { return stack[index]; }
std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& Chip8_32::get_video() { return video; }