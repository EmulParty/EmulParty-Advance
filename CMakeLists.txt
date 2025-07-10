cmake_minimum_required(VERSION 3.10)
project(chip8_dual_emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 설정
set(SDL2_INCLUDE_DIR "/usr/local/include/SDL2")
set(SDL2_LIBRARY "/usr/lib/x86_64-linux-gnu/libSDL2.so")

# include 디렉토리 설정
include_directories(include)
include_directories(include/common)
include_directories(include/core)
include_directories(include/platform)
include_directories(${SDL2_INCLUDE_DIR})

# 소스 파일들 명시적으로 지정 (GLOB_RECURSE 대신 명확하게)
set(CORE_SOURCES
    src/core/chip8.cpp
    src/core/chip8_32.cpp
    src/core/opcode_table.cpp
    src/core/opcode_table_32.cpp
    src/core/mode_selector.cpp
)

set(PLATFORM_SOURCES
    src/platform/platform.cpp
    src/platform/timer.cpp
)

set(MAIN_SOURCE
    src/main.cpp
)

# 실행 파일 생성
add_executable(chip8_dual
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${MAIN_SOURCE}
)

# SDL2 링크
target_link_libraries(chip8_dual ${SDL2_LIBRARY})

# 컴파일 옵션 추가 (디버그 정보 및 경고)
target_compile_options(chip8_dual PRIVATE -Wall -Wextra -g)

# 빌드 정보 출력
message(STATUS "=== CHIP-8 Dual Mode Emulator Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SDL2 Include: ${SDL2_INCLUDE_DIR}")
message(STATUS "SDL2 Library: ${SDL2_LIBRARY}")
message(STATUS "Core Sources: ${CORE_SOURCES}")
message(STATUS "Platform Sources: ${PLATFORM_SOURCES}")
message(STATUS "Executable: chip8_dual")
message(STATUS "=====================================================")

# 빌드 후 실행 예시 메시지
add_custom_target(usage
    COMMAND echo ""
    COMMAND echo "=== Usage Examples ==="
    COMMAND echo "8-bit mode:  ./chip8_dual roms/game.ch8"
    COMMAND echo "32-bit mode: ./chip8_dual roms/demo.ch32"
    COMMAND echo "======================"
    COMMAND echo ""
)