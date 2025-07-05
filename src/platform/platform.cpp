#include "platform.hpp"
#include "chip8_32.hpp"
#include <SDL2/SDL.h>
#include <iostream>

/// @brief Platform 클래스 생성자 - 멤버 초기화
/// @param title 윈도우 타이틀 (사용 안 함)
/// @param window_width 창의 가로 크기
/// @param window_height 창의 세로 크기
/// @param texture_width 텍스처의 너비 (CHIP-8 해상도: 64)
/// @param texture_height 텍스처의 높이 (CHIP-8 해상도: 32)
// 생성자: 멤버 변수 초기화 리스트 사용
Platform::Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height)
    : window_width_(window_width), window_height_(window_height),
      texture_width_(texture_width), texture_height_(texture_height),
      window_(nullptr), renderer_(nullptr), texture_(nullptr) {}

/// @brief SDL을 초기화하고 윈도우, 렌더러, 텍스처를 생성
/// @return 초기화 성공 여부
bool Platform::Initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 중앙에 윈도우 생성
    window_ = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        SDL_WINDOW_SHOWN
    );
    if (!window_) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 하드웨어 가속 렌더러 생성
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 텍스처는 64x32 해상도의 RGBA8888 포맷 (픽셀 스트리밍 가능)
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        texture_width_,
        texture_height_
    );
    if (!texture_) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        return false;
    }

    return true;
}

/// @brief 키보드 입력을 받아 CHIP-8 키패드 상태를 업데이트
/// @param keypad 16개 키의 상태 배열 (0: 해제, 1: 눌림)
/// @return 종료 신호 (true면 종료)
bool Platform::ProcessInput(std::array<uint8_t, NUM_KEYS>& keypad) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;

        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            bool is_pressed = (event.type == SDL_KEYDOWN);

            // CHIP-8 키패드와 PC 키보드 매핑
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

/// @brief 비디오 메모리를 화면에 그리는 함수
/// @param video CHIP-8의 64x32 비디오 버퍼 (0 또는 1로 픽셀 표현)
/// @param pitch 한 줄당 바이트 수 (보통 VIDEO_WIDTH * sizeof(uint32_t))
void Platform::Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch) {
    uint32_t pixels[VIDEO_WIDTH * VIDEO_HEIGHT];

    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
        // 흑백 픽셀 → RGBA 포맷 변환 (하얀색 또는 투명 배경)
        uint8_t pixel = video[i];
        if (pixel) {
            pixels[i] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), 255, 255, 255, 255); // 하양
        } else {
            pixels[i] = SDL_MapRGBA(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), 0, 0, 0, 255);       // 검정색
        }
    }

    SDL_UpdateTexture(texture_, nullptr, pixels, pitch);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255); // R,G,B,A → 검정
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

/// @brief Platform 소멸자 - SDL 자원 해제
Platform::~Platform() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}
