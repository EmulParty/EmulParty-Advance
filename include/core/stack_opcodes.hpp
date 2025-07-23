#pragma once

#include <cstdint>

// 전방 선언
class Chip8_32;

/**
 * @brief 스택 프레임 관련 명령어 집합 (0x11xxxxxx)
 * 
 * x86-64 기반 스택 프레임 명령어들을 32비트 CHIP-8 확장에 구현
 */

namespace StackOpcodes {
    
    // ===========================================
    // 1단계: 기본 스택 조작 명령어 (0x1100xxxx)
    // ===========================================
    
    /**
     * @brief PUSH RBP - RBP를 스택에 푸시 (0x11000000)
     * x86-64: push rbp
     */
    void OP_PUSH_RBP(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief PUSH RX - 레지스터 RX를 스택에 푸시 (0x1100RRXX)
     * x86-64: push rax, push rbx, etc.
     */
    void OP_PUSH_RX(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief POP RBP - 스택에서 RBP로 팝 (0x11010000)
     * x86-64: pop rbp
     */
    void OP_POP_RBP(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief POP RX - 스택에서 레지스터 RX로 팝 (0x1101RRXX)
     * x86-64: pop rax, pop rbx, etc.
     */
    void OP_POP_RX(Chip8_32& chip8_32, uint32_t opcode);
    
    // ===========================================
    // 2단계: 프레임 포인터 조작 (0x1102xxxx, 0x1103xxxx)
    // ===========================================
    
    /**
     * @brief MOV RBP, RSP - RSP 값을 RBP에 복사 (0x11020000)
     * x86-64: mov rbp, rsp
     */
    void OP_MOV_RBP_RSP(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief MOV RSP, RBP - RBP 값을 RSP에 복사 (0x11030000)
     * x86-64: mov rsp, rbp
     */
    void OP_MOV_RSP_RBP(Chip8_32& chip8_32, uint32_t opcode);
    
    // ===========================================
    // 3단계: 스택 포인터 조작 (0x1104xxxx, 0x1105xxxx)
    // ===========================================
    
    /**
     * @brief SUB RSP, NNNN - RSP에서 NNNN만큼 빼기 (0x1104NNNN)
     * x86-64: sub rsp, 16
     */
    void OP_SUB_RSP(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief ADD RSP, NNNN - RSP에 NNNN만큼 더하기 (0x1105NNNN)
     * x86-64: add rsp, 16
     */
    void OP_ADD_RSP(Chip8_32& chip8_32, uint32_t opcode);
    
    // ===========================================
    // 4단계: 함수 호출/반환 (0x1106xxxx, 0x1107xxxx)
    // ===========================================
    
    /**
     * @brief CALL_FUNC NNNN - 함수 호출 (0x1106NNNN)
     * x86-64: call func
     * 자동으로 반환 주소를 스택에 저장하고 함수로 점프
     */
    void OP_CALL_FUNC(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief RET_FUNC - 함수 반환 (0x11070000)
     * x86-64: ret
     * 자동으로 스택에서 반환 주소를 복원하고 점프
     */
    void OP_RET_FUNC(Chip8_32& chip8_32, uint32_t opcode);
    
    // ===========================================
    // 5단계: 스택 메모리 접근 (0x1108xxxx - 0x110Bxxxx)
    // ===========================================
    
    /**
     * @brief MOV [RBP-NN], RX - RX를 RBP-NN 위치에 저장 (0x1108RRNN)
     * x86-64: mov [rbp-8], rax
     */
    void OP_MOV_RBP_MINUS_RX(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief MOV RX, [RBP-NN] - RBP-NN 위치값을 RX에 로드 (0x1109RRNN)
     * x86-64: mov rax, [rbp-8]
     */
    void OP_MOV_RX_RBP_MINUS(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief MOV [RBP+NN], RX - RX를 RBP+NN 위치에 저장 (0x110ARRNN)
     * x86-64: mov [rbp+8], rax
     */
    void OP_MOV_RBP_PLUS_RX(Chip8_32& chip8_32, uint32_t opcode);
    
    /**
     * @brief MOV RX, [RBP+NN] - RBP+NN 위치값을 RX에 로드 (0x110BRRNN)
     * x86-64: mov rax, [rbp+8]
     */
    void OP_MOV_RX_RBP_PLUS(Chip8_32& chip8_32, uint32_t opcode);
    
    // ===========================================
    // 헬퍼 함수들
    // ===========================================
    
    /**
     * @brief 스택에 32비트 값 푸시
     * @param chip8_32 시스템 참조
     * @param value 푸시할 값
     * @return 성공 시 true, 스택 오버플로우 시 false
     */
    bool push_stack(Chip8_32& chip8_32, uint32_t value);
    
    /**
     * @brief 스택에서 32비트 값 팝
     * @param chip8_32 시스템 참조
     * @param value 팝한 값을 저장할 변수 참조
     * @return 성공 시 true, 스택 언더플로우 시 false
     */
    bool pop_stack(Chip8_32& chip8_32, uint32_t& value);
    
} // namespace StackOpcodes