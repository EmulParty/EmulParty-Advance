// gcc -o assembler_v1 assembler_v1.cpp
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

class Chip8_32_Assembler {
private:
    // 레지스터 매핑
    std::map<std::string, uint8_t> registers;
    
    // 명령어 매핑
    std::map<std::string, uint8_t> opcodes;
    
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
            throw std::invalid_argument("Empty immediate value");
        }
        
        if (value[0] == '#') {
            value = value.substr(1); // # 제거
            if (value.empty()) {
                throw std::invalid_argument("Empty value after #");
            }
        }
        
        try {
            if (value.substr(0, 2) == "0x" || value.substr(0, 2) == "0X") {
                return std::stoul(value, nullptr, 16);
            } else {
                return std::stoul(value, nullptr, 10);
            }
        } catch (const std::exception& e) {
            throw std::invalid_argument("Cannot parse immediate value: " + token);
        }
    }
    
    std::string parseMemoryAddress(const std::string& token) {
        std::string addr = token.substr(1, token.length() - 2); // [ ] 제거
        return addr;
    }
    
    AssemblyResult assembleLoadInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            return AssemblyResult(false, 0, "LD instruction requires 2 operands");
        }
        
        std::string dest = tokens[1];
        std::string src = tokens[2];
        
        // LD Rx, #immediate
        if (isRegister(dest) && isImmediate(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid register: " + dest);
            }
            uint8_t reg = registers[dest];
            try {
                uint32_t imm = parseImmediate(src);
                return AssemblyResult(true, (0x06 << 24) | (reg << 16) | (imm & 0xFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, "Invalid immediate value: " + src);
            }
        }
        
        // LD I, #address  
        if (dest == "I" && isImmediate(src)) {
            try {
                uint32_t addr = parseImmediate(src);
                return AssemblyResult(true, (0x0A << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, "Invalid address value: " + src);
            }
        }
        
        // LD [I], Rx (FX55 equivalent - 0x0F XX 0505)
        if (isMemoryAddress(dest) && isRegister(src)) {
            std::string addr_content = parseMemoryAddress(dest);
            if (addr_content == "I") {
                if (registers.find(src) == registers.end()) {
                    return AssemblyResult(false, 0, "Invalid register: " + src);
                }
                uint8_t reg = registers[src];
                return AssemblyResult(true, (0x0F << 24) | (reg << 16) | 0x0505);
            } else {
                return AssemblyResult(false, 0, "Unsupported memory addressing: [" + addr_content + "]");
            }
        }
        
        // LD Rx, [I] (FX65 equivalent - 0x0F XX 0605)  
        if (isRegister(dest) && isMemoryAddress(src)) {
            std::string addr_content = parseMemoryAddress(src);
            if (addr_content == "I") {
                if (registers.find(dest) == registers.end()) {
                    return AssemblyResult(false, 0, "Invalid register: " + dest);
                }
                uint8_t reg = registers[dest];
                return AssemblyResult(true, (0x0F << 24) | (reg << 16) | 0x0605);
            } else {
                return AssemblyResult(false, 0, "Unsupported memory addressing: [" + addr_content + "]");
            }
        }
        
        // LD Rx, Ry
        if (isRegister(dest) && isRegister(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid destination register: " + dest);
            }
            if (registers.find(src) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid source register: " + src);
            }
            uint8_t reg_dest = registers[dest];
            uint8_t reg_src = registers[src];
            return AssemblyResult(true, (0x08 << 24) | (reg_dest << 16) | (reg_src << 8) | 0x00);
        }
        
        return AssemblyResult(false, 0, "Invalid LD instruction syntax. Expected: LD reg, value or LD [I], reg or LD reg, [I]");
    }
    
    AssemblyResult assembleJumpInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, "JP instruction requires 1 operand (address)");
        }
        
        std::string target = tokens[1];
        if (isImmediate(target)) {
            try {
                uint32_t addr = parseImmediate(target);
                return AssemblyResult(true, (0x01 << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, "Invalid jump address: " + target);
            }
        }
        
        return AssemblyResult(false, 0, "JP instruction requires immediate address value");
    }
    
    AssemblyResult assembleCallInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, "CALL instruction requires 1 operand (address)");
        }
        
        std::string target = tokens[1];
        if (isImmediate(target)) {
            try {
                uint32_t addr = parseImmediate(target);
                return AssemblyResult(true, (0x02 << 24) | (addr & 0xFFFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, "Invalid call address: " + target);
            }
        }
        
        return AssemblyResult(false, 0, "CALL instruction requires immediate address value");
    }
    
    AssemblyResult assembleAddInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 3) {
            return AssemblyResult(false, 0, "ADD instruction requires 2 operands");
        }
        
        std::string dest = tokens[1];
        std::string src = tokens[2];
        
        // ADD Rx, #immediate
        if (isRegister(dest) && isImmediate(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid register: " + dest);
            }
            try {
                uint8_t reg = registers[dest];
                uint32_t imm = parseImmediate(src);
                return AssemblyResult(true, (0x07 << 24) | (reg << 16) | (imm & 0xFFFF));
            } catch (...) {
                return AssemblyResult(false, 0, "Invalid immediate value: " + src);
            }
        }
        
        // ADD Rx, Ry  
        if (isRegister(dest) && isRegister(src)) {
            if (registers.find(dest) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid destination register: " + dest);
            }
            if (registers.find(src) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid source register: " + src);
            }
            uint8_t reg_dest = registers[dest];
            uint8_t reg_src = registers[src];
            return AssemblyResult(true, (0x08 << 24) | (reg_dest << 16) | (reg_src << 8) | 0x04);
        }
        
        return AssemblyResult(false, 0, "Invalid ADD instruction syntax. Expected: ADD reg, value or ADD reg, reg");
    }
    
    AssemblyResult assembleDrawInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 4) {
            return AssemblyResult(false, 0, "DRW instruction requires 3 operands (x_reg, y_reg, height)");
        }
        
        std::string reg_x = tokens[1];
        std::string reg_y = tokens[2]; 
        std::string height = tokens[3];
        
        if (!isRegister(reg_x)) {
            return AssemblyResult(false, 0, "DRW first operand must be a register: " + reg_x);
        }
        if (!isRegister(reg_y)) {
            return AssemblyResult(false, 0, "DRW second operand must be a register: " + reg_y);
        }
        if (!isImmediate(height)) {
            return AssemblyResult(false, 0, "DRW third operand must be immediate value: " + height);
        }
        
        if (registers.find(reg_x) == registers.end()) {
            return AssemblyResult(false, 0, "Invalid X register: " + reg_x);
        }
        if (registers.find(reg_y) == registers.end()) {
            return AssemblyResult(false, 0, "Invalid Y register: " + reg_y);
        }
        
        try {
            uint8_t x = registers[reg_x];
            uint8_t y = registers[reg_y];
            uint8_t h = parseImmediate(height) & 0xFF;
            return AssemblyResult(true, (0x0D << 24) | (x << 16) | (y << 8) | h);
        } catch (...) {
            return AssemblyResult(false, 0, "Invalid height value: " + height);
        }
    }
    
    AssemblyResult assembleSyscallInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 4) {
            return AssemblyResult(false, 0, "SYSCALL instruction requires 3 operands (syscall_num, buffer_addr, fd)");
        }
        
        std::string syscall_num = tokens[1];
        std::string buffer_addr = tokens[2];
        std::string fd = tokens[3];
        
        if (!isImmediate(syscall_num)) {
            return AssemblyResult(false, 0, "SYSCALL number must be immediate value: " + syscall_num);
        }
        if (!isImmediate(buffer_addr)) {
            return AssemblyResult(false, 0, "Buffer address must be immediate value: " + buffer_addr);
        }
        if (!isImmediate(fd)) {
            return AssemblyResult(false, 0, "File descriptor must be immediate value: " + fd);
        }
        
        try {
            uint8_t sys_num = parseImmediate(syscall_num) & 0xF;
            uint16_t buf_addr = parseImmediate(buffer_addr) & 0xFFFF;
            uint8_t file_desc = parseImmediate(fd) & 0xF;
            
            return AssemblyResult(true, (0x10 << 24) | (sys_num << 20) | (buf_addr << 4) | file_desc);
        } catch (...) {
            return AssemblyResult(false, 0, "Invalid SYSCALL parameter values");
        }
    }
    
    AssemblyResult assembleStackInstruction(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
            return AssemblyResult(false, 0, "Stack instruction requires at least 1 operand");
        }
        
        std::string operation = tokens[0];
        
        // PUSH RBP
        if (operation == "PUSH" && tokens[1] == "RBP") {
            return AssemblyResult(true, 0x11000000);
        }
        
        // PUSH Rx
        if (operation == "PUSH" && isRegister(tokens[1])) {
            if (registers.find(tokens[1]) == registers.end()) {
                return AssemblyResult(false, 0, "Invalid register for PUSH: " + tokens[1]);
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
                return AssemblyResult(false, 0, "Invalid register for POP: " + tokens[1]);
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
            return AssemblyResult(false, 0, "Unsupported MOV operands: " + tokens[1] + ", " + tokens[2]);
        }
        
        if (operation == "PUSH") {
            return AssemblyResult(false, 0, "PUSH requires register operand");
        }
        if (operation == "POP") {
            return AssemblyResult(false, 0, "POP requires register operand"); 
        }
        if (operation == "MOV") {
            return AssemblyResult(false, 0, "MOV requires 2 operands");
        }
        
        return AssemblyResult(false, 0, "Unknown stack instruction: " + operation);
    }

public:
    Chip8_32_Assembler() {
        initializeRegisterMap();
        initializeOpcodeMap();
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
            return AssemblyResult(false, 0, "Empty line or comment only");
        }
        
        // 대문자로 변환
        std::transform(trimmed.begin(), trimmed.end(), trimmed.begin(), ::toupper);
        
        std::vector<std::string> tokens = tokenize(trimmed);
        if (tokens.empty()) {
            return AssemblyResult(false, 0, "No tokens found");
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
            return AssemblyResult(false, 0, "Unknown instruction: '" + instruction + "'. Valid instructions: CLS, RET, LD, JP, CALL, ADD, DRW, SYSCALL, PUSH, POP, MOV, etc.");
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
            return AssemblyResult(false, 0, "Instruction '" + instruction + "' is recognized but not yet implemented");
        }
    }
    
    void assemble(const std::string& input) {
        std::istringstream iss(input);
        std::string line;
        int line_number = 0;
        
        std::cout << "=== Chip-8 32bit Extension Assembler ===" << std::endl;
        std::cout << std::endl;
        
        while (std::getline(iss, line)) {
            line_number++;
            AssemblyResult result = assembleLine(line);
            
            if (result.success) {
                std::cout << "Line " << line_number << ": " << line << std::endl;
                std::cout << "Output: 0x" << std::hex << std::uppercase 
                         << std::setw(8) << std::setfill('0') << result.machine_code << std::endl;
                std::cout << std::endl;
            } else {
                // 빈 줄이나 주석만 있는 경우는 에러로 표시하지 않음
                if (result.error_message != "Empty line or comment only") {
                    std::cout << "Line " << line_number << " ERROR: " << line << std::endl;
                    std::cout << "Error: " << result.error_message << std::endl;
                    std::cout << std::endl;
                }
            }
        }
    }
};

int main() {
    Chip8_32_Assembler assembler;
    std::string input;
    
    std::cout << "Chip-8 32bit Extension Assembler" << std::endl;
    std::cout << "Enter assembly instructions (type 'quit' to exit):" << std::endl;
    std::cout << "Type 'help' for available instructions" << std::endl;
    std::cout << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);
        
        if (input == "quit" || input == "exit") {
            break;
        }
        
        if (input == "help") {
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
            std::cout << std::endl;
            continue;
        }
        
        if (!input.empty()) {
            AssemblyResult result = assembler.assembleLine(input);
            if (result.success) {
                std::cout << "0x" << std::hex << std::uppercase 
                         << std::setw(8) << std::setfill('0') << result.machine_code << std::endl;
            } else {
                std::cout << "❌ Error: " << result.error_message << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    // 예시 테스트 - 에러 케이스 포함
    std::cout << "\n=== Example Tests (Including Error Cases) ===" << std::endl;
    std::string test_program = R"(
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
    
    assembler.assemble(test_program);
    
    return 0;
}
