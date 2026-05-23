# CHIP-8 Extended Emulator v2.0

*BootROM 기반 아키텍처와 32비트 확장*

```
╔══════════════════════════════════════════════════════════════════╗
║                CHIP-8 Extended Emulator v2.0                     ║
║                   BootROM-Driven Architecture                    ║
╠══════════════════════════════════════════════════════════════════╣
║  Features: BootROM • SYSCALL • I/O Redirection • Auto-Detection  ║
║  Modes: 8-bit CHIP-8 (compatibility) + 32-bit Extended           ║
╚══════════════════════════════════════════════════════════════════╝

```

8비트 CHIP-8 아키텍처를 32비트 시스템으로 확장하면서도 하위 호환성을 유지하도록 설계했습니다.  

## 주요 기능

### **듀얼 프로세서 지원**

- **8비트 CHIP-8 모드**:  기존 CHIP-8 ROM과 완전 호환 (`.ch8`, `.c8`)
- **32비트 확장 모드**:  성능 향상을 위한 32비트 확장 (`.ch32`, `.c32`)
- **자동 모드 감지**: BootROM이 적절한 모드를 확장자를 기반으로 감지

### **32비트 확장**

- **64KB 메모리 공간**: 4KB에서 64KB로 확장
- **32개 범용 레지스터**: R0-R31 (각각 32비트, 8비트 16개 레지스터에서 확장)
- **32비트 명령어 세트**: 확장된 기능을 가진 4바이트 opcode
- **x86 모방 스택 프레임**: EBP / ESP를 이용한 스택 관리 가능

### **BootROM 기반 아키텍처**

- **통합 부트 시스템**: 모든 ROM 타입을 처리하는 단일 진입점 역할
- **동적 모드 전환**: 8비트와 32비트 모드 간 원활한 전환
- **SYSCALL 인터페이스**: I/O 작업을 위한 계층별 시스템 호출 지원
- **I/O 리다이렉션**: 파일 디스크립터 관리를 통한 SDL 기반 콘솔 I/O 지원 가능

### **전문적인 스택 프레임 시스템**

- **x86 호출 규약을 따름** : 상대 주소를 통한 변수 관리 가능
- **스택 레지스터**: RBP (R28), RSP (R29), RIP (R30), R31 (예약됨)
- **스택 연산**: PUSH/POP, SUB ESP, ADD ESP, MOV [EBP+offset]
- **함수 호출**: 자동 반환 주소 처리를 통한 CALL_FUNC/RET_FUNC
- **시각적 스택 디버깅**: 실시간 스택 프레임 시각화한 시뮬레이션 기능 제공

### **디버깅 시스템**

- **듀얼 디버거**: 8비트 및 32비트 모드용 별도 디버깅을 통해 프로세서별 디버거 기능 제공함
- **단계별 실행**: 명령어 수준 디버깅 가능
- **스택 프레임 시각화**
- **레지스터 검사**: 각 레지스터 상태 모니터링이 가능함
- **브레이크포인트 지원**: 특정 주소에 브레이크 포인트 설정
- **Opcode 역어셈블**: 사람이 읽을 수 있는 명령어 디코딩 지원 예정

## 설치

### 의존성

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev libsdl2-mixer-dev cmake build-essential

# macOS
brew install sdl2 sdl2_mixer cmake

# Arch Linux
sudo pacman -S sdl2 sdl2_mixer cmake base-devel

```

### 빌드 방법

```bash
# 저장소 클론
git clone https://github.com/yourusername/chip8-extended-emulator.git
cd chip8-extended-emulator

# 에뮬레이터 빌드
mkdir build && cd build
cmake ..
make -j$(nproc)

# 또는 제공된 Makefile 사용
make clean && make

```

## 사용법

### 기본 사용법

```bash
# 빌드 디렉토리에서 실행
cd build

# BootROM 시스템 시작 (ROM 타입 자동 감지)
./chip8_dual

# 디버그 모드 활성화
./chip8_dual --debug

# 도움말 표시
./chip8_dual --help

```

### ROM 파일 구성

```
roms/
├── Brick.ch8              # 클래식 8비트 Breakout 게임
├── CALC*.ch32             # 32비트 계산기 프로그램
├── maze.ch8/.ch32         # 미로 게임 (양쪽 모드)
├── pong.ch8/.ch32         # Pong 구현
├── sum_BOF.ch32           # 스택 오버플로우 데모
└── *.png                  # 스크린샷 및 문서

```

### 파일 확장자

- **`.ch8`, `.c8`**: 클래식 8비트 CHIP-8 ROM
- **`.ch32`, `.c32`**: 32비트 확장 CHIP-8 ROM
- **자동 감지**: 알 수 없는 확장자는 8비트 모드로 기본 설정, 파일명 잘못 입력시 강제 종료함

## 추가 기능

### BootROM 시스템

에뮬레이터는 다음을 제공하는 32비트 BootROM으로 시작합니다:

- ROM 파일 선택 인터페스
- 자동 모드 감지 및 전환
- 시스템 초기화 및 설정
- 고급 작업을 위한 SYSCALL 인터페이스

### 스택 프레임 관리

32비트 모드는 x86 스타일 스택 프레임을 구현합니다:

```
스택 레이아웃 (0xEFFF에서 아래로 증가):
┌────────────────────────────┐ ← 0xEFFF (STACK_START)
│         반환 주소          │
├────────────────────────────┤
│       저장된 EBP           │
├────────────────────────────┤ ← EBP (프레임 포인터)
│        지역 변수           │
├────────────────────────────┤ ← ESP (스택 포인터)
│            ...             │
└────────────────────────────┘ ← 0x8000 (STACK_END)

```

### 디버깅 명령어

디버그 모드에서 다음 명령어를 사용하세요:

- `s` 또는 `step`: 한 명령어 실행
- `c` 또는 `continue`: 실행 계속
- `sf`: 스택 프레임 시각화 표시
- `reg`: 모든 레지스터 표시
- `mem <addr>`: 메모리 내용 표시
- `break <addr>`: 브레이크포인트 설정
- `q` 또는 `quit`: 디버거 종료

### 스택 프레임 시각화 예제

```
=== 스택 프레임 시각화 ===
RBP (R28): 0x0000EFF0  RSP (R29): 0x0000EFE8
스택 사용량: 8/32752 바이트 (0.02%)

0xEFFF ┌─────────────────┐ 스택 시작
       │                 │
0xEFF0 ├─────────────────┤ EBP (프레임 베이스)
       │ 0x00000200      │ 반환 주소
0xEFEC ├─────────────────┤
       │ 0x00000000      │ 지역 변수
0xEFE8 ├─────────────────┤ ESP (스택 탑)
       │                 │
       │     FREE        │
       │                 │
0x8000 └─────────────────┘ 스택 끝

```

## 아키텍처 개요

### 메모리 레이아웃 (32비트 모드)

```
0x0000 - 0x01FF:  BootROM 및 시스템 영역
0x0200 - 0x0FFF:  프로그램 영역 (클래식 ROM 호환성)
0x1000 - 0x7FFF:  확장 프로그램 영역 (32비트 전용)
0x8000 - 0xEFFF:  스택 영역 (32KB)
0xF000 - 0xFFFF:  I/O 및 시스템 레지스터

```

### 레지스터 세트 (32비트 모드)

```
R0  - R27:  범용 레지스터 (32비트)
R28 (RBP):  베이스 포인터
R29 (RSP):  스택 포인터
R30 (RIP):  명령어 포인터
R31:        예약됨
I:          인덱스 레지스터 (32비트)
```

### 명령어 세트 확장

32비트 모드는 강력한 명령어를 추가합니다:

- **스택 연산**: `PUSH Rx`, `POP Rx`, `SUB RSP, imm16`
- **메모리 연산**: `MOV [RBP+offset], Rx`, `MOV Rx, [RBP+offset]`
- **함수 호출**: `CALL_FUNC addr`, `RET_FUNC`
- **시스템 호출**: I/O 리다이렉션을 통한 `SYSCALL imm16`

## 프로젝트 구조

```
chip8-extended-emulator/
├── CMakeLists.txt              # CMake 빌드 설정
├── LICENSE                     # 프로젝트 라이센스
├── README.md                   # 이 문서
├── include/                    # 헤더 파일
│   ├── boot/                   # BootROM 시스템 헤더
│   │   ├── boot_rom.hpp        # BootROM 인터페이스
│   │   └── boot_rom_data.hpp   # BootROM 바이너리 데이터
│   ├── common/                 # 공유 상수 및 유틸리티
│   │   └── constants.hpp       # 시스템 전체 상수
│   ├── core/                   # 핵심 에뮬레이션 엔진
│   │   ├── chip8.hpp           # 8비트 CHIP-8 구현
│   │   ├── chip8_32.hpp        # 32비트 확장 구현
│   │   ├── mode_selector.hpp   # 지능형 모드 선택
│   │   ├── opcode_table.hpp    # 8비트 명령어 테이블
│   │   ├── opcode_table_32.hpp # 32비트 명령어 테이블
│   │   ├── stack_frame.hpp     # x86-64 스타일 스택 프레임
│   │   └── stack_opcodes.hpp   # 스택 연산 명령어
│   ├── debugger/               # 고급 디버깅 시스템
│   │   └── debugger.hpp        # 듀얼 모드 디버거 인터페이스
│   ├── platform/               # 플랫폼 추상화 레이어
│   │   ├── platform.hpp        # SDL 플랫폼 인터페이스
│   │   └── timer.hpp           # 고해상도 타이밍
│   └── syscall/                # 시스템 호출 인터페이스
│       ├── io_device.hpp       # I/O 장치 추상화
│       ├── io_manager.hpp      # I/O 장치 관리
│       └── sdl_console_io.hpp  # SDL 콘솔 I/O 구현
├── src/                        # 소스 코드 구현
│   ├── boot/                   # BootROM 구현
│   │   ├── boot_rom.cpp        # BootROM 로더 및 인터페이스
│   │   └── boot_rom_data.cpp   # 임베디드 BootROM 바이너리
│   ├── core/                   # 핵심 에뮬레이션 로직
│   │   ├── chip8.cpp           # 8비트 CHIP-8 에뮬레이터
│   │   ├── chip8_32.cpp        # 32비트 확장 에뮬레이터
│   │   ├── mode_selector.cpp   # 자동 모드 감지
│   │   ├── opcode_table.cpp    # 8비트 명령어 구현
│   │   ├── opcode_table_32.cpp # 32비트 명령어 구현
│   │   ├── stack_frame.cpp     # 스택 프레임 관리
│   │   └── stack_opcodes.cpp   # 스택 연산 구현
│   ├── debugger/               # 디버깅 시스템
│   │   └── debugger.cpp        # 시각화를 통한 대화형 디버거
│   ├── main.cpp                # 애플리케이션 진입점
│   ├── platform/               # 플랫폼별 코드
│   │   ├── platform.cpp        # SDL 플랫폼 구현
│   │   └── timer.cpp           # 정밀 타이밍 구현
│   └── syscall/                # 시스템 호출 구현
│       ├── io_manager.cpp      # I/O 장치 레지스트리 및 라우팅
│       └── sdl_console_io.cpp  # SDL 기반 콘솔 I/O
├── roms/                       # ROM 파일 및 테스트 프로그램
└── test/                       # 테스트 스위트 및 검증
    ├── catch.hpp              # Catch2 테스팅 프레임워크
    ├── chip8_structure_checklist.md # 개발 체크리스트
    ├── set_sdl_env.sh         # SDL 환경 설정
    └── test_chip8.cpp         # 단위 테스트 및 검증

```

## 개발 및 테스트

### 테스트 실행

```bash
# 테스트 스위트 빌드 및 실행
make test

# 특정 테스트 카테고리 실행
make test-stack     # 스택 프레임 테스트
make test-opcodes   # 명령어 테스트
make test-io        # I/O 시스템 테스트

```

### 디버그 모드 기능

- **시각적 스택 검사**: 실시간 스택 프레임 다이어그램
- **명령어 단계 실행**: 명령어별로 코드를 단계적으로 실행
- **레지스터 모니터링**: 실시간 레지스터 변경 사항 관찰
- **메모리 검사**: 임의 주소의 메모리 내용 검사
- **Opcode 분석**: 설명과 함께 역어셈블된 명령어 확인

### 성능 모니터링

에뮬레이터는 성능 카운터를 포함합니다:

- **명령어 수**: 실행된 총 명령어 수
- **스택 연산**: PUSH/POP 연산 수
- **메모리 접근**: 읽기/쓰기 연산 통계
- **타이머 정확도**: 타이머 정밀도 모니터링

## 기술 사양

| 기능 | 8비트 모드 | 32비트 모드 |
| --- | --- | --- |
| 메모리 크기 | 4KB | 64KB |
| 레지스터 | 16 × 8비트 (V0-VF) | 32 × 32비트 (R0-R31) |
| 명령어 크기 | 2바이트 | 4바이트 |
| 스택 크기 | 16단계 | 32KB 동적 |
| 주소 공간 | 12비트 | 32비트 |
| 최대 ROM 크기 | 3.5KB | 60KB |

## 기여

기여를 환영합니다! 가이드라인은 [CONTRIBUTING.md](https://claude.ai/chat/CONTRIBUTING.md)를 참조하세요.

### 개발 설정

1. 저장소 포크
2. 기능 브랜치 생성: `git checkout -b feature/amazing-feature`
3. `.clang-format`의 코딩 스타일 가이드라인 준수
4. 새 기능에 대한 테스트 추가
5. 풀 리퀘스트 제출

### 코딩 표준

- 들여쓰기에 4개 공백 사용
- K&R 코딩 스타일 변형 준수
- 복잡한 알고리즘에 포괄적인 주석 추가
- 모든 새 기능에 대한 테스트 작성

## 라이센스

이 프로젝트는 Apache 2.0 라이센스 하에 라이센스됩니다 - 자세한 내용은 LICENSE 문서를 참조하세요.

## 보안 모델 (VMM Hardening)

EPA는 신뢰할 수 없는 게스트 ROM을 격리 실행하는 **가상 머신 모니터(VMM)** 다.  
v2.0에서 호스트(=실제 컴퓨터)를 보호하기 위해 4개 가드를 추가했다.  
자세한 설계는 [`docs/threat-model.md`](docs/threat-model.md), 실제 차단 시연은 [`docs/demo-evidence/`](docs/demo-evidence/) 참조.

### 신뢰 경계

```
Guest ROM (신뢰 X) ┃ EPA VMM (TCB) ┃ Host OS (보호 대상)
```

게스트는 SYSCALL과 메모리 접근으로만 VMM과 통신한다.  
공격자는 임의의 `.ch32` 바이트 또는 BootROM 입력 프롬프트 문자열을 제공할 수 있다고 가정한다.

### 차단한 4종 탈출 벡터

| 벡터 | CWE | 가드 모듈 | 작동 방식 |
|---|---|---|---|
| **파일시스템 탈출** (`../../etc/passwd`, `/etc/passwd`, symlink) | CWE-22 | `PathJail` | `roms/`를 `weakly_canonical`로 고정 후 `lexically_relative`로 prefix 검사 |
| **호스트 DoS** (거대 ROM·OOB) | CWE-400/248 | ROM 크기 상한 + `Chip8_32::halt()` | 64KB 초과 거부, OOB는 게스트 폴트로 흡수해 VMM 보존 |
| **최소권한 부재** (게스트의 LOAD_ROM 임의 호출) | CWE-269 | `SyscallPolicy` | PC로 BootROM/Guest 프로파일 분류 후 허용목록 게이트 |
| **감사 부재** | CWE-778 | `AuditLog` | JSON Lines로 `epa_audit.log`에 결정 이벤트 기록 |

### 헤드리스 시연 (호스트 파일 노출 ⇒ 차단)

```bash
cd build
./chip8_dual --headless --rom ../../etc/passwd --max-frames 60
# → [AUDIT] {"event":"path_jail","decision":"deny","input":"../../etc/passwd",
#           "reason":"forbidden_character", ...}

./chip8_dual --headless --rom /etc/passwd --max-frames 60       # 절대경로 차단
ln -s /etc/passwd ../roms/sneaky.ch32
./chip8_dual --headless --rom sneaky.ch32 --max-frames 60       # 심링크 탈출 차단

dd if=/dev/zero of=../roms/huge.ch32 bs=1024 count=100
./chip8_dual --headless --rom huge.ch32 --max-frames 60         # 100KB ROM 차단
```

### 회귀 테스트

```bash
cmake --build build --target test_hardening
./build/test_hardening
# All tests passed (23 assertions in 13 test cases)
```

### 기업 보안과의 매핑

- **PathJail**           — Firecracker `jailer`, runc 파일시스템 격리
- **ROM 크기 상한·halt** — 하이퍼바이저 fault isolation, cgroups
- **SyscallPolicy**      — VMM seccomp 프로파일, 컨테이너 capability drop
- **AuditLog (JSONL)**   — SIEM 적재, SOC 탐지, 침해 포렌식

---

## Reference

- Joseph Weisbecker의 원본 CHIP-8 사양
- 그래픽 및 입력 처리를 위한 SDL2 라이브러리
- 스택 프레임을 위한 x86-64 호출 규약 영감
- CHIP-8 ROM 보존을 위한 레트로 컴퓨팅 커뮤니티

## 지원

- **이슈**: GitHub Issues를 통해 버그 신고
- **문서**: `docs/` 디렉터리 참조
- **예제**: 샘플 ROM은 `roms/` 디렉터리 확인
- **커뮤니티**: 토론을 위한 Discord 서버 참여

---

개발자: CHO YONGJIN

Organization : WhitehatSchool EmulParty Team
