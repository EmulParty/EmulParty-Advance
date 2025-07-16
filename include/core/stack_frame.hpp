#pragma once

#include <cstdint>
#include <array>

// Stack Frame 관련 상수들
constexpr uint8_t MAX_LOCALS = 16;           // 지역변수 최대 개수
constexpr uint8_t MAX_STACK_FRAMES = 32;     // 최대 스택 프레임 개수
constexpr uint32_t STACK_CANARY = 0xDEADBEEF; // 스택 카나리 값(추후 srand(); 로 변경 예정)

/**
 * @brief 32비트 CHIP-8 스택 프레임 구조체
 * 
 * 스택 프레임은 함수 호출 시 생성되며 다음 정보를 포함합니다:
 * - return_address: 함수 반환 주소
 * - canary: 스택 오버플로우 감지용 카나리 값
 * - frame_pointer: 이전 프레임 포인터 (연결 리스트 구조)
 * - local_vars: 지역변수 배열 (최대 16개)
 */
struct StackFrame {
    uint32_t return_address;                    // 함수 반환 주소
    uint32_t canary;                           // 스택 오버플로우 감지용 카나리
    uint32_t frame_pointer;                    // 이전 프레임 포인터
    std::array<uint32_t, MAX_LOCALS> local_vars; // 지역변수 배열
    
    // 생성자
    StackFrame() : return_address(0), canary(STACK_CANARY), frame_pointer(0) {
        local_vars.fill(0);
    }
    
    // 카나리 값 검증 함수(이게 로직 상 오버헤드가 적음 다만 현대와 조금 다른 구조임)
    bool is_canary_valid() const {
        return canary == STACK_CANARY;
    }
    
    // 지역변수 접근 (경계 검사 포함)
    bool set_local(uint8_t index, uint32_t value) {
        if (index >= MAX_LOCALS) return false;
        local_vars[index] = value;
        return true;
    }
    
    bool get_local(uint8_t index, uint32_t& value) const {
        if (index >= MAX_LOCALS) return false;
        value = local_vars[index];
        return true;
    }
};

/**
 * @brief 스택 프레임 관리자
 * 
 * 스택 프레임의 생성, 삭제, 관리를 담당합니다.
 */
class StackFrameManager {
public:
    StackFrameManager();
    
    // 스택 프레임 생성
    bool create_frame(uint32_t return_addr, uint32_t current_fp);
    
    // 스택 프레임 복원 (카나리 검증 포함)
    bool restore_frame(uint32_t& return_addr, uint32_t& prev_fp);
    
    // 현재 프레임의 지역변수 접근
    bool set_current_local(uint8_t index, uint32_t value);
    bool get_current_local(uint8_t index, uint32_t& value) const;
    
    // 현재 프레임 포인터 관리
    uint32_t get_current_fp() const { return current_fp_; }
    void set_current_fp(uint32_t fp) { current_fp_ = fp; }
    
    // 스택 상태 확인
    bool is_stack_empty() const { return stack_depth_ == 0; }
    uint8_t get_stack_depth() const { return stack_depth_; }
    
    // 디버깅용 정보 출력
    void print_stack_info() const;
    
private:
    std::array<StackFrame, MAX_STACK_FRAMES> frame_stack_;  // 스택 프레임 배열
    uint8_t stack_depth_;                                   // 현재 스택 깊이
    uint32_t current_fp_;                                   // 현재 프레임 포인터
    
    // 내부 헬퍼 함수들
    bool is_stack_full() const { return stack_depth_ >= MAX_STACK_FRAMES; }
    StackFrame& get_current_frame() { return frame_stack_[stack_depth_ - 1]; }
    const StackFrame& get_current_frame() const { return frame_stack_[stack_depth_ - 1]; }
};