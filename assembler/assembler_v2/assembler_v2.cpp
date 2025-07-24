// g++ -o assembler_v2 assembler_v2.cpp
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <regex>

// 어셈블리 결과를 나타내는 구조체
struct AssemblyResult {
    bool success;
    uint32_t machine_code;
    std::string error_message;
    
    AssemblyResult(bool s, uint32_t code, const std::string& error = "") 
        : success(s), machine_code(code), error_message(error) {}
};

// 언어 설정
enum class Language {
    ENGLISH,
    KOREAN
};

class Chip8_32_Assembler {
private:
    // 레지스터 매핑
    std::map<std::string, uint8_t> registers;
    
    // 명령어 매핑
    std::map<std::string, uint8_t> opcodes;
    
    // 언어 설정
    Language current_language = Language::KOREAN;
    
    // 언어별 메시지 함수들
    std::string getMessage(const std::string& key, const std::string& param1 = "", const std::string& param2 = "") {
        if (current_language == Language::KOREAN) {
            if (key == "unknown_instruction") return "알 수 없는 명령어: '" + param1 + "'. 유효한 명령어: CLS, RET, LD, JP, CALL, ADD, DRW, SYSCALL, PUSH, POP, MOV 등";
            if (key == "ld_requires_2_operands") return "LD 명령어는 2개의 피연산자가 필요합니다";
            if (key == "invalid_register") return "잘못된 레지스터: " + param1;
            if (key == "invalid_immediate") return "잘못된 즉시값: " + param1;
            if (key == "unsupported_memory_addressing") return "지원하지 않는 메모리 주소 지정: [" + param1 + "]";
            if (key == "invalid_destination_register") return "잘못된 목적지 레지스터: " + param1;
            if (key == "invalid_source_register") return "잘못된 소스 레지스터: " + param1;
            if (key == "invalid_ld_syntax") return "잘못된 LD 명령어 문법. 예상: LD 레지스터, 값 또는 LD [I], 레지스터 또는 LD 레지스터, [I]";
            if (key == "jp_requires_1_operand") return "JP 명령어는 1개의 피연산자(주소)가 필요합니다";
            if (key == "invalid_jump_address") return "잘못된 점프 주소: " + param1;
            if (key == "jp_requires_immediate") return "JP 명령어는 즉시 주소값이 필요합니다";
            if (key == "call_requires_1_operand") return "CALL 명령어는 1개의 피연산자(주소)가 필요합니다";
            if (key == "invalid_call_address") return "잘못된 호출 주소: " + param1;
            if (key == "call_requires_immediate") return "CALL 명령어는 즉시 주소값이 필요합니다";
            if (key == "add_requires_2_operands") return "ADD 명령어는 2개의 피연산자가 필요합니다";
            if (key == "invalid_add_syntax") return "잘못된 ADD 명령어 문법. 예상: ADD 레지스터, 값 또는 ADD 레지스터, 레지스터";
            if (key == "drw_requires_3_operands") return "DRW 명령어는 3개의 피연산자(x_reg, y_reg, height)가 필요합니다";
            if (key == "drw_first_operand_register") return "DRW 첫 번째 피연산자는 레지스터여야 합니다: " + param1;
            if (key == "drw_second_operand_register") return "DRW 두 번째 피연산자는 레지스터여야 합니다: " + param1;
            if (key == "drw_third_operand_immediate") return "DRW 세 번째 피연산자는 즉시값이어야 합니다: " + param1;
            if (key == "invalid_x_register") return "잘못된 X 레지스터: " + param1;
            if (key == "invalid_y_register") return "잘못된 Y 레지스터: " + param1;
            if (key == "invalid_height_value") return "잘못된 높이값: " + param1;
            if (key == "syscall_requires_3_operands") return "SYSCALL 명령어는 3개의 피연산자(syscall_num, buffer_addr, fd)가 필요합니다";
            if (key == "syscall_num_immediate") return "SYSCALL 번호는 즉시값이어야 합니다: " + param1;
            if (key == "buffer_addr_immediate") return "버퍼 주소는 즉시값이어야 합니다: " + param1;
            if (key == "fd_immediate") return "파일 디스크립터는 즉시값이어야 합니다: " + param1;
            if (key == "invalid_syscall_params") return "잘못된 SYSCALL 매개변수 값들";
            if (key == "stack_requires_1_operand") return "스택 명령어는 최소 1개의 피연산자가 필요합니다";
            if (key == "invalid_push_register") return "PUSH에 잘못된 레지스터: " + param1;
            if (key == "invalid_pop_register") return "POP에 잘못된 레지스터: " + param1;
            if (key == "unsupported_mov_operands") return "지원하지 않는 MOV 피연산자: " + param1 + ", " + param2;
            if (key == "push_requires_register") return "PUSH는 레지스터 피연산자가 필요합니다";
            if (key == "pop_requires_register") return "POP는 레지스터 피연산자가 필요합니다";
            if (key == "mov_requires_2_operands") return "MOV는 2개의 피연산자가 필요합니다";
            if (key == "unknown_stack_instruction") return "알 수 없는 스택 명령어: " + param1;
            if (key == "empty_line") return "빈 줄 또는 주석만";
            if (key == "no_tokens") return "토큰을 찾을 수 없음";
            if (key == "instruction_not_implemented") return "명령어 '" + param1 + "'는 인식되지만 아직 구현되지 않았습니다";
            if (key == "empty_immediate") return "빈 즉시값";
            if (key == "empty_after_hash") return "# 다음에 빈 값";
            if (key == "cannot_parse_immediate") return "즉시값을 파싱할 수 없습니다: " + param1;
        } else {
            // English messages
            if (key == "unknown_instruction") return "Unknown instruction: '" + param1 + "'. Valid instructions: CLS, RET, LD, JP, CALL, ADD, DRW, SYSCALL, PUSH, POP, MOV, etc.";
            if (key == "ld_requires_2_operands") return "LD instruction requires 2 operands";
            if (key == "invalid_register") return "Invalid register: " + param1;
            if (key == "invalid_immediate") return "Invalid immediate value: " + param1;
            if (key == "unsupported_memory_addressing") return "Unsupported memory addressing: [" + param1 + "]";
            if (key == "invalid_destination_register") return "Invalid destination register: " + param1;
            if (key == "invalid_source_register") return "Invalid source register: " + param1;
            if (key == "invalid_ld_syntax") return "Invalid LD instruction syntax. Expected: LD reg, value or LD [I], reg or LD reg, [I]";
            if (key == "jp_requires_1_operand") return "JP instruction requires 1 operand (address)";
            if (key == "invalid_jump_address") return "Invalid jump address: " + param1;
            if (key == "jp_requires_immediate") return "JP instruction requires immediate address value";
            if (key == "call_requires_1_operand") return "CALL instruction requires 1 operand (address)";
            if (key == "invalid_call_address") return "Invalid call address: " + param1;
            if (key == "call_requires_immediate") return "CALL instruction requires immediate address value";
            if (key == "add_requires_2_operands") return "ADD instruction requires 2 operands";
            if (key == "invalid_add_syntax") return "Invalid ADD instruction syntax. Expected: ADD reg, value or ADD reg, reg";
            if (key == "drw_requires_3_operands") return "DRW instruction requires 3 operands (x_reg, y_reg, height)";
            if (key == "drw_first_operand_register") return "DRW first operand must be a register: " + param1;
            if (key == "drw_second_operand_register") return "DRW second operand must be a register: " + param1;
            if (key == "drw_third_operand_immediate") return "DRW third operand must be immediate value: " + param1;
            if (key == "invalid_x_register") return "Invalid X register: " + param1;
            if (key == "invalid_y_register") return "Invalid Y register: " + param1;
            if (key == "invalid_height_value") return "Invalid height value: " + param1;
            if (key == "syscall_requires_3_operands") return "SYSCALL instruction requires 3 operands (syscall_num, buffer_addr, fd)";
            if (key == "syscall_num_immediate") return "SYSCALL number must be immediate value: " + param1;
            if (key == "buffer_addr_immediate") return "Buffer address must be immediate value: " + param1;
            if (key == "fd_immediate") return "File descriptor must be immediate value: " + param1;
            if (key == "invalid_syscall_params") return "Invalid SYSCALL parameter values";
            if (key == "stack_requires_1_operand") return "Stack instruction requires at least 1 operand";
            if (key == "invalid_push_register") return "Invalid register for PUSH: " + param1;
            if (key == "invalid_pop_register") return "Invalid register for POP: " + param1;
            if (key == "unsupported_mov_operands") return "Unsupported MOV operands: " + param1 + ", " + param2;
            if (key == "push_requires_register") return "PUSH requires register operand";
            if (key == "pop_requires_register") return "POP requires register operand";
            if (key == "mov_requires_2_operands") return "MOV requires 2 operands";
            if (key == "unknown_stack_instruction") return "Unknown stack instruction: " + param1;
            if (key == "empty_line") return "Empty line or comment only";
            if (key == "no_tokens") return "No tokens found";
            if (key == "instruction_not_implemented") return "Instruction '" + param1 + "' is recognized but not yet implemented";
            if (key == "empty_immediate") return "Empty immediate value";
            if (key == "empty_after_hash") return "Empty value after #";
            if (key == "cannot_parse_immediate") return "Cannot parse immediate value: " + param1;
        }
        return "Unknown message key: " + key;
    }
    
    void initializeRegisterMap() {
        // R0~R31 레지스터 매핑
        for (int i = 0; i < 32; i++) {
            registers["R" + std::to_string(i)] = i;
        }
        
        // 특수 레지스터 별칭
        registers["RBP"] = 28;  // R28
        registers["RSP"] = 29;  // R29  
        registers["RIP"] = 30;  // R30
        
        // V0~VF (8비트 호환용)
        for (int i = 0; i < 16; i++) {
            char reg_name[4];
            sprintf(reg_name, "V%X", i);
            registers[reg_name] = i;
        }
    }
    
    void initializeOpcodeMap() {
        opcodes["CLS"] = 0x00;      // 화면 지우기
        opcodes["RET"] = 0x00;      // 서브루틴 반환
        opcodes["JP"] = 0x01;       // 절대 주소로 점프
        opcodes["CALL"] = 0x02;     // 서브루틴 호출
        opcodes["SE"] = 0x03;       // Skip if Equal
        opcodes["SNE"] = 0x04;      // Skip if Not Equal
        opcodes["LD"] = 0x06;       // Load
        opcodes["ADD"] = 0x07;      // Add
        opcodes["OR"] = 0x08;       // OR 연산
        opcodes["AND"] = 0x08;      // AND 연산
        opcodes["XOR"] = 0x08;      // XOR 연산
        opcodes["SUB"] = 0x08;      // SUB 연산
        opcodes["SHR"] = 0x08;      // Shift Right
        opcodes["SUBN"] = 0x08;     // Subtract (reverse)
        opcodes["SHL"] = 0x08;      // Shift Left
        opcodes["RND"] = 0x0C;      // Random
        opcodes["DRW"] = 0x0D;      // Draw sprite
        opcodes["SKP"] = 0x0E;      // Skip if key pressed
        opcodes["SKNP"] = 0x0E;     // Skip if key not pressed
        opcodes["SYSCALL"] = 0x10;  // System call
        
        // 스택 명령어
        opcodes["PUSH"] = 0x11;
        opcodes["POP"] = 0x11;
        opcodes["MOV"] = 0x11;
    }
    
    std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream iss(line);
        
        while (iss >> token) {
            // 쉼표 제거
            if (!token.empty() && token.back() == ',') {
                token.pop_back();
            }
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    bool isRegister(const std::string& token) {
        return registers.find(token) != registers.end();
    }
    
    bool isImmediate(const std::string& token) {
        if (token.empty()) return false;
        if (token[0] == '#') return true; // #값 형태
        if (token.substr(0, 2) == "0x" || token.substr(0, 2) == "0X") return true; // 16진수
        
        // 10진수 체크
        for (char c : token) {
            if (!isdigit(c) && c != '-') return false;
        }
        return true;
    }
    
    bool isMemoryAddress(const std::string& token) {
        return (token.front() == '[' && token.back() == ']');
    }
    
    uint32_t parseImmediate(const std::string& token) {
        std::string value = token;
        
        if (value.empty()) {
            throw std::invalid_argument(getMessage("empty_immediate"));
        }
        
        if (value[0] == '#') {
            value = value.substr(1); // # 제거
            if (value.empty()) {
                throw std::invalid_argument(getMessage("empty_after_hash"));
            }
        }
        
        try {
            if (value.substr(0, 2) == "0x" || value.substr(0, 2) == "0X") {
                return std::stoul(value, nullptr, 16);
            } else {
                return std::stoul(value, nullptr, 10);
            }
        } catch (const std::exception& e) {
            throw std::invalid_argument(getMessage("cannot_parse_immediate", token));
        }
    }
    
    std::string parseMemoryAddress(const std::string& token) {
        std::string addr = token.substr(1, token.length() - 2); // [ ] 제거
        return addr;
    }
    
    AssemblyResult assembleLoadInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            return AssemblyResult(false, 0, getMessage("ld_requires_2_operands"));
        }
        
        std::string dest = tokens[1];
        std::string src = tokens[2];
        
        // LD Rx, #immediate
        if (isRegister(dest) && isImmediate(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_register", dest));
            }
            uint8_t reg = registers[dest];
            try {
                uint32_t imm = parseImmediate(src);
                return AssemblyResult(true, (0x06 << 24) | (reg << 16) | (imm & 0xFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, getMessage("invalid_immediate", src));
            }
        }
        
        // LD I, #address  
        if (dest == "I" && isImmediate(src)) {
            try {
                uint32_t addr = parseImmediate(src);
                return AssemblyResult(true, (0x0A << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, getMessage("invalid_immediate", src));
            }
        }
        
        // LD [I], Rx (FX55 equivalent - 0x0F XX 0505)
        if (isMemoryAddress(dest) && isRegister(src)) {
            std::string addr_content = parseMemoryAddress(dest);
            if (addr_content == "I") {
                if (registers.find(src) == registers.end()) {
                    return AssemblyResult(false, 0, getMessage("invalid_register", src));
                }
                uint8_t reg = registers[src];
                return AssemblyResult(true, (0x0F << 24) | (reg << 16) | 0x0505);
            } else {
                return AssemblyResult(false, 0, getMessage("unsupported_memory_addressing", addr_content));
            }
        }
        
        // LD Rx, [I] (FX65 equivalent - 0x0F XX 0605)  
        if (isRegister(dest) && isMemoryAddress(src)) {
            std::string addr_content = parseMemoryAddress(src);
            if (addr_content == "I") {
                if (registers.find(dest) == registers.end()) {
                    return AssemblyResult(false, 0, getMessage("invalid_register", dest));
                }
                uint8_t reg = registers[dest];
                return AssemblyResult(true, (0x0F << 24) | (reg << 16) | 0x0605);
            } else {
                return AssemblyResult(false, 0, getMessage("unsupported_memory_addressing", addr_content));
            }
        }
        
        // LD Rx, Ry
        if (isRegister(dest) && isRegister(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_destination_register", dest));
            }
            if (registers.find(src) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_source_register", src));
            }
            uint8_t reg_dest = registers[dest];
            uint8_t reg_src = registers[src];
            return AssemblyResult(true, (0x08 << 24) | (reg_dest << 16) | (reg_src << 8) | 0x00);
        }
        
        return AssemblyResult(false, 0, getMessage("invalid_ld_syntax"));
    }
    
    AssemblyResult assembleJumpInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, getMessage("jp_requires_1_operand"));
        }
        
        std::string target = tokens[1];
        if (isImmediate(target)) {
            try {
                uint32_t addr = parseImmediate(target);
                return AssemblyResult(true, (0x01 << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, getMessage("invalid_jump_address", target));
            }
        }
        
        return AssemblyResult(false, 0, getMessage("jp_requires_immediate"));
    }
    
    AssemblyResult assembleCallInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, getMessage("call_requires_1_operand"));
        }
        
        std::string target = tokens[1];
        if (isImmediate(target)) {
            try {
                uint32_t addr = parseImmediate(target);
                return AssemblyResult(true, (0x02 << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, getMessage("invalid_call_address", target));
            }
        }
        
        return AssemblyResult(false, 0, getMessage("call_requires_immediate"));
    }
    
    AssemblyResult assembleAddInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            return AssemblyResult(false, 0, getMessage("add_requires_2_operands"));
        }
        
        std::string dest = tokens[1];
        std::string src = tokens[2];
        
        // ADD Rx, #immediate
        if (isRegister(dest) && isImmediate(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_register", dest));
            }
            try {
                uint8_t reg = registers[dest];
                uint32_t imm = parseImmediate(src);
                return AssemblyResult(true, (0x07 << 24) | (reg << 16) | (imm & 0xFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, getMessage("invalid_immediate", src));
            }
        }
        
        // ADD Rx, Ry  
        if (isRegister(dest) && isRegister(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_destination_register", dest));
            }
            if (registers.find(src) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_source_register", src));
            }
            uint8_t reg_dest = registers[dest];
            uint8_t reg_src = registers[src];
            return AssemblyResult(true, (0x08 << 24) | (reg_dest << 16) | (reg_src << 8) | 0x04);
        }
        
        return AssemblyResult(false, 0, getMessage("invalid_add_syntax"));
    }
    
    AssemblyResult assembleDrawInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 4) {
            return AssemblyResult(false, 0, getMessage("drw_requires_3_operands"));
        }
        
        std::string reg_x = tokens[1];
        std::string reg_y = tokens[2]; 
        std::string height = tokens[3];
        
        if (!isRegister(reg_x)) {
            return AssemblyResult(false, 0, getMessage("drw_first_operand_register", reg_x));
        }
        if (!isRegister(reg_y)) {
            return AssemblyResult(false, 0, getMessage("drw_second_operand_register", reg_y));
        }
        if (!isImmediate(height)) {
            return AssemblyResult(false, 0, getMessage("drw_third_operand_immediate", height));
        }
        
        if (registers.find(reg_x) == registers.end()) {
            return AssemblyResult(false, 0, getMessage("invalid_x_register", reg_x));
        }
        if (registers.find(reg_y) == registers.end()) {
            return AssemblyResult(false, 0, getMessage("invalid_y_register", reg_y));
        }
        
        try {
            uint8_t x = registers[reg_x];
            uint8_t y = registers[reg_y];
            uint8_t h = parseImmediate(height) & 0xFF;
            return AssemblyResult(true, (0x0D << 24) | (x << 16) | (y << 8) | h);
        } catch (...) {
            return AssemblyResult(false, 0, getMessage("invalid_height_value", height));
        }
    }
    
    AssemblyResult assembleSyscallInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 4) {
            return AssemblyResult(false, 0, getMessage("syscall_requires_3_operands"));
        }
        
        std::string syscall_num = tokens[1];
        std::string buffer_addr = tokens[2];
        std::string fd = tokens[3];
        
        if (!isImmediate(syscall_num)) {
            return AssemblyResult(false, 0, getMessage("syscall_num_immediate", syscall_num));
        }
        if (!isImmediate(buffer_addr)) {
            return AssemblyResult(false, 0, getMessage("buffer_addr_immediate", buffer_addr));
        }
        if (!isImmediate(fd)) {
            return AssemblyResult(false, 0, getMessage("fd_immediate", fd));
        }
        
        try {
            uint8_t sys_num = parseImmediate(syscall_num) & 0xF;
            uint16_t buf_addr = parseImmediate(buffer_addr) & 0xFFFF;
            uint8_t file_desc = parseImmediate(fd) & 0xF;
            
            return AssemblyResult(true, (0x10 << 24) | (sys_num << 20) | (buf_addr << 4) | file_desc);
        } catch (...) {
            return AssemblyResult(false, 0, getMessage("invalid_syscall_params"));
        }
    }
    
    AssemblyResult assembleStackInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, getMessage("stack_requires_1_operand"));
        }
        
        std::string operation = tokens[0];
        
        // PUSH RBP
        if (operation == "PUSH" && tokens[1] == "RBP") {
            return AssemblyResult(true, 0x11000000);
        }
        
        // PUSH Rx
        if (operation == "PUSH" && isRegister(tokens[1])) {
            if (registers.find(tokens[1]) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_push_register", tokens[1]));
            }
            uint8_t reg = registers[tokens[1]];
            return AssemblyResult(true, (0x11 << 24) | (0x00 << 16) | (reg << 8));
        }
        
        // POP RBP
        if (operation == "POP" && tokens[1] == "RBP") {
            return AssemblyResult(true, 0x11010000);
        }
        
        // POP Rx
        if (operation == "POP" && isRegister(tokens[1])) {
            if (registers.find(tokens[1]) == registers.end()) {
                return AssemblyResult(false, 0, getMessage("invalid_pop_register", tokens[1]));
            }
            uint8_t reg = registers[tokens[1]];
            return AssemblyResult(true, (0x11 << 24) | (0x01 << 16) | (reg << 8));
        }
        
        // MOV RBP, RSP
        if (operation == "MOV" && tokens.size() >= 3) {
            if (tokens[1] == "RBP" && tokens[2] == "RSP") {
                return AssemblyResult(true, 0x11020000);
            }
            if (tokens[1] == "RSP" && tokens[2] == "RBP") {
                return AssemblyResult(true, 0x11030000);
            }
            return AssemblyResult(false, 0, getMessage("unsupported_mov_operands", tokens[1], tokens[2]));
        }
        
        if (operation == "PUSH") {
            return AssemblyResult(false, 0, getMessage("push_requires_register"));
        }
        if (operation == "POP") {
            return AssemblyResult(false, 0, getMessage("pop_requires_register")); 
        }
        if (operation == "MOV") {
            return AssemblyResult(false, 0, getMessage("mov_requires_2_operands"));
        }
        
        return AssemblyResult(false, 0, getMessage("unknown_stack_instruction", operation));
    }

public:
    Chip8_32_Assembler() {
        initializeRegisterMap();
        initializeOpcodeMap();
    }
    
    // 언어 설정 함수
    void setLanguage(Language lang) {
        current_language = lang;
    }
    
    Language getLanguage() const {
        return current_language;
    }
    
    // 도움말 출력 함수
    void printHelp() {
        if (current_language == Language::KOREAN) {
            std::cout << "\n=== 사용 가능한 명령어 ===" << std::endl;
            std::cout << "CLS                    - 화면 지우기" << std::endl;
            std::cout << "RET                    - 서브루틴에서 반환" << std::endl;
            std::cout << "LD 레지스터, #값        - 즉시값 로드" << std::endl;
            std::cout << "LD I, #주소            - I 레지스터에 주소 로드" << std::endl;
            std::cout << "LD [I], 레지스터        - 레지스터를 메모리에 저장" << std::endl;
            std::cout << "LD 레지스터, [I]        - 메모리에서 레지스터로 로드" << std::endl;
            std::cout << "JP #주소               - 주소로 점프" << std::endl;
            std::cout << "CALL #주소             - 서브루틴 호출" << std::endl;
            std::cout << "ADD 레지스터, #값       - 레지스터에 즉시값 더하기" << std::endl;
            std::cout << "ADD 레지스터, 레지스터   - 레지스터끼리 더하기" << std::endl;
            std::cout << "DRW 레지스터, 레지스터, #높이  - 스프라이트 그리기" << std::endl;
            std::cout << "PUSH 레지스터          - 레지스터를 스택에 푸시" << std::endl;
            std::cout << "POP 레지스터           - 스택에서 레지스터로 팝" << std::endl;
            std::cout << "MOV RBP, RSP          - 스택 포인터 이동" << std::endl;
            std::cout << "SYSCALL #n, #주소, #fd - 시스템 호출" << std::endl;
            std::cout << "\n레지스터: R0-R31, V0-VF, I, RBP, RSP, RIP" << std::endl;
            std::cout << "값: #42 (10진수), #0x2A (16진수)" << std::endl;
        } else {
            std::cout << "\n=== Available Instructions ===" << std::endl;
            std::cout << "CLS                    - Clear screen" << std::endl;
            std::cout << "RET                    - Return from subroutine" << std::endl;
            std::cout << "LD reg, #value         - Load immediate value" << std::endl;
            std::cout << "LD I, #address         - Load address to I register" << std::endl;
            std::cout << "LD [I], reg            - Store register to memory" << std::endl;
            std::cout << "LD reg, [I]            - Load from memory to register" << std::endl;
            std::cout << "JP #address            - Jump to address" << std::endl;
            std::cout << "CALL #address          - Call subroutine" << std::endl;
            std::cout << "ADD reg, #value        - Add immediate to register" << std::endl;
            std::cout << "ADD reg, reg           - Add register to register" << std::endl;
            std::cout << "DRW reg, reg, #height  - Draw sprite" << std::endl;
            std::cout << "PUSH reg               - Push register to stack" << std::endl;
            std::cout << "POP reg                - Pop from stack to register" << std::endl;
            std::cout << "MOV RBP, RSP           - Move stack pointer" << std::endl;
            std::cout << "SYSCALL #n, #addr, #fd - System call" << std::endl;
            std::cout << "\nRegisters: R0-R31, V0-VF, I, RBP, RSP, RIP" << std::endl;
            std::cout << "Values: #42 (decimal), #0x2A (hex)" << std::endl;
        }
        std::cout << std::endl;
    }
    
    AssemblyResult assembleLine(const std::string& line) {
        // 주석 및 빈 줄 처리
        std::string trimmed = line;
        size_t comment_pos = trimmed.find(';');
        if (comment_pos != std::string::npos) {
            trimmed = trimmed.substr(0, comment_pos);
        }
        
        // 앞뒤 공백 제거
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) {
            return AssemblyResult(false, 0, getMessage("empty_line"));
        }
        
        // 대문자로 변환
        std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::toupper);
        
        std::vector<std::string> tokens = tokenize(trimmed);
        if (tokens.empty()) {
            return AssemblyResult(false, 0, getMessage("no_tokens"));
        }
        
        std::string instruction = tokens[0];
        
        // 정의된 명령어 목록
        std::vector<std::string> valid_instructions = {
            "CLS", "RET", "LD", "JP", "CALL", "ADD", "DRW", "SYSCALL", 
            "PUSH", "POP", "MOV", "SE", "SNE", "OR", "AND", "XOR", 
            "SUB", "SHR", "SUBN", "SHL", "RND", "SKP", "SKNP"
        };
        
        // 명령어가 정의되어 있는지 확인
        bool is_valid_instruction = std::find(valid_instructions.begin(), 
                                             valid_instructions.end(), 
                                             instruction) != valid_instructions.end();
        
        if (!is_valid_instruction) {
            return AssemblyResult(false, 0, getMessage("unknown_instruction", instruction));
        }
        
        // 명령어별 어셈블리
        if (instruction == "CLS") {
            return AssemblyResult(true, 0x00000E00);
        }
        else if (instruction == "RET") {
            return AssemblyResult(true, 0x00000E0E);
        }
        else if (instruction == "LD") {
            return assembleLoadInstruction(tokens);
        }
        else if (instruction == "JP") {
            return assembleJumpInstruction(tokens);
        }
        else if (instruction == "CALL") {
            return assembleCallInstruction(tokens);
        }
        else if (instruction == "ADD") {
            return assembleAddInstruction(tokens);
        }
        else if (instruction == "DRW") {
            return assembleDrawInstruction(tokens);
        }
        else if (instruction == "SYSCALL") {
            return assembleSyscallInstruction(tokens);
        }
        else if (instruction == "PUSH" || instruction == "POP" || instruction == "MOV") {
            return assembleStackInstruction(tokens);
        }
        else {
            return AssemblyResult(false, 0, getMessage("instruction_not_implemented", instruction));
        }
    }
    
    void assemble(const std::string& input) {
        std::istringstream iss(input);
        std::string line;
        int line_number = 0;
        
        if (current_language == Language::KOREAN) {
            std::cout << "=== Chip-8 32비트 확장 어셈블러 ===" << std::endl;
        } else {
            std::cout << "=== Chip-8 32bit Extension Assembler ===" << std::endl;
        }
        std::cout << std::endl;
        
        while (std::getline(iss, line)) {
            line_number++;
            AssemblyResult result = assembleLine(line);
            
            if (result.success) {
                if (current_language == Language::KOREAN) {
                    std::cout << "라인 " << line_number << ": " << line << std::endl;
                    std::cout << "출력: 0x" << std::hex << std::uppercase 
                             << std::setw(8) << std::setfill('0') << result.machine_code << std::endl;
                } else {
                    std::cout << "Line " << line_number << ": " << line << std::endl;
                    std::cout << "Output: 0x" << std::hex << std::uppercase 
                             << std::setw(8) << std::setfill('0') << result.machine_code << std::endl;
                }
                std::cout << std::endl;
            } else {
                // 빈 줄이나 주석만 있는 경우는 에러로 표시하지 않음
                std::string empty_msg = (current_language == Language::KOREAN) ? "빈 줄 또는 주석만" : "Empty line or comment only";
                if (result.error_message != empty_msg) {
                    if (current_language == Language::KOREAN) {
                        std::cout << "라인 " << line_number << " 오류: " << line << std::endl;
                        std::cout << "오류: " << result.error_message << std::endl;
                    } else {
                        std::cout << "Line " << line_number << " ERROR: " << line << std::endl;
                        std::cout << "Error: " << result.error_message << std::endl;
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
};

int main() {
    Chip8_32_Assembler assembler;
    std::string input;
    
    // 언어 선택
    std::cout << "언어를 선택하세요 / Choose Language:" << std::endl;
    std::cout << "1. 한국어 (Korean)" << std::endl;
    std::cout << "2. English" << std::endl;
    std::cout << "> ";
    
    std::string lang_choice;
    std::getline(std::cin, lang_choice);
    
    if (lang_choice == "2" || lang_choice == "english" || lang_choice == "English") {
        assembler.setLanguage(Language::ENGLISH);
    } else {
        assembler.setLanguage(Language::KOREAN);
    }
    
    if (assembler.getLanguage() == Language::KOREAN) {
        std::cout << "\nChip-8 32비트 확장 어셈블러" << std::endl;
        std::cout << "어셈블리 명렁어를 입력하세요 ('quit' 입력시 종료):" << std::endl;
        std::cout << "'help' 입력시 사용 가능한 명령어 목록을 볼 수 있습니다" << std::endl;
    } else {
        std::cout << "\nChip-8 32bit Extension Assembler" << std::endl;
        std::cout << "Enter assembly instructions (type 'quit' to exit):" << std::endl;
        std::cout << "Type 'help' for available instructions" << std::endl;
    }
    std::cout << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "quit" || input == "exit" || input == "종료") {
            break;
        }
        
        if (input == "help" || input == "도움말") {
            assembler.printHelp();
            continue;
        }
        
        if (input == "lang" || input == "언어") {
            if (assembler.getLanguage() == Language::KOREAN) {
                std::cout << "언어를 English로 변경하시겠습니까? (y/n): ";
            } else {
                std::cout << "Change language to Korean? (y/n): ";
            }
            std::string confirm;
            std::getline(std::cin, confirm);
            if (confirm == "y" || confirm == "Y" || confirm == "예") {
                Language new_lang = (assembler.getLanguage() == Language::KOREAN) ? Language::ENGLISH : Language::KOREAN;
                assembler.setLanguage(new_lang);
                if (new_lang == Language::KOREAN) {
                    std::cout << "언어가 한국어로 변경되었습니다." << std::endl;
                } else {
                    std::cout << "Language changed to English." << std::endl;
                }
            }
            std::cout << std::endl;
            continue;
        }
        
        if (!input.empty()) {
            AssemblyResult result = assembler.assembleLine(input);
            if (result.success) {
                std::cout << "0x" << std::hex << std::uppercase 
                         << std::setw(8) << std::setfill('0') << result.machine_code << std::endl;
            } else {
                if (assembler.getLanguage() == Language::KOREAN) {
                    std::cout << "❌ 오류: " << result.error_message << std::endl;
                } else {
                    std::cout << "❌ Error: " << result.error_message << std::endl;
                }
            }
            std::cout << std::endl;
        }
    }
    
    // 예시 테스트 - 에러 케이스 포함
    if (assembler.getLanguage() == Language::KOREAN) {
        std::cout << "\n=== 예시 테스트 (오류 케이스 포함) ===" << std::endl;
    } else {
        std::cout << "\n=== Example Tests (Including Error Cases) ===" << std::endl;
    }
    
    std::string test_program;
    if (assembler.getLanguage() == Language::KOREAN) {
        test_program = R"(
        CLS
        LD R0, #42
        LD I, #0x200
        LD [I], R0  
        LD R1, [I]
        ADD R0, #10
        ADD R0, R1
        DRW R0, R1, #5
        JP #0x200
        CALL #0x300
        RET
        PUSH RBP
        POP RBP
        MOV RBP, RSP
        SYSCALL #1, #0x200, #1
        ; === 오류 케이스 ===
        잘못된명령어 R0, #42
        LD 
        LD R0
        LD R99, #42
        ADD R0, 잘못된레지스터  
        DRW R0
    )";
    } else {
        test_program = R"(
        CLS
        LD R0, #42
        LD I, #0x200
        LD [I], R0
        LD R1, [I]
        ADD R0, #10
        ADD R0, R1
        DRW R0, R1, #5
        JP #0x200
        CALL #0x300
        RET
        PUSH RBP
        POP RBP
        MOV RBP, RSP
        SYSCALL #1, #0x200, #1
        ; === Error Cases ===
        INVALID_CMD R0, #42
        LD 
        LD R0
        LD R99, #42
        ADD R0, INVALID_REG  
        DRW R0
    )";
    }
    
    assembler.assemble(test_program);
    
    return 0;
}
