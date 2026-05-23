# EmulParty Advance — VMM 하드닝 프로젝트 (PROGRESS)

> **한 줄 정의**: EPA(가상 머신)를 "안전한 격리 런타임"으로 하드닝하여,
> 신뢰할 수 없는 게스트 ROM이 호스트를 침해하는 **VM 탈출(VM escape)**을 막는다.
>
> **기간**: 1주 (공부 병행) · **시작일**: 2026-05-19 · **브랜치**: dev
> **분야**: 컨테이너/VM 보안 (격리 런타임 보안 = gVisor·Firecracker·runc 보안팀의 영역)

---

# A. 기존 프로젝트 정확한 파악

## A-1. 정체

- **이름**: EmulParty Advance (EPA) / "CHIP-8 Extended Emulator v2.0"
- **출처**: WhitehatSchool 3기 "에뮤르파티" 팀 산출물. 상위 디렉터리에 최종보고서 PDF 존재
  (`(에뮤르파티)미니 에뮬레이터 만들기_WHS 3기 최종보고서`).
- **본질**: 1970년대 CHIP-8(인터프리터 기반 **가상 머신**)을 32비트로 확장한
  **교육용 가상 머신 / 에뮬레이터**. PDF가 직접 CHIP-8을 "가상 머신"으로 정의한다.
- **목적(원래)**: 저수준 시스템 구조와 보안 취약점(스택 BOF 등)을 안전한 가상 환경에서 실습.
- **라이선스**: Apache 2.0. 개발자: CHO YONGJIN 외 에뮤르파티 팀.

## A-2. 듀얼 모드 구조

| | 8비트 모드 (Chip8) | 32비트 모드 (EPA / Chip8_32) |
|---|---|---|
| 호환 | 기존 CHIP-8 ROM (`.ch8`/`.c8`) | 확장 ROM (`.ch32`/`.c32`) |
| 메모리 | 4KB | 64KB (`std::array<uint8_t,65536>`) |
| 레지스터 | 16 × 8비트 (V0-VF) | 32 × 32비트 (R0-R31, R28=RBP·R29=RSP·R30=RIP) |
| 명령어 | 2바이트 | 4바이트 |
| 추가 기능 | 없음 | BootROM, SYSCALL, x86식 스택 프레임 |

## A-3. 아키텍처 (PDF 6계층)

`Opcode Table` → `Processor` → `BootStrap Loader` → `SYSCALL & I/O` → `Platform Layer` → `Debugger`
의 계층형 구조. 모듈 디렉터리:

- `src/core/` — `chip8.cpp`(8비트), `chip8_32.cpp`(32비트), `opcode_table*.cpp`,
  `mode_selector.cpp`, `stack_frame.cpp`, `stack_opcodes.cpp`
- `src/boot/` — `boot_rom.cpp`, `boot_rom_data.cpp` (내장 BootROM 바이트코드)
- `src/syscall/` — `io_manager.cpp`(fd→장치 라우팅), `sdl_console_io.cpp`
- `src/platform/` — `platform.cpp`(SDL2), `timer.cpp`
- `src/debugger/` — `debugger.cpp` (듀얼 모드, `sf` 스택 시각화)
- 빌드: CMake + SDL2/SDL2_ttf (`CMakeLists.txt`, 타깃 `chip8_dual`)

## A-4. 실행 흐름 (보안상 가장 중요)

```
main.cpp
  └─ ModeSelector::select_and_run()
       └─ run_unified_bootrom_mode()           # 항상 32비트 BootROM으로 시작
            ├─ Chip8_32 생성, BootROM을 0x0000에 로드
            ├─ 부트 메시지 출력, 사용자에게 ROM 파일명 요청
            ├─ READ syscall  → 사용자가 파일명 입력 (입력버퍼 0x0200~0x02FF)
            └─ LOAD_ROM syscall (OP_10SAAAAF, opcode_table_32.cpp:679)
                 └─ ModeSelector::load_and_switch_mode(filename)
                      ├─ resolve_rom_path()    # ★ 여기서 호스트 경로 결정
                      ├─ ifstream으로 파일 read → g_rom_data
                      └─ 확장자별 8비트/32비트 모드 분기
```

핵심: **사용자(=게스트 입력)가 준 파일명이 검증 없이 호스트 파일 경로로 변환된다.**

## A-5. 현재 git 상태

- 브랜치 `dev` (main 브랜치 = `dev`).
- 미커밋 수정 파일: `mode_selector.{hpp,cpp}`, `boot_rom.cpp`, `boot_rom_data.cpp`, `main.cpp`.
  → 하드닝 작업은 충돌 최소화를 위해 가능하면 신규 파일 + 좁은 수정으로.

---

# B. 보안 관점 재정의 — EPA = VMM

EPA의 코드를 고치는 것은 "막연한 시스템 보안"이 아니다. EPA는 격리 런타임이고,
그 격리 경계를 고치는 것은 **VMM/하이퍼바이저 보안 = 컨테이너/VM 보안**이다.

| EPA 구성요소 | 가상화 세계의 정체 |
|---|---|
| ROM (`.ch32` 등) | **게스트 워크로드** — 신뢰할 수 없음 |
| EPA 에뮬레이터 코어 (C++) | **VMM / 하이퍼바이저** — TCB(신뢰 컴퓨팅 기반) |
| `SYSCALL` 인터페이스 | 게스트↔호스트 **경계** (컨테이너→커널 공격면과 동일) |
| 64KB 메모리 배열 | 게스트 물리 메모리 |
| 호스트 OS / 파일시스템 | 진짜 호스트 — 게스트로부터 보호 대상 |

**프로젝트 목표**: 악성 게스트 ROM이 이 경계를 넘어 호스트를 침해하지 못하도록
VMM(=EPA 코어)을 하드닝한다.

---

# C. 위협 모델 & 탈출 벡터

**신뢰 경계**: `게스트 ROM (신뢰 X)` ┃ `EPA VMM (신뢰 O)` ┃ `호스트 OS (보호 대상)`
**공격자 능력**: 임의의 ROM 파일 또는 부트 프롬프트 입력을 제공할 수 있음.
**보호 목표**: 호스트의 기밀성·무결성·가용성.

| # | 탈출 벡터 | 코드 근거 | 영향 |
|---|---|---|---|
| 1 | **파일시스템 탈출** — `LOAD_ROM` 파일명이 검증 없이 호스트 경로화. `../../etc/passwd`(경로 탐색) 및 `/etc/passwd`(절대경로 — `path("roms")/"/etc/passwd"`가 `roms` 무시) 모두 통함 | `mode_selector.cpp:24` `resolve_rom_path`, `:253` `load_and_switch_mode` | 호스트 파일 읽기 — 기밀성 침해 (CWE-22) |
| 2 | **호스트 DoS** — (a) ROM 파일 크기 무제한 `resize(size)` → 메모리 고갈, (b) `set/get_memory`의 `.at()` OOB 예외 미처리 → VMM 프로세스 crash | `mode_selector.cpp:270`, `chip8_32.hpp:83-84` | 가용성 침해 (CWE-400 / CWE-248) |
| 3 | **최소권한 부재** — 게스트는 모든 syscall(READ/WRITE/LOAD_ROM/CALC 등)을 무제한 호출. 권한·정책 개념 전무 | `opcode_table_32.cpp:534` `OP_10SAAAAF` `switch` | 공격면 과다 (CWE-269) |
| 4 | **감사 부재** — 게스트의 syscall/파일 접근 기록이 `std::cout` 디버그 출력뿐. 침해 탐지·포렌식 불가 | 전체 SYSCALL 경로 | 탐지 불가 (CWE-778) |

---

# D. 일주일 계획 (체크리스트)

### Day 1 — 위협 모델 확정 · 탈출 벡터 감사
- [ ] `chip8_32::load_rom` 및 opcode 핸들러의 메모리 접근 경로 추가 감사
- [ ] 벡터별 PoC 절차 문서화 (재현 단계)
- [ ] `docs/threat-model.md` 작성 (C절을 정식 문서로)
- [ ] before 스냅샷: 부트 프롬프트에 `../../etc/passwd` 입력 → 호스트 파일 노출 재현·기록

### Day 2 — 벡터 1 차단: FS Jail (★ 핵심)
- [ ] `roms/` 디렉터리를 canonical 경로로 확정 (`std::filesystem::weakly_canonical`)
- [ ] `resolve_rom_path` 결과가 `roms/` 하위인지 봉쇄 검증 (절대경로·`..`·symlink 탈출 거부)
- [ ] 탈출 시도 시 거부 + 사유 반환
- [ ] after 검증: 동일 입력 → 차단 확인

### Day 3 — 벡터 2 차단: DoS 하드닝 / 게스트 폴트
- [ ] ROM 파일 크기 상한 (예: 64KB 메모리 한도 기반) — `load_and_switch_mode`
- [ ] 메모리 접근 / `cycle()`을 try-catch로 감싸 OOB를 **게스트 폴트**로 처리
      (VMM crash 대신 해당 ROM만 halt) — `Chip8_32`에 halt 플래그 추가
- [ ] 메인 루프(`run_unified_bootrom_mode`)가 halt 플래그 확인

### Day 4 — 벡터 3 차단: Syscall 정책 (최소권한)
- [ ] `OP_10SAAAAF`의 `switch` 직전에 정책 게이트 삽입
- [ ] syscall 허용목록 — 기본 프로파일 + (시간 되면) ROM별 `.policy` 파일
- [ ] 거부 시 에러 반환, BootROM 자신은 예외(전체 허용)
- [ ] ※ 일정 빡빡하면 "기본 프로파일 하드코딩"으로 축소 가능

### Day 5 — 벡터 4 차단: 감사 로그 + 데모
- [ ] `epa_audit.log` 구조화 로깅 (syscall명·인자·결정·사유)
- [ ] before/after 탈출 데모 정리 (벡터 1~3, `--headless --rom`으로 재현 가능하게)

### Day 6 — 테스트 · 회귀
- [ ] Catch2 단위테스트 (`test/`) — jail 경로 판정, 크기 한도, 정책 결정
- [ ] (필요 시) `CMakeLists.txt`에 테스트 타깃 추가
- [ ] `--headless` 회귀 — 정상 ROM(`pong.ch32` 등) 깨지지 않는지 확인

### Day 7 — 문서화 · 마감
- [ ] `docs/threat-model.md` 최종본 (벡터별 before/after)
- [ ] README에 "VMM 하드닝 / 보안 모델" 섹션 추가
- [ ] 시연 write-up (스크린샷·로그 포함)
- [ ] 커밋 정리, 최종 검토

---

# E. 파일 변경 맵

| 종류 | 파일 | 내용 |
|---|---|---|
| 수정 | `src/core/mode_selector.cpp` | `resolve_rom_path` FS jail, ROM 크기 상한 |
| 수정 | `src/core/opcode_table_32.cpp` | `OP_10SAAAAF` 정책 게이트, 감사 로깅 |
| 수정 | `include/core/chip8_32.hpp` `.cpp` | halt 플래그 + 접근자, 정책 멤버 |
| 신규 | `include/syscall/syscall_policy.hpp` `src/syscall/syscall_policy.cpp` | syscall 허용목록·결정 엔진 |
| 신규 | `include/security/path_jail.hpp` `src/security/path_jail.cpp` | 경로 봉쇄 검증 |
| 신규 | `include/security/audit_log.hpp` `src/security/audit_log.cpp` | 구조화 감사 로그 |
| 신규 | `test/test_hardening.cpp` | Catch2 테스트 |
| 신규 | `docs/threat-model.md` | 위협 모델 정식 문서 |
| 수정 | `CMakeLists.txt` | 신규 소스 추가 |
| 수정 | `README.md` | 보안 모델 섹션 |

---

# F. 산출물 & 데모

1. **위협 모델 문서** — EPA를 VMM으로 본 신뢰 경계 + 탈출 벡터 4종.
2. **하드닝 패치** — 벡터 1~4 차단.
3. **before/after 탈출 데모** — 같은 공격 입력, 차단 결과 비교.
4. **테스트 스위트** — 회귀 방지.

핵심 데모: 부트 프롬프트에 `../../etc/passwd` 입력
→ before: 호스트 파일 노출 / after: jail이 차단 + 감사 로그 기록.
(별도 악성 ROM 제작 불필요 — 부트 프롬프트 입력으로 재현 가능)

---

# G. 컨테이너/VM·기업 보안 매핑

- **VM 탈출 방지** — 하이퍼바이저 보안의 1순위 (VM escape = 항상 critical CVE)
- **FS Jail** — Firecracker의 `jailer`, 컨테이너 파일시스템 격리와 동일 개념
- **게스트 폴트 처리** — VMM은 게스트 오류로 죽지 않는다 (fault isolation)
- **Syscall 정책** — VMM seccomp, 컨테이너 capability drop, 최소권한 원칙
- **감사 로그** — 런타임 보안·SOC 탐지·포렌식 (CWE-778)
- PDF 3.3절 후속과제(보안 시나리오 확장) 이행

---

# H. 상태 로그

| 날짜 | 내용 |
|---|---|
| 2026-05-19 | 기존 프로젝트 파악 완료. 방향 확정: EPA=VMM, 악성 ROM의 VM 탈출 방지. 탈출 벡터 4종 코드 근거 확인. PROGRESS.md 작성. |
