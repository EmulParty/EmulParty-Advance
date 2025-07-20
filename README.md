# CHIP-8 Dual Mode Emulator
**세상에 없던 32비트 확장 CHIP-8 에뮬레이터 + x86-64 기반 스택 프레임 시스템**

## 📋 프로젝트 개요

이 프로젝트는 전통적인 8비트 CHIP-8 에뮬레이터를 32비트로 확장하고, **x86-64 기반 스택 프레임 관리**와 **고급 보안 기능**을 추가한 교육용 에뮬레이터입니다. 

** 핵심 성과**: 세계 최초로 CHIP-8에 완전한 x86-64 스타일 스택 프레임을 구현한 에뮬레이터!

---

## 프로젝트 구조

```
EmulParty-Advance/
├── include/
│   ├── common/
│   │   └── constants.hpp          # 공통 상수 정의
│   ├── core/
│   │   ├── chip8.hpp             # 8비트 CHIP-8 코어
│   │   ├── chip8_32.hpp          # 32비트 CHIP-8 확장 코어
│   │   ├── mode_selector.hpp      # 듀얼 모드 선택기
│   │   ├── opcode_table.hpp       # 8비트 명령어 테이블
│   │   ├── opcode_table_32.hpp    # 32비트 명령어 테이블
│   │   ├── stack_frame.hpp        # x86-64 스타일 스택 프레임
│   │   └── stack_opcodes.hpp      # 스택 프레임 전용 명령어
│   ├── debugger/
│   │   └── debugger.hpp           # 스택 프레임 디버거 포함
│   ├── platform/
│   │   ├── platform.hpp          # SDL2 플랫폼 계층
│   │   └── timer.hpp              # 타이머 유틸리티
│   └── syscall/
│       ├── io_device.hpp         # I/O 추상화
│       ├── io_manager.hpp        # I/O 관리자
│       └── sdl_console_io.hpp    # SDL2 콘솔 I/O
├── src/
│   ├── core/
│   │   ├── chip8.cpp
│   │   ├── chip8_32.cpp
│   │   ├── mode_selector.cpp
│   │   ├── opcode_table.cpp
│   │   ├── opcode_table_32.cpp
│   │   ├── stack_frame.cpp        # 스택 프레임 시스템
│   │   └── stack_opcodes.cpp      # 스택 명령어 구현
│   ├── boot/
│   │   ├── boot_rom.cpp          # 통합 부트로더
│   │   └── boot_rom_data.cpp     # 부트로더 데이터
│   └── [기타 모듈들...]
├── roms/
│   ├── [8비트 ROM들: maze.ch8, pong.ch8 등]
│   ├── [32비트 ROM들: *.ch32]
│   └── sum.ch32                   # 스택 프레임 데모 ROM
└── build/                         # 빌드 출력 디렉토리
```

---

## 주요 기능

### 기본 기능
- **듀얼 모드 지원**: 8비트 클래식 CHIP-8 + 32비트 확장 모드
- **자동 모드 감지**: `.ch8/.c8` (8비트), `.ch32/.c32` (32비트)
- **통합 BootROM**: 파일 선택부터 모드 전환까지 자동화
- **SDL2 그래픽**: WSL2/X11 환경 완벽 지원
- **SYSCALL 시스템**: READ, WRITE, LOAD_ROM 시스템 호출

### **혁신적인 스택 프레임 시스템** 
- **x86-64 호출 규약**: 완전한 x86-64 스타일 스택 프레임 구현
- **레지스터 체계**: R28(RBP), R29(RSP), R30(RIP) 전용 할당
- **메모리 보안**: 스택 오버플로우/언더플로우 실시간 감지
- **함수 호출**: `CALL_FUNC`, `RET_FUNC`으로 완전한 함수 호출 지원

### 강화된 디버깅 기능
- **인터랙티브 디버거**: 단계별 실행, 브레이크포인트
- **스택 추적**: `sf` (stack frame), `st` (stack trace) 명령어
- **메모리 덤프**: 스택 프레임 시각화 및 메모리 분석
- **실시간 모니터링**: 레지스터, 메모리, 스택 상태 실시간 표시

---

## 스택 프레임 아키텍처

### 메모리 맵
```
0xF000-0xFFFF: 시스템 예약
0xEFFF: 스택 시작점 (RSP 초기값) ← 높은 주소
   ↓ 스택이 이 방향으로 성장 (x86-64와 동일)
0x8000: 스택 한계점 ← 낮은 주소
0x0200-0x7FFF: 프로그램 영역 (ROM)
0x0000-0x01FF: BootROM 및 시스템
```

### 레지스터 체계
```
R0 ~ R27:  범용 레지스터 (28개)
R28 (RBP): Base Pointer - 스택 프레임 베이스
R29 (RSP): Stack Pointer - 스택 꼭대기
R30 (RIP): Instruction Pointer - 다음 실행 명령어
R31:       예약됨 (확장용)
```

### 스택 프레임 구조 (x86-64 스타일)
```
높은 주소 (0xEFFF) ← 스택 시작
┌─────────────────┐
│ 반환 주소       │ RBP + 4 (CALL이 푸시)
├─────────────────┤
│ 이전 RBP        │ ← RBP (PUSH RBP가 저장)
├─────────────────┤
│ 매개변수 a      │ RBP - 4
├─────────────────┤
│ 매개변수 b      │ RBP - 8
├─────────────────┤
│ 매개변수 c      │ RBP - 12
├─────────────────┤
│ 지역변수들      │ RBP - 16, -20, ...
└─────────────────┘ ← RSP (현재 스택 top)
낮은 주소 (0x8000)
```

---

## 스택 프레임 명령어 세트 (0x11xxxxxx)

### 기본 스택 조작
| Opcode | 명령어 | x86-64 대응 | 설명 |
|--------|--------|-------------|------|
| `11000000` | `PUSH RBP` | `push rbp` | RBP를 스택에 푸시 |
| `1100RRXX` | `PUSH RX` | `push rax` | 레지스터 RX를 스택에 푸시 |
| `11010000` | `POP RBP` | `pop rbp` | 스택에서 RBP로 팝 |
| `1101RRXX` | `POP RX` | `pop rax` | 스택에서 레지스터 RX로 팝 |

### 프레임 포인터 조작
| Opcode | 명령어 | x86-64 대응 | 설명 |
|--------|--------|-------------|------|
| `11020000` | `MOV RBP, RSP` | `mov rbp, rsp` | RSP 값을 RBP에 복사 |
| `11030000` | `MOV RSP, RBP` | `mov rsp, rbp` | RBP 값을 RSP에 복사 |

### 스택 포인터 조작
| Opcode | 명령어 | x86-64 대응 | 설명 |
|--------|--------|-------------|------|
| `1104NNNN` | `SUB RSP, NNNN` | `sub rsp, 16` | RSP에서 NNNN만큼 빼기 |
| `1105NNNN` | `ADD RSP, NNNN` | `add rsp, 16` | RSP에 NNNN만큼 더하기 |

### 함수 호출/반환
| Opcode | 명령어 | x86-64 대응 | 설명 |
|--------|--------|-------------|------|
| `1106NNNN` | `CALL_FUNC NNNN` | `call func` | 함수 호출 (자동 프레임 설정) |
| `11070000` | `RET_FUNC` | `ret` | 함수 반환 (자동 프레임 해제) |

### 스택 메모리 접근
| Opcode | 명령어 | x86-64 대응 | 설명 |
|--------|--------|-------------|------|
| `1108RRNN` | `MOV [RBP-NN], RX` | `mov [rbp-8], rax` | RX를 RBP-NN 위치에 저장 |
| `1109RRNN` | `MOV RX, [RBP-NN]` | `mov rax, [rbp-8]` | RBP-NN 위치값을 RX에 로드 |
| `110ARRNN` | `MOV [RBP+NN], RX` | `mov [rbp+8], rax` | RX를 RBP+NN 위치에 저장 |
| `110BRRNN` | `MOV RX, [RBP+NN]` | `mov rax, [rbp+8]` | RBP+NN 위치값을 RX에 로드 |

---

## 시스템 요구사항

### 필수 의존성
- **CMake**: 3.10 이상
- **C++ 컴파일러**: GCC 7+ 또는 Clang 5+
- **SDL2**: 개발 라이브러리 포함

### Ubuntu/Debian 설치
```bash
sudo apt update
sudo apt install build-essential cmake libsdl2-dev libsdl2-ttf-dev
```

### WSL2 환경 (X11 필요)
```bash
# X11 서버 설치 (Windows에서)
# VcXsrv 또는 X410 사용 권장

# WSL2에서 DISPLAY 환경변수 설정
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
```

---

## 빌드 및 실행

### 1. 빌드
```bash
# 프로젝트 루트에서
mkdir -p build
cd build
cmake ..
make -j4
```

### 2. sum.ch32 ROM 생성 (스택 프레임 데모)
```bash
# ROM 생성기 컴파일 및 실행
g++ -o create_sum_rom ../tools/create_sum_rom.cpp
./create_sum_rom
```

### 3. 실행 방법

#### 일반 모드 (Normal Mode)
```bash
# BootROM이 자동으로 파일 선택 UI 제공
./chip8_dual

# 실행 후 파일명 입력:
# - sum.ch32    (스택 프레임 데모)
# - pong.ch8    (8비트 클래식)
# - maze.ch32   (32비트 확장)
```

#### 디버그 모드 (Stack Frame Debug)
```bash
# 스택 프레임 디버깅 모드
./chip8_dual --debug

# 실행 후 sum.ch32 입력하면 자동으로 스택 프레임 추적 시작
```

---

## 조작법

### 키보드 매핑 (게임용)
```
CHIP-8 키패드     →     QWERTY 키보드
┌─────--────────┐       ┌──────--───────┐
│ 1 │ 2 │ 3 │ C │       │ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │  →    │ Q │ W │ E │ R │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │       │ A │ S │ D │ F │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ A │ 0 │ B │ F │       │ Z │ X │ C │ V │
└───┴───┴───┴───┘       └───┴───┴───┴───┘
```

### 스택 프레임 디버거 명령어
| 명령어 | 설명 |
|--------|------|
| `s`, `step` | 다음 명령어 실행 |
| `c`, `continue` | 연속 실행 모드 |
| `sf`, `stack_frame` | 현재 스택 프레임 표시 |
| `st`, `stack_trace` | 함수 호출 스택 추적 |
| `mem <addr>` | 메모리 덤프 (기본: 스택 영역) |
| `regs` | 스택 관련 레지스터 표시 |
| `bp <addr>` | 브레이크포인트 설정 |
| `q`, `quit` | 디버거 종료 |

**예시:**
```
Debug> sf                # 스택 프레임 정보 표시
Debug> st                # 함수 호출 스택 추적
Debug> mem 0x8000        # 0x8000부터 메모리 덤프
Debug> bp 0x240          # sum 함수에 브레이크포인트
```

---

## **스택 프레임 데모: sum.ch32**

### 프로그램 개요
**sum(10, 20, 30) = 60**을 계산하는 32비트 ROM으로, 완전한 x86-64 스타일 함수 호출을 시연합니다.

### 어셈블리 코드 (PSEUDO)
```assembly
main:
    PUSH RBP              ; 프롤로그: 이전 프레임 저장
    MOV RBP, RSP          ; 프롤로그: 새 프레임 설정
    SUB RSP, 16           ; 프롤로그: 지역변수 공간 확보
    
    LD R0, 10             ; 매개변수 a = 10
    LD R1, 20             ; 매개변수 b = 20  
    LD R2, 30             ; 매개변수 c = 30
    CALL_FUNC sum         ; sum 함수 호출
    
    MOV [RBP-4], R0       ; 반환값 저장
    SYSCALL WRITE         ; 결과 출력: "sum(10, 20, 30) = 60"
    
    ADD RSP, 16           ; 에필로그: 스택 정리
    POP RBP               ; 에필로그: 프레임 복원
    RET_FUNC              ; 에필로그: 함수 반환

sum:
    PUSH RBP              ; sum 프롤로그
    MOV RBP, RSP          ; sum 프레임 설정
    SUB RSP, 16           ; sum 지역변수 공간
    
    MOV [RBP-4], R0       ; a 저장
    MOV [RBP-8], R1       ; b 저장
    MOV [RBP-12], R2      ; c 저장
    
    MOV R3, [RBP-4]       ; R3 = a
    ADD R3, [RBP-8]       ; R3 += b
    ADD R3, [RBP-12]      ; R3 += c
    MOV R0, R3            ; 반환값 설정
    
    ADD RSP, 16           ; sum 에필로그
    POP RBP               ; sum 프레임 복원
    RET_FUNC              ; sum 반환
```

### 실행 결과
```
$ ./chip8_dual --debug
Enter ROM filename: sum.ch32

🔧 [OPCODE] PUSH RBP (0x11000000) - x86-64: push rbp
  ✓ Successfully pushed RBP to stack

🔧 [OPCODE] MOV RBP, RSP (0x11020000) - x86-64: mov rbp, rsp
  ✓ Frame setup: RBP 0x0000 → 0xEFFB

[출력] sum(10, 20, 30) = 60
```

---

## 시스템 비교

| 기능 | 8비트 모드 | 32비트 확장 모드 |
|------|------------|------------------|
| **메모리 크기** | 4KB (4,096바이트) | 64KB (65,536바이트) |
| **레지스터** | 16개 × 8비트 (V0-VF) | 32개 × 32비트 (R0-R31) |
| **스택 크기** | 16 레벨 | 32 레벨 + 스택 프레임 |
| **명령어 크기** | 2바이트 | 4바이트 |
| **주소 공간** | 12비트 (4KB) | 24비트 (16MB) |
| **함수 호출** | CALL/RET만 | **완전한 x86-64 스타일** |
| **스택 프레임** | ❌ 없음 | ✅ **RBP/RSP/RIP 지원** |
| **메모리 보안** | 기본 | **오버플로우/언더플로우 감지** |
| **호환성** | 원본 CHIP-8 완전 호환 | 확장 기능 + 보안 |

---

## 개발 로드맵

### ✅ 완료된 기능
- ✅ 8비트 CHIP-8 코어 구현
- ✅ 32비트 확장 아키텍처
- ✅ SDL2 플랫폼 계층
- ✅ BootROM 통합 시스템
- ✅ SYSCALL 시스템 (READ/WRITE/LOAD_ROM)
- ✅ **x86-64 스타일 스택 프레임 시스템**
- ✅ **완전한 함수 호출/반환 메커니즘**
- ✅ **스택 프레임 전용 명령어 세트 (20개)**
- ✅ **스택 오버플로우/언더플로우 실시간 감지**
- ✅ **스택 프레임 디버거 (sf, st, mem 명령어)**
- ✅ **sum.ch32 데모 ROM 작성**

### 🚧 진행 중
- 🚧 고급 스택 보안 기능 (Stack Canary)
- 🚧 ROP 체인 감지 시스템
- 🚧 더 복잡한 함수 호출 데모 ROM

### 📅 예정된 기능
- 📅 버퍼 오버플로우 시뮬레이션 ROM
- 📅 Use-after-free 감지
- 📅 메모리 누수 추적
- 📅 취약점 시나리오 라이브러리
- 📅 자동화된 보안 테스트 슈트

---

## 🐞 문제 해결

### 일반적인 문제들

#### SDL2 초기화 실패
```bash
# 오류: SDL_Init failed
# 해결: SDL2 개발 라이브러리 설치
sudo apt install libsdl2-dev libsdl2-ttf-dev

# WSL2 환경에서 X11 연결 문제
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
```

#### 빌드 오류
```bash
# CMake 버전 문제
cmake --version  # 3.10+ 확인

# 헤더 파일 누락
# include 디렉토리 구조 확인
ls -la include/core/
```

#### ROM 로드 실패
```bash
# 파일 경로 확인
ls -la ../roms/

# 권한 문제
chmod +r ../roms/*.ch8 ../roms/*.ch32
```

#### 스택 프레임 오류
```bash
# 디버그 모드로 스택 상태 확인
./chip8_dual --debug
# 실행 후 'sf' 명령어로 스택 프레임 검사
```

---

## 🧪 테스트 ROM 목록

### 8비트 클래식 ROM
- `maze.ch8` - 미로 생성기
- `pong.ch8` - 퐁 게임  
- `tetris.ch8` - 테트리스
- `space_invaders.ch8` - 스페이스 인베이더
- `breakout.ch8` - 벽돌깨기

### 🆕 32비트 확장 ROM
- **`sum.ch32`** - **스택 프레임 데모 (sum 함수 호출)**
- `maze_complete.ch32` - 32비트 미로 게임
- `pong_complete.ch32` - 32비트 퐁 게임

### 📅 개발 예정 ROM
- `stack_overflow.ch32` - 스택 오버플로우 시뮬레이션
- `function_call_demo.ch32` - 중첩 함수 호출 데모
- `security_test.ch32` - 보안 기능 테스트

---

## 🎯 **핵심 성과: 스택 프레임 구현**

### 🏆 세계 최초 달성
1. **CHIP-8에 x86-64 스타일 스택 프레임 완전 구현**
2. **RBP/RSP/RIP 레지스터 기반 함수 호출 규약**
3. **실시간 스택 오버플로우/언더플로우 감지**
4. **완전한 스택 프레임 디버거 (sf, st 명령어)**

### 📈 기술적 혁신
- **메모리 보안**: 스택 경계 검사로 보안 강화
- **디버깅 혁신**: 실시간 스택 추적 및 프레임 시각화
- **호환성**: 8비트 원본과 100% 호환 유지
- **확장성**: 추가 보안 기능 확장 가능한 아키텍처

---

## 🤝 기여하기

### 개발 환경 설정
1. 프로젝트 포크
2. 새 브랜치 생성: `git checkout -b feature/새기능`
3. 변경사항 커밋: `git commit -m "스택 프레임 기능 추가"`
4. 브랜치 푸시: `git push origin feature/새기능`
5. Pull Request 생성

### 코딩 스타일
- **C++17** 표준 사용
- **#pragma once** 사용 (헤더 가드 대신)
- **명확한 변수명**과 주석
- **RAII 원칙** 준수
- **x86-64 스타일** 명명 규약 (RBP, RSP, RIP)

---

## 📜 라이선스
이 프로젝트는 **MIT 라이선스** 하에 배포됩니다.

---

## 🙏 감사의 말
- **SDL2 라이브러리** 개발팀
- **원본 CHIP-8 아키텍처** 설계자들
- **x86-64 아키텍처** 표준 제정자들
- **오픈소스 에뮬레이터** 커뮤니티

---

## ⚠️ 주의사항
이 에뮬레이터는 **교육 목적**으로 개발되었습니다. 스택 프레임과 보안 취약점을 학습하는 용도로 사용하세요.

---

## 🚀 **빠른 시작 - 스택 프레임 데모**

```bash
# 1. 빌드
mkdir build && cd build && cmake .. && make -j4

# 2. 실행
./chip8_dual --debug

# 3. sum.ch32 입력

# 4. 스택 프레임 관찰
Debug> sf    # 스택 프레임 상태 확인
Debug> st    # 함수 호출 스택 추적
Debug> s     # 단계별 실행으로 프레임 변화 관찰

# 결과: "sum(10, 20, 30) = 60" 출력!
```

