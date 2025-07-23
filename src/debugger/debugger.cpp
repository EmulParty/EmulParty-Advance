#include "debugger/debugger.hpp"
#include "core/chip8.hpp"
#include "core/chip8_32.hpp"
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

// 기존 코드 마지막 부분에 추가 (} // namespace chip8emu 바로 위에)

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
// 32비트 디버거 구현
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

    // 3) R16~R31
    std::cout << "\n📊 R16-R31:";
    for (int i = 16; i < 32; ++i) {
        std::cout << "R" << std::hex << i << "=" << toHex32(chip8_.get_R(i)) << "  ";
        if ((i + 1) % 8 == 0) std::cout << "\n          ";
    }
    std::cout << "\n";

    // 4) 특수 레지스터
    std::cout << "🎯 I=" << toHex32(chip8_.get_I())
              << "  SP=" << std::dec << static_cast<int>(chip8_.get_sp())
              << "  Delay=" << static_cast<int>(chip8_.get_delay_timer())
              << "  Sound=" << static_cast<int>(chip8_.get_sound_timer()) << "\n\n";

    // 5) 스택 (32비트용)
    std::cout << "📚 Stack: ";
    for (int i = 0; i < 32; ++i) {
        if (i == chip8_.get_sp()) {
            std::cout << "[" << toHex32(chip8_.get_stack(i)) << "]← ";
        } else if (i < chip8_.get_sp()) {
            std::cout << toHex32(chip8_.get_stack(i)) << " ";
        } else {
            std::cout << "........ ";
        }
        if ((i + 1) % 4 == 0) std::cout << "\n       ";
    }
    std::cout << "\n";

    std::cout << std::string(60, '-') << "\n";

    // 스텝 모드에서 사용자 입력 대기
    if (step_mode_) {
        handleDebugInput();
    }
}

std::string Debugger32::disassemble(uint32_t opcode) {
    uint8_t first = (opcode >> 24) & 0xFF;
    static const char* kNames[16] = {
        "SYS", "JP", "CALL", "SE", "SNE", "SE", "LD", "ADD",
        "ALU", "SNE", "LDI", "JP V0", "RND", "DRW", "KEY", "MISC"
    };
    
    std::ostringstream oss;
    oss << (first < 0x10 ? kNames[first] : "UNK") << "_32";
    return oss.str();
}

void Debugger32::handleDebugInput() {
    if (!step_mode_) return; // step 모드가 아니면 바로 리턴
    
    std::cout << "\n🐛 [DEBUG] PC=0x" << std::hex << chip8_.get_pc() << std::dec << std::endl;
    std::cout << "Enter command (s=step, c=continue, q=quit, h=help): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    // 입력이 비어있으면 기본적으로 step
    if (input.empty()) {
        input = "s";
    }
    
    if (input == "s" || input == "step") {
        // step_mode_는 그대로 유지 (다음에도 멈춤)
        std::cout << "➤ Stepping to next instruction..." << std::endl;
    }
    else if (input == "c" || input == "continue") {
        step_mode_ = false;
        std::cout << "➤ Continuing execution..." << std::endl;
    }
    else if (input == "q" || input == "quit") {
        enabled_ = false;  // 디버거 완전 종료
        std::cout << "➤ Exiting debugger..." << std::endl;
    }
    else if (input.substr(0, 2) == "bp") {
        // 브레이크포인트 설정
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
        std::cout << "  s, step       - Execute next instruction" << std::endl;
        std::cout << "  c, continue   - Continue execution" << std::endl;
        std::cout << "  q, quit       - Exit debugger" << std::endl;
        std::cout << "  bp <addr>     - Set breakpoint (hex)" << std::endl;
        std::cout << "  h, help       - Show this help" << std::endl;
        // help 후에는 다시 입력 받기
        handleDebugInput();
    }
    else {
        std::cout << "❌ Unknown command '" << input << "'. Type 'h' for help." << std::endl;
        // 잘못된 입력 후에는 다시 입력 받기
        handleDebugInput();
    }
}

} // namespace chip8emu