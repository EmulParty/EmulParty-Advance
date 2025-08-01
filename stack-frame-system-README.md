# Stack Frame System Feature Branch

## 개요
CHIP-8 32비트 확장 에뮬레이터의 x86-64 호출 규약 기반 스택 프레임 시스템을 구현하는 기능 브랜치입니다.

## 주요 기능

### Stack Frame System
- **x86-64 호출 규약**: 표준 x86-64 스택 프레임 구조 모방
- **32비트 확장**: 기존 CHIP-8의 16비트 스택을 32비트로 확장
- **오버/언더플로우 검사**: 스택 경계 조건 검사 및 오류 처리
- **디버깅 지원**: 스택 상태 출력 및 메모리 덤프 기능

## 레지스터 구조

### 스택 관련 특수 레지스터
- **RBP (R28)**: Base Pointer - 현재 스택 프레임의 기준점
- **RSP (R29)**: Stack Pointer - 스택의 현재 위치
- **RIP (R30)**: Instruction Pointer - 다음 실행할 명령어 (PC와 동기화)
- **R31**: 예약된 레지스터

## 구현 파일

### 헤더 파일
- `include/core/stack_frame.hpp`: 스택 프레임 시스템 정의
- `include/core/stack_opcodes.hpp`: 스택 관련 opcode 정의

### 소스 파일
- `src/core/stack_frame.cpp`: 스택 프레임 시스템 구현
- `src/core/stack_opcodes.cpp`: 스택 관련 opcode 구현

## 메모리 맵

| 주소 범위 | 용도 | 크기 |
|----------|------|------|
| 0x8000-0xEFFF | 스택 영역 | 28KB |
| 0xEFFF | 스택 시작점 (높은 주소) | - |
| 0x8000 | 스택 한계점 (낮은 주소) | - |

## 주요 기능

### 스택 초기화
```cpp
void initialize(Chip8_32& chip8_32);
```
- RSP를 스택 최상단(0xEFFF)으로 초기화
- RBP를 초기 프레임 위치로 설정
- RIP를 현재 PC와 동기화

### 스택 보호 기능
```cpp
bool check_stack_overflow(uint32_t rsp);
bool check_stack_underflow(uint32_t rsp);
```

### 디버깅 지원
```cpp
void print_stack_frame(const Chip8_32& chip8_32);
void dump_stack_memory(const Chip8_32& chip8_32, uint32_t start_addr, uint32_t end_addr);
```

## 스택 구조

### 메모리 레이아웃 (하향 성장)
```
0xEFFF  ← 스택 시작 (RSP 초기값)
  │
  ▼     (스택 성장 방향)
  │
  │     스택 데이터
  │
0x8000  ← 스택 한계점
```

### 프레임 구조
```
[높은 주소]
+----------------+ ← RBP (이전 프레임)
| 이전 RBP 값    |
+----------------+
| 리턴 주소      |
+----------------+
| 지역 변수들    |
+----------------+ ← RSP (현재 위치)
[낮은 주소]
```

## 특징
- **안전성**: 스택 오버플로우/언더플로우 검사
- **호환성**: x86-64 호출 규약과 유사한 구조
- **확장성**: 32비트 주소 공간 활용
- **디버깅**: 상세한 스택 상태 정보 제공

## 사용법

```cpp
#include "stack_frame.hpp"
#include "chip8_32.hpp"

Chip8_32 chip8;
StackFrame::initialize(chip8);

// 스택 상태 확인
chip8.print_stack_info();

// 스택 메모리 덤프
chip8.dump_stack(64);
```