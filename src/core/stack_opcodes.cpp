#include "stack_opcodes.hpp"
#include "chip8_32.hpp"
#include "stack_frame.hpp"
#include <iostream>
#include <iomanip>

namespace StackOpcodes {

#ifdef DEBUG
#define LOG_DEBUG(x) std::cout << x << std::endl
#else
#define LOG_DEBUG(x)
#endif

// ===========================================
// 헬퍼 함수들
// ===========================================

bool push_stack(Chip8_32& chip8_32, uint32_t value) {
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    if (rsp - 4 < StackFrame::STACK_END) {
        std::cerr << "[STACK] PUSH OVERFLOW! RSP=0x" << std::hex << rsp << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4); // 무한 루프 방지
        return false;
    }
    
    rsp -= 4;
    chip8_32.set_memory(rsp + 0, (value >> 24) & 0xFF);
    chip8_32.set_memory(rsp + 1, (value >> 16) & 0xFF);
    chip8_32.set_memory(rsp + 2, (value >> 8) & 0xFF);
    chip8_32.set_memory(rsp + 3, value & 0xFF);
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp);
    
    LOG_DEBUG("[STACK] PUSH 0x" << std::hex << value << " at RSP=0x" << rsp << std::dec);
    return true;
}

bool pop_stack(Chip8_32& chip8_32, uint32_t& value) {
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    if (rsp >= StackFrame::STACK_START) {
        std::cerr << "[STACK] POP UNDERFLOW! RSP=0x" << std::hex << rsp << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4); // 무한 루프 방지
        return false;
    }
    
    value = (chip8_32.get_memory(rsp + 0) << 24) |
            (chip8_32.get_memory(rsp + 1) << 16) |
            (chip8_32.get_memory(rsp + 2) << 8) |
            chip8_32.get_memory(rsp + 3);
    
    rsp += 4;
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp);
    
    LOG_DEBUG("[STACK] POP 0x" << std::hex << value << " from RSP=0x" << (rsp-4) << std::dec);
    return true;
}

void extract_register_and_offset(uint32_t opcode, uint8_t& reg_index, uint8_t& offset) {
    reg_index = (opcode & 0x0000FF00) >> 8; // 비트 15-8: 레지스터 인덱스
    offset = opcode & 0x000000FF;           // 비트 7-0: 오프셋
    LOG_DEBUG("[STACK DEBUG] opcode=0x" << std::hex << opcode 
              << " → reg_idx=" << std::dec << (int)reg_index
              << " offset=" << (int)offset);
}

// ===========================================
// 1단계: 기본 스택 조작 명령어
// ===========================================

void OP_PUSH_RBP(Chip8_32& chip8_32, uint32_t opcode) {
    LOG_DEBUG("[OPCODE] PUSH RBP (0x" << std::hex << opcode << ")" << std::dec);
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    if (push_stack(chip8_32, rbp)) {
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    } else {
        std::cerr << "[ERROR] PUSH RBP failed - stack overflow" << std::endl;
    }
}

void OP_PUSH_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x0000FF00) >> 8;
    LOG_DEBUG("[OPCODE] PUSH R" << static_cast<int>(reg_index) 
              << " (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t reg_value = chip8_32.get_R(reg_index);
    if (push_stack(chip8_32, reg_value)) {
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    } else {
        std::cerr << "[ERROR] PUSH R" << static_cast<int>(reg_index) << " failed - stack overflow" << std::endl;
    }
}

void OP_POP_RBP(Chip8_32& chip8_32, uint32_t opcode) {
    LOG_DEBUG("[OPCODE] POP RBP (0x" << std::hex << opcode << ")" << std::dec);
    uint32_t value;
    if (pop_stack(chip8_32, value)) {
        chip8_32.set_R(StackFrame::RBP_INDEX, value);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    } else {
        std::cerr << "[ERROR] POP RBP failed - stack underflow" << std::endl;
    }
}

void OP_POP_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x0000FF00) >> 8;
    LOG_DEBUG("[OPCODE] POP R" << static_cast<int>(reg_index) 
              << " (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t value;
    if (pop_stack(chip8_32, value)) {
        chip8_32.set_R(reg_index, value);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    } else {
        std::cerr << "[ERROR] POP R" << static_cast<int>(reg_index) << " failed - stack underflow" << std::endl;
    }
}

// ===========================================
// 2단계: 프레임 포인터 조작
// ===========================================

void OP_MOV_RBP_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    LOG_DEBUG("[OPCODE] MOV RBP, RSP (0x" << std::hex << opcode << ")" << std::dec);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    chip8_32.set_R(StackFrame::RBP_INDEX, rsp);
    LOG_DEBUG("[STACK] RBP = RSP = 0x" << std::hex << rsp << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

void OP_MOV_RSP_RBP(Chip8_32& chip8_32, uint32_t opcode) {
    LOG_DEBUG("[OPCODE] MOV RSP, RBP (0x" << std::hex << opcode << ")" << std::dec);
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    chip8_32.set_R(StackFrame::RSP_INDEX, rbp);
    LOG_DEBUG("[STACK] RSP = RBP = 0x" << std::hex << rbp << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// ===========================================
// 3단계: 스택 포인터 조작
// ===========================================

void OP_SUB_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    LOG_DEBUG("[OPCODE] SUB RSP, " << value << " (0x" << std::hex << opcode << ")" << std::dec);
    
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_rsp = rsp - value;
    
    if (StackFrame::check_stack_overflow(new_rsp)) {
        std::cerr << "[ERROR] SUB RSP would cause stack overflow" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    chip8_32.set_R(StackFrame::RSP_INDEX, new_rsp);
    LOG_DEBUG("[STACK] RSP: 0x" << std::hex << rsp << " -> 0x" << new_rsp << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

void OP_ADD_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    LOG_DEBUG("[OPCODE] ADD RSP, " << value << " (0x" << std::hex << opcode << ")" << std::dec);
    
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_rsp = rsp + value;
    
    if (StackFrame::check_stack_underflow(new_rsp)) {
        std::cerr << "[ERROR] ADD RSP would cause stack underflow" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    chip8_32.set_R(StackFrame::RSP_INDEX, new_rsp);
    LOG_DEBUG("[STACK] RSP: 0x" << std::hex << rsp << " -> 0x" << new_rsp << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// ===========================================
// 4단계: 함수 호출/반환
// ===========================================

void OP_CALL_FUNC(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t func_addr = opcode & 0x0000FFFF;
    LOG_DEBUG("[OPCODE] CALL_FUNC 0x" << std::hex << func_addr << " (0x" << opcode << ")" << std::dec);
    
    uint32_t return_addr = chip8_32.get_pc() + 4;
    if (push_stack(chip8_32, return_addr)) {
        chip8_32.set_pc(func_addr);
        LOG_DEBUG("[CALL] Jumping to 0x" << std::hex << func_addr 
                  << ", return addr 0x" << return_addr << " pushed" << std::dec);
    } else {
        std::cerr << "[ERROR] CALL_FUNC failed - stack overflow" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }
}

void OP_RET_FUNC(Chip8_32& chip8_32, uint32_t opcode) {
    LOG_DEBUG("[OPCODE] RET_FUNC (0x" << std::hex << opcode << ")" << std::dec);
    uint32_t return_addr;
    if (pop_stack(chip8_32, return_addr)) {
        chip8_32.set_pc(return_addr);
        LOG_DEBUG("[RET] Returning to 0x" << std::hex << return_addr << std::dec);
    } else {
        std::cerr << "[ERROR] RET_FUNC failed - stack underflow" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }
}

// ===========================================
// 5단계: 스택 메모리 접근
// ===========================================

void OP_MOV_RBP_MINUS_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);
    LOG_DEBUG("[OPCODE] MOV [RBP-" << static_cast<int>(offset) 
              << "], R" << static_cast<int>(reg_index) 
              << " (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    if (offset > rbp) {
        std::cerr << "[ERROR] Negative address: RBP=0x" << std::hex << rbp 
                  << ", offset=" << std::dec << static_cast<int>(offset) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    uint32_t addr = rbp - offset;
    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] Memory access out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t reg_value = chip8_32.get_R(reg_index);
    chip8_32.set_memory(addr + 0, (reg_value >> 24) & 0xFF);
    chip8_32.set_memory(addr + 1, (reg_value >> 16) & 0xFF);
    chip8_32.set_memory(addr + 2, (reg_value >> 8) & 0xFF);
    chip8_32.set_memory(addr + 3, reg_value & 0xFF);
    
    LOG_DEBUG("[STACK] Stored R" << static_cast<int>(reg_index) 
              << "=0x" << std::hex << reg_value << " at [RBP-" << std::dec << static_cast<int>(offset) 
              << "]=0x" << std::hex << addr << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

void OP_MOV_RX_RBP_MINUS(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);
    LOG_DEBUG("[OPCODE] MOV R" << static_cast<int>(reg_index) 
              << ", [RBP-" << static_cast<int>(offset) 
              << "] (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    if (offset > rbp) {
        std::cerr << "[ERROR] Negative address: RBP=0x" << std::hex << rbp 
                  << ", offset=" << std::dec << static_cast<int>(offset) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    uint32_t addr = rbp - offset;
    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] Memory access out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                     (chip8_32.get_memory(addr + 1) << 16) |
                     (chip8_32.get_memory(addr + 2) << 8) |
                     chip8_32.get_memory(addr + 3);
    chip8_32.set_R(reg_index, value);
    
    LOG_DEBUG("[STACK] Loaded R" << static_cast<int>(reg_index) 
              << "=0x" << std::hex << value << " from [RBP-" << std::dec << static_cast<int>(offset) 
              << "]=0x" << std::hex << addr << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

void OP_MOV_RBP_PLUS_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);
    LOG_DEBUG("[OPCODE] MOV [RBP+" << static_cast<int>(offset) 
              << "], R" << static_cast<int>(reg_index) 
              << " (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = rbp + offset;
    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] Memory access out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t reg_value = chip8_32.get_R(reg_index);
    chip8_32.set_memory(addr + 0, (reg_value >> 24) & 0xFF);
    chip8_32.set_memory(addr + 1, (reg_value >> 16) & 0xFF);
    chip8_32.set_memory(addr + 2, (reg_value >> 8) & 0xFF);
    chip8_32.set_memory(addr + 3, reg_value & 0xFF);
    
    LOG_DEBUG("[STACK] Stored R" << static_cast<int>(reg_index) 
              << "=0x" << std::hex << reg_value << " at [RBP+" << std::dec << static_cast<int>(offset) 
              << "]=0x" << std::hex << addr << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

void OP_MOV_RX_RBP_PLUS(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);
    LOG_DEBUG("[OPCODE] MOV R" << static_cast<int>(reg_index) 
              << ", [RBP+" << static_cast<int>(offset) 
              << "] (0x" << std::hex << opcode << ")" << std::dec);
    
    if (reg_index >= 32) {
        std::cerr << "[ERROR] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = rbp + offset;
    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] Memory access out of bounds: 0x" << std::hex << addr << std::dec << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }
    
    uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                     (chip8_32.get_memory(addr + 1) << 16) |
                     (chip8_32.get_memory(addr + 2) << 8) |
                     chip8_32.get_memory(addr + 3);
    chip8_32.set_R(reg_index, value);
    
    LOG_DEBUG("[STACK] Loaded R" << static_cast<int>(reg_index) 
              << "=0x" << std::hex << value << " from [RBP+" << std::dec << static_cast<int>(offset) 
              << "]=0x" << std::hex << addr << std::dec);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

} // namespace StackOpcodes