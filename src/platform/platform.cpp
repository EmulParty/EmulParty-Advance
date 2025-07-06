#include "platform.hpp"
#include "chip8_32.hpp"
#include <SDL2/SDL.h>
#include <iostream>

Platform::Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height)
    : window_width_(window_width), window_height_(window_height),
      texture_width_(texture_width), texture_height_(texture_height),
      window_(nullptr), renderer_(nullptr), texture_(nullptr) {}

bool Platform::Initialize() {
    // SDL_HINT로 소프트웨어 렌더러 우선
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    // SDL 초기화
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "[ERROR] SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // 환경 변수 출력
    const char* driver = SDL_GetCurrentVideoDriver();
    std::cout << "[INFO] SDL Video Driver: " << (driver ? driver : "NULL") << std::endl;

    // 창 생성
    window_ = SDL_CreateWindow(
        "CHIP-8 Emulator (WSL2)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::cerr << "[ERROR] SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // 렌더러 생성 (소프트웨어 fallback)
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::cerr << "[WARN] Hardware renderer failed, fallback to software: " << SDL_GetError() << std::endl;
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer_) {
            std::cerr << "[ERROR] SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            exit(1);
        }
    }

    // 텍스처 생성
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        texture_width_,
        texture_height_
    );
    if (!texture_) {
        std::cerr << "[ERROR] SDL_CreateTexture failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    std::cout << "[INFO] SDL initialization successful\n";
    return true;
}

bool Platform::ProcessInput(std::array<uint8_t, NUM_KEYS>& keypad) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
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
            }
        }
    }
    return false;
}

void Platform::Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch) {
    uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
        pixels[i] = video[i] ? 0xFFFFFFFF : 0x00000000;
    }

    SDL_UpdateTexture(texture_, nullptr, pixels, pitch);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

Platform::~Platform() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}
