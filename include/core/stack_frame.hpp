#pragma once

#include <cstdint>

// 전방 선언
class Chip8_32;

/**
 * @brief 스택 프레임 관리 시스템
 * 
 * x86-64 호출 규약을 기반으로 한 32비트 CHIP-8 확장 스택 프레임 시스템
 * - RBP (R28): Base Pointer - 스택의 바닥 (시작 지점)
 * - RSP (R29): Stack Pointer - 스택의 꼭대기 (끝 지점)  
 * - RIP (R30): Instruction Pointer - 다음 실행할 명령어 (기존 pc와 동일)
 */

namespace StackFrame {
    // 스택 관련 상수
    constexpr uint32_t STACK_START = 0xEFFF;     // 스택 시작점 (높은 주소)
    constexpr uint32_t STACK_END = 0x8000;       // 스택 한계점 (낮은 주소)
    constexpr uint32_t STACK_SIZE = STACK_START - STACK_END;  // 스택 크기
    
    // 레지스터 인덱스 정의
    constexpr uint8_t RBP_INDEX = 28;  // R28 (Base Pointer)
    constexpr uint8_t RSP_INDEX = 29;  // R29 (Stack Pointer)
    constexpr uint8_t RIP_INDEX = 30;  // R30 (Instruction Pointer)
    constexpr uint8_t RESERVED_INDEX = 31;  // R31 (예약됨)
    
    /**
     * @brief 스택 시스템 초기화
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     */
    void initialize(Chip8_32& chip8_32);
    
    /**
     * @brief 스택 오버플로우 검사
     * @param rsp 현재 스택 포인터 값
     * @return true if overflow detected, false otherwise
     */
    bool check_stack_overflow(uint32_t rsp);
    
    /**
     * @brief 스택 언더플로우 검사
     * @param rsp 현재 스택 포인터 값
     * @return true if underflow detected, false otherwise
     */
    bool check_stack_underflow(uint32_t rsp);
    
    /**
     * @brief 스택 프레임 정보 출력 (디버깅용)
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     */
    void print_stack_frame(const Chip8_32& chip8_32);
    
    /**
     * @brief 스택 메모리 덤프 (디버깅용)
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     * @param start_addr 시작 주소
     * @param end_addr 끝 주소
     */
    void dump_stack_memory(const Chip8_32& chip8_32, uint32_t start_addr, uint32_t end_addr);
}