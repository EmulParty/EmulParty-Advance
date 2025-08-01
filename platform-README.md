# Platform System (SDL2 기반 플랫폼 레이어)

## 🎯 개요
EmulParty-Advance의 SDL2 기반 플랫폼 시스템입니다. 윈도우 관리, 입력 처리, 렌더링, UI 인터페이스 등 시스템의 플랫폼별 기능을 담당합니다.

## 🚀 핵심 기능

### 🖥️ 윈도우 시스템
- **SDL2 윈도우**: 크로스 플랫폼 윈도우 생성 및 관리
- **렌더링 시스템**: 하드웨어 가속 2D 렌더링
- **텍스처 관리**: CHIP-8 디스플레이를 위한 텍스처 처리
- **폰트 시스템**: TTF 폰트 렌더링 (PressStart2P)

### 🎮 입력 시스템
- **키패드 매핑**: CHIP-8 16키 키패드 지원
- **파일 입력**: ROM 파일 선택 UI
- **콘솔 입력**: 텍스트 기반 명령어 입력
- **계산기 입력**: 인터랙티브 계산기 모드

### 🎨 UI 시스템
- **멀티 모드**: 게임/파일선택/콘솔/계산기 모드
- **텍스트 렌더링**: 고품질 TTF 텍스트 출력
- **중앙 정렬**: 자동 텍스트 중앙 정렬
- **큰 폰트**: 제목/로고용 80px 폰트

## 🏗️ 아키텍처 구조

### 플랫폼 컴포넌트
```
Platform System
├── Window Manager       # SDL2 윈도우 관리
├── Renderer            # 2D 하드웨어 렌더링
├── Input Handler       # 키보드/마우스 입력
├── Font System         # TTF 폰트 렌더링
├── Texture Manager     # 텍스처 생성/관리
├── Timer System        # 정확한 타이밍 제어
└── IO Interface        # 파일/콘솔 I/O
```

### 입력 모드
```
Input Modes:
FILE_INPUT      파일 선택 모드
GAME           게임 실행 모드  
CONSOLE_INPUT   콘솔 명령 모드
CALCULATOR      계산기 모드
```

## 🔧 핵심 클래스

### Platform 클래스
```cpp
class Platform {
private:
    // SDL2 시스템
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    TTF_Font* font_;
    TTF_Font* font_large_;
    
    // 윈도우 설정
    int window_width_;
    int window_height_;
    int texture_width_;
    int texture_height_;
    
    // 입력 시스템
    InputMode current_mode_;
    std::string input_buffer_;
    std::queue<std::string> console_input_queue_;
    
    // 계산기 상태
    std::string calc_num1_;
    std::string calc_num2_;
    std::string calc_operation_;
    std::string calc_result_;
    
public:
    Platform(const char* title, int w, int h, int tw, int th);
    bool Initialize();
    bool ProcessInput(std::array<uint8_t, 16>& keypad);
    void Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch);
    void SwitchToGameMode();
    void SwitchToConsoleMode();
    void SwitchToCalculatorMode();
};
```

### Timer 클래스
```cpp
class Timer {
private:
    uint32_t start_ticks_;
    uint32_t paused_ticks_;
    bool paused_;
    bool started_;

public:
    Timer();
    void start();
    void stop();
    void pause();
    void unpause();
    uint32_t getTicks() const;
    bool isStarted() const;
    bool isPaused() const;
};
```

## 🎮 키 매핑

### CHIP-8 키패드 매핑
```
원본 CHIP-8 키패드:    키보드 매핑:
┌─┬─┬─┬─┐             ┌─┬─┬─┬─┐
│1│2│3│C│             │1│2│3│4│
├─┼─┼─┼─┤             ├─┼─┼─┼─┤
│4│5│6│D│             │Q│W│E│R│
├─┼─┼─┼─┤             ├─┼─┼─┼─┤
│7│8│9│E│             │A│S│D│F│
├─┼─┼─┼─┤             ├─┼─┼─┼─┤
│A│0│B│F│             │Z│X│C│V│
└─┴─┴─┴─┘             └─┴─┴─┴─┘
```

### 시스템 키
```
ESC     - 파일 선택 모드로 전환
F1      - 콘솔 모드로 전환
F2      - 계산기 모드로 전환
ENTER   - 입력 확인
BACKSPACE - 문자 삭제
```

## 🖥️ UI 모드별 기능

### 파일 입력 모드
```cpp
// ROM 파일 선택 UI
void RenderFileInputUI() {
    RenderTextCenteredLarge("EmulParty-Advance", 50, GREEN);
    RenderTextCentered("Enter ROM filename:", 150, WHITE);
    RenderTextCentered(input_buffer_ + "_", 200, YELLOW);
    RenderTextCentered("Press ESC to browse files", 300, GRAY);
}
```

### 콘솔 입력 모드
```cpp
// 명령어 입력 UI  
void RenderConsoleInputUI() {
    RenderTextCentered("Console Mode", 50, GREEN);
    RenderTextCentered("Enter command:", 150, WHITE);
    RenderTextCentered(current_console_input_ + "_", 200, YELLOW);
    RenderConsoleOutput();
}
```

### 계산기 모드
```cpp
// 인터랙티브 계산기 UI
void RenderCalculatorUI() {
    RenderTextCenteredLarge("Calculator", 50, GREEN);
    
    if (calc_input_phase_ == 0) {
        RenderTextCentered("Enter first number:", 150, WHITE);
        RenderTextCentered(calc_num1_ + "_", 200, YELLOW);
    } else if (calc_input_phase_ == 1) {
        RenderTextCentered("Enter second number:", 150, WHITE);
        RenderTextCentered(calc_num2_ + "_", 200, YELLOW);
    } else {
        RenderTextCentered("Enter operator (+,-,*,/):", 150, WHITE);
        RenderTextCentered(calc_operation_ + "_", 200, YELLOW);
    }
    
    if (!calc_result_.empty()) {
        RenderTextCentered("Result: " + calc_display_result_, 300, GREEN);
    }
}
```

## 🎨 렌더링 시스템

### 텍스트 렌더링
```cpp
// 기본 텍스트 렌더링
void RenderText(const std::string& text, int x, int y, SDL_Color color);

// 중앙 정렬 텍스트
void RenderTextCentered(const std::string& text, int y, SDL_Color color);

// 큰 폰트 중앙 정렬
void RenderTextCenteredLarge(const std::string& text, int y, SDL_Color color);
```

### 색상 정의
```cpp
const SDL_Color WHITE = {255, 255, 255, 255};
const SDL_Color GREEN = {0, 255, 0, 255};
const SDL_Color YELLOW = {255, 255, 0, 255};
const SDL_Color RED = {255, 0, 0, 255};
const SDL_Color GRAY = {128, 128, 128, 255};
```

## ⏱️ 타이머 시스템

### 정밀 타이밍
```cpp
class Timer {
public:
    void start();           // 타이머 시작
    void stop();            // 타이머 정지
    void pause();           // 타이머 일시정지
    void unpause();         // 타이머 재개
    uint32_t getTicks() const;  // 경과 시간 조회
    bool isStarted() const;     // 시작 상태 확인
    bool isPaused() const;      // 일시정지 상태 확인
};
```

### 사용 예제
```cpp
Timer gameTimer;
gameTimer.start();

// 게임 루프에서
uint32_t elapsed = gameTimer.getTicks();
if (elapsed >= 16) {  // 60 FPS 제어
    // 업데이트 로직
    gameTimer.start();  // 타이머 리셋
}
```

## 💻 사용 예제

### 기본 초기화
```cpp
// 플랫폼 시스템 초기화
Platform platform("EmulParty-Advance", 800, 600, 64, 32);
if (!platform.Initialize()) {
    std::cerr << "Platform initialization failed!" << std::endl;
    return -1;
}

Timer timer;
timer.start();
```

### 메인 루프
```cpp
std::array<uint8_t, 16> keypad{};
std::array<uint8_t, 64 * 32> video{};

bool running = true;
while (running) {
    // 입력 처리
    running = platform.ProcessInput(keypad);
    
    // 화면 업데이트
    platform.Update(video, 64);
    
    // 모드별 업데이트
    if (platform.IsFileSelected()) {
        std::string filename = platform.GetSelectedFile();
        // ROM 로드 로직
        platform.SwitchToGameMode();
    }
    
    // 타이밍 제어
    SDL_Delay(16);  // 60 FPS
}
```

### 계산기 모드 사용
```cpp
// 계산기 모드로 전환
platform.SwitchToCalculatorMode();

// 계산기 업데이트
platform.UpdateCalculator();

// 입력 완료 확인
if (platform.IsCalculatorInputReady()) {
    std::string result = platform.GetCalculatorInput();
    std::cout << "Calculator result: " << result << std::endl;
    platform.ClearCalculatorInput();
}
```

## 🔗 I/O 시스템 연동

### 콘솔 I/O 연동
```cpp
#include "../syscall/sdl_console_io.hpp"

// SDL 콘솔 I/O와 연동
SDLConsoleIO console_io(&platform);

// 입력 요청
console_io.request_input("Enter value: ");

// 입력 대기
while (!console_io.is_input_ready()) {
    platform.ProcessEvents();
    SDL_Delay(10);
}

// 입력 값 획득
std::string input = console_io.get_input();
```

## 🧪 테스트 및 검증

### 기본 기능 테스트
```cpp
void TestPlatformBasics() {
    Platform platform("Test", 800, 600, 64, 32);
    assert(platform.Initialize());
    
    // 키패드 테스트
    std::array<uint8_t, 16> keypad{};
    bool result = platform.ProcessInput(keypad);
    assert(result);
    
    // 화면 업데이트 테스트
    std::array<uint8_t, 64 * 32> video{};
    platform.Update(video, 64);
}
```

### 모드 전환 테스트
```cpp
void TestModeSwitch() {
    Platform platform("Test", 800, 600, 64, 32);
    platform.Initialize();
    
    // 각 모드로 전환 테스트
    platform.SwitchToGameMode();
    platform.SwitchToConsoleMode();
    platform.SwitchToCalculatorMode();
    
    // 파일 선택 테스트
    platform.ResetFileInput();
    assert(!platform.IsFileSelected());
}
```

## 🔧 빌드 설정

### CMake 설정
```cmake
# SDL2 의존성
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

# 플랫폼 라이브러리
add_library(platform
    src/platform/platform.cpp
    src/platform/timer.cpp
)

target_link_libraries(platform 
    ${SDL2_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES}
)

target_include_directories(platform PRIVATE
    include/
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)
```

### 환경 설정
```bash
# SDL2 환경 변수 설정
source set_sdl_env.sh

# 빌드
mkdir build && cd build
cmake ..
make
```

## 🎯 성능 최적화

### 렌더링 최적화
- **하드웨어 가속**: SDL2 하드웨어 렌더링 사용
- **텍스처 캐싱**: 폰트 텍스처 재사용
- **배치 렌더링**: 텍스트 렌더링 배치 처리

### 메모리 최적화
- **RAII 패턴**: 자동 리소스 관리
- **스마트 포인터**: 메모리 누수 방지
- **버퍼 재사용**: 입력 버퍼 재활용

## 🔗 연관 문서

- [Core Engine README](core-engine-README.md) - 32비트 엔진
- [Debugger README](debugger-README.md) - 디버깅 시스템
- [I/O Manager](../syscall/io_manager.hpp) - I/O 시스템

---

*EmulParty-Advance Platform System v1.0*  
*SDL2 기반 크로스 플랫폼 시스템*