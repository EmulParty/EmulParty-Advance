#pragma once
#include <cstdint>
#include <string>
#include <set>

// 전방 선언 (네임스페이스 없이)
class Chip8;
class Chip8_32;

namespace chip8emu {

// 8비트용 디버거
class Debugger8 {
public:
    explicit Debugger8(Chip8& chip8)
        : chip8_(chip8), enabled_(false), step_mode_(false) {}

    void enable(bool on = true) { enabled_ = on; }
    bool isEnabled() const { return enabled_; }
    void setStepMode(bool on = true) { step_mode_ = on; }
    bool isStepMode() const { return step_mode_; }

    void addBreakpoint(uint16_t address) { breakpoints_.insert(address); }
    void removeBreakpoint(uint16_t address) { breakpoints_.erase(address); }
    bool hasBreakpoint(uint16_t address) const { return breakpoints_.count(address) > 0; }
    void clearBreakpoints() { breakpoints_.clear(); }

    void printState(uint32_t opcode);
    std::string disassemble(uint32_t opcode);
    void handleDebugInput();

private:
    Chip8& chip8_;
    bool enabled_;
    bool step_mode_;
    std::set<uint16_t> breakpoints_;

    std::string toHex8(uint8_t value) const;
    std::string toHex16(uint16_t value) const;
    std::string toHex32(uint32_t value) const;
};

// 32비트용 디버거
class Debugger32 {
public:
    explicit Debugger32(Chip8_32& chip8)
        : chip8_(chip8), enabled_(false), step_mode_(false) {}

    void enable(bool on = true) { enabled_ = on; }
    bool isEnabled() const { return enabled_; }
    void setStepMode(bool on = true) { step_mode_ = on; }
    bool isStepMode() const { return step_mode_; }

    void addBreakpoint(uint16_t address) { breakpoints_.insert(address); }
    void removeBreakpoint(uint16_t address) { breakpoints_.erase(address); }
    bool hasBreakpoint(uint16_t address) const { return breakpoints_.count(address) > 0; }
    void clearBreakpoints() { breakpoints_.clear(); }

    void printState(uint32_t opcode);
    std::string disassemble(uint32_t opcode);
    void handleDebugInput();

private:
    Chip8_32& chip8_;
    bool enabled_;
    bool step_mode_;
    std::set<uint16_t> breakpoints_;

    std::string toHex8(uint8_t value) const;
    std::string toHex16(uint16_t value) const;
    std::string toHex32(uint32_t value) const;
};

} // namespace chip8emu