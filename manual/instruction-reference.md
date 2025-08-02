# 명령어 레퍼런스: 8비트에서 32비트로의 진화

E.P.A의 기존 8비트 명령어 세트가 어떻게 32비트 확장 명령어로 진화했는지 상세히 설명합니다. 기존 CHIP-8 프로그래머들이 32비트 모드로 쉽게 마이그레이션할 수 있도록 비교 분석을 제공합니다.

## 개요

기존 CHIP-8은 35개의 명령어로 구성된 간단한 명령어 세트를 가지고 있었습니다. 32비트 확장에서는 이 모든 명령어를 유지하면서도 현대적인 프로그래밍을 위한 새로운 명령어들을 대폭 추가했습니다.

## 명령어 진화 개요

### 기본 구조 변화

| 특징 | 8비트 CHIP-8 | 32비트 확장 |
| --- | --- | --- |
| 명령어 크기 | 2바이트 | 4바이트 |
| 주소 공간 | 12비트 (4KB) | 32비트 (64KB) |
| 레지스터 수 | 16개 (V0-VF) | 32개 (R0-R31) |
| 레지스터 크기 | 8비트 | 32비트 |
| 즉시값 범위 | 8비트/12비트 | 16비트/24비트 |
| 스택 크기 | 16단계 | 32KB 동적 |

### 명령어 분류

```
8비트 CHIP-8 (35개 명령어)
├── 시스템 제어 (2개): 00E0, 00EE
├── 점프/호출 (3개): 1NNN, 2NNN, BNNN
├── 조건 분기 (6개): 3XNN, 4XNN, 5XY0, 9XY0, EX9E, EXA1
├── 레지스터 연산 (15개): 6XNN, 7XNN, 8XY0~8XYE
├── 메모리/인덱스 (2개): ANNN, FX1E
├── 난수/그래픽 (2개): CXNN, DXYN
└── 타이머/입출력 (5개): FX07, FX0A, FX15, FX18, FX29, FX33, FX55, FX65

32비트 확장 (70+ 개 명령어)
├── 호환 명령어 (35개): 기존 명령어 32비트 확장
├── 스택 연산 (12개): PUSH, POP, CALL_FUNC, RET_FUNC 등
├── 메모리 연산 (8개): MOV [EBP+offset], 32비트 로드/스토어
├── 산술 확장 (10개): 32비트 사칙연산, 비트연산
├── 시스템 호출 (5개): SYSCALL, I/O 리다이렉션
└── 고급 제어 (15개): 32비트 점프, 조건 분기 등

```

## 기존 명령어의 32비트 확장

### 시스템 제어 명령어

### 00E0 - 화면 지우기

**8비트 버전:**

```
00E0: 화면 지우기 (64x32 픽셀)

```

**32비트 확장:**

```cpp
// 동일한 기능이지만 32비트 명령어 형식으로
// 0x00000000E0: 화면 지우기 (64x32 픽셀, 호환성 유지)

void OP_00E0_32(Chip8_32& chip8_32, uint32_t opcode) {
    // 비디오 메모리 초기화 (동일한 64x32 해상도)
    std::memset(chip8_32.video.data(), 0, sizeof(chip8_32.video));
    chip8_32.draw_flag = true;
    chip8_32.set_pc(chip8_32.get_pc() + 4);  // 4바이트 증가
}

```

### 00EE - 서브루틴 반환

**8비트 버전:**

```
00EE: 스택에서 주소를 꺼내 PC에 설정
- sp 감소
- PC = stack[sp]

```

**32비트 확장:**

```cpp
// 기본 반환은 동일하지만 32비트 주소 지원
void OP_00EE_32(Chip8_32& chip8_32, uint32_t opcode) {
    if (chip8_32.get_sp() > 0) {
        chip8_32.set_sp(chip8_32.get_sp() - 1);
        uint32_t return_addr = chip8_32.stack_at(chip8_32.get_sp());
        chip8_32.set_pc(return_addr);  // 32비트 주소 지원
    }
}

// 새로운 스택 프레임 반환 (x86 스타일)
void OP_RET_FUNC_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t return_addr;
    if (pop_stack(chip8_32, return_addr)) {
        chip8_32.set_pc(return_addr);
    }
}

```

### 점프 및 호출 명령어

### 1NNN - 무조건 점프

**8비트 버전:**

```
1NNN: PC = NNN (12비트 주소)
- 주소 범위: 0x000 ~ 0xFFF (4KB)

```

**32비트 확장:**

```cpp
// 8비트 호환 (12비트 주소)
void OP_1NNN_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00000FFF;  // 하위 12비트만 사용 (호환성)
    chip8_32.set_pc(addr);
}

// 새로운 32비트 점프 (24비트 주소)
void OP_JMP_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00FFFFFF;  // 하위 24비트 사용 (16MB 범위)
    chip8_32.set_pc(addr);
}

```

### 2NNN - 서브루틴 호출

**8비트 버전:**

```
2NNN: 서브루틴 호출
- stack[sp] = PC
- sp++
- PC = NNN

```

**32비트 확장:**

```cpp
// 8비트 호환 서브루틴 호출
void OP_2NNN_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00000FFF;

    // 기존 스택 사용 (16단계 호환)
    chip8_32.stack_at(chip8_32.get_sp()) = chip8_32.get_pc() + 4;
    chip8_32.set_sp(chip8_32.get_sp() + 1);
    chip8_32.set_pc(addr);
}

// 새로운 x86 스타일 함수 호출
void OP_CALL_FUNC_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t func_addr = opcode & 0x00FFFFFF;
    uint32_t return_addr = chip8_32.get_pc() + 4;

    // x86 스타일 스택 프레임 사용
    if (push_stack(chip8_32, return_addr)) {
        chip8_32.set_pc(func_addr);
    }
}

```

### 레지스터 연산 명령어

### 6XNN - 레지스터에 값 설정

**8비트 버전:**

```
6XNN: VX = NN (8비트 값)
- VX: V0~VF (16개 레지스터)
- NN: 0x00~0xFF

```

**32비트 확장:**

```cpp
// 8비트 호환 (8비트 값을 32비트 레지스터에)
void OP_6XNN_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x00000F00) >> 8;
    uint8_t value = opcode & 0x000000FF;

    if (reg_x < 16) {  // V0~VF 호환성
        chip8_32.set_R(reg_x, value);  // 8비트 값을 32비트 레지스터에
    }
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// 새로운 32비트 즉시값 로드
void OP_MOV_RX_IMM32(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x1F000000) >> 24;  // 5비트로 R0~R31 지원
    uint32_t value = opcode & 0x00FFFFFF;         // 24비트 즉시값

    chip8_32.set_R(reg_x, value);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 8XY0~8XYE - 레지스터 간 연산

**8비트 버전:**

```
8XY0: VX = VY           (복사)
8XY1: VX = VX | VY      (OR)
8XY2: VX = VX & VY      (AND)
8XY3: VX = VX ^ VY      (XOR)
8XY4: VX = VX + VY      (덧셈, 캐리 플래그)
8XY5: VX = VX - VY      (뺄셈, 보로우 플래그)
8XY6: VX = VY >> 1      (우측 시프트)
8XY7: VX = VY - VX      (역뺄셈)
8XYE: VX = VY << 1      (좌측 시프트)

```

**32비트 확장:**

```cpp
// 8비트 호환 버전들 (VF는 플래그 레지스터로 유지)
void OP_8XY4_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x00000F00) >> 8;
    uint8_t reg_y = (opcode & 0x000000F0) >> 4;

    if (reg_x < 16 && reg_y < 16) {  // V0~VF 호환
        uint32_t sum = chip8_32.get_R(reg_x) + chip8_32.get_R(reg_y);
        chip8_32.set_R(15, sum > 0xFF ? 1 : 0);  // VF에 캐리 플래그
        chip8_32.set_R(reg_x, sum & 0xFF);       // 8비트로 제한 (호환성)
    }
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// 새로운 32비트 산술 연산
void OP_ADD_RX_RY_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x1F000000) >> 24;  // 5비트
    uint8_t reg_y = (opcode & 0x001F0000) >> 16;  // 5비트

    uint64_t sum = (uint64_t)chip8_32.get_R(reg_x) + chip8_32.get_R(reg_y);
    chip8_32.set_R(31, sum > 0xFFFFFFFF ? 1 : 0);  // R31에 캐리 플래그
    chip8_32.set_R(reg_x, (uint32_t)sum);          // 32비트 전체 사용
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 메모리 및 인덱스 명령어

### ANNN - 인덱스 레지스터 설정

**8비트 버전:**

```
ANNN: I = NNN (12비트 주소)
- I 레지스터: 16비트
- 주소 범위: 0x000~0xFFF

```

**32비트 확장:**

```cpp
// 8비트 호환
void OP_ANNN_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00000FFF;
    chip8_32.set_I(addr);  // I는 32비트이지만 하위 12비트만 사용
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// 새로운 32비트 인덱스 설정
void OP_SET_I_32(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00FFFFFF;  // 24비트 주소
    chip8_32.set_I(addr);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

## 새로운 32비트 전용 명령어

### 스택 프레임 연산

### PUSH/POP 연산

```cpp
// PUSH Rx - 레지스터를 스택에 푸시
// 명령어 형식: 0xE0 XX 00 00 (XX = 레지스터 번호)
void OP_PUSH_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x00FF0000) >> 16;
    uint32_t value = chip8_32.get_R(reg_index);

    if (push_stack(chip8_32, value)) {
        LOG_DEBUG("[STACK] PUSH R" << (int)reg_index << "=0x" << std::hex << value);
    } else {
        std::cerr << "[ERROR] 스택 오버플로우!" << std::endl;
    }
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// POP Rx - 스택에서 레지스터로 팝
// 명령어 형식: 0xE1 XX 00 00
void OP_POP_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x00FF0000) >> 16;
    uint32_t value;

    if (pop_stack(chip8_32, value)) {
        chip8_32.set_R(reg_index, value);
        LOG_DEBUG("[STACK] POP R" << (int)reg_index << "=0x" << std::hex << value);
    } else {
        std::cerr << "[ERROR] 스택 언더플로우!" << std::endl;
    }
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 스택 포인터 조작

```cpp
// SUB ESP, imm16 - 스택 공간 할당
// 명령어 형식: 0xE2 00 XX XX (XXXX = 할당할 바이트 수)
void OP_SUB_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    uint32_t esp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_esp = esp - value;

    if (StackFrame::check_stack_overflow(new_esp)) {
        std::cerr << "[ERROR] SUB ESP 스택 오버플로우" << std::endl;
        return;
    }

    chip8_32.set_R(StackFrame::RSP_INDEX, new_esp);
    LOG_DEBUG("[STACK] SUB ESP, " << value << " (ESP: 0x" << std::hex << esp
              << " -> 0x" << new_esp << ")");
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// ADD ESP, imm16 - 스택 공간 해제
// 명령어 형식: 0xE3 00 XX XX
void OP_ADD_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    uint32_t esp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_esp = esp + value;

    chip8_32.set_R(StackFrame::RSP_INDEX, new_esp);
    LOG_DEBUG("[STACK] ADD ESP, " << value);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 메모리 간접 주소 지정

### EBP 기반 메모리 접근

```cpp
// MOV [EBP+offset], Rx - 지역변수에 저장
// 명령어 형식: 0xE4 RR OO 00 (RR=레지스터, OO=오프셋)
void OP_MOV_RBP_PLUS_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x00FF0000) >> 16;
    uint8_t offset = (opcode & 0x0000FF00) >> 8;

    uint32_t ebp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = ebp + offset;

    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] 메모리 접근 범위 초과" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }

    uint32_t reg_value = chip8_32.get_R(reg_index);

    // 32비트 값을 빅엔디안으로 저장
    chip8_32.set_memory(addr + 0, (reg_value >> 24) & 0xFF);
    chip8_32.set_memory(addr + 1, (reg_value >> 16) & 0xFF);
    chip8_32.set_memory(addr + 2, (reg_value >> 8) & 0xFF);
    chip8_32.set_memory(addr + 3, reg_value & 0xFF);

    LOG_DEBUG("[MEMORY] MOV [EBP+" << (int)offset << "], R" << (int)reg_index
              << " (0x" << std::hex << reg_value << " -> [0x" << addr << "])");
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// MOV Rx, [EBP+offset] - 지역변수에서 로드
// 명령어 형식: 0xE5 RR OO 00
void OP_MOV_RX_RBP_PLUS(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x00FF0000) >> 16;
    uint8_t offset = (opcode & 0x0000FF00) >> 8;

    uint32_t ebp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = ebp + offset;

    if (addr + 3 >= MEMORY_SIZE_32) {
        std::cerr << "[ERROR] 메모리 접근 범위 초과" << std::endl;
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }

    // 빅엔디안으로 32비트 값 읽기
    uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                     (chip8_32.get_memory(addr + 1) << 16) |
                     (chip8_32.get_memory(addr + 2) << 8) |
                     chip8_32.get_memory(addr + 3);

    chip8_32.set_R(reg_index, value);

    LOG_DEBUG("[MEMORY] MOV R" << (int)reg_index << ", [EBP+" << (int)offset
              << "] (0x" << std::hex << value << " <- [0x" << addr << "])");
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 고급 산술 연산

### 32비트 곱셈과 나눗셈

```cpp
// MUL Rx, Ry - 32비트 곱셈
// 명령어 형식: 0xF0 XX YY 00
void OP_MUL_RX_RY(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
    uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

    uint64_t result = (uint64_t)chip8_32.get_R(reg_x) * chip8_32.get_R(reg_y);

    chip8_32.set_R(reg_x, (uint32_t)(result & 0xFFFFFFFF));      // 하위 32비트
    chip8_32.set_R(31, (uint32_t)((result >> 32) & 0xFFFFFFFF)); // 상위 32비트 (R31에)

    LOG_DEBUG("[ALU] MUL R" << (int)reg_x << ", R" << (int)reg_y
              << " = 0x" << std::hex << result);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// DIV Rx, Ry - 32비트 나눗셈
// 명령어 형식: 0xF1 XX YY 00
void OP_DIV_RX_RY(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
    uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

    uint32_t dividend = chip8_32.get_R(reg_x);
    uint32_t divisor = chip8_32.get_R(reg_y);

    if (divisor == 0) {
        std::cerr << "[ERROR] 0으로 나누기 시도!" << std::endl;
        chip8_32.set_R(31, 1);  // 에러 플래그
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        return;
    }

    uint32_t quotient = dividend / divisor;
    uint32_t remainder = dividend % divisor;

    chip8_32.set_R(reg_x, quotient);   // 몫
    chip8_32.set_R(30, remainder);     // 나머지 (R30에)
    chip8_32.set_R(31, 0);             // 성공 플래그

    LOG_DEBUG("[ALU] DIV R" << (int)reg_x << ", R" << (int)reg_y
              << " = " << quotient << " 나머지 " << remainder);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

### 조건부 점프 확장

### 32비트 레지스터 비교

```cpp
// CMP Rx, Ry - 32비트 레지스터 비교
// 명령어 형식: 0xF2 XX YY 00
void OP_CMP_RX_RY(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
    uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

    uint32_t val_x = chip8_32.get_R(reg_x);
    uint32_t val_y = chip8_32.get_R(reg_y);

    // 플래그 레지스터 (R31) 설정
    uint32_t flags = 0;
    if (val_x == val_y) flags |= 0x01;  // Zero flag
    if (val_x > val_y)  flags |= 0x02;  // Greater flag
    if (val_x < val_y)  flags |= 0x04;  // Less flag

    chip8_32.set_R(31, flags);

    LOG_DEBUG("[CMP] R" << (int)reg_x << "(0x" << std::hex << val_x
              << ") vs R" << (int)reg_y << "(0x" << val_y
              << ") flags=0x" << flags);
    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

// JEQ addr - 같으면 점프
// 명령어 형식: 0xF3 XX XX XX (XXXXXX = 24비트 주소)
void OP_JEQ(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t addr = opcode & 0x00FFFFFF;
    uint32_t flags = chip8_32.get_R(31);

    if (flags & 0x01) {  // Zero flag 체크
        chip8_32.set_pc(addr);
        LOG_DEBUG("[JUMP] JEQ 0x" << std::hex << addr << " (taken)");
    } else {
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        LOG_DEBUG("[JUMP] JEQ 0x" << std::hex << addr << " (not taken)");
    }
}

```

### SYSCALL 명령어

```cpp
// SYSCALL imm16 - 시스템 호출
// 명령어 형식: 0xA0 XX XX XX (XXXXXX = 시스템 호출 번호)
void OP_SYSCALL(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t syscall_number = opcode & 0x00FFFFFF;

    LOG_DEBUG("[SYSCALL] 0x" << std::hex << syscall_number << " 호출");

    switch (syscall_number) {
        case 0x0010:
            syscall_file_select(chip8_32);
            break;
        case 0x0020:
            syscall_file_load(chip8_32);
            break;
        case 0x0030:
            syscall_mode_switch(chip8_32);
            break;
        case 0x0040:
            syscall_read(chip8_32);
            break;
        case 0x0041:
            syscall_write(chip8_32);
            break;
        default:
            std::cerr << "[ERROR] 알 수 없는 SYSCALL: 0x"
                      << std::hex << syscall_number << std::endl;
            chip8_32.set_R(0, 0);  // 실패 코드
            break;
    }

    chip8_32.set_pc(chip8_32.get_pc() + 4);
}

```

## 명령어 인코딩 비교

### 8비트 명령어 인코딩

```
8비트 CHIP-8 명령어 형식 (2바이트):
┌────┬────┬────┬────┐
│ OP │ X  │ Y  │ N  │  
└────┴────┴────┴────┘      
 4bit 4bit 4bit 4bit           

예시:
8XY4: │1000│ X │ Y │0100│ = VX = VX + VY
ANNN: │1010│   NNN   │ = I = NNN

```

### 32비트 명령어 인코딩

```
32비트 확장 명령어 형식 (4바이트):
┌────────┬────────┬────────┬────────┐
│   OP   │   X    │   Y    │   IMM  │  기본 형식
└────────┴────────┴────────┴────────┘
  8bit     8bit     8bit     8bit

┌────────┬──────────────────────────┐
│   OP   │         24bit IMM        │  즉시값 형식
└────────┴──────────────────────────┘
  8bit            24bit

┌────────┬────────┬─────────────────┐
│   OP   │   REG  │     16bit IMM   │  레지스터+즉시값 형식
└────────┴────────┴─────────────────┘
  8bit     8bit          16bit

예시:
E0 1C 00 00: PUSH R28 (EBP)
E4 05 08 00: MOV [EBP+8], R5
F0 03 07 00: MUL R3, R7
A0 00 10 00: SYSCALL 0x0010

```

## 호환성 매트릭스

### 8비트 → 32비트 마이그레이션

| 8비트 명령어 | 32비트 호환 | 32비트 확장 | 비고 |
| --- | --- | --- | --- |
| 00E0 | 완전 호환 | 00 00 00 E0 | 동일한 64x32 해상도 |
| 00EE | 완전 호환 | 00 00 00 EE | 기존 스택과 새 스택 모두 지원 |
| 1NNN | 완전 호환 | 1N NN 00 00 | 12비트 주소 유지, 24비트 확장 가능 |
| 2NNN | 완전 호환 | 2N NN 00 00 | 기존 호출 + CALL_FUNC 추가 |
| 3XNN | 완전 호환 | 3X NN 00 00 | 8비트 비교, 32비트 CMP 추가 |
| 6XNN | 완전 호환 | 6X NN 00 00 | 8비트 값, 32비트 MOV 추가 |
| 8XY4 | 완전 호환 | 8X Y4 00 00 | 8비트 덧셈, 32비트 ADD 추가 |
| ANNN | 완전 호환 | AN NN 00 00 | 12비트 주소, 24비트 확장 가능 |
| DXYN | 완전 호환 | DX YN 00 00 | 동일한 스프라이트 시스템 |

### 새로운 32비트 전용 명령어

| 명령어 | 인코딩 | 기능 | x86 대응 |
| --- | --- | --- | --- |
| PUSH Rx | E0 XX 00 00 | 스택에 푸시 | push eax |
| POP Rx | E1 XX 00 00 | 스택에서 팝 | pop eax |
| SUB ESP, imm | E2 00 XX XX | 스택 할당 | sub esp, 16 |
| ADD ESP, imm | E3 00 XX XX | 스택 해제 | add esp, 16 |
| MOV [EBP+off], Rx | E4 XX YY 00 | 지역변수 저장 | mov [ebp-8], eax |
| MOV Rx, [EBP+off] | E5 XX YY 00 | 지역변수 로드 | mov eax, [ebp-8] |
| CALL_FUNC addr | E6 XX XX XX | 함수 호출 | call function |
| RET_FUNC | E7 00 00 00 | 함수 반환 | ret |
| MUL Rx, Ry | F0 XX YY 00 | 32비트 곱셈 | mul ebx |
| DIV Rx, Ry | F1 XX YY 00 | 32비트 나눗셈 | div ebx |
| CMP Rx, Ry | F2 XX YY 00 | 32비트 비교 | cmp eax, ebx |
| JEQ addr | F3 XX XX XX | 같으면 점프 | je label |
| JNE addr | F4 XX XX XX | 다르면 점프 | jne label |
| JGT addr | F5 XX XX XX | 크면 점프 | jg label |
| JLT addr | F6 XX XX XX | 작으면 점프 | jl label |
| SYSCALL num | A0 XX XX XX | 시스템 호출 | int 0x80 |

## 실제 코드 변환 예제

### 간단한 덧셈 함수

**8비트 CHIP-8 버전:**

```
; add_two_numbers: V0 + V1 → V2
add_function:
    8024        ; V0 = V0 + V1 (8비트 덧셈)
    8020        ; V2 = V0 (결과 복사)
    00EE        ; 반환

; 호출부
6005        ; V0 = 5
6103        ; V1 = 3
2300        ; CALL add_function (0x300)

```

**32비트 확장 버전 (호환 모드):**

```
; 기존 8비트 코드가 그대로 동작
add_function:
    80240000    ; V0 = V0 + V1 (32비트 명령어 형식, 8비트 연산)
    80200000    ; V2 = V0
    0000000EE   ; 반환

; 호출부
60050000    ; V0 = 5
61030000    ; V1 = 3
23000000    ; CALL add_function (0x300)

```

**32비트 확장 버전 (네이티브 모드):**

```
; x86 스타일 함수 (매개변수를 스택으로 전달)
add_function:
    E01C0000    ; PUSH R28 (EBP)           - push ebp
    8C1C1D00    ; MOV R28, R29 (EBP=ESP)   - mov ebp, esp
    E2000800    ; SUB ESP, 8               - sub esp, 8 (지역변수 공간)

    E5020800    ; MOV R2, [EBP+8]         - mov eax, [ebp+8]  (첫 번째 인수)
    E503C0000   ; MOV R3, [EBP+12]        - mov ebx, [ebp+12] (두 번째 인수)
    F0020300    ; ADD R2, R3              - add eax, ebx
    E402FC00    ; MOV [EBP-4], R2         - mov [ebp-4], eax  (결과 저장)

    851C0200    ; MOV R0, [EBP-4]         - mov eax, [ebp-4]  (반환값)
    8D1D1C00    ; MOV R29, R28            - mov esp, ebp
    E11C0000    ; POP R28                 - pop ebp
    E7000000    ; RET_FUNC                - ret

; 호출부
E0050000    ; PUSH 5                   - push 5
E0030000    ; PUSH 3                   - push 3
E6000300    ; CALL_FUNC add_function   - call add_function
E3000800    ; ADD ESP, 8               - add esp, 8 (인수 정리)

```

### 팩토리얼 계산 (재귀)

**8비트 CHIP-8에서는 불가능 (스택 제한)**

**32비트 확장 버전:**

```
; factorial(n) - 재귀 구현
factorial:
    E01C0000    ; PUSH EBP
    8C1C1D00    ; MOV EBP, ESP
    E2001000    ; SUB ESP, 16 (지역변수 4개 공간)

    E5000800    ; MOV R0, [EBP+8]         ; n 로드
    F2000100    ; CMP R0, 1               ; n과 1 비교
    F4001800    ; JLE base_case           ; n <= 1이면 기저 사례

    ; 재귀 사례: n * factorial(n-1)
    80000100    ; SUB R0, 1               ; n - 1
    E0000000    ; PUSH R0                 ; factorial(n-1) 인수
    E6000000    ; CALL_FUNC factorial     ; 재귀 호출
    E3000400    ; ADD ESP, 4              ; 인수 정리

    E5010800    ; MOV R1, [EBP+8]        ; 원래 n 로드
    F0000100    ; MUL R0, R1              ; result = n * factorial(n-1)
    F3002000    ; JMP end_function

base_case:
    6C000100    ; MOV R0, 1               ; factorial(1) = 1

end_function:
    8D1D1C00    ; MOV ESP, EBP
    E11C0000    ; POP EBP
    E7000000    ; RET_FUNC

```

### I/O 사용 계산기

**8비트 CHIP-8에서는 불가능 (I/O 없음)**

**32비트 확장 버전:**

```
calculator_main:
    E01C0000    ; PUSH EBP
    8C1C1D00    ; MOV EBP, ESP
    E2004000    ; SUB ESP, 64 (입력 버퍼)

input_loop:
    ; 프롬프트 출력
    6C010000    ; MOV R1, 1 (stdout)
    6C02prompt  ; MOV R2, prompt_address
    6C031400    ; MOV R3, 20 (길이)
    A0004100    ; SYSCALL 0x0041 (write)

    ; 사용자 입력
    6C000000    ; MOV R0, 0 (stdin)
    8C021D00    ; MOV R2, ESP (버퍼 주소)
    6C033F00    ; MOV R3, 63 (최대 길이)
    A0004000    ; SYSCALL 0x0040 (read)

    ; 입력 파싱
    E0021D00    ; PUSH ESP (입력 문자열)
    E6000000    ; CALL_FUNC parse_expression
    E3000400    ; ADD ESP, 4

    ; 결과 출력
    E0000000    ; PUSH R0 (결과)
    E6000000    ; CALL_FUNC print_number
    E3000400    ; ADD ESP, 4

    F3001000    ; JMP input_loop

```

## 디버깅 도구

### 명령어 역어셈블러

```cpp
std::string disassemble_32bit(uint32_t opcode) {
    uint8_t op = (opcode & 0xFF000000) >> 24;

    switch (op) {
        case 0x00:
            if ((opcode & 0x00FFFFFF) == 0x0000E0) return "CLS";
            if ((opcode & 0x00FFFFFF) == 0x0000EE) return "RET";
            break;

        case 0x10: {
            uint32_t addr = opcode & 0x00FFFFFF;
            return "JMP 0x" + toHex(addr);
        }

        case 0x60: {
            uint8_t reg = (opcode & 0x00FF0000) >> 16;
            uint8_t val = (opcode & 0x0000FF00) >> 8;
            return "MOV V" + std::to_string(reg) + ", " + std::to_string(val);
        }

        case 0xE0: {
            uint8_t reg = (opcode & 0x00FF0000) >> 16;
            return "PUSH R" + std::to_string(reg);
        }

        case 0xE1: {
            uint8_t reg = (opcode & 0x00FF0000) >> 16;
            return "POP R" + std::to_string(reg);
        }

        case 0xF0: {
            uint8_t rx = (opcode & 0x00FF0000) >> 16;
            uint8_t ry = (opcode & 0x0000FF00) >> 8;
            return "MUL R" + std::to_string(rx) + ", R" + std::to_string(ry);
        }

        case 0xA0: {
            uint32_t syscall_num = opcode & 0x00FFFFFF;
            return "SYSCALL 0x" + toHex(syscall_num);
        }

        default:
            return "UNKNOWN 0x" + toHex(opcode);
    }

    return "INVALID";
}

```

### 실행 추적

```bash
# 디버그 모드에서 명령어 추적
(debug) trace on
명령어 추적 활성화

# 실행 예시
PC:0x0204 [E01C0000] PUSH R28          R28=0x0000EFF0 ESP=0x0000EFEC
PC:0x0208 [8C1C1D00] MOV R28, R29      R28=0x0000EFEC R29=0x0000EFEC
PC:0x020C [E2001000] SUB ESP, 16       ESP=0x0000EFEC -> 0x0000EFDC
PC:0x0210 [E5000800] MOV R0, [EBP+8]   R0=0x00000005 [0x0000EFF4]=0x00000005

# 스택 상태 확인
(debug) sf
=== 스택 프레임 시각화 ===
EBP: 0x0000EFEC  ESP: 0x0000EFDC
0xEFF0 │ 0x00000005 │ 매개변수 1
0xEFEC │ 0x0000EFF0 │ 저장된 EBP
0xEFE8 │ 0x00000210 │ 반환 주소
0xEFE4 │ 0x00000000 │ 지역변수
0xEFE0 │ 0x00000000 │ 지역변수
0xEFDC │            │ ESP

```

## 최적화 팁

### 8비트 호환성 유지

```cpp
// 8비트 코드를 그대로 사용하면서 성능 향상
class OptimizedEmulator {
    bool use_32bit_registers = false;

public:
    void optimize_for_8bit() {
        // 8비트 연산만 사용하는 경우 최적화
        if (all_operations_8bit()) {
            use_32bit_registers = false;
        }
    }

    void handle_vx_operation(uint8_t vx, uint8_t value) {
        if (use_32bit_registers) {
            R[vx] = value;  // 32비트 레지스터 사용
        } else {
            V[vx] = value;  // 8비트 레지스터 사용 (더 빠름)
        }
    }
};

```

### 스택 프레임 최적화

```cpp
// 간단한 함수는 스택 프레임 생략
void optimized_simple_function() {
    // 지역변수가 없고 다른 함수를 호출하지 않는 경우
    // 프롤로그/에필로그 생략 가능

    // PUSH EBP    <- 생략
    // MOV EBP, ESP <- 생략

    // 실제 작업만 수행
    // 결과를 R0에 저장

    // MOV ESP, EBP <- 생략
    // POP EBP     <- 생략
    // RET_FUNC만 실행
}

```

## 결론

E.P.A의 32비트 확장은 단순한 명령어 확장을 넘어서 새로운 가능성을 제공합니다. : 

### 주요 혁신점

1. **완전한 하위 호환성**: 모든 8비트 코드가 수정 없이 동작
2. **x86 스타일 아키텍처**: 현대 프로그래머에게 친숙한 환경과 상대 주소 연산의 가능성 제공
3. **확장성**: SYSCALL을 통한 새로운 기능 확장 가능
4. **디버깅 지원**: 전문적인 개발 도구 제공

### 활용 분야

- **교육**: 어셈블리 언어와 시스템 프로그래밍 학습
- **프로토타이핑**: 간단한 알고리즘 구현 및 테스트
- **게임 개발 가능**
- **임베디드 시뮬레이션**: 제한된 환경에서의 프로그래밍 연습

### 다음 단계

1. **실습**: `roms/CALC*.ch32` 파일들을 분석하여 실제 사용법 학습
2. **개발**: 자신만의 32비트 CHIP-8 프로그램 작성
3. **확장**: 새로운 SYSCALL이나 명령어 추가 기여
4. **최적화**: 성능 향상을 위한 컴파일러 기법 연구

---

## 참고 자료

- [설치 가이드](https://claude.ai/chat/installation.md) - 개발 환경 구축
- [스택 프레임](https://claude.ai/chat/stack-frames.md) - x86 스타일 스택 관리
- [SYSCALL 시스템](https://claude.ai/chat/syscall.md) - BootROM과 시스템 호출
- Intel x86 Instruction Set Reference
- 프로젝트 소스: `src/core/opcode_table_32.cpp`, `include/core/stack_opcodes.hpp`
