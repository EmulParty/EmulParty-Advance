# Threat Model — EmulParty Advance (EPA) as a VMM

> **한 줄 정의**: EPA는 신뢰할 수 없는 게스트 ROM을 격리 실행하는 **가상 머신 모니터(VMM)** 다.
> 본 문서는 그 격리 경계를 공격자가 어떻게 뚫을 수 있는지(=VM 탈출)와,
> EPA를 하드닝하여 어떻게 막았는지를 정리한다.
>
> 위협 모델 작성일 : 2026-05-22 · 대상 버전 : EPA dev 브랜치 (커밋 9b65dc3 시점)

---

## 1. 시스템과 신뢰 경계

```
┌───────────────────────────────────────────────────────────────┐
│  Host OS  (Linux / macOS)   ← 보호 대상 (기밀·무결·가용성)    │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │  EPA VMM (C++ / SDL2)   ← TCB : 신뢰 컴퓨팅 기반        │  │
│  │  - chip8 / chip8_32, opcode tables, SYSCALL, BootROM    │  │
│  │  - PathJail, AuditLog, SyscallPolicy  ← 본 하드닝       │  │
│  │  ┌───────────────────────────────────────────────────┐  │  │
│  │  │  Guest ROM (.ch8 / .ch32)   ← 신뢰 X (공격자)     │  │  │
│  │  │  64KB 메모리, 32 × 32bit 레지스터, SYSCALL 만 통신 │  │  │
│  │  └───────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────┘

신뢰 경계 (Trust Boundary)
  ① 게스트 ROM       ┃ EPA VMM    : SYSCALL · 메모리 접근
  ② EPA VMM         ┃ 호스트 OS  : 파일시스템 · 프로세스 자원
```

| 구성 요소 | 가상화 세계 대응 | 신뢰도 |
|---|---|---|
| `.ch32` ROM 파일 | 게스트 워크로드 | **신뢰 X** — 임의 바이트 가능 |
| BootROM 입력 프롬프트 | 게스트로부터의 첫 입력 | **신뢰 X** — 공격자가 임의 문자열 입력 가능 |
| EPA C++ 코어 | VMM / 하이퍼바이저 | **신뢰 O** — TCB |
| 64KB 게스트 메모리 | 게스트 물리 메모리 | 격리 대상 |
| 호스트 파일시스템 | 호스트 자원 | **보호 대상** |

---

## 2. 공격자 모델

| 항목 | 가정 |
|---|---|
| 능력 | 임의의 ROM 바이트 + 임의의 BootROM 프롬프트 입력 제공 |
| 위치 | 게스트 (≒ 컨테이너 내부) |
| 목표 | 호스트 파일 읽기·쓰기, VMM crash, 다른 게스트 자원 점유 |
| 비목표(이번 모델 범위 외) | 커널·드라이버 익스플로잇, SDL2 라이브러리 0-day, 네트워크 공격 |

---

## 3. 탈출 벡터 4종

각 벡터는 PDF 보고서·코드 감사로 식별되었으며, 차단 전/후를 **`docs/demo-evidence/run_log.txt`** 에 그대로 보존했다.

### 벡터 1 — 파일시스템 탈출 (Path Traversal / Absolute Path / Symlink)

**CWE-22, CWE-23**

- **근거 코드** : `src/core/mode_selector.cpp::resolve_rom_path` (기존)
  ```cpp
  // BEFORE
  return std::filesystem::path("roms") / filename;  // 검증 없음
  ```
- **공격 입력**
  - `../../etc/passwd`  → `roms/../../etc/passwd` 로 합쳐져 호스트 파일 읽힘
  - `/etc/passwd`        → `path("roms") / "/etc/passwd"` 가 `roms` 무시 (C++ filesystem 규약)
  - `roms/sneaky.ch32 → /etc/passwd` symlink
- **영향** : 호스트 임의 파일 노출. 기밀성 침해.
- **차단** : `PathJail::resolve`
  - 파일명에 `/`·`\`·NUL 포함 시 즉시 `forbidden_character` 거부
  - 입력 자체가 절대경로면 `absolute_path` 거부
  - 합친 경로를 `weakly_canonical`로 정규화 → `lexically_relative`로 jail 루트 prefix 검사 → `..`로 시작하면 `escapes_root` 거부 (심링크 경유 탈출도 동일 경로로 잡힘)
- **시연 라인** (`docs/demo-evidence/run_log.txt:10·18·26`)

### 벡터 2 — 호스트 DoS (메모리 고갈 / VMM crash)

**CWE-400, CWE-248**

- **근거 코드 (a)** : `src/core/mode_selector.cpp::load_and_switch_mode`
  ```cpp
  // BEFORE
  g_rom_data.resize(size);  // size = 파일 크기. 상한 없음.
  ```
  → 공격자가 거대 파일을 제출하면 호스트 RAM 고갈.
- **근거 코드 (b)** : `include/core/chip8_32.hpp::get_memory/set_memory`
  ```cpp
  uint8_t get_memory(int index) const { return memory.at(index); }  // throws
  ```
  → 게스트 opcode가 OOB 주소를 건드리면 `std::out_of_range` 비핸들 → VMM 프로세스 죽음.
- **공격 입력**
  - 100KB짜리 빈 `.ch32` 파일
  - 잘못된 jump 주소로 OOB 메모리 접근하는 게스트 ROM
- **영향** : 가용성 침해 — 한 게스트가 VMM 자체를 내림.
- **차단**
  - (a) `load_and_switch_mode`에서 `kMaxRomBytes = 64*1024` 상한 검사. 초과 시 `rom_size deny`.
  - (b) `Chip8_32::cycle()`을 `try-catch`로 감싸 OOB를 흡수, `halt(reason)` 호출. main loop가 `is_halted()` 확인하고 해당 게스트만 정지. VMM은 살아남음.
- **시연 라인** (`docs/demo-evidence/run_log.txt:35`)

### 벡터 3 — 최소권한 부재 (Excessive Privilege)

**CWE-269**

- **근거 코드** : `src/core/opcode_table_32.cpp::OP_10SAAAAF` (switch 직진입)
  - 게스트가 LOAD_ROM(=호스트 파일 마운트), DEBUG 등 모든 syscall을 무제한 호출 가능했다.
- **공격 입력** : 악성 ROM이 LOAD_ROM syscall로 임의 파일명을 재요청 → 벡터 1·2 연쇄 시도.
- **영향** : 공격면 과다. 한 syscall로 다른 벡터를 트리거.
- **차단** : `SyscallPolicy::evaluate(profile, syscall_num)` 게이트
  - PC 값으로 자동 분류 (`<0x0200` → BootROM, 이상 → Guest)
  - Guest 프로파일은 READ/WRITE/GETPID/EXIT/CALC만 허용
  - LOAD_ROM·DEBUG·미정의 syscall은 deny → R16에 `0xFFFFFFFF` 채우고 PC만 진행
  - 결정(allow/deny)은 매 호출마다 `syscall_policy` 이벤트로 감사 로그에 기록
- **회귀** : BootROM 자신이 호출하는 LOAD_ROM은 BootROM 프로파일이라 정상 통과 — 시연 라인 `baseline.txt:5·6`

### 벡터 4 — 감사 부재 (Insufficient Logging)

**CWE-778**

- **근거 코드** : 전체 SYSCALL/파일 접근 경로. 게스트 행위 기록이 `std::cout` 디버그 출력에 흩어져 있어 침해 탐지·포렌식 불가.
- **영향** : 차단했더라도 누가 어떤 시도를 했는지 SOC가 알 수 없음.
- **차단** : `AuditLog` (JSON Lines)
  - 보안 결정 지점 모두에서 `event/decision/입력/사유` 포함 한 줄 JSON 기록
  - `stderr`와 `epa_audit.log` 동시 출력
  - SIEM 적재 가능한 표준 포맷
- **샘플** (`docs/demo-evidence/epa_audit.jsonl`)
  ```json
  {"ts":"2026-05-23T08:46:33Z","event":"path_jail","decision":"deny",
   "input":"../../etc/passwd","reason":"forbidden_character",
   "detail":"filename contains path separator or NUL",
   "caller":"load_and_switch_mode"}
  ```

---

## 4. 가드 매트릭스 (4벡터 × 4가드)

| 가드 \ 벡터 | V1 FS 탈출 | V2 DoS | V3 권한 | V4 감사 |
|---|---|---|---|---|
| **PathJail** | ✅ 차단 | — | — | 결정 기록 |
| **ROM 크기 상한** | — | ✅ (a) | — | 결정 기록 |
| **게스트 폴트 (halt)** | — | ✅ (b) | — | 폴트 기록 |
| **SyscallPolicy** | 간접 차단 (LOAD_ROM 차단으로 V1 재시도 봉쇄) | — | ✅ | 결정 기록 |
| **AuditLog** | — | — | — | ✅ |

---

## 5. 잔존 위협 (Out of Scope, 추후 과제)

| 항목 | 비고 |
|---|---|
| SDL2 / SDL2_ttf 라이브러리 자체 취약점 | 외부 의존성. 버전 핀 고정으로 부분 완화 |
| 사이드 채널 (타이밍, 캐시) | 격리 강도가 낮은 인터프리터 — 학습용으로 수용 |
| 멀티 게스트 동시 실행 | 현 VMM은 단일 게스트. 멀티 시 자원 분리 필요 |
| 정책 파일(`.policy`) 외부 로딩 | 현재 정책은 하드코딩. 향후 `roms/<name>.policy`로 ROM별 권한 차등화 가능 |

---

## 6. 검증 방법

```bash
# 빌드
cd build && cmake .. && make -j$(nproc)

# 정상 ROM 회귀
./chip8_dual --headless --rom pong.ch32 --max-frames 60

# 4가지 공격 입력 일괄 시연
./chip8_dual --headless --rom ../../etc/passwd --max-frames 60   # V1a
./chip8_dual --headless --rom /etc/passwd      --max-frames 60   # V1b
ln -s /etc/passwd ../roms/sneaky.ch32 \
  && ./chip8_dual --headless --rom sneaky.ch32 --max-frames 60   # V1c
dd if=/dev/zero of=../roms/huge.ch32 bs=1024 count=100 \
  && ./chip8_dual --headless --rom huge.ch32  --max-frames 60    # V2

# 결과는 epa_audit.log에 JSON Lines로 누적
```

기대 결과 : 모든 공격 입력에서 `path_jail` 또는 `rom_size` deny 이벤트가 기록되며, 호스트 파일은 단 한 바이트도 노출되지 않는다.

---

## 7. 기업 보안 매핑

| 본 하드닝 | 산업·기업 보안 영역 |
|---|---|
| PathJail | 컨테이너 chroot·jail, Firecracker `jailer`, runc 파일시스템 격리 |
| ROM 크기 상한 + halt | 하이퍼바이저 fault isolation, cgroups 메모리 제한 |
| SyscallPolicy | VMM seccomp 프로파일, 컨테이너 capability drop, 제로트러스트 최소권한 |
| AuditLog (JSON Lines) | SIEM 적재, SOC 탐지, 침해사고 포렌식 (CWE-778 대응) |

— EPA를 VMM으로 본 위협 모델 끝.
