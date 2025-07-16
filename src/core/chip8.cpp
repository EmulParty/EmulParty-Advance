#include "../../include/core/chip8.hpp"
#include "../../include/core/opcode_table.hpp"
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

// 생성자 - 에뮬레이터 초기화
Chip8::Chip8() {
    reset();
}

// 초기 상태로 리셋
void Chip8::reset() {
    pc = 0x200;  // 프로그램 시작 주소
    opcode = 0;
    I = 0;       // 인덱스 레지스터
    sp = 0;      // 스택 포인터

    // 모든 메모리, 레지스터, 화면, 키보드 초기화
    std::memset(memory.data(), 0, sizeof(memory));
    std::memset(V.data(), 0, sizeof(V));
    std::memset(video.data(), 0, sizeof(video));
    std::memset(stack.data(), 0, sizeof(stack));
    std::memset(keypad.data(), 0, sizeof(keypad));

    delay_timer = 0;
    sound_timer = 0;

    // 메모리 0x000~0x050에 폰트셋 복사
    std::memcpy(memory.data(), chip8_fontset, sizeof(chip8_fontset));

    draw_flag = false;  // 화면 다시 그릴 필요 없음
}

// ROM 파일을 메모리에 로드 (0x200부터)
bool Chip8::load_rom(const char* filename) {
    std::ifstream rom(filename, std::ios::binary | std::ios::ate);
    if (!rom.is_open()) return false;

    std::streamsize size = rom.tellg();     // 파일 크기 구함
    rom.seekg(0, std::ios::beg);            // 파일 처음으로 이동
    std::vector<char> buffer(size);         // 버퍼에 ROM 데이터 담기
    if (!rom.read(buffer.data(), size)) return false;

    // 0x200부터 메모리에 ROM 데이터 복사
    for (size_t i = 0; i < static_cast<size_t>(size); ++i) {
        memory[0x200 + i] = static_cast<uint8_t>(buffer[i]);
    }

    return true;
}

// 하나의 사이클 수행: Fetch → Decode → Execute
void Chip8::cycle() {
    // 1. Fetch (pc가 가리키는 주소에서 2바이트(opcode)를 가져와서 하나의 명령어로 만듦)
    opcode = (memory[pc] << 8) | memory[pc + 1];
    // 2. Decode + Execute ( opcode를 보고 어떤 명령인지 해석 후, 해당 명령에 맞는 함수 실행)
    OpcodeTable::Execute(*this, opcode);
}

// 화면이 그려져야 하는지 여부를 외부에 알림
bool Chip8::needs_redraw() const {
    return draw_flag;
}

// 화면 플래그 초기화 (draw 완료됨)
void Chip8::clear_draw_flag() {
    draw_flag = false;
}

// 화면 버퍼 읽기 (렌더링용)
const uint8_t* Chip8::get_video_buffer() const {
    return video.data();
}

// 키보드 상태 배열 포인터 반환
uint8_t* Chip8::get_keypad() {
    return keypad.data();
}

// 스택 접근
uint16_t& Chip8::stack_at(uint8_t index) { 
    return stack[index]; 
}

// 화면 버퍼 참조 반환 (픽셀 조작용)
std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& Chip8::get_video() {
    return video;
}