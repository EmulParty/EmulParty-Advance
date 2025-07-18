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
    ${CMAKE_SOURCE_DIR}/include/boot
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/debugger
)

# 🔧 mode_selector.cpp가 필요로 하는 모든 파일들 추가
set(SRC_FILES
    src/core/chip8.cpp                      # ✅ 8비트 CHIP-8 (mode_selector에서 사용)
    src/core/chip8_32.cpp                   # ✅ 32비트 CHIP-8
    src/core/opcode_table.cpp               # ✅ 8비트 opcode 테이블 (mode_selector에서 사용)
    src/core/opcode_table_32.cpp            # ✅ 32비트 opcode 테이블
    src/core/mode_selector.cpp              # ✅ 모드 선택기
    src/boot/boot_rom.cpp
    src/boot/boot_rom_data.cpp
    src/syscall/io_manager.cpp
    src/syscall/sdl_console_io.cpp
    src/platform/platform.cpp
    src/platform/timer.cpp                  # ✅ timer 함수들 (mode_selector에서 사용)
    src/debugger/debugger.cpp               # ✅ 디버거 (mode_selector에서 사용)
    src/main.cpp
)

# pkg-config로 SDL2/SDL2_ttf 검색
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# include 경로 추가
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})

# 실행 파일 생성
add_executable(chip8_dual ${SRC_FILES})

# 라이브러리 링크
target_link_libraries(chip8_dual ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})

# 🔧 추가: 컴파일 옵션
target_compile_options(chip8_dual PRIVATE -Wall -Wextra)