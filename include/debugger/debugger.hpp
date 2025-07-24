#pragma once
#include <cstdint>
#include <string>
#include <set>
#include <vector>  // 🔧 추가: std::vector 사용을 위해 필수
#include <iomanip>  // 🔧 추가: std::setw, std::setfill 사용을 위해

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

/**
 * @brief 스택 프레임 시각화 도구
 */
enum class StackCellType {
    EMPTY,       // 빈 공간
    OLD_RBP,     // 이전 RBP
    PARAMETER,   // 매개변수
    LOCAL_VAR,   // 지역변수
    RESULT,      // 결과값
    HIGHLIGHT    // 현재 작업 중
};

struct StackCell {
    uint32_t address;   // 메모리 주소
    uint32_t value;     // 저장된 값
    StackCellType type; // 셀 타입
    std::string label;  // 표시할 라벨 ("a = 10", "old RBP" 등)
    bool is_active;     // 현재 활성화된 셀인지

    StackCell(uint32_t addr, uint32_t val, StackCellType t, const std::string& lbl, bool active = false)
        : address(addr), value(val), type(t), label(lbl), is_active(active) {}
};

class StackVisualizer {
public:
    StackVisualizer() = default;
    
    /**
     * @brief 기본 스택 프레임 다이어그램 그리기
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     * @param phase 현재 단계 이름
     * @param highlight_addr 하이라이트할 주소 (0이면 하이라이트 없음)
     */
    void drawStackFrame(const Chip8_32& chip8_32, const std::string& phase = "", uint32_t highlight_addr = 0);
    
    /**
     * @brief 🔥 애니메이션 스택 프레임 시각화 (4.3 단계)
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     * @param phase 현재 단계 이름
     * @param instruction 현재 실행 중인 명령어
     * @param wait_for_input 사용자 입력 대기 여부
     */
    void animateStackFrame(const Chip8_32& chip8_32, const std::string& phase, 
                          const std::string& instruction = "", bool wait_for_input = true);
    
    /**
     * @brief 🚀 대화형 스택 프레임 디버깅 (메인 진입점)
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     */
    void interactiveStackDebug(Chip8_32& chip8_32);
    
    /**
     * @brief 스택 셀 추가
     * @param addr 주소
     * @param value 값
     * @param type 셀 타입
     * @param label 라벨
     */
    void addStackCell(uint32_t addr, uint32_t value, StackCellType type, const std::string& label);
    
    /**
     * @brief 특정 주소 하이라이트
     * @param address 하이라이트할 주소
     */
    void highlightCell(uint32_t address);
    
    /**
     * @brief 모든 셀 초기화
     */
    void clearCells();

private:
    std::vector<StackCell> stack_cells_;  // 🔧 수정: 이제 제대로 선언됨
    
    /**
     * @brief 🔥 실시간 스택 메모리 분석 (4.2 단계)
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     */
    void analyzeRealTimeStack(const Chip8_32& chip8_32);
    
    /**
     * @brief 🎨 스택 셀 포맷팅
     * @param addr 주소
     * @param value 값
     * @param rbp RBP 값
     * @param rsp RSP 값
     * @param highlight_addr 하이라이트할 주소
     * @return 포맷된 문자열
     */
    std::string formatStackCell(uint32_t addr, uint32_t value, uint32_t rbp, uint32_t rsp, uint32_t highlight_addr);
    
    /**
     * @brief 🎯 포인터 정보 반환
     * @param addr 주소
     * @param rbp RBP 값
     * @param rsp RSP 값
     * @return 포인터 정보 문자열
     */
    std::string getPointerInfo(uint32_t addr, uint32_t rbp, uint32_t rsp);
    
    /**
     * @brief 🔥 고급 포인터 정보 출력
     * @param chip8_32 32비트 CHIP-8 시스템 참조
     * @param rbp RBP 값
     * @param rsp RSP 값
     */
    void drawAdvancedPointers(const Chip8_32& chip8_32, uint32_t rbp, uint32_t rsp);
    
    /**
     * @brief 스택 박스 그리기
     * @param start_addr 시작 주소
     * @param end_addr 끝 주소
     */
    void drawStackBox(uint32_t start_addr, uint32_t end_addr);
    
    /**
     * @brief 개별 스택 셀 그리기
     * @param cell 그릴 셀
     */
    void drawStackCell(const StackCell& cell);
    
    /**
     * @brief 포인터 정보 출력 (RBP, RSP)
     * @param rbp Base Pointer 값
     * @param rsp Stack Pointer 값
     */
    void drawPointers(uint32_t rbp, uint32_t rsp);
    
    /**
     * @brief 🎬 애니메이션 헬퍼 함수들
     */
    void clearScreen();
    void waitForUser(const std::string& message = "Press ENTER to continue...");
    void showInstructionInfo(const std::string& instruction, const std::string& description);
    void simulateFunctionCall(Chip8_32& chip8_32);  // 🔧 추가
    
    /**
     * @brief 셀 타입에 따른 이모지 반환
     * @param type 셀 타입
     * @return 해당하는 이모지 문자열
     */
    std::string getTypeEmoji(StackCellType type);
    
    /**
     * @brief 주소에 해당하는 셀 찾기
     * @param addr 찾을 주소
     * @return 찾은 셀의 포인터 (없으면 nullptr)
     */
    StackCell* findCell(uint32_t addr);
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
    
    // 🔧 추가: 누락된 함수 선언
    void drawStackDiagram(const Chip8_32& chip8_32, uint32_t highlight_addr = 0);
    
    // 🔥 **4단계 완성: 스택 프레임 시각화 통합**
    std::string getStackInstructionName(uint32_t opcode);

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