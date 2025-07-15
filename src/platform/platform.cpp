// platform.cpp - 수정된 구현부
#include "platform.hpp"
#include "chip8_32.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>

// 생성자 - 초기화 순서를 헤더의 선언 순서와 맞춤
Platform::Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height)
    : window_(nullptr), renderer_(nullptr), texture_(nullptr), font_(nullptr),
      window_width_(window_width), window_height_(window_height),
      texture_width_(texture_width), texture_height_(texture_height),
      current_mode_(InputMode::FILE_INPUT),
      input_buffer_(""),
      file_selected_(false) {
    // title 매개변수 사용 (경고 제거)
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

    // 폰트 로드 시도 (여러 경로 시도)
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",           // Ubuntu/Debian
        "/usr/share/fonts/TTF/DejaVuSans.ttf",                      // Arch Linux
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", // CentOS/RHEL
        "/System/Library/Fonts/Helvetica.ttc",                      // macOS
        "C:\\Windows\\Fonts\\arial.ttf",                            // Windows
        "./fonts/DejaVuSans.ttf",                                   // 로컬 폰트 폴더
        nullptr
    };

    for (int i = 0; font_paths[i] != nullptr; ++i) {
        font_ = TTF_OpenFont(font_paths[i], 24);
        if (font_) {
            std::cout << "[INFO] Font loaded: " << font_paths[i] << std::endl;
            break;
        }
    }

    // 폰트 로드 실패 시 경고만 출력하고 계속 진행
    if (!font_) {
        std::cerr << "[WARN] Could not load TTF font, text will not be displayed" << std::endl;
        // 폰트 없이도 동작하도록 하기 위해 종료하지 않음
    }

    SDL_StartTextInput();
    
    std::cout << "[INFO] SDL initialization successful" << std::endl;
    std::cout << "[INFO] Text input enabled for file selection" << std::endl;
    return true;
}

bool Platform::ProcessInput(std::array<uint8_t, 16>& keypad) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;

        if (current_mode_ == InputMode::FILE_INPUT) {
            if (ProcessFileInput(event)) {
                return true;
            }
        }
        else {
            if (ProcessGameInput(event, keypad)) {
                return true;
            }
        }
    }    
    return false;
}

bool Platform::ProcessGameInput(SDL_Event& event, std::array<uint8_t, 16>& keypad) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool is_pressed = (event.type == SDL_KEYDOWN);
        switch (event.key.keysym.sym) {
            case SDLK_1: keypad[0x1] = is_pressed; break;
            case SDLK_2: keypad[0x2] = is_pressed; break;
            case SDLK_3: keypad[0x3] = is_pressed; break;
            case SDLK_4: keypad[0xC] = is_pressed; break;
            case SDLK_q: keypad[0x4] = is_pressed; break;
            case SDLK_w: keypad[0x5] = is_pressed; break;
            case SDLK_e: keypad[0x6] = is_pressed; break;
            case SDLK_r: keypad[0xD] = is_pressed; break;
            case SDLK_a: keypad[0x7] = is_pressed; break;
            case SDLK_s: keypad[0x8] = is_pressed; break;
            case SDLK_d: keypad[0x9] = is_pressed; break;
            case SDLK_f: keypad[0xE] = is_pressed; break;
            case SDLK_z: keypad[0xA] = is_pressed; break;
            case SDLK_x: keypad[0x0] = is_pressed; break;
            case SDLK_c: keypad[0xB] = is_pressed; break;
            case SDLK_v: keypad[0xF] = is_pressed; break;
            case SDLK_ESCAPE: return true; // ESC 키로 종료
            default: break;
        }
    } 
    return false;
}

bool Platform::ProcessFileInput(SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        // 영문자, 숫자, 점, 언더스코어, 하이픈만 허용
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
            return true; // ESC 키로 종료
        }
    }
    return false;
}

void Platform::RenderFileInputUI() {
    // 배경을 어두운 파란색으로 설정
    SDL_SetRenderDrawColor(renderer_, 20, 30, 50, 255);
    SDL_RenderClear(renderer_);

    // 폰트가 있는 경우에만 텍스트 렌더링
    if (font_) {
        // 색상 정의
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color yellow = {255, 255, 0, 255};
        SDL_Color green = {0, 255, 0, 255};
        SDL_Color cyan = {0, 255, 255, 255};

        RenderTextCentered("CHIP-8 Emulator", 80, white);
        RenderTextCentered("32-bit Extended Mode", 110, cyan);
        
        // 파일 입력 프롬프트
        RenderText("Enter ROM filename:", 50, 180, white);
        
        // 입력 박스 그리기
        SDL_Rect input_box = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &input_box);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &input_box);
        
        // 입력된 텍스트 + 커서 렌더링
        std::string display_text = input_buffer_ + "_";
        RenderText(display_text, 55, 220, green);
        
        // 도움말 텍스트
        RenderText("Examples: game.ch8, pong.rom", 50, 280, yellow);
        RenderText("Press ENTER to load ROM", 50, 310, white);
        RenderText("Press ESC to exit", 50, 340, white);
    } else {
        // 폰트가 없는 경우 간단한 사각형만 표시
        SDL_Rect placeholder = {50, 210, 540, 40};
        SDL_SetRenderDrawColor(renderer_, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer_, &placeholder);
        SDL_SetRenderDrawColor(renderer_, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer_, &placeholder);
        
        // 상태 표시용 작은 사각형
        SDL_Rect status = {50, 280, 200, 20};
        SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 255);
        SDL_RenderFillRect(renderer_, &status);
    }

    SDL_RenderPresent(renderer_);
}

void Platform::RenderText(const std::string& text, int x, int y, SDL_Color color) {
    if (!font_) return;  // 폰트가 없으면 렌더링 건너뛰기

    SDL_Surface* text_surface = TTF_RenderText_Blended(font_, text.c_str(), color);
    if (!text_surface) {
        std::cerr << "[WARN] TTF_RenderText_Blended failed: " << TTF_GetError() << std::endl;
        return;
    }

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer_, text_surface);
    if (!text_texture) {
        std::cerr << "[WARN] SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
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

void Platform::Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int /* pitch */) {
    uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
    
    // 서명 불일치 경고 해결
    for (size_t i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
        pixels[i] = video[i] ? 0xFFFFFFFF : 0x00000000;
    }

    int actual_pitch = VIDEO_WIDTH * sizeof(uint32_t);
    SDL_UpdateTexture(texture_, nullptr, pixels, actual_pitch);
    
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
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

Platform::~Platform() {
    SDL_StopTextInput();
    if (font_) TTF_CloseFont(font_);
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
}