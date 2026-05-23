cmake_minimum_required(VERSION 3.16)
project(NeoCHIP-8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include 디렉토리
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/core
    ${CMAKE_SOURCE_DIR}/include/platform
    ${CMAKE_SOURCE_DIR}/include/syscall
    ${CMAKE_SOURCE_DIR}/include/security
    ${CMAKE_SOURCE_DIR}/include/boot
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/debugger
)

# 🔧 mode_selector.cpp가 필요로 하는 모든 파일들 + 스택 프레임 시스템 추가
set(SRC_FILES
    src/core/chip8.cpp                      # ✅ 8비트 CHIP-8 (mode_selector에서 사용)
    src/core/chip8_32.cpp                   # ✅ 32비트 CHIP-8
    src/core/opcode_table.cpp               # ✅ 8비트 opcode 테이블 (mode_selector에서 사용)
    src/core/opcode_table_32.cpp            # ✅ 32비트 opcode 테이블
    src/core/mode_selector.cpp              # ✅ 모드 선택기
    src/core/stack_frame.cpp                # 🆕 스택 프레임 관리 시스템
    src/core/stack_opcodes.cpp              # 🆕 스택 명령어 구현
    src/boot/boot_rom.cpp
    src/boot/boot_rom_data.cpp
    src/syscall/io_manager.cpp
    src/syscall/sdl_console_io.cpp
    src/syscall/syscall_policy.cpp          # 🔒 VMM 하드닝: syscall 최소권한
    src/security/path_jail.cpp              # 🔒 VMM 하드닝: FS jail
    src/security/audit_log.cpp              # 🔒 VMM 하드닝: 구조화 감사 로그
    src/platform/platform.cpp
    src/platform/timer.cpp                  # ✅ timer 함수들 (mode_selector에서 사용)
    src/debugger/debugger.cpp               # ✅ 디버거 (mode_selector에서 사용)
    src/main.cpp
)

# pkg-config로 SDL2/SDL2_ttf 검색
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# 실행 파일 생성
add_executable(chip8_dual ${SRC_FILES})
# 인클루드 디렉터리 (add_executable 이후에 위치해야 함)
target_include_directories(chip8_dual PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# 라이브러리 링크
target_link_libraries(chip8_dual ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# 🔧 추가: 컴파일 옵션
# 컴파일 플래그
target_compile_options(chip8_dual PRIVATE -Wall -Wextra ${SDL2_CFLAGS_OTHER} ${SDL2_TTF_CFLAGS_OTHER})

# ── 단위 테스트: VMM 하드닝 회귀 방지 (Catch2) ──────────────────────────────
# SDL/플랫폼 의존 없는 순수 정책 코드만 묶어서 가볍게 돌린다.
add_executable(test_hardening
    test/test_hardening.cpp
    src/security/path_jail.cpp
    src/security/audit_log.cpp
    src/syscall/syscall_policy.cpp
)
target_include_directories(test_hardening PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/test
)
target_compile_options(test_hardening PRIVATE -Wall -Wextra)