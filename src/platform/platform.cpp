// platform.cpp - 완전 리팩토링 버전 🔥
#include "platform.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

// ===========================================
// 🏗️ 생성자 & 소멸자
// ===========================================

Platform::Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr), font_(nullptr),
      window_width_(window_width), window_height_(window_height),
      texture_width_(texture_width), texture_height_(texture_height),
      current_mode_(InputMode::FILE_INPUT),  // 🎯 시작은 파일 입력 모드
      input_buffer_(""),
      file_selected_(false),
      console_input_ready_(false),
      input_ready_(false) {
    std::cout << "[Platform] Initializing: " << title << std::endl;
    std::cout << "[Platform] Initial mode: FILE_INPUT (" << static_cast<int>(current_mode_) << ")" << std::endl;
}

Platform::~Platform() {
    std::cout << "[Platform] Shutting down..." << std::endl;
    SDL_StopTextInput();
    if (font_) TTF_CloseFont(font_);
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
}

// ===========================================
// 🚀 초기화
// ===========================================

bool Platform::Initialize() {
    std::cout << "[Platform] Starting SDL initialization..." << std::endl;
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    // SDL 초기화
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "[ERROR] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // TTF 초기화
    if (TTF_Init() == -1) {
        std::cerr << "[ERROR] TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    // 드라이버 정보
    const char* driver = SDL_GetCurrentVideoDriver();
    std::cout << "[Platform] SDL Video Driver: " << (driver ? driver : "NULL") << std::endl;

    // 창 생성
    window_ = SDL_CreateWindow("CHIP-8 EMULATOR - STACK FRAME EDITION", 
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              window_width_, window_height_, 
                              SDL_WINDOW_SHOWN);

    if (!window_) {
        std::cerr << "[ERROR] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // 렌더러 생성 (하드웨어 가속 시도 후 소프트웨어 폴백)
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        std::cout << "[Platform] Hardware renderer failed, trying software..." << std::endl;
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer_) {
            std::cerr << "[ERROR] SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window_);
            TTF_Quit();
            SDL_Quit();
            return false;
        }
    }

    // 텍스처 생성
    texture_ = SDL_CreateTexture(renderer_,
                                SDL_PIXELFORMAT_RGBA8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                texture_width_, texture_height_);
    if (!texture_) {
        std::cerr << "[ERROR] SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // 폰트 로드 (여러 시스템 폰트 시도)
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",           // Linux
        "/usr/share/fonts/TTF/DejaVuSans.ttf",                      // Linux Alt
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", // Linux
        "/System/Library/Fonts/Helvetica.ttc",                      // macOS
        "C:\\Windows\\Fonts\\arial.ttf",                            // Windows
        "./fonts/DejaVuSans.ttf",                                   // Local
        nullptr
    };

    for (int i = 0; font_paths[i] != nullptr; ++i) {
        font_ = TTF_OpenFont(font_paths[i], 20);  // 적절한 폰트 크기
        if (font_) {
            std::cout << "[Platform] Font loaded: " << font_paths[i] << std::endl;
            break;
        }
    }

    if (!font_) {
        std::cerr << "[WARN] No font loaded - text will not be displayed!" << std::endl;
    }

    // 텍스트 입력 활성화 (파일 입력 모드용)
    SDL_StartTextInput();
    
    std::cout << "[Platform] SDL initialization successful!" << std::endl;
    std::cout << "[Platform] Current mode: " << static_cast<int>(current_mode_) << std::endl;
    
    return true;
}

// ===========================================
// 🔥 핵심: UPDATE 함수 (완전 수정)
// ===========================================

void Platform::Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int /* pitch */) {
    
    // 🚨 디버깅 로그 (1초마다)
    static int debug_counter = 0;
    if (debug_counter % 60 == 0) {
        std::cout << "[Platform] Update() called - Mode: " << static_cast<int>(current_mode_) << std::endl;
    }
    debug_counter++;
    
    // 🎯 **모드별 렌더링 - 가장 먼저 체크!**
    switch (current_mode_) {
        case InputMode::FILE_INPUT:
            RenderFileInputUI();
            return; // 파일 입력 UI만 렌더링하고 종료
            
        case InputMode::CONSOLE_INPUT:
            RenderConsoleInputUI();
            return; // 콘솔 입력 UI만 렌더링하고 종료
            
        case InputMode::GAME:
            // 게임 모드일 때만 아래 코드 실행
            break;
            
        default:
            std::cerr << "[Platform] ERROR: Unknown mode: " << static_cast<int>(current_mode_) << std::endl;
            return;
    }
    
    // 🎮 **게임 모드 렌더링**
    
    // 게임 화면 데이터를 픽셀로 변환
    uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
    for (size_t i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
        pixels[i] = video[i] ? 0xFFFFFFFF : 0x00000000;
    }

    // 텍스처 업데이트
    int actual_pitch = VIDEO_WIDTH * sizeof(uint32_t);
    SDL_UpdateTexture(texture_, nullptr, pixels, actual_pitch);
    
    // 화면 렌더링
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    
    // 게임 모드에서는 콘솔 출력도 표시
    RenderConsoleOutput();
    
    SDL_RenderPresent(renderer_);
}

// ===========================================
// 🎨 파일 입력 UI (E.P.A 스타일)
// ===========================================

void Platform::RenderFileInputUI() {
    // 완전 검은색 배경
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    if (font_) {
        SDL_Color white = {255, 255, 255, 255};
        
        int y_pos = 60;

        // 🎯 상단 스코어 영역 (1UP HIGH SCORE 2UP)
        RenderTextCentered("1UP      HIGH SCORE      2UP", y_pos, white);
        y_pos += 25;
        RenderTextCentered("00           40000           00", y_pos, white);
        y_pos += 40;

        // 🔹 구분선
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_Rect line = {50, y_pos, window_width_ - 100, 2};
        SDL_RenderFillRect(renderer_, &line);
        y_pos += 60;

        // 🎯 E.P.A 메인 타이틀
        RenderTextCentered("E . P . A", y_pos, white);
        y_pos += 80;

        // 📝 입력 안내문구
        RenderTextCentered("ENTER ROM FILE NAME:", y_pos, white);
        y_pos += 50;

        // 🎯 입력 박스 (중앙 정렬)
        int box_width = 500;
        int box_height = 40;
        int box_x = (window_width_ - box_width) / 2;
        
        SDL_Rect input_box = {box_x, y_pos, box_width, box_height};
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);  // 검은색 배경
        SDL_RenderFillRect(renderer_, &input_box);
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);  // 흰색 테두리
        SDL_RenderDrawRect(renderer_, &input_box);

        // 📟 입력 텍스트 + 깜빡이는 커서
        std::string display_text = input_buffer_;
        if (SDL_GetTicks() % 1000 < 500) {  // 0.5초마다 깜빡임
            display_text += "_";
        }
        if (!display_text.empty()) {
            RenderText(display_text, box_x + 10, y_pos + 10, white);
        }
        y_pos += 80;

        // 🎮 ESC 안내문구
        RenderTextCentered("PRESS ESC TO QUIT", y_pos, white);
        
        // 🔻 하단 크레딧
        RenderTextCentered("© WHITEHAT TEAM EAMULPARTY", window_height_ - 60, white);

    } else {
        // 🚨 폰트 없을 때 기본 박스
        std::cout << "[Platform] No font - rendering basic input box" << std::endl;
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_Rect input_box = {100, 250, window_width_ - 200, 50};
        SDL_RenderDrawRect(renderer_, &input_box);
    }

    SDL_RenderPresent(renderer_);
}

// ===========================================
// 🖥️ 콘솔 입력 UI (터미널 스타일)
// ===========================================

void Platform::RenderConsoleInputUI() {
    // 터미널 배경 - 완전 검은색
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);

    if (font_) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color light_gray = {180, 180, 180, 255};
        SDL_Color green = {0, 255, 0, 255};

        int y_pos = 30;

        // 🎯 터미널 헤더
        RenderTextCentered("████████████████████████████████████████", y_pos, white);
        y_pos += 20;
        RenderTextCentered("██  C H I P - 8   C O N S O L E  ██", y_pos, white);
        y_pos += 20;
        RenderTextCentered("██    STACK FRAME DEBUG MODE    ██", y_pos, green);
        y_pos += 20;
        RenderTextCentered("████████████████████████████████████████", y_pos, white);
        y_pos += 40;

        // 🖥️ 터미널 상태 라인
        RenderText("chip8@console:~/roms$ Console mode activated", 30, y_pos, green);
        y_pos += 20;
        RenderText("system> Status: Ready for input", 30, y_pos, white);
        y_pos += 20;
        RenderText("loader> Stack frame system online", 30, y_pos, white);
        y_pos += 30;

        // 🔹 구분선
        RenderTextCentered("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", y_pos, light_gray);
        y_pos += 30;

        // 📝 입력 프롬프트
        RenderText("stack_debug@chip8:~$ ", 30, y_pos, white);
        y_pos += 30;

        // 🎯 입력 박스
        SDL_Rect input_bg = {30, y_pos, window_width_ - 60, 35};
        SDL_SetRenderDrawColor(renderer_, 20, 20, 20, 255);
        SDL_RenderFillRect(renderer_, &input_bg);
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer_, &input_bg);

        // 📟 입력 텍스트 + 커서
        std::string display_text = current_console_input_;
        if (SDL_GetTicks() % 1000 < 500) {
            display_text += "█";
        }
        RenderText(display_text, 35, y_pos + 8, white);
        y_pos += 60;

        // 🎮 도움말
        RenderText("Commands: [ENTER] Execute, [ESC] Return to game", 30, y_pos, light_gray);
        
        // 🔻 하단 상태바
        SDL_Rect status_bar = {0, window_height_ - 25, window_width_, 25};
        SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer_, &status_bar);
        RenderText("CONSOLE MODE", 10, window_height_ - 18, green);
        RenderText("F1: Toggle", window_width_ - 80, window_height_ - 18, light_gray);

    } else {
        // 폰트 없을 때 기본 박스
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
        SDL_Rect input_box = {30, 200, window_width_ - 60, 30};
        SDL_RenderDrawRect(renderer_, &input_box);
    }

    SDL_RenderPresent(renderer_);
}

// ===========================================
// 🎯 이벤트 처리
// ===========================================

bool Platform::ProcessInput(std::array<uint8_t, 16>& keypad) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            std::cout << "[Platform] SDL_QUIT received" << std::endl;
            return true;
        }

        // 모드별 이벤트 처리
        switch (current_mode_) {
            case InputMode::FILE_INPUT:
                if (ProcessFileInput(event)) return true;
                break;
            case InputMode::CONSOLE_INPUT:
                if (ProcessConsoleInput(event)) return true;
                break;
            case InputMode::GAME:
                if (ProcessGameInput(event, keypad)) return true;
                break;
        }
    }    
    return false;
}

bool Platform::ProcessFileInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        char c = event.text.text[0];
        // 파일명에 허용되는 문자만 입력
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-') {
            input_buffer_ += event.text.text;
            std::cout << "[Platform] Input: " << input_buffer_ << std::endl;
        }
    }
    else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                if (!input_buffer_.empty()) {
                    std::cout << "[Platform] File selected: " << input_buffer_ << std::endl;
                    file_selected_ = true;
                    current_mode_ = InputMode::GAME;
                    SDL_StopTextInput();
                }
                break;
            case SDLK_BACKSPACE:
                if (!input_buffer_.empty()) {
                    input_buffer_.pop_back();
                    std::cout << "[Platform] Input: " << input_buffer_ << std::endl;
                }
                break;
            case SDLK_ESCAPE:
                std::cout << "[Platform] Exit requested" << std::endl;
                return true;
        }
    }
    return false;
}

bool Platform::ProcessGameInput(SDL_Event& event, std::array<uint8_t, 16>& keypad) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool is_pressed = (event.type == SDL_KEYDOWN);
        
        // F1 키로 콘솔 모드 전환
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
            std::cout << "[Platform] F1 pressed - switching to console mode" << std::endl;
            SwitchToConsoleMode();
            return false;
        }
        
        // CHIP-8 키패드 매핑
        switch (event.key.keysym.sym) {
            // 1 2 3 C
            case SDLK_1: keypad[0x1] = is_pressed; break;
            case SDLK_2: keypad[0x2] = is_pressed; break;
            case SDLK_3: keypad[0x3] = is_pressed; break;
            case SDLK_4: keypad[0xC] = is_pressed; break;
            
            // 4 5 6 D  
            case SDLK_q: keypad[0x4] = is_pressed; break;
            case SDLK_w: keypad[0x5] = is_pressed; break;
            case SDLK_e: keypad[0x6] = is_pressed; break;
            case SDLK_r: keypad[0xD] = is_pressed; break;
            
            // 7 8 9 E
            case SDLK_a: keypad[0x7] = is_pressed; break;
            case SDLK_s: keypad[0x8] = is_pressed; break;
            case SDLK_d: keypad[0x9] = is_pressed; break;
            case SDLK_f: keypad[0xE] = is_pressed; break;
            
            // A 0 B F
            case SDLK_z: keypad[0xA] = is_pressed; break;
            case SDLK_x: keypad[0x0] = is_pressed; break;
            case SDLK_c: keypad[0xB] = is_pressed; break;
            case SDLK_v: keypad[0xF] = is_pressed; break;
            
            case SDLK_ESCAPE: 
                std::cout << "[Platform] ESC in game mode - exiting" << std::endl;
                return true;
        }
    } 
    return false;
}

bool Platform::ProcessConsoleInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        current_console_input_ += event.text.text;
    }
    else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_RETURN:
                if (!current_console_input_.empty()) {
                    console_input_queue_.push(current_console_input_);
                    console_input_ready_ = true;
                    std::cout << "[Platform] Console input: " << current_console_input_ << std::endl;
                    current_console_input_.clear();
                    SwitchToGameMode(); // 입력 완료 후 게임 모드로 복귀
                }
                break;
            case SDLK_BACKSPACE:
                if (!current_console_input_.empty()) {
                    current_console_input_.pop_back();
                }
                break;
            case SDLK_ESCAPE:
                std::cout << "[Platform] ESC in console mode - returning to game" << std::endl;
                SwitchToGameMode();
                break;
        }
    }
    return false;
}

// ===========================================
// 🔧 모드 전환 함수들
// ===========================================

void Platform::SwitchToGameMode() {
    std::cout << "[Platform] Switching to GAME mode" << std::endl;
    current_mode_ = InputMode::GAME;
    SDL_StopTextInput();
}

void Platform::SwitchToConsoleMode() {
    std::cout << "[Platform] Switching to CONSOLE mode" << std::endl;
    current_mode_ = InputMode::CONSOLE_INPUT;
    current_console_input_.clear();
    console_input_ready_ = false;
    SDL_StartTextInput();
}

void Platform::ResetFileInput() {
    std::cout << "[Platform] Resetting file input" << std::endl;
    input_buffer_.clear();
    file_selected_ = false;
    current_mode_ = InputMode::FILE_INPUT;
    SDL_StartTextInput();
}

// ===========================================
// 🎨 렌더링 헬퍼 함수들
// ===========================================

void Platform::RenderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font_) return;

    SDL_Surface* text_surface = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!text_surface) return;

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer_, text_surface);
    if (!text_texture) {
        SDL_FreeSurface(text_surface);
        return;
    }

    SDL_Rect dest_rect = {x, y, text_surface->w, text_surface->h};
    SDL_RenderCopy(renderer_, text_texture, nullptr, &dest_rect);

    SDL_DestroyTexture(text_texture);
    SDL_FreeSurface(text_surface);
}

void Platform::RenderTextCentered(const std::string& text, int y, SDL_Color color) {
    if (!font_) return;

    int text_width, text_height;
    TTF_SizeText(font_, text.c_str(), &text_width, &text_height);
    int x = (window_width_ - text_width) / 2;
    RenderText(text, x, y, color);
}

void Platform::RenderConsoleOutput() {
    if (console_output_.empty()) return;
    
    int y = window_height_ - 200;
    for (const auto& line : console_output_) {
        RenderText(line, 10, y, {0, 255, 0, 255});  // 초록색으로 콘솔 출력
        y += 20;
        if (y > window_height_ - 20) break;  // 화면 하단 넘어가지 않게
    }
}

void Platform::RenderTextQueue(const std::string& text) {
    if (console_output_.size() >= 8) {  // 최대 8줄만 유지
        console_output_.erase(console_output_.begin());
    }
    console_output_.push_back(text);
}

// ===========================================
// 🔧 Getter/Setter 함수들
// ===========================================

std::string Platform::GetSelectedFile() {
    return input_buffer_;
}

bool Platform::IsFileSelected() const {
    return file_selected_;
}

bool Platform::IsConsoleInputReady() const {
    return console_input_ready_ || !console_input_queue_.empty();
}

std::string Platform::GetConsoleInput() {
    console_input_ready_ = false;
    if (!console_input_queue_.empty()) {
        std::string input = console_input_queue_.front();
        console_input_queue_.pop();
        return input;
    }
    return "";
}

void Platform::ClearConsoleInput() {
    while (!console_input_queue_.empty()) {
        console_input_queue_.pop();
    }
    current_console_input_.clear();
    console_input_ready_ = false;
}

// ===========================================
// 🔧 기타 함수들 (호환성 유지)
// ===========================================

void Platform::UpdateFileInput() {
    // Update()에서 자동으로 처리됨
}

void Platform::UpdateConsoleInput() {
    // Update()에서 자동으로 처리됨
}

void Platform::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // 처리는 ProcessInput에서
        }
        // 실제 처리는 ProcessInput에서 수행
    }
}

void Platform::ForceConsoleMode() {
    std::cout << "[Platform] FORCING console mode..." << std::endl;
    SwitchToConsoleMode();
}

void Platform::RequestConsoleInput(const std::string& prompt) {
    std::cout << "[Platform] Console input requested: " << prompt << std::endl;
    SwitchToConsoleMode();
}