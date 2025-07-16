#include "../../include/core/stack_frame.hpp"
#include <iostream>
#include <iomanip>

StackFrameManager::StackFrameManager() 
    : stack_depth_(0), current_fp_(0) {
    // 모든 스택 프레임을 기본값으로 초기화
    for (auto& frame : frame_stack_) {
        frame = StackFrame();
    }
}

bool StackFrameManager::create_frame(uint32_t return_addr, uint32_t current_fp) {
    // 스택 오버플로우 검사
    if (is_stack_full()) {
        std::cerr << "[STACK_FRAME] Stack overflow! Cannot create new frame." << std::endl;
        return false;
    }
    
    // 새 프레임 생성
    StackFrame new_frame;
    new_frame.return_address = return_addr;
    new_frame.canary = STACK_CANARY;
    new_frame.frame_pointer = current_fp;  // 이전 프레임 포인터 저장
    new_frame.local_vars.fill(0);          // 지역변수 초기화
    
    // 스택에 프레임 추가
    frame_stack_[stack_depth_] = new_frame;
    stack_depth_++;
    current_fp_ = stack_depth_ - 1;  // 현재 프레임 포인터 업데이트
    
    std::cout << "[STACK_FRAME] Created frame at depth " << static_cast<int>(stack_depth_) 
              << ", return_addr=0x" << std::hex << return_addr << std::dec << std::endl;
    
    return true;
}

bool StackFrameManager::restore_frame(uint32_t& return_addr, uint32_t& prev_fp) {
    // 스택 언더플로우 검사
    if (is_stack_empty()) {
        std::cerr << "[STACK_FRAME] Stack underflow! No frame to restore." << std::endl;
        return false;
    }
    
    // 현재 프레임 가져오기
    StackFrame& current_frame = get_current_frame();
    
    // 카나리 값 검증
    if (!current_frame.is_canary_valid()) {
        std::cerr << "[STACK_FRAME] Stack corruption detected! Canary mismatch." << std::endl;
        std::cerr << "Expected: 0x" << std::hex << STACK_CANARY 
                  << ", Got: 0x" << current_frame.canary << std::dec << std::endl;
        return false;
    }
    
    // 반환 정보 설정
    return_addr = current_frame.return_address;
    prev_fp = current_frame.frame_pointer;
    
    std::cout << "[STACK_FRAME] Restored frame from depth " << static_cast<int>(stack_depth_) 
              << ", return_addr=0x" << std::hex << return_addr << std::dec << std::endl;
    
    // 스택 정리
    stack_depth_--;
    current_fp_ = prev_fp;
    
    return true;
}

bool StackFrameManager::set_current_local(uint8_t index, uint32_t value) {
    if (is_stack_empty()) {
        std::cerr << "[STACK_FRAME] No active frame to set local variable." << std::endl;
        return false;
    }
    
    StackFrame& current_frame = get_current_frame();
    bool success = current_frame.set_local(index, value);
    
    if (success) {
        std::cout << "[STACK_FRAME] Set local[" << static_cast<int>(index) 
                  << "] = 0x" << std::hex << value << std::dec << std::endl;
    } else {
        std::cerr << "[STACK_FRAME] Invalid local variable index: " << static_cast<int>(index) << std::endl;
    }
    
    return success;
}

bool StackFrameManager::get_current_local(uint8_t index, uint32_t& value) const {
    if (is_stack_empty()) {
        std::cerr << "[STACK_FRAME] No active frame to get local variable." << std::endl;
        return false;
    }
    
    const StackFrame& current_frame = get_current_frame();
    bool success = current_frame.get_local(index, value);
    
    if (success) {
        std::cout << "[STACK_FRAME] Get local[" << static_cast<int>(index) 
                  << "] = 0x" << std::hex << value << std::dec << std::endl;
    } else {
        std::cerr << "[STACK_FRAME] Invalid local variable index: " << static_cast<int>(index) << std::endl;
    }
    
    return success;
}

void StackFrameManager::print_stack_info() const {
    std::cout << "\n=== STACK FRAME INFO ===" << std::endl;
    std::cout << "Stack depth: " << static_cast<int>(stack_depth_) << std::endl;
    std::cout << "Current FP: " << current_fp_ << std::endl;
    
    if (stack_depth_ > 0) {
        std::cout << "Current frame:" << std::endl;
        const StackFrame& frame = get_current_frame();
        std::cout << "  Return addr: 0x" << std::hex << frame.return_address << std::dec << std::endl;
        std::cout << "  Canary: 0x" << std::hex << frame.canary << std::dec 
                  << (frame.is_canary_valid() ? " (VALID)" : " (CORRUPTED)") << std::endl;
        std::cout << "  Frame ptr: " << frame.frame_pointer << std::endl;
        
        std::cout << "  Local vars:" << std::endl;
        for (size_t i = 0; i < MAX_LOCALS; ++i) {
            if (frame.local_vars[i] != 0) {
                std::cout << "    [" << i << "] = 0x" << std::hex << frame.local_vars[i] << std::dec << std::endl;
            }
        }
    }
    std::cout << "========================\n" << std::endl;
}