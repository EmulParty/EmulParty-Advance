# ✅ CHIP-8 Emulator 프로젝트 구조 점검 보고서

본 문서는 `chip8-emulator` 프로젝트가 명세된 코딩 컨벤션과 구조를 얼마나 잘 준수하고 있는지에 대한 점검 결과를 요약한 것입니다.

---

## 📁 디렉토리 구조 및 파일명 점검

| 항목 | 상태 | 설명 |
|------|------|------|
| **1. 디렉터리 구조** | ✅ | `include/core`, `src/core`, `include/platform`, `src/platform` 디렉토리 구성 완전 일치 |
| **2. 파일명 규칙** (snake_case) | ✅ | `chip8.cpp`, `opcode_table.cpp`, `platform.cpp` 등 모두 소문자 + 스네이크 케이스 사용 |
| **3. 헤더-소스 1:1 대응** | ✅ | `chip8.hpp` ↔ `chip8.cpp`, `platform.hpp` ↔ `platform.cpp` 등 명확히 대응됨 |
| **4. 헤더 자체 포함성(Self-contained)** | ✅ | `chip8.hpp`, `opcode_table.hpp` 등 모두 필요한 표준 헤더(<array>, <cstdint> 등) 직접 include함 |
| **5. `#pragma once` 사용 여부** | ✅ | 모든 헤더파일에 `#pragma once` 존재 |
| **6. using namespace std 없음** | ✅ | 모든 파일에서 `using namespace std` 전역 사용 없음. 필요한 경우 std:: 붙여 사용함 |
| **7. 클래스/함수 명명 규칙** | ✅ | 클래스: `Chip8`, `Platform` 등 PascalCase / 함수, 변수: `load_rom()`, `draw_flag` 등 snake_case |
| **8. PC, SP 명확히 관리** | ✅ | 명령어 처리 내에서 `pc += 2`, `sp++`, `sp--` 등을 명령어마다 직접 처리하고 있음 |
| **9. FDE 분리 구조** | ✅ | main.cpp → chip8.cpp의 `cycle()` 호출 → `opcode_table.cpp`에서 명령어 처리하는 구조 분리 완료 |
| **10. 테스트 디렉터리 (`test/`) 존재** | ⚠️ | 디렉터리는 존재하지만 실제 테스트 코드 없음 (추가 예정) |
| **11. 메인 루프 내 역할 분리** | ✅ | main에서는 `cycle()`, `ProcessInput()`, `Update()`로 역할 잘 분리 |
| **12. Magic Number 최소화** | ✅ | 0x200, 60Hz 등은 의미 있는 상수로 관리되며 타이머 간격도 `const`로 정의됨 |
| **13. SDL 관련 Platform 캡슐화** | ✅ | SDL 관련 기능은 모두 `platform.cpp`에 분리되어 있고, `chip8.cpp`는 SDL에 의존하지 않음 |
| **14. switch-case 대신 테이블 사용** | ✅ | `opcode_table.cpp`에서 함수 포인터 기반 테이블 방식으로 구현 완료 |
| **15. clang-format용 스타일 유지** | ✅ | 인덴트, 중괄호, 줄간격 등 스타일 일관성 유지됨. |

---

## 🛠️ 개선 여지 또는 추천 사항

| 항목 | 제안 |
|------|------|
| 🔧 `test/` | Catch2 샘플 테스트 케이스를 `test_chip8.cpp`로 넣기 |
| 📦 `.clang-format` | 스타일 일관 유지를 위한 설정 파일 추가 (`IndentWidth: 4`, `ColumnLimit: 100` 등) |
| 📝 Doxygen 주석 | 주요 클래스와 함수에 `///`나 `/** */` 형식으로 기능 설명 추가 |

---
