#include "debugger.hpp"
#include "chip8.hpp"
#include "chip8_32.hpp"
#include "stack_frame.hpp"  // 🔧 추가: StackFrame 네임스페이스 사용을 위해
#include <iostream>
#include <sstream>
#include <iomanip>
#include <atomic>

namespace {
    // 디버거가 커서 이동·화면 덮어쓰기를 이용하는 동적 UI를 사용할지 여부.
    // false 로 두면 매 명령마다 한 줄씩 로그를 쌓는 연속 출력 모드가 된다.
    constexpr bool USE_DYNAMIC_DEBUG_UI = false;
}

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
        // 디버거 시작 시 상태 초기화
        step_mode_ = false;
        clearBreakpoints();
        first_debug_print_ = true;  // UI 다시 출력하도록 리셋
        std::cout << "[INFO] 8-bit debugger initialized" << std::endl;
    }
    enabled_ = on;
}

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

    // 간단 로그 모드: 화면 리프레시 대신 한 줄씩 누적 출력
    if (!USE_DYNAMIC_DEBUG_UI) {
        uint16_t pc = chip8_.get_pc();

        std::ostringstream oss;
        uint16_t op16 = static_cast<uint16_t>(opcode & 0xFFFF);
        oss << "[8] PC=" << toHex16(pc)
            << " OPC=" << toHex16(op16) << ' ' << std::left << std::setw(6) << disassemble(opcode)
            << " I=" << toHex16(chip8_.get_I())
            << " V0=" << toHex8(chip8_.get_V(0))
            << " V1=" << toHex8(chip8_.get_V(1))
            << " V2=" << toHex8(chip8_.get_V(2))
            << " V3=" << toHex8(chip8_.get_V(3))
            << " VF=" << toHex8(chip8_.get_V(15));
        std::cout << oss.str() << std::endl;

        // 8비트 시스템에서도 동일 메모리 범위(0x0620~0x062F)를 출력
        {
            std::ostringstream memLine;
            memLine << "[MEM][EFF0-EFFF] ";
            for (int w = 0; w < 4; ++w) {
                uint16_t base = 0x0620 + w * 4;
                uint32_t val = (chip8_.get_memory(base) << 24) |
                               (chip8_.get_memory(base + 1) << 16) |
                               (chip8_.get_memory(base + 2) << 8) |
                               chip8_.get_memory(base + 3);
                memLine << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << val;
                if (w != 3) memLine << ' ';
            }
            std::cout << memLine.str() << std::endl;
        }

        // 스텝 모드라면 입력을 계속 지원한다.
        if (step_mode_) {
            handleDebugInput();
        }
        return; // 동적 UI 코드 건너뜀
    }

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
        std::cout << "PC = [    ]  Opcode = [        ]  -> \n";
        
        // V0~V7
        std::cout << "V0-V7:\n";
        for (int i = 0; i < 8; ++i) {
            std::cout << "V" << std::hex << i << " = [  ]  ";
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        
        // V8~VF
        std::cout << "V8-VF:\n";
        for (int i = 8; i < 16; ++i) {
            std::cout << "V" << std::hex << i << " = [  ]  ";
            if ((i + 1) % 4 == 0 || i == 15) std::cout << "\n";
        }
        
        // Special Registers
        std::cout << "Special Registers:\n";
        std::cout << "I = [    ]  SP = [        ]  Delay = [        ]  Sound = [        ]\n";
        std::cout << "------------------------------------------------------------\n";
        
        first_debug_print_ = false;
    }

    // 값들만 업데이트 (커서 위치 이동으로)
    // PC 값 업데이트 (3번째 줄, 6번째 위치)
    std::cout << "\033[s";  // 현재 위치 저장
    std::cout << "\033[3;6H" << toHex16(pc);  // PC 위치로 이동하여 값 출력
    
    // Opcode 값 업데이트 (3번째 줄, 19번째 위치)
    std::cout << "\033[3;19H" << toHex32(opcode);
    
    // 명령어 이름 업데이트 (3번째 줄, 38번째 위치)
    std::cout << "\033[3;38H" << std::left << std::setw(10) << disassemble(opcode);
    
    // V0~V7 값들 업데이트
    for (int i = 0; i < 8; ++i) {
        int line = 5 + (i / 4);  // 5번째 줄부터 시작
        int col = 6 + (i % 4) * 9;  // 각 레지스터당 9칸 간격
        std::cout << "\033[" << line << ";" << col << "H" << toHex8(chip8_.get_V(i));
    }
    
    // V8~VF 값들 업데이트
    for (int i = 8; i < 16; ++i) {
        int line = 7 + ((i - 8) / 4);  // 7번째 줄부터 시작
        int col = 6 + ((i - 8) % 4) * 9;
        std::cout << "\033[" << line << ";" << col << "H" << toHex8(chip8_.get_V(i));
    }
    
    // Special Registers 업데이트 (9번째 줄)
    std::cout << "\033[9;5H" << toHex16(chip8_.get_I());  // I
    std::cout << "\033[9;17H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_sp());  // SP
    std::cout << "\033[9;33H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_delay_timer());  // Delay
    std::cout << "\033[9;49H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_sound_timer());  // Sound
    
    // 저장된 위치로 복원 후 줄바꿈 및 fill 문자 초기화로 다른 출력에 영향이 없도록 한다.
    std::cout << "\033[u" << std::setfill(' ') << '\n';
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
        // 디버거 시작 시 상태 초기화
        step_mode_ = false;
        clearBreakpoints();
        first_debug_print_ = true;  // UI 다시 출력하도록 리셋
        std::cout << "[INFO] 32-bit debugger initialized" << std::endl;
    }
    enabled_ = on;
}

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

    // 간단 로그 모드: 화면 리프레시 대신 한 줄씩 누적 출력
    if (!USE_DYNAMIC_DEBUG_UI) {
        uint32_t pc = chip8_.get_pc();

        static uint32_t last_logged_pc32 = 0xFFFFFFFF;
        if (pc == last_logged_pc32) {
            // 같은 PC에서 반복 호출되면 중복 로그 생략
            if (step_mode_) {
                handleDebugInput();
            }
            return;
        }
        last_logged_pc32 = pc;

        // 구분선
        std::cout << "---------------------------------------------------------------------" << std::endl;

        // 헤더 라인 : PC / OP / I
        std::ostringstream hdr;
        hdr << "[32] PC=" << toHex16(static_cast<uint16_t>(pc))
            << " OPC=" << toHex32(opcode) << ' ' << disassemble(opcode)
            << " I=" << toHex32(chip8_.get_I());
        std::cout << hdr.str() << std::endl;

        // 레지스터 8개씩 4줄 출력
        for (int row = 0; row < 4; ++row) {
            std::ostringstream line;
            for (int col = 0; col < 8; ++col) {
                int idx = row * 8 + col;
                line << "R" << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << idx
                     << "=" << toHex32(chip8_.get_R(idx));
                if (col != 7) line << " ";
            }
            std::cout << line.str() << std::endl;
        }

        // 메모리 0x0620~0x062F 16바이트를 4바이트씩 묶어 출력
        std::ostringstream memLine;
        memLine << "[MEM][0620-062F] ";
        for (int word = 0; word < 4; ++word) {
            uint32_t base = 0x0620 + word * 4;
            uint32_t value = (chip8_.get_memory(base) << 24) |
                            (chip8_.get_memory(base + 1) << 16) |
                            (chip8_.get_memory(base + 2) << 8) |
                            chip8_.get_memory(base + 3);
            memLine << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
            if (word != 3) memLine << ' ';
        }
        std::cout << memLine.str() << std::endl;

        std::cout << std::dec;  // 형식 초기화

        if (step_mode_) {
            handleDebugInput();
        }
        return; // 동적 UI 코드 건너뜀
    }

    // 브레이크포인트 체크
    uint32_t pc = chip8_.get_pc();
    if (hasBreakpoint(static_cast<uint16_t>(pc))) {
        std::cout << "\nBREAKPOINT HIT at " << toHex32(pc) << "\n";
        step_mode_ = true;
    }

    if (first_debug_print_) {
        // 처음에만 전체 UI 출력
        std::cout << "\n============================================================\n";
        std::cout << "32-bit CHIP-8 Debug State\n";
        std::cout << "============================================================\n";
        
        // PC, Opcode 줄
        std::cout << "PC = [        ]  Opcode = [        ]  -> \n";
        
        // R0~R15
        std::cout << "R0-R15:\n";
        for (int i = 0; i < 16; ++i) {
            std::cout << "R" << std::dec << std::setw(2) << std::setfill('0') << i << " = [        ]  ";
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        
        // R16~R31
        std::cout << "\nR16-R31:\n";
        for (int i = 16; i < 32; ++i) {
            if (i == StackFrame::RBP_INDEX) {
                std::cout << "RBP = [        ]  ";
            } else if (i == StackFrame::RSP_INDEX) {
                std::cout << "RSP = [        ]  ";
            } else if (i == StackFrame::RIP_INDEX) {
                std::cout << "RIP = [        ]  ";
            } else {
                std::cout << "R" << std::dec << std::setw(2) << std::setfill('0') << i << " = [        ]  ";
            }
            if ((i + 1) % 4 == 0) std::cout << "\n";
        }
        
        // Special Registers
        std::cout << "\nSpecial Registers:\n";
        std::cout << "I = [        ]  SP = [        ]  Delay = [        ]  Sound = [        ]\n";
        std::cout << "------------------------------------------------------------\n";
        
        first_debug_print_ = false;
    }

    // 값들만 업데이트 (커서 위치 이동으로)
    // PC 값 업데이트 (3번째 줄, 7번째 위치)
    std::cout << "\033[s";  // 현재 위치 저장
    std::cout << "\033[3;7H" << toHex32(pc);  // PC 위치로 이동하여 값 출력
    
    // Opcode 값 업데이트 (3번째 줄, 23번째 위치)
    std::cout << "\033[3;23H" << toHex32(opcode);
    
    // 명령어 이름 업데이트 (3번째 줄, 42번째 위치)
    std::cout << "\033[3;42H" << std::left << std::setw(10) << disassemble(opcode);
    
    // R0~R15 값들 업데이트
    for (int i = 0; i < 16; ++i) {
        int line = 5 + (i / 4);  // 5번째 줄부터 시작
        int col = 7 + (i % 4) * 17;  // 각 레지스터당 17칸 간격
        std::cout << "\033[" << line << ";" << col << "H" << toHex32(chip8_.get_R(i));
    }
    
    // R16~R31 값들 업데이트  
    for (int i = 16; i < 32; ++i) {
        int line = 10 + ((i - 16) / 4);  // 10번째 줄부터 시작
        int col = 7 + ((i - 16) % 4) * 17;
        std::cout << "\033[" << line << ";" << col << "H" << toHex32(chip8_.get_R(i));
    }
    
    // Special Registers 업데이트 (14번째 줄)
    std::cout << "\033[14;5H" << toHex32(chip8_.get_I());  // I
    std::cout << "\033[14;21H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_sp());  // SP
    std::cout << "\033[14;37H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_delay_timer());  // Delay
    std::cout << "\033[14;53H" << std::setw(8) << std::setfill('0') << static_cast<int>(chip8_.get_sound_timer());  // Sound
    
    // 저장된 위치로 복원 후 줄바꿈 및 fill 문자 초기화로 다른 출력에 영향이 없도록 한다.
    std::cout << "\033[u" << std::setfill(' ') << '\n';
    std::cout << std::flush;

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        std::cout << "\n";
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

// 🔧 추가: 누락된 함수 구현
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

// 🎬 **4.3 스택 프레임 애니메이션 구현**
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


// 🎬 애니메이션 헬퍼 함수들
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
    
    clearScreen();
    std::cout << "\nCOMPLETE STACK FRAME SIMULATION (Start Function -> main)\n";
    std::cout << "=========================================================\n\n";
    std::cout << "Simulating: int main() { int a=" << num1 << "; int b=" << num2 << "; int c=a+b; return 0; }\n\n";
    
    // ==================== STEP 1: START FUNCTION CALLS main() ====================
    std::cout << "STEP 1: Start Function calls main()\n";
    
    // Return address 저장 (Start Function에서 main 호출 후 돌아갈 주소)
    uint32_t return_address = 0x1000;  // Start Function의 다음 명령어 주소
    chip8_32.set_R(StackFrame::RSP_INDEX, original_rsp - 4);
    uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    chip8_32.set_memory(rsp, (return_address >> 24) & 0xFF);
    chip8_32.set_memory(rsp + 1, (return_address >> 16) & 0xFF);
    chip8_32.set_memory(rsp + 2, (return_address >> 8) & 0xFF);
    chip8_32.set_memory(rsp + 3, return_address & 0xFF);
    
    animateStackFrame(chip8_32, "STEP 1: CALL main()", "CALL main ; push return address");
    
    // ==================== STEP 2: main() FUNCTION PROLOGUE ====================
    std::cout << "STEP 2: main() Function Prologue\n";
    
    // 이전 RBP 저장
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp - 4);
    rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    chip8_32.set_memory(rsp, (original_rbp >> 24) & 0xFF);
    chip8_32.set_memory(rsp + 1, (original_rbp >> 16) & 0xFF);
    chip8_32.set_memory(rsp + 2, (original_rbp >> 8) & 0xFF);
    chip8_32.set_memory(rsp + 3, original_rbp & 0xFF);
    
    animateStackFrame(chip8_32, "STEP 2: PROLOGUE", "PUSH RBP ; save old frame pointer");
    
    // RBP = RSP (새로운 스택 프레임 설정)
    chip8_32.set_R(StackFrame::RBP_INDEX, rsp);
    uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
    
    animateStackFrame(chip8_32, "STEP 2: PROLOGUE", "MOV RBP, RSP ; establish new frame");
    
    // 지역변수를 위한 공간 할당 (16바이트: a, b, c + 정렬)
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp - 16);
    
    animateStackFrame(chip8_32, "STEP 2: PROLOGUE", "SUB RSP, 16 ; allocate space for local variables");
    
    // ==================== STEP 3: 변수 a 선언 및 초기화 ====================
    std::cout << "STEP 3: Declare and initialize variable a\n";
    
    uint32_t var_a_addr = rbp - 4;
    chip8_32.set_memory(var_a_addr, (num1 >> 24) & 0xFF);
    chip8_32.set_memory(var_a_addr + 1, (num1 >> 16) & 0xFF);
    chip8_32.set_memory(var_a_addr + 2, (num1 >> 8) & 0xFF);
    chip8_32.set_memory(var_a_addr + 3, num1 & 0xFF);
    
    animateStackFrame(chip8_32, "STEP 3: DECLARE a", "MOV [RBP-4], " + std::to_string(num1) + " ; int a = " + std::to_string(num1));
    
    // ==================== STEP 4: 변수 b 선언 및 초기화 ====================
    std::cout << "STEP 4: Declare and initialize variable b\n";
    
    uint32_t var_b_addr = rbp - 8;
    chip8_32.set_memory(var_b_addr, (num2 >> 24) & 0xFF);
    chip8_32.set_memory(var_b_addr + 1, (num2 >> 16) & 0xFF);
    chip8_32.set_memory(var_b_addr + 2, (num2 >> 8) & 0xFF);
    chip8_32.set_memory(var_b_addr + 3, num2 & 0xFF);
    
    animateStackFrame(chip8_32, "STEP 4: DECLARE b", "MOV [RBP-8], " + std::to_string(num2) + " ; int b = " + std::to_string(num2));
    
    // ==================== STEP 5: 덧셈 연산 수행 ====================
    std::cout << "STEP 5: Perform addition operation\n";
    
    animateStackFrame(chip8_32, "STEP 5: CALCULATION", "MOV RAX, [RBP-4] ; load a=" + std::to_string(num1) + " into RAX");
    
    animateStackFrame(chip8_32, "STEP 5: CALCULATION", "MOV RDX, [RBP-8] ; load b=" + std::to_string(num2) + " into RDX");
    
    animateStackFrame(chip8_32, "STEP 5: CALCULATION", "ADD RAX, RDX ; RAX = " + std::to_string(num1) + " + " + std::to_string(num2) + " = " + std::to_string(num1 + num2));
    
    // ==================== STEP 6: 결과를 변수 c에 저장 ====================
    std::cout << "STEP 6: Store result in variable c\n";
    
    uint32_t result = num1 + num2;
    uint32_t var_c_addr = rbp - 12;
    chip8_32.set_memory(var_c_addr, (result >> 24) & 0xFF);
    chip8_32.set_memory(var_c_addr + 1, (result >> 16) & 0xFF);
    chip8_32.set_memory(var_c_addr + 2, (result >> 8) & 0xFF);
    chip8_32.set_memory(var_c_addr + 3, result & 0xFF);
    
    animateStackFrame(chip8_32, "STEP 6: STORE RESULT", "MOV [RBP-12], RAX ; int c = " + std::to_string(result));
    
    // ==================== STEP 7: main() 함수 종료 준비 ====================
    std::cout << "STEP 7: main() function cleanup and return\n";
    
    // RAX에 반환값 0 설정
    animateStackFrame(chip8_32, "STEP 7: RETURN PREP", "MOV RAX, 0 ; set return value");
    
    // RSP = RBP (스택 포인터 복원)
    chip8_32.set_R(StackFrame::RSP_INDEX, rbp);
    
    animateStackFrame(chip8_32, "STEP 7: EPILOGUE", "MOV RSP, RBP ; restore stack pointer");
    
    // RBP 복원
    rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t restored_rbp = (chip8_32.get_memory(rsp) << 24) |
                           (chip8_32.get_memory(rsp + 1) << 16) |
                           (chip8_32.get_memory(rsp + 2) << 8) |
                           chip8_32.get_memory(rsp + 3);
    chip8_32.set_R(StackFrame::RBP_INDEX, restored_rbp);
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp + 4);
    
    animateStackFrame(chip8_32, "STEP 7: EPILOGUE", "POP RBP ; restore old frame pointer");
    
    // ==================== STEP 8: Start Function으로 복귀 ====================
    std::cout << "STEP 8: Return to Start Function\n";
    
    // Return address로 복귀
    rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
    chip8_32.set_R(StackFrame::RSP_INDEX, rsp + 4);  // return address pop
    
    animateStackFrame(chip8_32, "STEP 8: RETURN", "RET ; return to Start Function");
    
    // 최종 결과 표시
    clearScreen();
    std::cout << "\n=========================================================\n";
    std::cout << "         COMPLETE STACK FRAME SIMULATION DONE!         \n";
    std::cout << "=========================================================\n\n";
    std::cout << "MEMORY LAYOUT:\n";
    std::cout << "   RBP + 4: Return Address (Start Function)\n";
    std::cout << "   RBP:     Previous RBP\n";
    std::cout << "   RBP - 4: int a = " << num1 << "\n";
    std::cout << "   RBP - 8: int b = " << num2 << "\n";
    std::cout << "   RBP - 12: int c = " << result << " (a + b)\n\n";
    std::cout << "COMPLETE FUNCTION CALL CYCLE: Start -> main() -> return\n";
    std::cout << "Proper x86-64 calling convention followed\n";
    std::cout << "Stack frame correctly established and destroyed\n\n";
    
}

void StackVisualizer::resetVisualizerState() {
    clearCells();
    std::cout << "[INFO] Stack visualizer state reset" << std::endl;
}

} // namespace chip8emu