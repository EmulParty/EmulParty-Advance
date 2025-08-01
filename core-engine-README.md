# Core Engine Feature Branch

## 개요
CHIP-8 32비트 확장 에뮬레이터의 핵심 엔진을 구현하는 기능 브랜치입니다.

## 주요 기능

### CHIP-8 32비트 확장 엔진
- **확장된 메모리**: 4KB → 64KB (16배 확장)
- **확장된 레지스터**: 16개 8비트 → 32개 32비트 레지스터 (R0~R31)
- **확장된 스택**: 16단계 → 32단계 스택
- **32비트 명령어**: 16비트 → 32비트 opcode 처리

### 시스템 레지스터
- **I 레지스터**: 16비트 → 32비트 인덱스 레지스터
- **PC**: 32비트 프로그램 카운터
- **특수 레지스터**: RBP(R28), RSP(R29), RIP(R30)

## 구현 파일

### 헤더 파일
- `include/core/chip8_32.hpp`: CHIP-8 32비트 엔진 클래스 정의
- `include/core/opcode_table_32.hpp`: 32비트 명령어 테이블
- `include/core/mode_selector.hpp`: 모드 선택기

### 소스 파일
- `src/core/chip8_32.cpp`: CHIP-8 32비트 엔진 구현
- `src/core/opcode_table_32.cpp`: 32비트 명령어 테이블 구현
- `src/core/mode_selector.cpp`: 모드 선택기 구현

## 메모리 맵

| 주소 범위 | 용도 | 크기 |
|----------|------|------|
| 0x0000-0x00FF | Boot ROM 영역 | 256 bytes |
| 0x0050-0x00A0 | 폰트셋 | 80 bytes |
| 0x0200-0xFFFF | 프로그램/데이터 영역 | ~65KB |

## 레지스터 구조

### 범용 레지스터 (32개)
- **R0-R27**: 일반 목적 32비트 레지스터
- **R28 (RBP)**: Base Pointer
- **R29 (RSP)**: Stack Pointer
- **R30 (RIP)**: Instruction Pointer (PC와 동기화)
- **R31**: 예약됨

### 시스템 레지스터
- **I**: 32비트 인덱스 레지스터
- **PC**: 32비트 프로그램 카운터
- **SP**: 8비트 스택 포인터 (0-31)

## 주요 기능

### CPU 사이클
```cpp
void cycle(); // Fetch-Decode-Execute 수행
```

### 메모리 관리
```cpp
uint8_t get_memory(int index);
void set_memory(int index, uint8_t value);
```

### 레지스터 접근
```cpp
uint32_t get_R(int index);
void set_R(int index, uint32_t value);
```

### ROM 로딩
```cpp
bool load_rom(const char* filename);
```

## 특징
- 64KB 메모리 공간으로 대용량 프로그램 지원
- 32비트 레지스터로 향상된 연산 능력
- Boot ROM 시스템과 연동
- I/O 관리자와 통합
- 디버깅 지원 기능

## 호환성
- 기존 CHIP-8 프로그램과 하위 호환
- 32비트 확장 기능 추가 지원