#pragma once

#include <array>
#include <cstdint>
#include "common/constants.hpp"

constexpr unsigned int MEMORY_SIZE = 16384;
constexpr unsigned int NUM_REGISTERS = 32;
constexpr unsigned int STACK_SIZE = 64;

class Chip8 {
public:
Chip8(); // 생성자: 초기화 수행

    bool draw_flag; // 화면을 다시 그려야 하는 경우 true로 설정

    void reset();
    bool load_rom(const char* filename); // ROM 파일을 메모리에 로드
    void cycle(); // Fetch - Decode - Execute 수행 (CPU 한 사이클)

    bool needs_redraw() const; // 화면 출력이 필요한지 여부를 반환하는 함수        
    void clear_draw_flag(); // 화면 갱신 플래그를 false로 초기화          
    const uint8_t* get_video_buffer() const; // 비디오 버퍼에 대한 포인터를 반환 
    uint8_t* get_keypad(); // 키보드 상태 배열에 대한 포인터를 반환           

    // 외부에서 키 입력 및 디스플레이 버퍼 접근을 위해 공개
    std::array<uint8_t, NUM_KEYS> keypad;        // 키 상태 배열
    std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT> video; // 화면 버퍼

    std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& get_video();
    
    // 공개 타이머 값 (SDL에서 비프음 등을 처리할 수 있도록)
    uint8_t delay_timer;
    uint8_t sound_timer;

    // 프로그램 카운터 (change)
    uint32_t get_pc() const { return pc; }
    void set_pc(uint32_t value) { pc = value; }

    // R 레지스터 접근 (change)
    uint32_t get_R(int index) const { return R.at(index); }
    void set_R(int index, uint32_t value) { R.at(index) = value; }

    // 메모리 접근
    uint8_t get_memory(int index) const { return memory.at(index); }
    void set_memory(int index, uint8_t value) { memory.at(index) = value; }

    // 인덱스 레지스터 I (change)
    uint32_t get_I() const { return I; }
    void set_I(uint32_t value) { I = value; }

    // 스택 (change)
    uint32_t get_stack(int index) const { return stack.at(index); }
    void set_stack(int index, uint32_t value) { stack.at(index) = value; }

    // 스택 포인터
    uint8_t get_sp() const { return sp; }
    void set_sp(uint8_t value) { sp = value; }

    // 비디오 메모리
    uint8_t get_video(int index) const { return video.at(index); }
    void set_video(int index, uint8_t value) { video.at(index) = value; }

    // 키보드
    uint8_t get_key(int index) const { return keypad.at(index); }
    void set_key(int index, uint8_t value) { keypad.at(index) = value; }

    // 사운드 타이머
    uint8_t get_sound_timer() const { return sound_timer; }
    void set_sound_timer(uint8_t value) { sound_timer = value; }

    // 딜레이 타이머
    uint8_t get_delay_timer() const { return delay_timer; }
    void set_delay_timer(uint8_t value) { delay_timer = value; }

    // 드로우 플래그
    bool get_draw_flag() const { return draw_flag; }
    void set_draw_flag(bool value) { draw_flag = value; }

    uint32_t& stack_at(uint8_t index); // 참조 리턴 (change)

private:
    std::array<uint8_t, MEMORY_SIZE> memory;     // 4KB 메모리
    std::array<uint32_t, NUM_REGISTERS> R;        // 범용 레지스터 R0~R31 (change)
    uint32_t I;                                  // 인덱스 레지스터 (메모리 주소)
    uint32_t pc;                                 // 프로그램 카운터

    std::array<uint32_t, STACK_SIZE> stack;      // 스택 (CALL/RET 용) (change)
    uint8_t sp;                                  // 스택 포인터

    uint32_t opcode;                             // 현재 실행 중인 명령어 (4바이트) (change)

    // 내부 함수: opcode 처리기 (Cycle 내부에서 호출) (change)
    void ExecuteOpcode(uint32_t opcode);
};