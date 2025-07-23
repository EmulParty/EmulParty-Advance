cmake_minimum_required(VERSION 3.10)
project(chip8_emulator)

set(CMAKE_CXX_STANDARD 17)

# SDL2 경로 수동 지정 (수동 설치 환경 대응)
set(SDL2_INCLUDE_DIR "/usr/local/include/SDL2")
set(SDL2_LIBRARY "/usr/lib/x86_64-linux-gnu/libSDL2.so")

include_directories(${SDL2_INCLUDE_DIR})
link_libraries(${SDL2_LIBRARY})

# include 디렉토리 추가
include_directories(include)
include_directories(include/core)
include_directories(include/platform)

# 소스 파일 수집
file(GLOB_RECURSE CORE_SRC src/core/*.cpp)
file(GLOB_RECURSE PLATFORM_SRC src/platform/*.cpp)

add_executable(chip8
    ${CORE_SRC}
    ${PLATFORM_SRC}
    src/main.cpp
)

# SDL2 링크
target_link_libraries(chip8 ${SDL2_LIBRARY})

