cmake_minimum_required(VERSION 3.10)
project(EmulParty-Advance)

set(CMAKE_CXX_STANDARD 17)

# SDL2 경로 설정
set(SDL2_PATH "C:/SDL2/SDL2-2.24.0/x86_64-w64-mingw32")

# SDL2 + 프로젝트 헤더 포함 경로
include_directories("${SDL2_PATH}/include")
include_directories("${CMAKE_SOURCE_DIR}/include") 
link_directories("${SDL2_PATH}/lib")

# SDL2 main() 정의 충돌 방지
add_definitions(-DSDL_MAIN_HANDLED) 

# 소스 파일 등록
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(EmulParty-Advance ${SOURCES})

# SDL2 링크
target_link_libraries(EmulParty-Advance SDL2 SDL2main)

# Windows에서 필요한 추가 라이브러리들
if(WIN32)
    target_link_libraries(EmulParty-Advance mingw32)
endif()