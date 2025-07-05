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

// 생성자 : reset() 호출로 에뮬레이터 초기화
Chip8_32::Chip8_32() {
    loaded_rom_size = 0;  // ← 추가
    reset();
}

// 초기 상태로 리셋
void Chip8_32::reset() {
    pc = 0x200;  // 프로그램 시작 주소
    opcode = 0;
    I = 0;       // 인덱스 레지스터
    sp = 0;      // 스택 포인터

    // 모든 메모리, 레지스터, 화면, 키보드 초기화
    std::memset(memory.data(), 0, sizeof(memory));  //64KB 메모리 초기화
    std::memset(R.data(), 0, sizeof(R));    // 32개의 레지스터 초기화
    std::memset(video.data(), 0, sizeof(video));    // 화면 버퍼 초기화
    std::memset(stack.data(), 0, sizeof(stack));    // 32단계 스택 초기화
    std::memset(keypad.data(), 0, sizeof(keypad));  // 키패드 초기화

    delay_timer = 0;
    sound_timer = 0;

    // 메모리 0x000~0x050에 폰트셋 복사
    std::memcpy(memory.data(), chip8_fontset, sizeof(chip8_fontset));

    draw_flag = false;  // 화면 다시 그릴 필요 없음

    std::cout << "32-bit CHIP-8 system reset complete" << std::endl;
}

void dump_memory(const Chip8_32& chip8_32, uint32_t start = 0x0200, uint32_t end = 0xFFFF) {
    printf("\n=== Full Memory Dump (0x%04X ~ 0x%04X) ===\n", start, end);
    for (uint32_t addr = start; addr <= end; addr += 16) {
        printf("0x%04X: ", addr);
        for (int i = 0; i < 16; ++i) {
            if (addr + i <= end) {
                printf("%02X ", chip8_32.get_memory(addr + i));
            }
        }
        printf("\n");
    }
}

// ROM 파일을 메모리에 로드 (0x200부터)
bool Chip8_32::load_rom(const char* filename) {
    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom.is_open()) {
        std::cerr << "Failed to open ROM file: " << filename << std::endl;
        return false;
    } 
    
    std::streamsize size = rom.tellg();     // 파일 크기 구함
    loaded_rom_size = static_cast<size_t>(size);
    rom.seekg(0, std::ios::beg);            // 파일 처음으로 이동
    
    if (size < 0 || size > MEMORY_SIZE_32 - 0x200) {
        std::cerr << "ROM file size is invalid or too large: " << size << std::endl;
        return false;
    }

    std::vector<char> buffer(size);         // 버퍼에 ROM 데이터 담기
    if (!rom.read(buffer.data(), size)) {
        std::cerr << "Failed to read ROM file: " << filename << std::endl;
        return false;
    }

    // 0x200부터 메모리에 ROM 데이터 복사
    for (size_t i = 0; i < static_cast<size_t>(size); ++i) {
        memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
    }

    // 디버그: 로드된 메모리 확인
    std::cout << "Loaded memory at 0x200:" << std::endl;
    for (int i = 0; i < 8; i++) {
        printf("0x%02X ", memory[0x200 + i]);  // printf로 명확하게
    }
    printf("\n");
    std::cout << "Loaded ROM: " << filename << " (" << size << " bytes)" << std::endl;
    std::cout << "Available memory space: " << (MEMORY_SIZE_32 - 0x200) << " bytes" << std::endl;

    // 전체 메모리 덤프
    dump_memory(*this, 0x0200, 0x0200 + size);
    
    return true;
}



// 하나의 사이클 수행: Fetch → Decode → Execute
void Chip8_32::cycle() {
    
    // 메모리 경계 체크
    if (pc >= MEMORY_SIZE_32 - 3) {
        std::cerr << "PC out of memory bounds: " << pc << std::endl;
        return;
    }

    // 1. Fetch
    opcode = (static_cast<uint32_t>(memory[pc]) << 24) |
             (static_cast<uint32_t>(memory[pc + 1]) << 16) |
             (static_cast<uint32_t>(memory[pc + 2]) << 8) |
             static_cast<uint32_t>(memory[pc + 3]);
    
    // 올바른 디버깅 출력 (leading zero 포함)
    printf("PC: 0x%04X Opcode: 0x%08X [%02X %02X %02X %02X]\n", 
        pc, opcode, memory[pc], memory[pc+1], memory[pc+2], memory[pc+3]);

    // 2. Execute
    OpcodeTable_32::Execute(*this, opcode);
}

// 화면이 그려져야 하는지 여부를 외부에 알림
bool Chip8_32::needs_redraw() const {
    return draw_flag;
}

// 화면 플래그 초기화 (draw 완료됨)
void Chip8_32::clear_draw_flag() {
    draw_flag = false;
}

// 화면 버퍼 읽기 (렌더링용)
const uint8_t* Chip8_32::get_video_buffer() const {
    return video.data();
}

// 키보드 상태 배열 포인터 반환
uint8_t* Chip8_32::get_keypad() {
    return keypad.data();
}

// 스택 접근
uint32_t& Chip8_32::stack_at(uint8_t index) { 
    return stack[index]; 
}

// 화면 버퍼 참조 반환 (픽셀 조작용)
std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& Chip8_32::get_video() {
    return video;
}
