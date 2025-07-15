cmake_minimum_required(VERSION 3.10)
project(chip8_dual_emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL2 및 SDL2_TTF 설정
set(SDL2_INCLUDE_DIR "/usr/local/include/SDL2")
set(SDL2_LIBRARY "/usr/lib/x86_64-linux-gnu/libSDL2.so")
set(SDL2_TTF_LIBRARY "/usr/lib/x86_64-linux-gnu/libSDL2_ttf.so")

# pkg-config를 사용한 SDL2_TTF 찾기 (fallback)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
    if(SDL2_TTF_FOUND)
        message(STATUS "Found SDL2_TTF via pkg-config")
        set(SDL2_TTF_INCLUDE_DIRS ${SDL2_TTF_INCLUDE_DIRS})
        set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARIES})
    else()
        message(STATUS "Using manual SDL2_TTF configuration")
        set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})
    endif()
else()
    message(STATUS "pkg-config not found, using manual SDL2_TTF configuration")
    set(SDL2_TTF_LIBRARIES ${SDL2_TTF_LIBRARY})
endif()

# include 디렉토리 설정
include_directories(include)
include_directories(include/common)
include_directories(include/core)
include_directories(include/platform)
include_directories(include/debugger)
include_directories(${SDL2_INCLUDE_DIR})
if(SDL2_TTF_INCLUDE_DIRS)
    include_directories(${SDL2_TTF_INCLUDE_DIRS})
endif()

# 소스 파일들 명시적으로 지정
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

set(DEBUGGER_SOURCES
    src/debugger/debugger.cpp
)

set(MAIN_SOURCE
    src/main.cpp
)

# 실행 파일 생성
add_executable(chip8_dual
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${DEBUGGER_SOURCES}
    ${MAIN_SOURCE}
)

# 라이브러리 링크 (SDL2와 SDL2_TTF)
target_link_libraries(chip8_dual 
    ${SDL2_LIBRARY}
    ${SDL2_TTF_LIBRARIES}
)

# 컴파일 옵션 추가 (디버그 정보 및 경고)
target_compile_options(chip8_dual PRIVATE -Wall -Wextra -g)

# 빌드 타입별 최적화 설정
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(chip8_dual PRIVATE -O0 -DDEBUG)
    message(STATUS "Debug build configuration")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(chip8_dual PRIVATE -O3 -DNDEBUG)
    message(STATUS "Release build configuration")
else()
    target_compile_options(chip8_dual PRIVATE -O2)
    message(STATUS "Default build configuration")
endif()

# 빌드 정보 출력
message(STATUS "=== CHIP-8 Dual Mode Emulator Build Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2 Include: ${SDL2_INCLUDE_DIR}")
message(STATUS "SDL2 Library: ${SDL2_LIBRARY}")
message(STATUS "SDL2_TTF Library: ${SDL2_TTF_LIBRARIES}")
message(STATUS "Core Sources: ${CORE_SOURCES}")
message(STATUS "Platform Sources: ${PLATFORM_SOURCES}")
message(STATUS "Debugger Sources: ${DEBUGGER_SOURCES}")
message(STATUS "Executable: chip8_dual")
message(STATUS "=====================================================")

# 폰트 디렉토리 생성 및 복사 (선택사항)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/fonts)
if(EXISTS ${CMAKE_SOURCE_DIR}/fonts)
    file(COPY ${CMAKE_SOURCE_DIR}/fonts/ DESTINATION ${CMAKE_BINARY_DIR}/fonts/)
    message(STATUS "Font directory copied to build directory")
endif()

# 빌드 후 실행 예시 메시지
add_custom_target(usage
    COMMAND echo ""
    COMMAND echo "=== CHIP-8 Extended Emulator Usage ==="
    COMMAND echo "Normal mode:     ./chip8_dual"
    COMMAND echo "Debug mode:      ./chip8_dual --debug"
    COMMAND echo "Help:            ./chip8_dual --help"
    COMMAND echo "Version:         ./chip8_dual --version"
    COMMAND echo ""
    COMMAND echo "File Input: Enter ROM filename in SDL window"
    COMMAND echo "ROM Location: ../roms/ directory"
    COMMAND echo "Supported: .ch8, .ch32, .rom files"
    COMMAND echo ""
    COMMAND echo "Controls: 1234/QWER/ASDF/ZXCV keypad"
    COMMAND echo "=========================================="
    COMMAND echo ""
)

# 의존성 검사 타겟
add_custom_target(check-deps
    COMMAND echo "Checking SDL2 dependencies..."
    COMMAND pkg-config --exists sdl2 && echo "✓ SDL2 found" || echo "✗ SDL2 not found"
    COMMAND pkg-config --exists SDL2_ttf && echo "✓ SDL2_TTF found" || echo "✗ SDL2_TTF not found"
    COMMAND echo "If dependencies are missing, install with:"
    COMMAND echo "  Ubuntu/Debian: sudo apt install libsdl2-dev libsdl2-ttf-dev"
    COMMAND echo "  Arch Linux: sudo pacman -S sdl2 sdl2_ttf"
    COMMAND echo ""
)

# 설치 규칙 (선택사항)
install(TARGETS chip8_dual DESTINATION bin)
if(EXISTS ${CMAKE_BINARY_DIR}/fonts)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/fonts/ DESTINATION share/chip8_dual/fonts)
endif()

# 테스트 빌드 타겟 (개발용)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_custom_target(test-build
        COMMAND echo "Building test configuration..."
        COMMAND make clean
        COMMAND make -j4
        COMMAND echo "Test build completed"
        DEPENDS chip8_dual
    )
endif()

# Valgrind 메모리 검사 타겟 (디버그 빌드에서만)
if(CMAKE_BUILD_TYPE MATCHES Debug)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(memcheck
            COMMAND echo "Running memory check with Valgrind..."
            COMMAND ${VALGRIND_EXECUTABLE} --leak-check=full --show-leak-kinds=all ./chip8_dual --help
            DEPENDS chip8_dual
        )
        message(STATUS "Valgrind memcheck target available")
    endif()
endif()