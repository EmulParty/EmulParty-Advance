#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include "common/constants.hpp"
#include "timer.hpp"


// CHIP-8 확장 버전은 우선 64KB 메모리를 사용합니다. 
constexpr unsigned int MEMORY_SIZE_32 = 65536;

// R0~R31 32비트 레지스터 (기존 V0~VF 16개 -> 32개로 확장)
constexpr unsigned int NUM_REGISTERS_32 = 32;  //R0~R31

// 기존 16단계 -> 32단계로 2배 확장
constexpr unsigned int STACK_SIZE_32 = 32; 

class Chip8_32 {
private:
    // 메모리 (4KB -> 64KB 확장)
    std::array<uint8_t, MEMORY_SIZE_32> memory;     // 64KB 메모리, 각 셀은 8비트
    
    // 레지스터 (기존 16개 -> 32개로 확장, 8비트 -> 32비트)
    std::array<uint32_t, NUM_REGISTERS_32> R;        // 32개의 32비트 범용 레지스터
    
    // 시스템 레지스터들 (16비트 -> 32비트 확장)
    uint32_t I;                                  // 인덱스 레지스터 (메모리 주소)
    uint32_t pc;                                 // 프로그램 카운터

    // 스택 관련 (16비트 주소 -> 32비트 주소, 스택 크기 16단계 -> 32단계로 확장)
    std::array<uint32_t, STACK_SIZE_32> stack;      // 서브루틴 호출 스택 (32비트 주소 저장)
    uint8_t sp;                                  // 스택 포인터 (0-31이므로 8비트로 충분)

    // 명령어 처리 (16비트 -> 32비트 확장)
    uint32_t opcode;                             // 현재 실행 중인 명령어 (4바이트)

    size_t loaded_rom_size; 

    uint32_t last_timer_update = 0;

public:
    Chip8_32(); // 생성자: 초기화 수행

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

    // 프로그램 카운터
    uint32_t get_pc() const { return pc; }
    void set_pc(uint32_t value) { pc = value; }

    // R 레지스터 접근
    uint32_t get_R(int index) const { return R.at(index); }
    void set_R(int index, uint32_t value) { R.at(index) = value; }

    // 메모리 접근 (주소는 32비트, 데이터는 8비트 유지)  <- 재검토
    uint8_t get_memory(int index) const { return memory.at(index); }
    void set_memory(int index, uint8_t value) { memory.at(index) = value; }

    // 32비트 인덱스 레지스터 I (기존 16비트 -> 32비트로 확장)
    uint32_t get_I() const { return I; }
    void set_I(uint32_t value) { I = value; }

    // 스택
    uint32_t get_stack(int index) const { return stack.at(index); }
    void set_stack(int index, uint32_t value) { stack.at(index) = value; }

    // 스택 포인터
    uint8_t get_sp() const { return sp; }
    void set_sp(uint8_t value) { sp = value; }

    // 비디오 메모리
    uint8_t get_video(int index) const { return video.at(index); }
    void set_video(int index, uint8_t value) { video.at(index) = value; }

    // 키보드
    bool get_key(int index) const { return keypad.at(index); }
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

    uint32_t& stack_at(uint8_t index); // 참조 리턴
    uint32_t getCurrentOpcode() const {
        return opcode;
    }
};
