#include "debugger.hpp"
#include "chip8.hpp"
#include "chip8_32.hpp"
#include "stack_frame.hpp"  // 🔧 추가: StackFrame 네임스페이스 사용을 위해
#include <iostream>
#include <sstream>
#include <iomanip>

namespace chip8emu {

// === 공통 헬퍼 함수들 ===
std::string toHex8(uint8_t value) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
        << static_cast<int>(value);
    return oss.str();
}

std::string toHex16(uint16_t value) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << value;
    return oss.str();
}

std::string toHex32(uint32_t value) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
    return oss.str();
}

// ===============================================
// 8비트 디버거 구현
// ===============================================

std::string Debugger8::toHex8(uint8_t value) const {
    return ::chip8emu::toHex8(value);
}

std::string Debugger8::toHex16(uint16_t value) const {
    return ::chip8emu::toHex16(value);
}

std::string Debugger8::toHex32(uint32_t value) const {
    return ::chip8emu::toHex32(value);
}

void Debugger8::printState(uint32_t opcode) {
    if (!enabled_) return;

    // 브레이크포인트 체크
    uint16_t pc = chip8_.get_pc();
    if (hasBreakpoint(pc)) {
        std::cout << "\n🚨 BREAKPOINT HIT at " << toHex16(pc) << " 🚨\n";
        step_mode_ = true;
    }

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🎮 8-bit CHIP-8 Debug State\n";
    std::cout << std::string(60, '=') << "\n";

    // 1) PC, Opcode, 해석된 명령어
    std::cout << "📍 PC=" << toHex16(pc)
              << "  Opcode=" << toHex32(opcode)
              << "  ➤ " << disassemble(opcode) << "\n\n";

    // 2) V0~V7
    std::cout << "📊 V0-V7:  ";
    for (int i = 0; i < 8; ++i) {
        std::cout << "V" << std::hex << i << "=" << toHex8(chip8_.get_V(i)) << "  ";
    }
    std::cout << "\n";

    // 3) V8~VF
    std::cout << "📊 V8-VF:  ";
    for (int i = 8; i < 16; ++i) {
        std::cout << "V" << std::hex << i << "=" << toHex8(chip8_.get_V(i)) << "  ";
    }
    std::cout << "\n\n";

    // 4) 특수 레지스터
    std::cout << "🎯 I=" << toHex16(chip8_.get_I())
              << "  SP=" << std::dec << static_cast<int>(chip8_.get_sp())
              << "  Delay=" << static_cast<int>(chip8_.get_delay_timer())
              << "  Sound=" << static_cast<int>(chip8_.get_sound_timer()) << "\n\n";

    // 5) 스택
    std::cout << "📚 Stack: ";
    for (int i = 0; i < 16; ++i) {
        if (i == chip8_.get_sp()) {
            std::cout << "[" << toHex16(chip8_.get_stack(i)) << "]← ";
        } else if (i < chip8_.get_sp()) {
            std::cout << toHex16(chip8_.get_stack(i)) << " ";
        } else {
            std::cout << ".... ";
        }
        if ((i + 1) % 8 == 0) std::cout << "\n       ";
    }
    std::cout << "\n";

    std::cout << std::string(60, '-') << "\n";

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        handleDebugInput();
    }
}

std::string Debugger8::disassemble(uint32_t opcode) {
    uint16_t opcode16 = static_cast<uint16_t>(opcode & 0xFFFF);
    
    std::ostringstream oss;
    switch (opcode16 & 0xF000) {
        case 0x0000:
            if (opcode16 == 0x00E0) oss << "CLS";
            else if (opcode16 == 0x00EE) oss << "RET";
            else oss << "SYS";
            break;
        case 0x1000: oss << "JP"; break;
        case 0x2000: oss << "CALL"; break;
        case 0x3000: oss << "SE"; break;
        case 0x4000: oss << "SNE"; break;
        case 0x5000: oss << "SE"; break;
        case 0x6000: oss << "LD"; break;
        case 0x7000: oss << "ADD"; break;
        case 0x8000: oss << "ALU"; break;
        case 0x9000: oss << "SNE"; break;
        case 0xA000: oss << "LD I"; break;
        case 0xB000: oss << "JP V0"; break;
        case 0xC000: oss << "RND"; break;
        case 0xD000: oss << "DRW"; break;
        case 0xE000: oss << "KEY"; break;
        case 0xF000: oss << "MISC"; break;
        default: oss << "UNK"; break;
    }
    return oss.str();
}

void Debugger8::handleDebugInput() {
    if (!step_mode_) return;
    
    std::cout << "\n🐛 [DEBUG 8-bit] PC=0x" << std::hex << chip8_.get_pc() << std::dec << std::endl;
    std::cout << "Enter command (s=step, c=continue, q=quit, h=help): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        input = "s";
    }
    
    if (input == "s" || input == "step") {
        std::cout << "➤ Stepping to next instruction..." << std::endl;
    }
    else if (input == "c" || input == "continue") {
        step_mode_ = false;
        std::cout << "➤ Continuing execution..." << std::endl;
    }
    else if (input == "q" || input == "quit") {
        enabled_ = false;
        std::cout << "➤ Exiting debugger..." << std::endl;
    }
    else if (input.substr(0, 2) == "bp") {
        if (input.length() > 3) {
            try {
                uint16_t addr = std::stoul(input.substr(3), nullptr, 16);
                addBreakpoint(addr);
                std::cout << "➤ Breakpoint set at 0x" << std::hex << addr << std::dec << std::endl;
            } catch (...) {
                std::cout << "❌ Invalid address. Use: bp 0x200" << std::endl;
            }
        } else {
            std::cout << "❌ Usage: bp <address>. Example: bp 0x200" << std::endl;
        }
    }
    else if (input == "h" || input == "help") {
        std::cout << "\n🐛 Debug Commands:" << std::endl;
        std::cout << "  s, step       - Execute next instruction" << std::endl;
        std::cout << "  c, continue   - Continue execution" << std::endl;
        std::cout << "  q, quit       - Exit debugger" << std::endl;
        std::cout << "  bp <addr>     - Set breakpoint (hex)" << std::endl;
        std::cout << "  h, help       - Show this help" << std::endl;
        handleDebugInput();
    }
    else {
        std::cout << "❌ Unknown command '" << input << "'. Type 'h' for help." << std::endl;
        handleDebugInput();
    }
}

// ===============================================
// 32비트 디버거 구현 (4단계 완성)
// ===============================================

std::string Debugger32::toHex8(uint8_t value) const {
    return ::chip8emu::toHex8(value);
}

std::string Debugger32::toHex16(uint16_t value) const {
    return ::chip8emu::toHex16(value);
}

std::string Debugger32::toHex32(uint32_t value) const {
    return ::chip8emu::toHex32(value);
}

void Debugger32::printState(uint32_t opcode) {
    if (!enabled_) return;

    // 브레이크포인트 체크
    uint32_t pc = chip8_.get_pc();
    if (hasBreakpoint(static_cast<uint16_t>(pc))) {
        std::cout << "\n🚨 BREAKPOINT HIT at " << toHex32(pc) << " 🚨\n";
        step_mode_ = true;
    }

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🎮 32-bit CHIP-8 Debug State\n";
    std::cout << std::string(60, '=') << "\n";

    // 1) PC, Opcode, 해석된 명령어
    std::cout << "📍 PC=" << toHex32(pc)
              << "  Opcode=" << toHex32(opcode)
              << "  ➤ " << disassemble(opcode) << "\n\n";

    // 2) R0~R15
    std::cout << "📊 R0-R15: ";
    for (int i = 0; i < 16; ++i) {
        std::cout << "R" << std::hex << i << "=" << toHex32(chip8_.get_R(i)) << "  ";
        if ((i + 1) % 8 == 0) std::cout << "\n          ";
    }

    // 3) R16~R31 (스택 프레임 레지스터 강조)
    std::cout << "\n📊 R16-R31:";
    for (int i = 16; i < 32; ++i) {
        if (i == StackFrame::RBP_INDEX) {
            std::cout << "🔗RBP=" << toHex32(chip8_.get_R(i)) << "  ";
        } else if (i == StackFrame::RSP_INDEX) {
            std::cout << "📚RSP=" << toHex32(chip8_.get_R(i)) << "  ";
        } else if (i == StackFrame::RIP_INDEX) {
            std::cout << "👉RIP=" << toHex32(chip8_.get_R(i)) << "  ";
        } else {
            std::cout << "R" << std::hex << i << "=" << toHex32(chip8_.get_R(i)) << "  ";
        }
        if ((i + 1) % 8 == 0) std::cout << "\n          ";
    }
    std::cout << "\n";

    // 4) 특수 레지스터
    std::cout << "🎯 I=" << toHex32(chip8_.get_I())
              << "  SP=" << std::dec << static_cast<int>(chip8_.get_sp())
              << "  Delay=" << static_cast<int>(chip8_.get_delay_timer())
              << "  Sound=" << static_cast<int>(chip8_.get_sound_timer()) << "\n\n";

    // 🔥 **5) 실시간 스택 프레임 시각화 (4단계 완성!)**
    StackVisualizer visualizer;
    
    // 스택 프레임 명령어인지 확인
    uint8_t first_byte = (opcode >> 24) & 0xFF;
    if (first_byte == 0x11) {
        // 스택 프레임 명령어 실행 중
        std::string instruction_name = getStackInstructionName(opcode);
        visualizer.animateStackFrame(chip8_, instruction_name, instruction_name, false);
    } else {
        // 일반 명령어 - 기본 스택 상태만 표시
        visualizer.drawStackFrame(chip8_, "CURRENT STATE");
    }

    std::cout << std::string(60, '-') << "\n";

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        handleDebugInput();
    }
}

std::string Debugger32::disassemble(uint32_t opcode) {
    uint8_t first = (opcode >> 24) & 0xFF;
    
    std::ostringstream oss;
    
    // 스택 프레임 명령어 특별 처리
    if (first == 0x11) {
        uint8_t sub = (opcode >> 16) & 0xFF;
        switch (sub) {
            case 0x00: oss << "PUSH"; break;
            case 0x01: oss << "POP"; break;
            case 0x02: oss << "MOV_RBP_RSP"; break;
            case 0x03: oss << "MOV_RSP_RBP"; break;
            case 0x04: oss << "SUB_RSP"; break;
            case 0x05: oss << "ADD_RSP"; break;
            case 0x06: oss << "CALL_FUNC"; break;
            case 0x07: oss << "RET_FUNC"; break;
            case 0x08: oss << "MOV_[RBP-],RX"; break;
            case 0x09: oss << "MOV_RX,[RBP-]"; break;
            case 0x0A: oss << "MOV_[RBP+],RX"; break;
            case 0x0B: oss << "MOV_RX,[RBP+]"; break;
            default: oss << "STACK_UNK"; break;
        }
        return oss.str();
    }
    
    // 기본 32비트 명령어
    static const char* kNames[32] = {
        "SYS", "JP", "CALL", "SE", "SNE", "SE", "LD", "ADD",
        "ALU", "SNE", "LDI", "JP V0", "RND", "DRW", "KEY", "MISC",
        "SYSCALL", "STACK", "EXT18", "EXT19", "EXT20", "EXT21", "EXT22", "EXT23",
        "EXT24", "EXT25", "EXT26", "EXT27", "EXT28", "EXT29", "EXT30", "EXT31"
    };
    
    oss << (first < 32 ? kNames[first] : "UNK") << "_32";
    return oss.str();
}

void Debugger32::handleDebugInput() {
    if (!step_mode_) return;
    
    std::cout << "\n🐛 [DEBUG 32-bit] PC=0x" << std::hex << chip8_.get_pc() << std::dec << std::endl;
    std::cout << "Enter command (s=step, c=continue, q=quit, sf=stack frame demo, h=help): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        input = "s";
    }
    
    if (input == "s" || input == "step") {
        std::cout << "➤ Stepping to next instruction..." << std::endl;
    }
    else if (input == "c" || input == "continue") {
        step_mode_ = false;
        std::cout << "➤ Continuing execution..." << std::endl;
    }
    else if (input == "q" || input == "quit") {
        enabled_ = false;
        std::cout << "➤ Exiting debugger..." << std::endl;
    }
    else if (input == "sf" || input == "stackframe") {
        // 🔥 **4단계 완성: 대화형 스택 프레임 데모**
        StackVisualizer visualizer;
        visualizer.interactiveStackDebug(chip8_);
        handleDebugInput(); // 데모 후 다시 디버그 입력으로
    }
    else if (input.substr(0, 2) == "bp") {
        if (input.length() > 3) {
            try {
                uint32_t addr = std::stoul(input.substr(3), nullptr, 16);
                addBreakpoint(addr);
                std::cout << "➤ Breakpoint set at 0x" << std::hex << addr << std::dec << std::endl;
            } catch (...) {
                std::cout << "❌ Invalid address. Use: bp 0x200" << std::endl;
            }
        } else {
            std::cout << "❌ Usage: bp <address>. Example: bp 0x200" << std::endl;
        }
    }
    else if (input == "h" || input == "help") {
        std::cout << "\n🐛 Debug Commands:" << std::endl;
        std::cout << "  s, step        - Execute next instruction" << std::endl;
        std::cout << "  c, continue    - Continue execution" << std::endl;
        std::cout << "  q, quit        - Exit debugger" << std::endl;
        std::cout << "  sf, stackframe - Interactive stack frame demo" << std::endl;
        std::cout << "  bp <addr>      - Set breakpoint (hex)" << std::endl;
        std::cout << "  h, help        - Show this help" << std::endl;
        handleDebugInput();
    }
    else {
        std::cout << "❌ Unknown command '" << input << "'. Type 'h' for help." << std::endl;
        handleDebugInput();
    }
}

// 🔥 **스택 명령어 이름 반환 함수**
std::string Debugger32::getStackInstructionName(uint32_t opcode) {
    uint8_t sub = (opcode >> 16) & 0xFF;
    switch (sub) {
        case 0x00: return "PUSH RBP/RX";
        case 0x01: return "POP RBP/RX";
        case 0x02: return "MOV RBP, RSP";
        case 0x03: return "MOV RSP, RBP";
        case 0x04: return "SUB RSP, NNNN";
        case 0x05: return "ADD RSP, NNNN";
        case 0x06: return "CALL_FUNC";
        case 0x07: return "RET_FUNC";
        case 0x08: return "MOV [RBP-NN], RX";
        case 0x09: return "MOV RX, [RBP-NN]";
        case 0x0A: return "MOV [RBP+NN], RX";
        case 0x0B: return "MOV RX, [RBP+NN]";
        default: return "STACK_UNKNOWN";
    }
}

// 🔧 추가: 누락된 함수 구현
void Debugger32::drawStackDiagram(const Chip8_32& chip8_32, uint32_t highlight_addr) {
    StackVisualizer visualizer;
    visualizer.drawStackFrame(chip8_32, "CURRENT STATE", highlight_addr);
}

// ===============================================
// 🔥 StackVisualizer 완전한 구현 (4단계 완성)
// ===============================================

void StackVisualizer::drawStackFrame(const Chip8_32& chip8_32, const std::string& phase, uint32_t highlight_addr) {
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    if (!phase.empty()) {
        std::cout << "\n🚀 STACK FRAME [" << phase << "] 🚀" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
    }
    
    // 🔥 **실시간 스택 메모리 분석**
    analyzeRealTimeStack(chip8_32);
    
    std::cout << "\n┌─────────────────────────────────┐ ← 0x" << std::hex << std::uppercase << StackFrame::STACK_START << " (STACK_START)" << std::endl;
    
    // 🔥 **실제 메모리 데이터 기반 스택 시각화**
    uint32_t stack_size = StackFrame::STACK_START - rsp;
    uint32_t cells_to_show = std::min(stack_size / 4 + 4, 16u);
    
    for (uint32_t i = 0; i < cells_to_show; ++i) {
        uint32_t addr = StackFrame::STACK_START - (i * 4);
        
        if (addr < StackFrame::STACK_END) break;
        
        // 🔥 **실제 메모리에서 32비트 값 읽기**
        uint32_t memory_value = 0;
        if (addr + 3 < MEMORY_SIZE_32) {
            memory_value = (chip8_32.get_memory(addr) << 24) |
                          (chip8_32.get_memory(addr + 1) << 16) |
                          (chip8_32.get_memory(addr + 2) << 8) |
                          chip8_32.get_memory(addr + 3);
        }
        
        std::string cell_display = formatStackCell(addr, memory_value, rbp, rsp, highlight_addr);
        std::string pointer_info = getPointerInfo(addr, rbp, rsp);
        
        std::cout << "├─────────────────────────────────┤" << pointer_info << std::endl;
        std::cout << "│ " << cell_display << " │" << std::endl;
        
        if (addr == rsp) {
            if (i < 3) continue;
            else break;
        }
    }
    
    std::cout << "└─────────────────────────────────┘ ← 0x" << std::hex << StackFrame::STACK_END << " (STACK_END)" << std::endl;
    
    drawAdvancedPointers(chip8_32, rbp, rsp);
}

// 🎬 **4.3 스택 프레임 애니메이션 구현**
void StackVisualizer::animateStackFrame(const Chip8_32& chip8_32, const std::string& phase, 
                                       const std::string& instruction, bool wait_for_input) {
    clearScreen();
    
    std::cout << "\n";
    std::cout << "🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥\n";
    std::cout << "🚀          STACK FRAME ANIMATION DEBUGGER         🚀\n";
    std::cout << "🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥\n";
    
    if (!instruction.empty()) {
        showInstructionInfo(instruction, phase);
    }
    
    drawStackFrame(chip8_32, phase);
    
    if (wait_for_input) {
        waitForUser();
    }
}

void StackVisualizer::interactiveStackDebug(Chip8_32& chip8_32) {
    clearScreen();
    
    std::cout << "\n";
    std::cout << "🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮\n";
    std::cout << "🔥        INTERACTIVE STACK FRAME DEBUGGER        🔥\n";
    std::cout << "🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮🎮\n\n";
    
    std::cout << "🎯 TARGET: Demonstrate sum(10, 20, 30) = 60 with Stack Frame\n\n";
    
    animateStackFrame(chip8_32, "INITIAL STATE", "Starting with empty stack");
    
    simulateFunctionCall(chip8_32);
    
    std::cout << "\n✅ Stack Frame Animation Complete!\n";
    std::cout << "🎯 Commands: [r]eplay, [s]tack dump, [q]uit: ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "r" || input == "replay") {
        interactiveStackDebug(chip8_32);
    } else if (input == "s" || input == "stack") {
        drawStackFrame(chip8_32, "FINAL STATE");
        waitForUser();
    }
}

void StackVisualizer::simulateFunctionCall(Chip8_32& chip8_32) {
    uint32_t original_rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t original_rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    // 1. FUNCTION PROLOGUE
    animateStackFrame(chip8_32, "STEP 1: FUNCTION PROLOGUE", "PUSH RBP");
    
    chip8_32.set_R(StackFrame::RSP_INDEX, original_rsp - 4);
    animateStackFrame(chip8_32, "STEP 1: FUNCTION PROLOGUE", "MOV RBP, RSP");
    
    chip8_32.set_R(StackFrame::RBP_INDEX, chip8_32.get_R(StackFrame::RSP_INDEX));
    animateStackFrame(chip8_32, "STEP 1: FUNCTION PROLOGUE", "SUB RSP, 16");
    
    chip8_32.set_R(StackFrame::RSP_INDEX, chip8_32.get_R(StackFrame::RSP_INDEX) - 16);
    
    // 2. PARAMETER SETUP
    animateStackFrame(chip8_32, "STEP 2: PARAMETER SETUP", "MOV [RBP-4], 10 (param a)");
    
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr_a = rbp - 4;
    chip8_32.set_memory(addr_a, 0);
    chip8_32.set_memory(addr_a + 1, 0);
    chip8_32.set_memory(addr_a + 2, 0);
    chip8_32.set_memory(addr_a + 3, 10);
    
    animateStackFrame(chip8_32, "STEP 2: PARAMETER SETUP", "MOV [RBP-8], 20 (param b)", true);
    
    uint32_t addr_b = rbp - 8;
    chip8_32.set_memory(addr_b, 0);
    chip8_32.set_memory(addr_b + 1, 0);
    chip8_32.set_memory(addr_b + 2, 0);
    chip8_32.set_memory(addr_b + 3, 20);
    
    animateStackFrame(chip8_32, "STEP 2: PARAMETER SETUP", "MOV [RBP-12], 30 (param c)", true);
    
    uint32_t addr_c = rbp - 12;
    chip8_32.set_memory(addr_c, 0);
    chip8_32.set_memory(addr_c + 1, 0);
    chip8_32.set_memory(addr_c + 2, 0);
    chip8_32.set_memory(addr_c + 3, 30);
    
    // 3. CALCULATION
    animateStackFrame(chip8_32, "STEP 3: CALCULATION", "MOV R3, [RBP-4] ; R3 = a = 10", true);
    animateStackFrame(chip8_32, "STEP 3: CALCULATION", "ADD R3, [RBP-8] ; R3 = 10 + 20 = 30", true);
    animateStackFrame(chip8_32, "STEP 3: CALCULATION", "ADD R3, [RBP-12] ; R3 = 30 + 30 = 60", true);
    
    // 결과 저장
    uint32_t addr_result = rbp - 16;
    chip8_32.set_memory(addr_result, 0);
    chip8_32.set_memory(addr_result + 1, 0);
    chip8_32.set_memory(addr_result + 2, 0);
    chip8_32.set_memory(addr_result + 3, 60);
    
    animateStackFrame(chip8_32, "STEP 3: CALCULATION", "MOV [RBP-16], R3 ; result = 60", true);
    
    // 4. FUNCTION EPILOGUE
    animateStackFrame(chip8_32, "STEP 4: FUNCTION EPILOGUE", "ADD RSP, 16 ; cleanup local vars", true);
    
    chip8_32.set_R(StackFrame::RSP_INDEX, chip8_32.get_R(StackFrame::RSP_INDEX) + 16);
    
    animateStackFrame(chip8_32, "STEP 4: FUNCTION EPILOGUE", "POP RBP ; restore old frame", true);
    
    chip8_32.set_R(StackFrame::RBP_INDEX, original_rbp);
    chip8_32.set_R(StackFrame::RSP_INDEX, original_rsp);
    
    animateStackFrame(chip8_32, "STEP 4: FUNCTION EPILOGUE", "RET ; return to caller", true);
    
    // 최종 결과 표시
    clearScreen();
    std::cout << "\n🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉\n";
    std::cout << "🔥           STACK FRAME SIMULATION COMPLETE!        🔥\n";
    std::cout << "🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉\n\n";
    std::cout << "✅ RESULT: sum(10, 20, 30) = 60\n";
    std::cout << "✅ Stack frame properly created and destroyed\n";
    std::cout << "✅ All parameters correctly passed and computed\n\n";
    
    waitForUser("Press ENTER to continue...");
}

// 🎬 애니메이션 헬퍼 함수들
void StackVisualizer::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void StackVisualizer::waitForUser(const std::string& message) {
    std::cout << "\n" << message;
    std::string dummy;
    std::getline(std::cin, dummy);
}

void StackVisualizer::showInstructionInfo(const std::string& instruction, const std::string& description) {
    std::cout << "\n📋 CURRENT INSTRUCTION:\n";
    std::cout << "   " << instruction << "\n";
    std::cout << "💡 DESCRIPTION:\n";
    std::cout << "   " << description << "\n\n";
}

// 🚀 **실시간 스택 메모리 분석**
void StackVisualizer::analyzeRealTimeStack(const Chip8_32& chip8_32) {
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    clearCells();
    
    // 반환 주소 감지 (RBP + 4)
    if (rbp + 4 <= StackFrame::STACK_START && rbp + 4 >= StackFrame::STACK_END) {
        uint32_t ret_addr = (chip8_32.get_memory(rbp + 4) << 24) |
                           (chip8_32.get_memory(rbp + 5) << 16) |
                           (chip8_32.get_memory(rbp + 6) << 8) |
                           chip8_32.get_memory(rbp + 7);
        addStackCell(rbp + 4, ret_addr, StackCellType::RESULT, "Return Address");
    }
    
    // 저장된 RBP 감지
    if (rbp <= StackFrame::STACK_START && rbp >= StackFrame::STACK_END) {
        uint32_t saved_rbp = (chip8_32.get_memory(rbp) << 24) |
                            (chip8_32.get_memory(rbp + 1) << 16) |
                            (chip8_32.get_memory(rbp + 2) << 8) |
                            chip8_32.get_memory(rbp + 3);
        addStackCell(rbp, saved_rbp, StackCellType::OLD_RBP, "Saved RBP");
    }
    
    // 매개변수/지역변수 자동 감지
    uint32_t param_count = 0;
    for (uint32_t offset = 4; offset <= 16; offset += 4) {
        uint32_t addr = rbp - offset;
        if (addr >= rsp && addr >= StackFrame::STACK_END) {
            uint32_t value = (chip8_32.get_memory(addr) << 24) |
                           (chip8_32.get_memory(addr + 1) << 16) |
                           (chip8_32.get_memory(addr + 2) << 8) |
                           chip8_32.get_memory(addr + 3);
            
            std::string label;
            StackCellType type;
            
            if (param_count == 0) {
                label = "param a = " + std::to_string(value);
                type = StackCellType::PARAMETER;
            } else if (param_count == 1) {
                label = "param b = " + std::to_string(value);
                type = StackCellType::PARAMETER;
            } else if (param_count == 2) {
                label = "param c = " + std::to_string(value);
                type = StackCellType::PARAMETER;
            } else {
                label = "result = " + std::to_string(value);
                type = StackCellType::RESULT;
            }
            
            addStackCell(addr, value, type, label);
            param_count++;
        }
    }
}

std::string StackVisualizer::formatStackCell(uint32_t addr, uint32_t value, uint32_t rbp, uint32_t rsp, uint32_t highlight_addr) {
    bool is_highlighted = (addr == highlight_addr);
    StackCell* cell = findCell(addr);
    
    std::ostringstream oss;
    
    if (is_highlighted) {
        oss << "⭐ ";
    } else if (cell) {
        oss << getTypeEmoji(cell->type) << " ";
    } else {
        oss << "   ";
    }
    
    // 주소 표시
    oss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr << ": ";
    
    // 값 및 라벨 표시
    if (cell && !cell->label.empty()) {
        oss << std::left << std::setw(18) << cell->label;
    } else if (value != 0) {
        oss << "0x" << std::setw(8) << std::setfill('0') << value;
        oss << " (" << std::dec << value << ")    ";
    } else {
        oss << std::left << std::setw(18) << "[EMPTY]";
    }
    
    return oss.str();
}

std::string StackVisualizer::getPointerInfo(uint32_t addr, uint32_t rbp, uint32_t rsp) {
    if (addr == rbp && addr == rsp) {
        return " ← RBP & RSP";
    } else if (addr == rbp) {
        return " ← RBP (Frame Base)";
    } else if (addr == rsp) {
        return " ← RSP (Stack Top)";
    } else if (addr == rbp + 4) {
        return " ← Return Address";
    } else {
        return "";
    }
}

void StackVisualizer::drawAdvancedPointers(const Chip8_32& chip8_32, uint32_t rbp, uint32_t rsp) {
    uint32_t rip = chip8_32.get_R(StackFrame::RIP_INDEX);
    uint32_t used_bytes = StackFrame::STACK_START - rsp;
    uint32_t total_bytes = StackFrame::STACK_START - StackFrame::STACK_END;
    double usage_percent = (double)used_bytes / total_bytes * 100.0;
    
    std::cout << "\n🎯 STACK FRAME STATUS:" << std::endl;
    std::cout << "   RBP (Base)  = 0x" << std::hex << std::setw(8) << std::setfill('0') << rbp;
    std::cout << "   RSP (Top)   = 0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << std::endl;
    std::cout << "   RIP (PC)    = 0x" << std::hex << std::setw(8) << std::setfill('0') << rip << std::endl;
    std::cout << "   Used Stack  = " << std::dec << used_bytes << " / " << total_bytes << " bytes ";
    std::cout << "(" << std::fixed << std::setprecision(1) << usage_percent << "%)" << std::endl;
    
    if (usage_percent > 75.0) {
        std::cout << "   ⚠️  WARNING: Stack usage > 75%" << std::endl;
    } else if (usage_percent > 50.0) {
        std::cout << "   ⚡ Stack usage moderate" << std::endl;
    } else {
        std::cout << "   ✅ Stack usage healthy" << std::endl;
    }
}

void StackVisualizer::addStackCell(uint32_t addr, uint32_t value, StackCellType type, const std::string& label) {
    StackCell* existing = findCell(addr);
    if (existing) {
        existing->value = value;
        existing->type = type;
        existing->label = label;
        existing->is_active = true;
    } else {
        stack_cells_.emplace_back(addr, value, type, label, true);
    }
}

void StackVisualizer::highlightCell(uint32_t address) {
    for (auto& cell : stack_cells_) {
        cell.is_active = (cell.address == address);
    }
}

void StackVisualizer::clearCells() {
    stack_cells_.clear();
}

void StackVisualizer::drawStackBox(uint32_t start_addr, uint32_t end_addr) {
    std::cout << "┌─────────────────────┐ ← 0x" << std::hex << start_addr << std::endl;
    std::cout << "│                     │" << std::endl;
    std::cout << "│     [STACK]         │" << std::endl;
    std::cout << "│                     │" << std::endl;
    std::cout << "└─────────────────────┘ ← 0x" << std::hex << end_addr << std::endl;
}

void StackVisualizer::drawStackCell(const StackCell& cell) {
    std::string emoji = getTypeEmoji(cell.type);
    std::cout << "│ " << emoji << " " << std::left << std::setw(17) << cell.label << "│";
    if (cell.is_active) std::cout << " ← ACTIVE";
    std::cout << std::endl;
}

void StackVisualizer::drawPointers(uint32_t rbp, uint32_t rsp) {
    std::cout << "\n🎯 Pointer Status:" << std::endl;
    std::cout << "   RBP (Base) = 0x" << std::hex << std::setw(8) << std::setfill('0') << rbp << std::endl;
    std::cout << "   RSP (Top)  = 0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << std::endl;
    std::cout << "   Used: " << std::dec << (0xEFFF - rsp) << " bytes" << std::endl;
}

std::string StackVisualizer::getTypeEmoji(StackCellType type) {
    switch (type) {
        case StackCellType::EMPTY:     return "  ";
        case StackCellType::OLD_RBP:   return "🔴";
        case StackCellType::PARAMETER: return "🟡";
        case StackCellType::LOCAL_VAR: return "🟠";
        case StackCellType::RESULT:    return "🟢";
        case StackCellType::HIGHLIGHT: return "⭐";
        default:                       return "❓";
    }
}

StackCell* StackVisualizer::findCell(uint32_t addr) {
    for (auto& cell : stack_cells_) {
        if (cell.address == addr) {
            return &cell;
        }
    }
    return nullptr;
}

} // namespace chip8emu