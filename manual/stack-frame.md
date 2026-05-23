# x86 스타일 스택 프레임 시스템

 32비트 확장 모드의 x86 스타일 스택 프레임 시스템에 대해 설명합니다.

## 개요

기존 CHIP-8의 단순한 16단계 스택을 x86 프로세서의 호출 규약을 기반으로 한 동적 스택 프레임 시스템으로 확장했습니다. Intel x86 아키텍처의 EBP, ESP, EIP 레지스터 개념을 도입하여 복잡한 함수 호출, 지역 변수 관리, 그리고 전문적인 프로그래밍이 가능해졌습니다.

## 스택 프레임 아키텍처

### 메모리 레이아웃

```
32비트 CHIP-8 메모리 맵:
┌─────────────────────────────────────┐ 0xFFFF
│          I/O & 시스템 레지스터       │
├─────────────────────────────────────┤ 0xF000
│                                     │
│           스택 영역 (32KB)          │ ← 스택이 여기서 동작
│         (아래로 확장)               │
│                                     │
├─────────────────────────────────────┤ 0x8000 (STACK_END)
│        확장 프로그램 영역           │
├─────────────────────────────────────┤ 0x1000
│      프로그램 영역 (호환성)          │
├─────────────────────────────────────┤ 0x0200
│       BootROM & 시스템 영역         │
└─────────────────────────────────────┘ 0x0000

```

### x86 스타일 스택 관리 레지스터

```cpp
// include/core/stack_frame.hpp에서 정의
constexpr uint8_t RBP_INDEX = 28;  // R28 → EBP (Extended Base Pointer)
constexpr uint8_t RSP_INDEX = 29;  // R29 → ESP (Extended Stack Pointer)
constexpr uint8_t RIP_INDEX = 30;  // R30 → EIP (Extended Instruction Pointer)
constexpr uint8_t RESERVED_INDEX = 31;  // R31 (예약됨)

```

**EBP (R28) - Extended Base Pointer**

- Intel x86의 EBP 레지스터와 동일한 역할
- 현재 함수의 스택 프레임 시작점 (Stack Frame Base)
- 지역 변수와 매개변수 접근의 기준점
- `[EBP+4]`, `[EBP-8]` 형태로 오프셋 접근

**ESP (R29) - Extended Stack Pointer**

- Intel x86의 ESP 레지스터와 동일한 역할
- 스택의 현재 최상위 위치 (Stack Top)
- PUSH/POP 연산에 따라 동적으로 변경
- `SUB ESP, 12` 형태로 스택 공간 할당

**EIP (R30) - Extended Instruction Pointer**

- Intel x86의 EIP 레지스터와 동일한 역할
- 다음 실행할 명령어의 주소
- 함수 호출 시 반환 주소로 스택에 저장

## x86 스타일 스택 프레임 구조

### 표준 스택 프레임 레이아웃 (Intel x86 호환)

```
스택 증가 방향 (높은 주소 → 낮은 주소)

더 높은 주소       ┌─────────────────┐ ← 0xEFFF (STACK_START)
                   │   이전 함수의   │
                   │   스택 프레임   │
                   ├─────────────────┤
                   │  매개변수 #n    │ ← [EBP + 8 + 4*n]
                   │  매개변수 #2    │ ← [EBP + 12]
                   │  매개변수 #1    │ ← [EBP + 8]
                   │   반환 주소     │ ← [EBP + 4]
                   ├─────────────────┤ ← EBP (현재 프레임 시작)
                   │  저장된 EBP     │ ← [EBP + 0]
                   ├─────────────────┤
                   │  지역변수 #1    │ ← [EBP - 4]
                   │  지역변수 #2    │ ← [EBP - 8]
                   │  지역변수 #n    │ ← [EBP - 4*n]
                   ├─────────────────┤ ← ESP (현재 스택 탑)
                   │                 │
                   │   빈 공간       │
더 낮은 주소        └─────────────────┘ ← 0x8000 (STACK_END)

```

### 함수 호출 규약 (x86 Calling Convention)

### 1. 함수 호출 과정 (CALL 명령어)

```
; 의사 코드 (실제 32비트 CHIP-8 명령어)
PUSH 매개변수_3       ; 매개변수를 오른쪽부터 스택에 푸시
PUSH 매개변수_2
PUSH 매개변수_1
CALL_FUNC 함수주소    ; 반환 주소를 푸시하고 점프

```

```cpp
// src/core/stack_opcodes.cpp - CALL_FUNC 구현
void OP_CALL_FUNC(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t func_addr = opcode & 0x0000FFFF;

    // 1. 현재 EIP (반환 주소)를 스택에 푸시
    uint32_t return_addr = chip8_32.get_pc() + 4;
    if (push_stack(chip8_32, return_addr)) {
        // 2. 함수로 점프
        chip8_32.set_pc(func_addr);
    }
}

```

### 2. 함수 진입 시 프롤로그 (Function Prologue)

```
; 표준 x86 함수 프롤로그
PUSH EBP              ; 이전 EBP 저장
MOV EBP, ESP          ; 새 프레임 베이스 설정
SUB ESP, 지역변수크기  ; 지역 변수 공간 할당

```

```cpp
// 실제 32비트 CHIP-8 구현 예제
void function_prologue_example() {
    // PUSH R28 (EBP)        - 이전 EBP 저장
    // MOV R28, R29          - EBP = ESP
    // SUB RSP, 16           - 지역변수 4개 (4바이트씩) 공간 할당
}

```

### 3. 함수 종료 시 에필로그 (Function Epilogue)

```
; 표준 x86 함수 에필로그
MOV ESP, EBP          ; 스택 포인터 복원
POP EBP               ; 이전 EBP 복원
RET_FUNC              ; 반환 주소로 점프

```

```cpp
// src/core/stack_opcodes.cpp - RET_FUNC 구현
void OP_RET_FUNC(Chip8_32& chip8_32, uint32_t opcode) {
    uint32_t return_addr;
    if (pop_stack(chip8_32, return_addr)) {
        chip8_32.set_pc(return_addr);  // 반환 주소로 점프
    }
}

```

## 스택 연산 명령어

### 기본 스택 연산

### PUSH 연산

```cpp
// PUSH Rx - 레지스터 값을 스택에 푸시
void OP_PUSH_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x0F00) >> 8;
    uint32_t value = chip8_32.get_R(reg_index);
    push_stack(chip8_32, value);
}

```

### POP 연산

```cpp
// POP Rx - 스택에서 값을 꺼내 레지스터에 저장
void OP_POP_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index = (opcode & 0x0F00) >> 8;
    uint32_t value;
    if (pop_stack(chip8_32, value)) {
        chip8_32.set_R(reg_index, value);
    }
}

```

### 스택 포인터 조작

### SUB ESP (스택 공간 할당)

```cpp
// SUB ESP, imm16 - 스택 공간 할당 (x86의 SUB ESP, n과 동일)
void OP_SUB_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    uint32_t esp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_esp = esp - value;

    if (StackFrame::check_stack_overflow(new_esp)) {
        std::cerr << "스택 오버플로우!" << std::endl;
        return;
    }

    chip8_32.set_R(StackFrame::RSP_INDEX, new_esp);
}

```

### ADD ESP (스택 공간 해제)

```cpp
// ADD ESP, imm16 - 스택 공간 해제 (x86의 ADD ESP, n과 동일)
void OP_ADD_RSP(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t value = opcode & 0x0000FFFF;
    uint32_t esp = chip8_32.get_R(StackFrame::RSP_INDEX);
    uint32_t new_esp = esp + value;

    chip8_32.set_R(StackFrame::RSP_INDEX, new_esp);
}

```

### 메모리 접근 연산

### MOV [EBP+offset], Rx

```cpp
// MOV [EBP+offset], Rx - 지역변수에 값 저장 (x86과 동일)
void OP_MOV_RBP_PLUS_RX(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);

    uint32_t ebp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = ebp + offset;
    uint32_t reg_value = chip8_32.get_R(reg_index);

    // 4바이트로 메모리에 저장 (32비트)
    chip8_32.set_memory(addr + 0, (reg_value >> 24) & 0xFF);
    chip8_32.set_memory(addr + 1, (reg_value >> 16) & 0xFF);
    chip8_32.set_memory(addr + 2, (reg_value >> 8) & 0xFF);
    chip8_32.set_memory(addr + 3, reg_value & 0xFF);
}

```

### MOV Rx, [EBP+offset]

```cpp
// MOV Rx, [EBP+offset] - 지역변수에서 값 로드 (x86과 동일)
void OP_MOV_RX_RBP_PLUS(Chip8_32& chip8_32, uint32_t opcode) {
    uint8_t reg_index, offset;
    extract_register_and_offset(opcode, reg_index, offset);

    uint32_t ebp = chip8_32.get_R(StackFrame::RBP_INDEX);
    uint32_t addr = ebp + offset;

    // 메모리에서 4바이트 읽기 (32비트)
    uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                     (chip8_32.get_memory(addr + 1) << 16) |
                     (chip8_32.get_memory(addr + 2) << 8) |
                     chip8_32.get_memory(addr + 3);

    chip8_32.set_R(reg_index, value);
}

```

## 실제 사용 예제

### 간단한 함수 호출 예제

```cpp
// 두 수를 더하는 함수 (add_function)
// 매개변수: a, b
// 반환값: a + b

// 함수 호출부
PUSH R1              // 매개변수 b 푸시
PUSH R0              // 매개변수 a 푸시
CALL_FUNC 0x300      // add_function 호출

// add_function (0x300 주소)
PUSH R28             // 이전 EBP 저장
MOV R28, R29         // EBP = ESP (새 프레임 설정)
SUB RSP, 4           // 지역변수 1개 공간 할당

// 매개변수 접근
MOV R2, [R28+8]      // a = 첫 번째 매개변수
MOV R3, [R28+12]     // b = 두 번째 매개변수

// 연산 수행
ADD R2, R3           // result = a + b
MOV [R28-4], R2      // 지역변수에 결과 저장

// 함수 종료
MOV R0, [R28-4]      // 반환값을 R0에 설정
MOV R29, R28         // ESP = EBP (스택 복원)
POP R28              // 이전 EBP 복원
RET_FUNC             // 호출자로 반환

```

### 복잡한 중첩 함수 호출

```cpp
// factorial(n) 함수 - 재귀 호출 예제
// 매개변수: n
// 반환값: n!

factorial:
    PUSH R28             // 프롤로그
    MOV R28, R29
    SUB RSP, 8          // 지역변수 2개 공간

    MOV R0, [R28+8]     // n 로드
    CMP R0, 1           // n과 1 비교
    JLE base_case       // n <= 1이면 기저 사례

    // 재귀 사례: n * factorial(n-1)
    SUB R0, 1           // n - 1
    PUSH R0             // factorial(n-1) 호출 준비
    CALL_FUNC factorial
    ADD RSP, 4          // 매개변수 정리

    MOV R1, [R28+8]     // 원래 n 로드
    MUL R0, R1          // result = n * factorial(n-1)
    JMP end_function

base_case:
    MOV R0, 1           // factorial(1) = 1

end_function:
    MOV R29, R28        // 에필로그
    POP R28
    RET_FUNC

```

## 디버깅 및 시각화

### 스택 프레임 시각화

디버그 모드에서 `sf` 명령어를 사용하면 현재 스택 상태를 시각적으로 확인할 수 있습니다:

```
=== x86 스타일 스택 프레임 시각화 ===
EBP (R28): 0x0000EFF0  ESP (R29): 0x0000EFE8
스택 사용량: 8/32752 바이트 (0.02%)

0xEFFF ┌─────────────────┐ 스택 시작 (STACK_START)
       │                 │
0xEFF4 ├─────────────────┤
       │ 0x00000200      │ 반환 주소 [EBP+4]
0xEFF0 ├─────────────────┤ EBP (프레임 베이스)
       │ 0x0000EFF8      │ 저장된 EBP [EBP+0]
0xEFEC ├─────────────────┤
       │ 0x0000000A      │ 지역변수 a [EBP-4]
0xEFE8 ├─────────────────┤ ESP (스택 탑)
       │                 │
       │    사용 가능    │
       │                 │
0x8000 └─────────────────┘ 스택 끝 (STACK_END)

```

### 디버깅 명령어

```bash
# 스택 프레임 시각화
(debug) sf

# 스택 메모리 덤프
(debug) stack 0xEFE0 0xEFFF

# EBP 기준 메모리 확인
(debug) mem [EBP-8]
(debug) mem [EBP+4]

# 레지스터 상태
(debug) reg
EBP (R28): 0x0000EFF0
ESP (R29): 0x0000EFE8
EIP (R30): 0x00000304

```

## 성능 및 안전성

### 스택 오버플로우 방지

```cpp
namespace StackFrame {
    constexpr uint32_t STACK_START = 0xEFFF;     // 32KB 스택 공간
    constexpr uint32_t STACK_END = 0x8000;

    bool check_stack_overflow(uint32_t esp) {
        if (esp < STACK_END) {
            std::cerr << "스택 오버플로우 감지!" << std::endl;
            return true;
        }
        return false;
    }

    bool check_stack_underflow(uint32_t esp) {
        if (esp > STACK_START) {
            std::cerr << "스택 언더플로우 감지!" << std::endl;
            return true;
        }
        return false;
    }
}

```

### 메모리 정렬

x86과 동일하게 4바이트 정렬을 사용합니다:

```cpp
// 32비트 정렬된 스택 연산
void aligned_stack_operation(Chip8_32& chip8_32, uint32_t value) {
    uint32_t esp = chip8_32.get_R(StackFrame::RSP_INDEX);
    esp = (esp - 4) & ~3;  // 4바이트 정렬
    chip8_32.set_R(StackFrame::RSP_INDEX, esp);
}

```

## x86과의 비교

| 특징 | Intel x86 | CHIP-8 32비트 확장 |
| --- | --- | --- |
| 스택 포인터 | ESP | R29 (ESP) |
| 베이스 포인터 | EBP | R28 (EBP) |
| 명령어 포인터 | EIP | R30 (EIP) |
| 스택 방향 | 아래로 증가 | 아래로 증가 |
| 호출 규약 | cdecl, stdcall 등 | 단순화된 cdecl |
| 매개변수 전달 | 스택 (cdecl) | 스택 |
| 반환값 | EAX | R0 |
| 프롤로그/에필로그 | 동일 패턴 | 동일 패턴 |

## 장점과 활용

### 장점

1. **표준 호출 규약**: x86과 유사한 친숙한 프로그래밍 모델
2. **동적 스택**: 함수 중첩 호출과 재귀 지원
3. **지역 변수**: 각 함수별 독립적인 변수 공간
4. **디버깅 지원**: 스택 추적과 시각화 가능
5. **확장성**: 복잡한 프로그램 구조 지원

### 활용 사례

- 계산기 프로그램 (`CALC*.ch32`)
- 재귀 알고리즘 구현
- 복잡한 게임 로직
- 함수형 프로그래밍 패턴

## 다음 단계

스택 프레임 시스템을 학습했다면:

1. [**SYSCALL 문서**](https://claude.ai/chat/syscall.md) - BootROM과 시스템 호출 인터페이스
2. [**명령어 레퍼런스**](https://claude.ai/chat/instruction-reference.md) - 전체 32비트 명령어 세트
3. **실제 예제 분석** - `roms/CALC*.ch32` 파일들의 스택 사용법 확인

## 참고 자료

- Intel x86 Architecture Manual
- System V ABI (Application Binary Interface)
- x86 Assembly Language Programming
- 프로젝트 소스: `src/core/stack_frame.cpp`, `src/core/stack_opcodes.cpp`
