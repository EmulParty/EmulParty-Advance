# **설치 가이드**

CHIP-8 Extended Emulator (E.P.A)을 설치하고 빌드하는 방법을 안내합니다.

## **시스템 요구사항**

### **최소 요구사항**

- **OS**: Linux (Ubuntu 18.04+), macOS (10.14+), Windows 10+
- **컴파일러**: GCC 7+ 또는 Clang 6+ (C++17 지원)
- **RAM**: 최소 512MB
- **저장공간**: 100MB (소스코드 + 빌드 파일)

### **권장 요구사항**

- **OS**: Ubuntu 20.04+ 또는 macOS 11+
- **컴파일러**: GCC 9+ 또는 Clang 10+
- **RAM**: 2GB 이상
- **저장공간**: 500MB (개발 환경 포함)

## **의존성 설치**

### **Ubuntu/Debian**

```bash
*# 기본 개발 도구 설치*
sudo apt update
sudo apt install build-essential cmake git

*# SDL2 라이브러리 설치*
sudo apt install libsdl2-dev libsdl2-mixer-dev

*# 선택사항: 디버깅 도구*
sudo apt install gdb valgrind
```

### **macOS**

Homebrew를 사용한 설치:

```bash
`*# Homebrew 설치 (없는 경우)*
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

*# 필요한 패키지 설치*
brew install cmake git sdl2 sdl2_mixer

*# 선택사항: 디버깅 도구*
brew install gdb lldb`
```

### **Arch Linux**

```bash
*# 기본 개발 도구 설치*
sudo pacman -S base-devel cmake git

*# SDL2 라이브러리 설치*
sudo pacman -S sdl2 sdl2_mixer

*# 선택사항: 디버깅 도구*
sudo pacman -S gdb valgrind
```

### **Windows (WSL 권장)**

Windows에서는 WSL(Windows Subsystem for Linux) 사용을 권장합니다:

```bash
*# WSL Ubuntu에서 실행*
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libsdl2-dev libsdl2-mixer-dev

*# X11 포워딩을 위한 설정 (GUI 실행용)*
sudo apt install x11-apps
```

## **소스코드 다운로드**

```bash
*# 저장소 클론*
git clone https://github.com/yourusername/chip8-extended-emulator.git
cd chip8-extended-emulator

*# 서브모듈 초기화 (있는 경우)*
git submodule update --init --recursive
```

## **빌드 과정**

### **1. 빌드 디렉토리 생성**

```bash
mkdir build
cd build
```

### **2. CMake 설정**

```bash
*# 기본 릴리즈 빌드*
cmake ..

*# 또는 디버그 빌드 (개발용)*
cmake -DCMAKE_BUILD_TYPE=Debug ..

*# 또는 최적화된 릴리즈 빌드*
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### **3. 컴파일**

```bash
*# 병렬 빌드 (빠른 컴파일)*
make -j$(nproc)

*# 또는 단일 스레드 빌드*
make
```

### **4. 설치 확인**

```bash
*# 실행 파일 확인*
ls -la chip8_dual

*# 간단한 실행 테스트*
./chip8_dual --help
```

## **실행 및 테스트**

### **첫 번째 실행**

```bash
*# 빌드 디렉토리에서 실행*
cd build

*# BootROM 시스템 시작*
./chip8_dual

*# 디버그 모드로 실행*
./chip8_dual --debug
```

### **ROM 파일 테스트**

```bash
*# 8비트 클래식 ROM 실행 (자동 감지)*
./chip8_dual

*# BootROM에서 파일 선택:# - ../roms/Brick.ch8 (8비트 모드로 자동 전환)# - ../roms/pong.ch8 (8비트 모드로 자동 전환)# - ../roms/calc.ch32 (32비트 모드 유지)*
```

## **빌드 옵션**

### **CMake 설정 옵션**

```bash
*# 디버그 정보 포함*
cmake -DCMAKE_BUILD_TYPE=Debug ..

*# 최적화 빌드*
cmake -DCMAKE_BUILD_TYPE=Release ..

*# 사용자 정의 설치 경로*
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..

*# 컴파일러 지정*
cmake -DCMAKE_CXX_COMPILER=clang++ ..
```

### **환경 변수 설정**

```bash
`*# SDL2 경로 수동 지정 (필요한 경우)*
export SDL2_DIR=/usr/local/lib/cmake/SDL2
*# 컴파일러 플래그 추가*
export CXXFLAGS="-Wall -Wextra -O2"`
```

## **문제 해결**

### **일반적인 문제들**

### **1. SDL2를 찾을 수 없음**

```bash
*# 에러 메시지 예시*
CMake Error: Could not find SDL2

*# 해결 방법*
sudo apt install libsdl2-dev libsdl2-mixer-dev  *# Ubuntu*
brew install sdl2 sdl2_mixer                    *# macOS*
```

### **2. 컴파일러 버전 문제**

```bash
*# 에러 메시지 예시*
error: C++17 features are not supported

*# 해결 방법 - 최신 컴파일러 설치*
sudo apt install gcc-9 g++-9  *# Ubuntu*
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

### **3. 실행 시 라이브러리 오류**

```bash
*# 에러 메시지 예시*
error while loading shared libraries: libSDL2-2.0.so.0

*# 해결 방법*
sudo ldconfig                    *# Linux*
export DYLD_LIBRARY_PATH=/usr/local/lib  *# macOS*
```

### **4. 권한 문제**

```bash
*# 빌드 디렉토리 권한 확인*
ls -la build/

*# 권한 수정*
chmod +x build/chip8_dual
```

### **디버깅 도구**

### **메모리 누수 검사**

```bash
*# Valgrind 사용 (Linux)*
valgrind --leak-check=full ./chip8_dual

*# AddressSanitizer 사용*
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
make
./chip8_dual
```

### **디버거 실행**

```bash
*# GDB로 디버깅*
gdb ./chip8_dual
(gdb) run --debug
(gdb) bt  *# 백트레이스# LLDB로 디버깅 (macOS)*
lldb ./chip8_dual
(lldb) run --debug
(lldb) bt  *# 백트레이스*
```

## **개발 환경 설정**

### **IDE 설정**

### **Visual Studio Code**

```bash
`*# C++ 확장 설치*
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools

*# 프로젝트 열기*
code .`
```

### **CLion**

1. "Open" → CMakeLists.txt 선택
2. CMake 설정 자동 감지
3. 빌드 설정에서 타겟 `chip8_dual` 선택

### **코드 품질 도구**

```bash
*# Clang-format 설치 및 사용*
sudo apt install clang-format
clang-format -i src/**/*.cpp include/**/*.hpp

*# Static analysis*
sudo apt install cppcheck
cppcheck --enable=all src/ include/
```

## **성능 최적화**

### **최적화 빌드**

```bash
*# 최고 성능 빌드*
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make -j$(nproc)
```

### **프로파일링**

```bash
*# 프로파일링 정보 포함 빌드*
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make

*# gprof 사용*
./chip8_dual
gprof chip8_dual gmon.out > profile.txt
```

## **다음 단계**

설치가 완료되었다면:

1. [**스택 프레임 문서**](https://claude.ai/chat/stack-frames.md) - x86-64 스타일 스택 프레임 시스템 학습
2. [**SYSCALL 문서**](https://claude.ai/chat/syscall.md) - BootROM 아키텍처와 시스템 호출 이해
3. [**명령어 레퍼런스**](https://claude.ai/chat/instruction-reference.md) - 8비트에서 32비트로의 명령어 진화 확인

## **지원**

문제가 지속된다면:

- **GitHub Issues**: 버그 신고 및 기술 지원
- **토론**: 커뮤니티 Q&A
- **문서**: 다른 문서들에서 추가 정보 확인
