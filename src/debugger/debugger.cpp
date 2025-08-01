#include "debugger.hpp"
#include "chip8.hpp"
#include "chip8_32.hpp"
#include "stack_frame.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace chip8emu {

// 정적 변수 정의
bool Debugger8::first_debug_print_ = true;
bool Debugger32::first_debug_print_ = true;

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

void Debugger8::enable(bool on) {
    if (on && !enabled_) {
        step_mode_ = false;
        clearBreakpoints();
        first_debug_print_ = true;
        std::cout << "[INFO] 8-bit debugger initialized" << std::endl;
    }
    enabled_ = on;
}

void Debugger8::printState(uint32_t opcode) {
    if (!enabled_) return;

    // 브레이크포인트 체크
    uint16_t pc = chip8_.get_pc();
    if (hasBreakpoint(pc)) {
        std::cout << "\nBREAKPOINT HIT at " << toHex16(pc) << "\n";
        step_mode_ = true;
    }

    if (first_debug_print_) {
        // 처음에만 전체 UI 출력
        std::cout << "\n============================================================\n";
        std::cout << "8-bit CHIP-8 Debug State\n";
        std::cout << "============================================================\n";
        
        // PC, Opcode 줄
        std::cout << "PC=[    ]  Opcode=[        ]  \n";
        
        // V0~V7
        std::cout << "V0-V7: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << "V" << std::hex << i
                      << "=[  ]  ";
        }
        std::cout << "\n";

        // V8~VF
        std::cout << "V8-VF: ";
        for (int i = 8; i < 16; ++i) {
            std::cout << "V" << std::hex << i
                      << "=[  ]  ";
        }
        std::cout << "\n";

        // I, SP, 타이머
        std::cout << "I=[    ]  SP=[  ]  Delay=[  ]  Sound=[  ]\n";
        
        // 스택 내용 (16개)
        std::cout << "Stack: ";
        for (int i = 0; i < 16; ++i) {
            std::cout << "[    ] ";
        }
        std::cout << "\n--------------------------------------------------\n";
        
        first_debug_print_ = false;
    }

    // 내부 상태 가져오기
    uint16_t I     = chip8_.get_I();
    uint8_t  sp    = chip8_.get_sp();
    const uint8_t* V     = chip8_.getV8();
    const uint16_t* stack = chip8_.getStack16();
    uint8_t  delay = chip8_.get_delay_timer();
    uint8_t  sound = chip8_.get_sound_timer();

    // 값들만 업데이트 (커서 위치 이동으로)
    std::cout << "\033[s";  // 현재 위치 저장
    
    // PC 값 업데이트 (4번째 줄, 4번째 위치)
    std::cout << "\033[4;4H" << ::chip8emu::toHex16(pc);
    
    // Opcode 값 업데이트 (4번째 줄, 19번째 위치)
    std::cout << "\033[4;19H" << ::chip8emu::toHex32(opcode);
    
    // V0~V7 값들 업데이트 (5번째 줄)
    for (int i = 0; i < 8; ++i) {
        int col = 10 + i * 7;  // V0부터 7칸 간격
        std::cout << "\033[5;" << col << "H" << ::chip8emu::toHex8(V[i]);
    }
    
    // V8~VF 값들 업데이트 (6번째 줄)
    for (int i = 8; i < 16; ++i) {
        int col = 10 + (i - 8) * 7;  // V8부터 7칸 간격
        std::cout << "\033[6;" << col << "H" << ::chip8emu::toHex8(V[i]);
    }
    
    // I, SP, 타이머 업데이트 (7번째 줄)
    std::cout << "\033[7;3H" << ::chip8emu::toHex16(I);      // I
    std::cout << "\033[7;15H" << ::chip8emu::toHex8(sp);     // SP
    std::cout << "\033[7;25H" << ::chip8emu::toHex8(delay);  // Delay
    std::cout << "\033[7;36H" << ::chip8emu::toHex8(sound);  // Sound
    
    // 스택 값들 업데이트 (8번째 줄)
    for (int i = 0; i < 16; ++i) {
        int col = 8 + i * 7;  // 7칸 간격
        std::cout << "\033[8;" << col << "H" << ::chip8emu::toHex16(stack[i]);
        
        // SP 표시
        if (i == sp) {
            std::cout << "\033[8;" << (col + 4) << "H<";
        }
    }
    
    std::cout << "\033[u";  // 저장된 위치로 복원
    std::cout << std::flush;

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        std::cout << "\n";
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
    
    std::cout << "\n[DEBUG 8-bit] PC=0x" << std::hex << chip8_.get_pc() << std::dec << std::endl;
    std::cout << "Enter command (s=step, c=continue, q=quit, h=help): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        input = "s";
    }
    
    if (input == "s" || input == "step") {
        std::cout << "[INFO] Stepping to next instruction..." << std::endl;
    }
    else if (input == "c" || input == "continue") {
        step_mode_ = false;
        std::cout << "[INFO] Continuing execution..." << std::endl;
    }
    else if (input == "q" || input == "quit") {
        enabled_ = false;
        std::cout << "[INFO] Exiting debugger..." << std::endl;
    }
    else if (input.substr(0, 2) == "bp") {
        if (input.length() > 3) {
            try {
                uint16_t addr = std::stoul(input.substr(3), nullptr, 16);
                addBreakpoint(addr);
                std::cout << "[INFO] Breakpoint set at 0x" << std::hex << addr << std::dec << std::endl;
            } catch (...) {
                std::cout << "ERROR: Invalid address. Use: bp 0x200" << std::endl;
            }
        } else {
            std::cout << "ERROR: Usage: bp <address>. Example: bp 0x200" << std::endl;
        }
    }
    else if (input == "h" || input == "help") {
        std::cout << "\nDebug Commands:" << std::endl;
        std::cout << "  s, step       - Execute next instruction" << std::endl;
        std::cout << "  c, continue   - Continue execution" << std::endl;
        std::cout << "  q, quit       - Exit debugger" << std::endl;
        std::cout << "  bp <addr>     - Set breakpoint (hex)" << std::endl;
        std::cout << "  h, help       - Show this help" << std::endl;
        handleDebugInput();
    }
    else {
        std::cout << "ERROR: Unknown command '" << input << "'. Type 'h' for help." << std::endl;
        handleDebugInput();
    }
}

void Debugger8::resetDebugState() {
    enabled_ = false;
    step_mode_ = false;
    clearBreakpoints();
    std::cout << "[INFO] 8-bit debugger state reset" << std::endl;
}

// ===============================================
// 32비트 디버거 구현 (4단계 완성)
// ===============================================

void Debugger32::enable(bool on) {
    if (on && !enabled_) {
        step_mode_ = false;
        clearBreakpoints();
        first_debug_print_ = true;
        std::cout << "[INFO] 32-bit debugger initialized" << std::endl;
    }
    enabled_ = on;
}

void Debugger32::printState(uint32_t opcode) {
    if (!enabled_) return;

    // 브레이크포인트 체크
    uint32_t pc = chip8_.get_pc();
    if (hasBreakpoint(static_cast<uint16_t>(pc))) {
        std::cout << "\nBREAKPOINT HIT at " << ::chip8emu::toHex32(pc) << "\n";
        step_mode_ = true;
    }

    if (first_debug_print_) {
        // 처음에만 전체 UI 출력
        std::cout << "\n============================================================\n";
        std::cout << "32-bit CHIP-8 Debug State\n";
        std::cout << "============================================================\n";
        
        // PC, Opcode 줄
        std::cout << "PC=[        ]  Opcode=[        ]  \n";
        
        // V0~V7
        std::cout << "V0-V7: ";
        for (int i = 0; i < 8; ++i) {
            std::cout << "V" << std::hex << i
                      << "=[        ]  ";
        }
        std::cout << "\n";

        // V8~VF
        std::cout << "V8-VF: ";
        for (int i = 8; i < 16; ++i) {
            std::cout << "V" << std::hex << i
                      << "=[        ]  ";
        }
        std::cout << "\n";

        // I, SP, 타이머
        std::cout << "I=[        ]  SP=[  ]  Delay=[  ]  Sound=[  ]\n";
        
        // 스택 내용 (32개, 32비트)
        std::cout << "Stack: ";
        for (int i = 0; i < 32; ++i) {
            std::cout << "[        ] ";
            if ((i + 1) % 8 == 0) std::cout << "\n       ";
        }
        std::cout << "\n--------------------------------------------------\n";
        
        first_debug_print_ = false;
    }

    // 내부 상태 가져오기
    uint32_t I     = chip8_.get_I();
    uint8_t  sp    = chip8_.get_sp();
    const uint32_t* V     = chip8_.getV32();
    const uint32_t* stack = chip8_.getStack32();
    uint8_t  delay = chip8_.get_delay_timer();
    uint8_t  sound = chip8_.get_sound_timer();

    // 값들만 업데이트 (커서 위치 이동으로)
    std::cout << "\033[s";  // 현재 위치 저장
    
    // PC 값 업데이트 (4번째 줄, 4번째 위치)
    std::cout << "\033[4;4H" << toHex32(pc);
    
    // Opcode 값 업데이트 (4번째 줄, 23번째 위치)
    std::cout << "\033[4;23H" << toHex32(opcode);
    
    // V0~V7 값들 업데이트 (5번째 줄)
    for (int i = 0; i < 8; ++i) {
        int col = 10 + i * 13;  // V0부터 13칸 간격
        std::cout << "\033[5;" << col << "H" << toHex32(V[i]);
    }
    
    // V8~VF 값들 업데이트 (6번째 줄)
    for (int i = 8; i < 16; ++i) {
        int col = 10 + (i - 8) * 13;  // V8부터 13칸 간격
        std::cout << "\033[6;" << col << "H" << toHex32(V[i]);
    }
    
    // I, SP, 타이머 업데이트 (7번째 줄)
    std::cout << "\033[7;3H" << toHex32(I);      // I
    std::cout << "\033[7;19H" << toHex8(sp);     // SP
    std::cout << "\033[7;33H" << toHex8(delay);  // Delay
    std::cout << "\033[7;46H" << toHex8(sound);  // Sound
    
    // 스택 값들 업데이트 (8번째 줄부터)
    for (int i = 0; i < 32; ++i) {
        int line = 8 + (i / 8);  // 8줄부터 시작, 8개씩
        int col = 8 + (i % 8) * 11;  // 11칸 간격
        std::cout << "\033[" << line << ";" << col << "H" << toHex32(stack[i]);
        
        // SP 표시
        if (i == sp) {
            std::cout << "\033[" << line << ";" << (col + 8) << "H<";
        }
    }
    
    std::cout << "\033[u";  // 저장된 위치로 복원
    std::cout << std::flush;

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        std::cout << "\n";
        handleDebugInput();
    }
}

std::string Debugger32::disassemble(uint32_t opcode) {
    // 32비트에서는 전체 4바이트를 사용하여 분류
    uint8_t first = (opcode >> 24) & 0xFF;

    // 기존과 동일한 분류 방식 사용하되, 32비트 표시
    static const char* kNames32[16] = {
        "SYS", "JP", "CALL", "SE", "SNE", "SE", "LD", "ADD",
        "ALU", "SNE", "LDI", "JP V0", "RND", "DRW", "KEY", "MISC" };

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
    
    // 32비트 분류: 상위 바이트를 기준으로 분류
    uint8_t classification = (first >> 4) & 0x0F;
    oss << (classification < 16 ? kNames32[classification] : "UNK");
    return oss.str();
}

void Debugger32::handleDebugInput() {
    if (!step_mode_) return;
    
    std::cout << "\n[DEBUG 32-bit] PC=0x" << std::hex << chip8_.get_pc() << std::dec << std::endl;
    std::cout << "Enter command (s=step, c=continue, q=quit, sf=stack frame demo, h=help): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input.empty()) {
        input = "s";
    }
    
    if (input == "s" || input == "step") {
        std::cout << "[INFO] Stepping to next instruction..." << std::endl;
    }
    else if (input == "c" || input == "continue") {
        step_mode_ = false;
        std::cout << "[INFO] Continuing execution..." << std::endl;
    }
    else if (input == "q" || input == "quit") {
        enabled_ = false;
        std::cout << "[INFO] Exiting debugger..." << std::endl;
    }
    else if (input == "sf" || input == "stackframe") {
        // **4단계 완성: 대화형 스택 프레임 데모**
        StackVisualizer visualizer;
        visualizer.resetVisualizerState();
        visualizer.interactiveStackDebug(chip8_);
        handleDebugInput(); // 데모 후 다시 디버그 입력으로
    }
    else if (input.substr(0, 2) == "bp") {
        if (input.length() > 3) {
            try {
                uint32_t addr = std::stoul(input.substr(3), nullptr, 16);
                addBreakpoint(addr);
                std::cout << "[INFO] Breakpoint set at 0x" << std::hex << addr << std::dec << std::endl;
            } catch (...) {
                std::cout << "ERROR: Invalid address. Use: bp 0x200" << std::endl;
            }
        } else {
            std::cout << "ERROR: Usage: bp <address>. Example: bp 0x200" << std::endl;
        }
    }
    else if (input == "h" || input == "help") {
        std::cout << "\nDebug Commands:" << std::endl;
        std::cout << "  s, step        - Execute next instruction" << std::endl;
        std::cout << "  c, continue    - Continue execution" << std::endl;
        std::cout << "  q, quit        - Exit debugger" << std::endl;
        std::cout << "  sf, stackframe - Interactive stack frame demo" << std::endl;
        std::cout << "  bp <addr>      - Set breakpoint (hex)" << std::endl;
        std::cout << "  h, help        - Show this help" << std::endl;
        handleDebugInput();
    }
    else {
        std::cout << "ERROR: Unknown command '" << input << "'. Type 'h' for help." << std::endl;
        handleDebugInput();
    }
}

void Debugger32::resetDebugState() {
    enabled_ = false;
    step_mode_ = false;
    clearBreakpoints();
    std::cout << "[INFO] 32-bit debugger state reset" << std::endl;
}

// **스택 명령어 이름 반환 함수**
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

// 누락된 함수 구현
void Debugger32::drawStackDiagram(const Chip8_32& chip8_32, uint32_t highlight_addr) {
    StackVisualizer visualizer;
    visualizer.drawStackFrame(chip8_32, "CURRENT STATE", highlight_addr);
}

// ===============================================
// StackVisualizer 완전한 구현 (4단계 완성)
// ===============================================

void StackVisualizer::drawStackFrame(const Chip8_32& chip8_32, const std::string& phase, uint32_t /* highlight_addr */) {
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    if (!phase.empty()) {
        std::string title = "STACK FRAME [" + phase + "]";
        int total_width = 65;
        int padding = (total_width - title.length()) / 2;
        std::cout << "\n" << std::string(padding, ' ') << title << std::endl;
        std::cout << std::string(total_width, '=') << std::endl;
    }
    
    // 실시간 스택 메모리 분석
    analyzeRealTimeStack(chip8_32);
    
    std::cout << "\n┌─────────────────────────────────────────────────────────────────┐ ← 0x" << std::hex << std::uppercase << StackFrame::STACK_START << " (STACK_START)" << std::endl;
    
    // Return Address (RBP + 4) 표시 - Start Function에서 main 호출 시
    if (rbp + 4 <= StackFrame::STACK_START) {
        uint32_t ret_addr = (chip8_32.get_memory(rbp + 4) << 24) |
                           (chip8_32.get_memory(rbp + 5) << 16) |
                           (chip8_32.get_memory(rbp + 6) << 8) |
                           chip8_32.get_memory(rbp + 7);
        if (ret_addr != 0) {
            std::cout << "├─────────────────────────────────────────────────────────────────┤" << std::endl;
            std::ostringstream ret_stream;
            ret_stream << "(RET) Return Address = 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (ret_addr & 0xFFFF);
            std::string ret_content = ret_stream.str();
            // 박스 전체 길이 67자 - "│ 0x1234: " (9자) - "│" (1자) = 57자 -> 56자로 조정
            if (ret_content.length() > 56) ret_content = ret_content.substr(0, 56);
            ret_content.resize(56, ' ');
            std::cout << "│ 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (rbp + 4) 
                      << ": " << ret_content << "│" << std::endl;
        }
    }
    
    // RBP 표시
    uint32_t saved_rbp = (chip8_32.get_memory(rbp) << 24) |
                        (chip8_32.get_memory(rbp + 1) << 16) |
                        (chip8_32.get_memory(rbp + 2) << 8) |
                        chip8_32.get_memory(rbp + 3);
    std::cout << "├─────────────────────────────────────────────────────────────────┤ ← RBP" << std::endl;
    std::ostringstream rbp_stream;
    rbp_stream << "(REG) Saved RBP = 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (saved_rbp & 0xFFFF);
    std::string rbp_content = rbp_stream.str();
    // 박스 전체 길이 67자 - "│ 0x1234: " (9자) - "│" (1자) = 57자 -> 56자로 조정
    if (rbp_content.length() > 56) rbp_content = rbp_content.substr(0, 56);
    rbp_content.resize(56, ' ');
    std::cout << "│ 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << rbp 
              << ": " << rbp_content << "│" << std::endl;
    
    // 지역변수들 표시
    for (int i = 1; i <= 3; i++) {
        uint32_t addr = rbp - (i * 4);
        if (addr >= rsp && addr >= StackFrame::STACK_END) {
            uint32_t value = (chip8_32.get_memory(addr) << 24) |
                           (chip8_32.get_memory(addr + 1) << 16) |
                           (chip8_32.get_memory(addr + 2) << 8) |
                           chip8_32.get_memory(addr + 3);
            
            std::cout << "├─────────────────────────────────────────────────────────────────┤" << std::endl;
            std::cout << "│ 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr << ": ";
            
            std::string content;
            if (i == 1 && value != 0 && value < 100000) {
                content = "(VAR) int a = " + std::to_string(value);
            } else if (i == 2 && value != 0 && value < 100000) {
                content = "(VAR) int b = " + std::to_string(value);
            } else if (i == 3 && value != 0 && value < 100000) {
                content = "(VAR) int c = " + std::to_string(value);
            } else {
                content = "(VAR) [empty]";
            }
            
            // 박스 전체 길이 67자 - "│ 0x1234: " (9자) - "│" (1자) = 57자 -> 56자로 조정
            if (content.length() > 56) content = content.substr(0, 56);
            content.resize(56, ' ');
            std::cout << content << "│";
            
            // RSP 표시
            if (addr == rsp) {
                std::cout << " ← RSP" << std::endl;
            } else {
                std::cout << std::endl;
            }
        }
    }
    
    std::cout << "└─────────────────────────────────────────────────────────────────┘ ← 0x" << std::hex << StackFrame::STACK_END << " (STACK_END)" << std::endl;
    
    // 고정된 STACK FRAME STATUS
    drawFixedStackStatus(chip8_32, rbp, rsp);
}

// 스택 프레임 애니메이션 구현
void StackVisualizer::animateStackFrame(const Chip8_32& chip8_32, const std::string& phase, 
                                       const std::string& instruction, bool wait_for_input) {
    clearScreen();
    
    std::cout << "\n";
    std::cout << "=============================================\n";
    std::cout << "         STACK FRAME ANIMATION DEBUGGER         \n";
    std::cout << "=============================================\n";
    
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
    std::cout << "=============================================\n";
    std::cout << "       INTERACTIVE STACK FRAME DEBUGGER       \n";
    std::cout << "=============================================\n\n";
    
    // 사용자 입력받기
    auto [num1, num2] = getUserInput();
    
    std::cout << "\nTARGET: Demonstrate add(" << num1 << ", " << num2 << ") = " << (num1 + num2) << " with x86-64 Stack Frame\n\n";
    
    animateStackFrame(chip8_32, "INITIAL STATE", "Starting with empty stack");
    
    // x86-64 스타일 덧셈 함수 시뮬레이션
    simulateX86AddFunction(chip8_32, num1, num2);
    
    std::cout << "\nStack Frame Animation Complete!\n";
    std::cout << "Commands: [r]eplay, [s]tack dump, [q]uit: ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "r" || input == "replay") {
        interactiveStackDebug(chip8_32);
    } else if (input == "s" || input == "stack") {
        drawStackFrame(chip8_32, "FINAL STATE");
        }
}


// 애니메이션 헬퍼 함수들
void StackVisualizer::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void StackVisualizer::waitForUser(const std::string& message) {
    std::string msg = message.empty() ? "Press ENTER to continue..." : message;
    std::cout << "\n" << msg;
    std::string dummy;
    std::getline(std::cin, dummy);
}

void StackVisualizer::showInstructionInfo(const std::string& instruction, const std::string& description) {
    std::cout << "\nCURRENT INSTRUCTION:\n";
    std::cout << "   " << instruction << "\n";
    std::cout << "DESCRIPTION:\n";
    std::cout << "   " << description << "\n\n";
}

// **실시간 스택 메모리 분석 (메인 스택 프레임 전용)**
void StackVisualizer::analyzeRealTimeStack(const Chip8_32& chip8_32) {
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    clearCells();
    
    // 메인 스택 프레임 구조 분석
    // [RBP]     - 메인 함수의 base pointer 
    // [RBP-4]   - int a (첫 번째 지역변수)
    // [RBP-8]   - int b (두 번째 지역변수)
    // [RBP-12]  - int result (세 번째 지역변수)
    // [RSP]     - 현재 스택 포인터
    
    // 메인 함수의 RBP는 초기값 그대로
    if (rbp <= StackFrame::STACK_START && rbp >= StackFrame::STACK_END) {
        uint32_t rbp_value = (chip8_32.get_memory(rbp) << 24) |
                           (chip8_32.get_memory(rbp + 1) << 16) |
                           (chip8_32.get_memory(rbp + 2) << 8) |
                           chip8_32.get_memory(rbp + 3);
        addStackCell(rbp, rbp_value, StackCellType::OLD_RBP, "Main RBP");
    }
    
    // 지역변수 a 감지 (RBP - 4)
    if (rbp - 4 >= StackFrame::STACK_END && rbp - 4 + 3 < StackFrame::STACK_START) {
        uint32_t var_a = (chip8_32.get_memory(rbp - 4) << 24) |
                        (chip8_32.get_memory(rbp - 3) << 16) |
                        (chip8_32.get_memory(rbp - 2) << 8) |
                        chip8_32.get_memory(rbp - 1);
        if (var_a != 0) {
            addStackCell(rbp - 4, var_a, StackCellType::LOCAL_VAR, "int a = " + std::to_string(var_a));
        }
    }
    
    // 지역변수 b 감지 (RBP - 8)
    if (rbp - 8 >= StackFrame::STACK_END && rbp - 8 + 3 < StackFrame::STACK_START) {
        uint32_t var_b = (chip8_32.get_memory(rbp - 8) << 24) |
                        (chip8_32.get_memory(rbp - 7) << 16) |
                        (chip8_32.get_memory(rbp - 6) << 8) |
                        chip8_32.get_memory(rbp - 5);
        if (var_b != 0) {
            addStackCell(rbp - 8, var_b, StackCellType::LOCAL_VAR, "int b = " + std::to_string(var_b));
        }
    }
    
    // 지역변수 result 감지 (RBP - 12)
    if (rbp - 12 >= StackFrame::STACK_END && rbp - 12 + 3 < StackFrame::STACK_START) {
        uint32_t var_result = (chip8_32.get_memory(rbp - 12) << 24) |
                             (chip8_32.get_memory(rbp - 11) << 16) |
                             (chip8_32.get_memory(rbp - 10) << 8) |
                             chip8_32.get_memory(rbp - 9);
        if (var_result != 0) {
            addStackCell(rbp - 12, var_result, StackCellType::RESULT, "int result = " + std::to_string(var_result));
        }
    }
    
    // 현재 스택 톱 마킹
    if (rsp != rbp && rsp >= StackFrame::STACK_END && rsp < StackFrame::STACK_START) {
        addStackCell(rsp, 0, StackCellType::HIGHLIGHT, "← RSP (Stack Top)");
    }
}

std::string StackVisualizer::formatStackCell(uint32_t addr, uint32_t value, uint32_t /* rbp */, uint32_t /* rsp */, uint32_t highlight_addr) {
    bool is_highlighted = (addr == highlight_addr);
    StackCell* cell = findCell(addr);
    
    std::ostringstream oss;
    
    if (is_highlighted) {
        oss << "[*] ";
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

void StackVisualizer::drawFixedStackStatus(const Chip8_32& chip8_32, uint32_t rbp, uint32_t rsp) {
    uint32_t rip = chip8_32.get_R(StackFrame::RIP_INDEX);
    uint32_t used_bytes = StackFrame::STACK_START - rsp;
    uint32_t total_bytes = StackFrame::STACK_START - StackFrame::STACK_END;
    double usage_percent = (double)used_bytes / total_bytes * 100.0;
    
    std::cout << "\nSTACK FRAME STATUS:" << std::endl;
    std::cout << "   RBP (Base)  = [0x" << std::hex << std::setw(8) << std::setfill('0') << rbp << "]" << std::endl;
    std::cout << "   RSP (Top)   = [0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << "]" << std::endl;
    std::cout << "   RIP (PC)    = [0x" << std::hex << std::setw(8) << std::setfill('0') << rip << "]" << std::endl;
    std::cout << "   Used Stack  = [" << std::dec << used_bytes << " / " << total_bytes << " bytes]" << std::endl;
    std::cout << "   Status      = [" << std::fixed << std::setprecision(1) << usage_percent << "% used]" << std::endl;
}

void StackVisualizer::drawAdvancedPointers(const Chip8_32& /* chip8_32 */, uint32_t /* rbp */, uint32_t /* rsp */) {
    // 이 함수는 더 이상 사용하지 않음 - drawFixedStackStatus로 대체
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
    std::cout << "\nPointer Status:" << std::endl;
    std::cout << "   RBP (Base) = 0x" << std::hex << std::setw(8) << std::setfill('0') << rbp << std::endl;
    std::cout << "   RSP (Top)  = 0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << std::endl;
    std::cout << "   Used: " << std::dec << (0xEFFF - rsp) << " bytes" << std::endl;
}

std::string StackVisualizer::getTypeEmoji(StackCellType type) {
    switch (type) {
        case StackCellType::EMPTY:     return "  ";
        case StackCellType::OLD_RBP:   return "[R]";
        case StackCellType::PARAMETER: return "[P]";
        case StackCellType::LOCAL_VAR: return "[L]";
        case StackCellType::RESULT:    return "[T]";
        case StackCellType::HIGHLIGHT: return "[*]";
        default:                       return "[?]";
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

// **새로운 함수들 구현**

std::pair<uint32_t, uint32_t> StackVisualizer::getUserInput() {
    uint32_t num1, num2;
    
    std::cout << "Enter two 3-digit numbers for addition:\n";
    
    while (true) {
        std::cout << "First number (100-999): ";
        if (std::cin >> num1 && num1 >= 100 && num1 <= 999) {
            break;
        }
        std::cout << "Invalid input! Please enter a 3-digit number (100-999).\n";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    
    while (true) {
        std::cout << "Second number (100-999): ";
        if (std::cin >> num2 && num2 >= 100 && num2 <= 999) {
            break;
        }
        std::cout << "Invalid input! Please enter a 3-digit number (100-999).\n";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    
    // 입력 버퍼 정리
    std::cin.ignore(10000, '\n');
    
    std::cout << "\nNumbers entered: " << num1 << " and " << num2 << "\n";
    waitForUser("Press ENTER to start simulation...");
    
    return {num1, num2};
}

void StackVisualizer::simulateX86AddFunction(Chip8_32& chip8_32, uint32_t num1, uint32_t num2) {
    uint32_t original_rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t original_rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    
    std::cout << "\nSimulating: int main() { int a=" << num1 << "; int b=" << num2 << "; int c=a+b; return 0; }\n";
    
    // 간소화된 스택 프레임 시뮬레이션
    uint32_t rbp = original_rsp - 8;
    chip8_32.set_R(StackFrame::RBP_INDEX, rbp);
    chip8_32.set_R(StackFrame::RSP_INDEX, rbp - 16);
    
    // 변수 저장
    uint32_t var_a_addr = rbp - 4;
    uint32_t var_b_addr = rbp - 8;
    uint32_t var_c_addr = rbp - 12;
    uint32_t result = num1 + num2;
    
    // 메모리에 변수 저장
    chip8_32.set_memory(var_a_addr, (num1 >> 24) & 0xFF);
    chip8_32.set_memory(var_a_addr + 1, (num1 >> 16) & 0xFF);
    chip8_32.set_memory(var_a_addr + 2, (num1 >> 8) & 0xFF);
    chip8_32.set_memory(var_a_addr + 3, num1 & 0xFF);
    
    chip8_32.set_memory(var_b_addr, (num2 >> 24) & 0xFF);
    chip8_32.set_memory(var_b_addr + 1, (num2 >> 16) & 0xFF);
    chip8_32.set_memory(var_b_addr + 2, (num2 >> 8) & 0xFF);
    chip8_32.set_memory(var_b_addr + 3, num2 & 0xFF);
    
    chip8_32.set_memory(var_c_addr, (result >> 24) & 0xFF);
    chip8_32.set_memory(var_c_addr + 1, (result >> 16) & 0xFF);
    chip8_32.set_memory(var_c_addr + 2, (result >> 8) & 0xFF);
    chip8_32.set_memory(var_c_addr + 3, result & 0xFF);
    
    animateStackFrame(chip8_32, "FINAL RESULT", "Stack frame with a=" + std::to_string(num1) + ", b=" + std::to_string(num2) + ", c=" + std::to_string(result));
    
    std::cout << "\nStack simulation complete: " << num1 << " + " << num2 << " = " << result << std::endl;
}

void StackVisualizer::resetVisualizerState() {
    clearCells();
    std::cout << "[INFO] Stack visualizer state reset" << std::endl;
}

} // namespace chip8emu