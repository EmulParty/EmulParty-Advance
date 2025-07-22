// platform.cpp - 콘솔 입력 완전 수정 버전
#include "platform.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

// 생성자
Platform::Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr), font_(nullptr),
      window_width_(window_width), window_height_(window_height),
      texture_width_(texture_width), texture_height_(texture_height),
      current_mode_(InputMode::FILE_INPUT),
      input_buffer_(""),
      file_selected_(false),
      console_input_ready_(false),
      input_ready_(false),
      // 계산기 관련 초기화
      calc_num1_(""),
      calc_num2_(""), 
      calc_operation_(""),
      calc_result_(""),
      calc_input_phase_(0),
      calc_input_ready_(false),
      calc_display_result_("") {
    std::cout << "[INFO] Platform initializing: " << title << std::endl;
}

bool Platform::Initialize() {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    // SDL과 TTF 초기화
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "[ERROR] SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "[ERROR] TTF_Init failed: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    const char* driver = SDL_GetCurrentVideoDriver();
    std::cout << "[INFO] SDL Video Driver: " << (driver ? driver : "NULL") << std::endl;

    // 창 생성
    window_ = SDL_CreateWindow("CHIP-8 Emulator", 
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              window_width_, window_height_, 
                              SDL_WINDOW_SHOWN);

    if (!window_) {
        std::cerr << "[ERROR] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    // 렌더러 생성
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::cerr << "[WARN] Hardware renderer failed, fallback to software: " << SDL_GetError() << std::endl;
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

    // 폰트 로드 시도
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "C:\\Windows\\Fonts\\arial.ttf",
        "./fonts/DejaVuSans.ttf",
        nullptr
    };

    for (int i = 0; font_paths[i] != nullptr; ++i) {
        font_ = TTF_OpenFont(font_paths[i], 24);
        if (font_) {
            std::cout << "[INFO] Font loaded: " << font_paths[i] << std::endl;
            break;
        }
    }

    if (!font_) {
        std::cerr << "[WARN] Could not load TTF font, text will not be displayed" << std::endl;
    }

    // 윈도우 생성 직후
    SDL_StartTextInput();
    
    std::cout << "[INFO] SDL initialization successful" << std::endl;
    return true;
}

bool Platform::ProcessInput(std::array<uint8_t, 16>& keypad) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;

        switch (current_mode_) {
            case InputMode::FILE_INPUT:
                if (ProcessFileInput(event)) return true;
                break;
            case InputMode::CONSOLE_INPUT:
                if (ProcessConsoleInput(event)) return true;
                break;
            case InputMode::CALCULATOR:
                if (ProcessCalculatorInput(event)) return true;
                break;
            case InputMode::GAME:
                // 🔧 **핵심 수정: 실제 키패드 전달**
                if (ProcessGameInput(event, keypad)) return true;
                break;
        }
    }    
    return false;
}

bool Platform::ProcessGameInput(SDL_Event& event, std::array<uint8_t, 16>& keypad) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool is_pressed = (event.type == SDL_KEYDOWN);
        
        // F1 키로 콘솔 모드 전환
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1) {
            SwitchToConsoleMode();
            return false;
        }
        
        // 🔧 **키패드 매핑 개선**
        switch (event.key.keysym.sym) {
            // 첫 번째 줄: 1 2 3 C
            case SDLK_1: keypad[0x1] = is_pressed; break;
            case SDLK_2: keypad[0x2] = is_pressed; break;
            case SDLK_3: keypad[0x3] = is_pressed; break;
            case SDLK_4: keypad[0xC] = is_pressed; break;
            
            // 두 번째 줄: 4 5 6 D  
            case SDLK_q: keypad[0x4] = is_pressed; break;
            case SDLK_w: keypad[0x5] = is_pressed; break;
            case SDLK_e: keypad[0x6] = is_pressed; break;
            case SDLK_r: keypad[0xD] = is_pressed; break;
            
            // 세 번째 줄: 7 8 9 E
            case SDLK_a: keypad[0x7] = is_pressed; break;
            case SDLK_s: keypad[0x8] = is_pressed; break;
            case SDLK_d: keypad[0x9] = is_pressed; break;
            case SDLK_f: keypad[0xE] = is_pressed; break;
            
            // 네 번째 줄: A 0 B F
            case SDLK_z: keypad[0xA] = is_pressed; break;
            case SDLK_x: keypad[0x0] = is_pressed; break;
            case SDLK_c: keypad[0xB] = is_pressed; break;
            case SDLK_v: keypad[0xF] = is_pressed; break;
            
            case SDLK_ESCAPE: return true;
            default: break;
        }
        
        // 🔧 **디버깅 출력 추가**
        if (is_pressed) {
            std::cout << "[Platform] Key pressed - updating keypad" << std::endl;
            for (int i = 0; i < 16; ++i) {
                if (keypad[i]) {
                    std::cout << "  Key 0x" << std::hex << i << " is pressed" << std::dec << std::endl;
                }
            }
        }
    } 
    return false;
}

bool Platform::ProcessFileInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        char c = event.text.text[0];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-') {
            input_buffer_ += event.text.text;
        }
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
            if (!input_buffer_.empty()) {
                file_selected_ = true;
                current_mode_ = InputMode::GAME;
                SDL_StopTextInput();
            }
        }
        else if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (!input_buffer_.empty()) {
                input_buffer_.pop_back();
            }
        }
        else if (event.key.keysym.sym == SDLK_ESCAPE) {
            return true;
        }
    }
    return false;
}

// SDL 이벤트 처리 (모드별 분기)
void Platform::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            // 종료 처리 (필요시)
        }
        switch (current_mode_) {
            case InputMode::FILE_INPUT:
                ProcessFileInput(event);
                break;
            case InputMode::CONSOLE_INPUT:
                ProcessConsoleInput(event);
                break;
            case InputMode::GAME: {
                std::array<uint8_t, 16> dummy_keypad{};
                ProcessGameInput(event, dummy_keypad);
                break;
            }
        }
    }
}

// 콘솔 입력 이벤트 처리
bool Platform::ProcessConsoleInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        current_console_input_ += event.text.text;
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RETURN) {
            if (!current_console_input_.empty()) {
                console_input_queue_.push(current_console_input_);
                console_input_ready_ = true;
                current_console_input_.clear();
                SwitchToGameMode(); // 입력 완료 후 게임 모드로 복귀
            }
        }
        else if (event.key.keysym.sym == SDLK_BACKSPACE) {
            if (!current_console_input_.empty()) {
                current_console_input_.pop_back();
            }
        }
        else if (event.key.keysym.sym == SDLK_ESCAPE) {
            SwitchToGameMode(); // ESC로 게임 모드 복귀
        }
    }
    return false;
}

void Platform::RenderFileInputUI() {
    SDL_SetRenderDrawColor(renderer_, 20, 30, 50, 255);
    SDL_RenderClear(renderer_);

    if (font_) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Color green = {0, 255, 0, 255};
        SDL_Color cyan = {0, 255, 255, 255};

        RenderTextCentered("CHIP-8 Emulator", 80, white);
        RenderTextCentered("32-bit Extended Mode", 110, cyan);
        
        RenderText("Enter ROM filename:", 50, 180, white);
        
        SDL_Rect input_box = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &input_box);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &input_box);
        
        std::string display_text = input_buffer_ + "_";
        RenderText(display_text, 55, 220, green);
        
        RenderText("Examples: game.ch8, pong.rom", 50, 280, yellow);
        RenderText("Press ENTER to load ROM", 50, 310, white);
        RenderText("Press ESC to exit", 50, 340, white);
    } else {
        SDL_Rect placeholder = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &placeholder);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &placeholder);
    }

    SDL_RenderPresent(renderer_);
}

// 🔧 **수정된 콘솔 입력 UI**
void Platform::RenderConsoleInputUI() {
    // 전체 화면을 어둡게
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 10, 20, 40, 200);
    SDL_Rect full_screen = {0, 0, window_width_, window_height_};
    SDL_RenderFillRect(renderer_, &full_screen);

    if (font_) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Color green = {0, 255, 0, 255};

        // 제목
        RenderTextCentered("CHIP-8 Console Input", 80, white);
        
        // 프롬프트
        RenderText("Enter ROM filename:", 50, 180, yellow);
        
        // 입력 박스
        SDL_Rect input_box = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &input_box);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &input_box);
        
        // 입력 텍스트
        std::string display_text = current_console_input_ + "_";
        RenderText(display_text, 55, 220, green);
        
        // 도움말
        RenderText("Examples: pong.ch8, tetris.ch32", 50, 280, yellow);
        RenderText("Press ENTER to load ROM", 50, 310, white);
        RenderText("Press ESC to cancel", 50, 340, white);
        
        std::cout << "[Platform] Console UI rendered - input: " << current_console_input_ << std::endl;
    } else {
        // 폰트 없을 때 기본 박스
        SDL_Rect placeholder = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &placeholder);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &placeholder);
        
        std::cout << "[Platform] Console UI rendered (no font)" << std::endl;
    }

    // 콘솔 출력도 함께 표시
    RenderConsoleOutput();
    SDL_RenderPresent(renderer_);
}

void Platform::RenderTextQueue(const std::string& text) {
    if (console_output_.size() >= 10) {
        console_output_.erase(console_output_.begin());
    }
    console_output_.push_back(text);
}

void Platform::RenderConsoleOutput() {
    int y = window_height_ - 200;
    for (const auto& line : console_output_) {
        RenderText(line, 10, y, {255, 255, 255, 255});
        y += 20;
    }
}

void Platform::RenderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font_) return;

    SDL_Surface* text_surface = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!text_surface) {
        return;
    }

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

void Platform::UpdateFileInput() {
    RenderFileInputUI();
}

void Platform::UpdateConsoleInput() {
    RenderConsoleInputUI();
}

// 🔧 **핵심: 수정된 Update 함수**
void Platform::Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int /* pitch */) {
    // 디버깅: 현재 모드 출력
    static int frame_count = 0;
    if (frame_count % 60 == 0) {  // 1초마다 출력
        std::cout << "[Platform] Current mode: " << static_cast<int>(current_mode_) << std::endl;
    }
    frame_count++;
    
    // 화면 기본 렌더링
    uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
    for (size_t i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
        pixels[i] = video[i] ? 0xFFFFFFFF : 0x00000000;
    }

    int actual_pitch = VIDEO_WIDTH * sizeof(uint32_t);
    SDL_UpdateTexture(texture_, nullptr, pixels, actual_pitch);
    
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    
    // 모드별 UI 렌더링 - 강제 실행
    switch (current_mode_) {
        case InputMode::CONSOLE_INPUT:
            std::cout << "[Platform] Rendering console input UI" << std::endl;
            RenderConsoleInputUI();
            return; // RenderConsoleInputUI()에서 이미 SDL_RenderPresent() 호출
        case InputMode::FILE_INPUT:
            std::cout << "[Platform] Rendering file input UI" << std::endl;
            RenderFileInputUI();
            return; // RenderFileInputUI()에서 이미 SDL_RenderPresent() 호출
        case InputMode::CALCULATOR:
            RenderCalculatorUI();
            return; // RenderCalculatorUI()에서 이미 SDL_RenderPresent() 호출RENDER
        case InputMode::GAME:
            // 게임 모드에서는 콘솔 출력만 표시
            RenderConsoleOutput();
            break;
    }
    
    SDL_RenderPresent(renderer_);
}

// 계산기 관련 함수들 추가 구현 
void Platform::SwitchToCalculatorMode() {
    current_mode_ = InputMode::CALCULATOR;
    calc_num1_.clear();
    calc_num2_.clear(); 
    calc_operation_.clear();
    calc_result_.clear();
    calc_input_phase_ = 0;
    calc_input_ready_ = false;
    calc_display_result_.clear();
    SDL_StartTextInput();
    
    std::cout << "[Platform] SWITCHED TO CALCULATOR MODE" << std::endl;
}

bool Platform::IsCalculatorInputReady() const {
    return calc_input_ready_;
}

std::string Platform::GetCalculatorInput() {
    calc_input_ready_ = false;
    // "10 5 2" 형식으로 반환
    return calc_num1_ + " " + calc_num2_ + " " + calc_operation_;
}

void Platform::ClearCalculatorInput() {
    calc_num1_.clear();
    calc_num2_.clear();
    calc_operation_.clear();
    calc_result_.clear();
    calc_input_phase_ = 0;
    calc_input_ready_ = false;
    calc_display_result_.clear();
}

void Platform::UpdateCalculator() {
    RenderCalculatorUI();
}

bool Platform::ProcessCalculatorInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        char c = event.text.text[0];
        
        if (calc_input_phase_ == 0) {  // 첫 번째 숫자 입력
            if (c >= '0' && c <= '9') {
                calc_num1_ += c;
            }
        }
        else if (calc_input_phase_ == 1) {  // 두 번째 숫자 입력
            if (c >= '0' && c <= '9') {
                calc_num2_ += c;
            }
        }
        else if (calc_input_phase_ == 2) {  // 연산자 입력
            if (c >= '1' && c <= '4') {
                calc_operation_ = c;
                CalculateResult();  // 즉시 계산 수행
                calc_input_ready_ = true;
            }
        }
    }
    else if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_SPACE) {
            // 스페이스로 다음 입력 단계로 이동
            if (calc_input_phase_ == 0 && !calc_num1_.empty()) {
                calc_input_phase_ = 1;
            }
            else if (calc_input_phase_ == 1 && !calc_num2_.empty()) {
                calc_input_phase_ = 2;
            }
        }
        else if (event.key.keysym.sym == SDLK_RETURN) {
            // 엔터로 계산기 종료하고 게임 모드 복귀
            if (calc_input_ready_) {
                SwitchToGameMode();
            }
        }
        else if (event.key.keysym.sym == SDLK_BACKSPACE) {
            // 백스페이스로 현재 입력 삭제
            if (calc_input_phase_ == 0 && !calc_num1_.empty()) {
                calc_num1_.pop_back();
            }
            else if (calc_input_phase_ == 1 && !calc_num2_.empty()) {
                calc_num2_.pop_back();
            }
            else if (calc_input_phase_ == 2) {
                calc_operation_.clear();
                calc_result_.clear();
                calc_display_result_.clear();
                calc_input_ready_ = false;
            }
        }
        else if (event.key.keysym.sym == SDLK_ESCAPE) {
            // ESC로 게임 모드 복귀
            SwitchToGameMode();
        }
    }
    return false;
}

void Platform::RenderCalculatorUI() {
    // 전체 화면을 어둡게
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 10, 20, 40, 200);
    SDL_Rect full_screen = {0, 0, window_width_, window_height_};
    SDL_RenderFillRect(renderer_, &full_screen);

    if (font_) {
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Color green = {0, 255, 0, 255};
        SDL_Color cyan = {0, 255, 255, 255};
        SDL_Color red = {255, 100, 100, 255};

        // 제목
        RenderTextCentered("CHIP-8 Calculator", 50, cyan);
        
        // 구분선
        SDL_SetRenderDrawColor(renderer_, 100, 150, 200, 255);
        SDL_Rect separator = {50, 80, 540, 2};
        SDL_RenderFillRect(renderer_, &separator);
        
        // 사용법 안내
        RenderText("Enter: number number operation", 50, 100, white);
        RenderText("Operations: 1=+ 2=- 3=* 4=/", 50, 120, yellow);
        
        // 입력 표시 영역
        std::string input_display = "Input: ";
        
        // 첫 번째 숫자
        if (calc_input_phase_ >= 0) {
            input_display += calc_num1_.empty() ? "_" : calc_num1_;
        }
        
        input_display += " ";
        
        // 두 번째 숫자  
        if (calc_input_phase_ >= 1) {
            input_display += calc_num2_.empty() ? "_" : calc_num2_;
        }
        
        input_display += " ";
        
        // 연산자
        if (calc_input_phase_ >= 2) {
            input_display += calc_operation_.empty() ? "_" : GetOperationSymbol(calc_operation_);
        }
        
        // 현재 입력 단계 표시
        if (calc_input_phase_ == 0) input_display += " ← Enter first number";
        else if (calc_input_phase_ == 1) input_display += " ← Enter second number"; 
        else if (calc_input_phase_ == 2 && calc_operation_.empty()) input_display += " ← Enter operation (1-4)";
        
        RenderText(input_display, 50, 160, green);
        
        // 단계별 가이드
        RenderText("       ↑    ↑   ↑", 50, 180, red);
        RenderText("     num1 num2 op", 50, 200, red);
        
        // 계산 결과 표시
        if (!calc_display_result_.empty()) {
            RenderText("Result: " + calc_display_result_, 50, 240, cyan);
        }
        
        // 조작 가이드
        RenderText("Press SPACE to move to next field", 50, 280, white);
        RenderText("Press ENTER to confirm and return to game", 50, 300, white);
        RenderText("Press ESC to cancel", 50, 320, white);
        
    } else {
        // 폰트 없을 때 기본 박스들
        SDL_Rect input_box1 = {50, 160, 100, 30};
        SDL_Rect input_box2 = {160, 160, 100, 30};
        SDL_Rect input_box3 = {270, 160, 50, 30};
        SDL_Rect result_box = {50, 240, 200, 30};
        
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &input_box1);
        SDL_RenderFillRect(renderer_, &input_box2);
        SDL_RenderFillRect(renderer_, &input_box3);
        SDL_RenderFillRect(renderer_, &result_box);
        
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &input_box1);
        SDL_RenderDrawRect(renderer_, &input_box2);
        SDL_RenderDrawRect(renderer_, &input_box3);
        SDL_RenderDrawRect(renderer_, &result_box);
    }

    SDL_RenderPresent(renderer_);
}

void Platform::CalculateResult() {
    if (calc_num1_.empty() || calc_num2_.empty() || calc_operation_.empty()) {
        return;
    }
    
    try {
        int num1 = std::stoi(calc_num1_);
        int num2 = std::stoi(calc_num2_);
        int result = 0;
        
        switch (calc_operation_[0]) {
            case '1': // 덧셈
                result = num1 + num2;
                break;
            case '2': // 뺄셈
                result = num1 - num2;
                break;
            case '3': // 곱셈
                result = num1 * num2;
                break;
            case '4': // 나눗셈
                if (num2 != 0) {
                    result = num1 / num2;
                } else {
                    calc_display_result_ = "Error: Division by zero";
                    return;
                }
                break;
            default:
                calc_display_result_ = "Error: Invalid operation";
                return;
        }
        
        calc_result_ = std::to_string(result);
        calc_display_result_ = std::to_string(num1) + " " + GetOperationSymbol(calc_operation_) + " " + std::to_string(num2) + " = " + calc_result_;
        
        std::cout << "[Calculator] " << calc_display_result_ << std::endl;
        
    } catch (const std::exception& e) {
        calc_display_result_ = "Error: Invalid input";
        std::cerr << "[Calculator] Parse error: " << e.what() << std::endl;
    }
}

std::string Platform::GetOperationSymbol(const std::string& op) {
    if (op.empty()) return "";
    
    switch (op[0]) {
        case '1': return "+";
        case '2': return "-";
        case '3': return "*";
        case '4': return "/";
        default: return "?";
    }
}

std::string Platform::GetSelectedFile() {
    return input_buffer_;
}

bool Platform::IsFileSelected() const {
    return file_selected_;
}

void Platform::ResetFileInput() {
    input_buffer_.clear();
    file_selected_ = false;
    current_mode_ = InputMode::FILE_INPUT;
    SDL_StartTextInput();
}

void Platform::SwitchToGameMode() {
    current_mode_ = InputMode::GAME;
    SDL_StopTextInput();
}

// 🔧 **수정된 SwitchToConsoleMode**
void Platform::SwitchToConsoleMode() {
    current_mode_ = InputMode::CONSOLE_INPUT;
    current_console_input_.clear();
    console_input_ready_ = false;
    SDL_StartTextInput();
    
    std::cout << "[Platform] SWITCHED TO CONSOLE MODE - current_mode_ = " 
              << static_cast<int>(current_mode_) << std::endl;
}

void Platform::RequestConsoleInput(const std::string& prompt) {
    std::cout << "[Platform] Requesting console input: " << prompt << std::endl;
    SwitchToConsoleMode();
}

// 🔧 **새로 추가: ForceConsoleMode**
void Platform::ForceConsoleMode() {
    std::cout << "[Platform] FORCING console mode..." << std::endl;
    current_mode_ = InputMode::CONSOLE_INPUT;
    current_console_input_.clear();
    console_input_ready_ = false;
    SDL_StartTextInput();
    
    // 즉시 한 번 렌더링
    SDL_SetRenderDrawColor(renderer_, 10, 20, 40, 255);
    SDL_RenderClear(renderer_);
    RenderConsoleInputUI();
    SDL_RenderPresent(renderer_);
    
    std::cout << "[Platform] Console mode forced and rendered" << std::endl;
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

Platform::~Platform() {
    SDL_StopTextInput();
    if (font_) TTF_CloseFont(font_);
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
}