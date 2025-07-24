#include "stack_frame.hpp"
#include "chip8_32.hpp"
#include <iostream>
#include <iomanip>

namespace StackFrame {

void initialize(Chip8_32& chip8_32) {
    std::cout << "[StackFrame] Initializing stack system..." << std::endl;
    
    // RSP (R29) 초기화 - 스택 최상단으로 설정
    chip8_32.set_R(RSP_INDEX, STACK_START);
    
    // RBP (R28) 초기화 - 첫 번째 프레임에서는 0으로 설정
    chip8_32.set_R(RBP_INDEX, STACK_START);
    
    // RIP (R30)는 기존 pc와 동기화 (pc → RIP 이름 변경 예정)
    chip8_32.set_R(RIP_INDEX, chip8_32.get_pc());
    
    // R31 예약 레지스터 초기화
    chip8_32.set_R(RESERVED_INDEX, 0x0000);
    
    std::cout << "[StackFrame] Stack initialized:" << std::endl;
    std::cout << "  RSP (R29) = 0x" << std::hex << chip8_32.get_R(RSP_INDEX) << std::endl;
    std::cout << "  RBP (R28) = 0x" << std::hex << chip8_32.get_R(RBP_INDEX) << std::endl;
    std::cout << "  RIP (R30) = 0x" << std::hex << chip8_32.get_R(RIP_INDEX) << std::dec << std::endl;
}

bool check_stack_overflow(uint32_t rsp) {
    if (rsp < STACK_END) {
        std::cerr << "[StackFrame] STACK OVERFLOW! RSP=0x" 
                  << std::hex << rsp << " < STACK_END=0x" << STACK_END << std::dec << std::endl;
        return true;
    }
    return false;
}

bool check_stack_underflow(uint32_t rsp) {
    if (rsp > STACK_START) {
        std::cerr << "[StackFrame] STACK UNDERFLOW! RSP=0x" 
                  << std::hex << rsp << " > STACK_START=0x" << STACK_START << std::dec << std::endl;
        return true;
    }
    return false;
}

void print_stack_frame(const Chip8_32& chip8_32) {
    uint32_t rbp = chip8_32.get_R(RBP_INDEX);
    uint32_t rsp = chip8_32.get_R(RSP_INDEX);
    uint32_t rip = chip8_32.get_R(RIP_INDEX);
    
    std::cout << "\n=== STACK FRAME INFO ===" << std::endl;
    std::cout << "RBP (R28): 0x" << std::hex << std::setw(8) << std::setfill('0') << rbp << std::endl;
    std::cout << "RSP (R29): 0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << std::endl;
    std::cout << "RIP (R30): 0x" << std::hex << std::setw(8) << std::setfill('0') << rip << std::endl;
    
    uint32_t used = (STACK_START >= rsp) ? (STACK_START - rsp) : 0;
    uint32_t free = (rsp >= STACK_END) ? (rsp - STACK_END) : 0;
    std::cout << "Stack Used: " << used << " bytes" << std::endl;
    std::cout << "Stack Free: " << free << " bytes" << std::endl;

    std::cout << "Stack Free: " << std::dec << (rsp - STACK_END) << " bytes" << std::endl;
    std::cout << "=========================" << std::dec << std::endl;
}

void dump_stack_memory(const Chip8_32& chip8_32, uint32_t start_addr, uint32_t end_addr) {
    std::cout << "\n=== STACK MEMORY DUMP ===" << std::endl;
    std::cout << "Range: 0x" << std::hex << start_addr << " - 0x" << end_addr << std::endl;
    
    // 4바이트씩 출력 (32비트 워드 단위)
    for (uint32_t addr = start_addr; addr <= end_addr && addr + 3 < MEMORY_SIZE_32; addr += 4) {
        if (addr + 3 < MEMORY_SIZE_32) {
            uint32_t word = (chip8_32.get_memory(addr) << 24) |
                           (chip8_32.get_memory(addr + 1) << 16) |
                           (chip8_32.get_memory(addr + 2) << 8) |
                           chip8_32.get_memory(addr + 3);
            
            std::cout << "0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                      << ": 0x" << std::setw(8) << word;
            
            // RSP, RBP 표시
            uint32_t rsp = chip8_32.get_R(RSP_INDEX);
            uint32_t rbp = chip8_32.get_R(RBP_INDEX);
            
            if (addr == rsp) std::cout << " <-- RSP";
            if (addr == rbp) std::cout << " <-- RBP";
            
            std::cout << std::endl;
        }
    }
    std::cout << "=========================" << std::dec << std::endl;
}

} // namespace StackFrame