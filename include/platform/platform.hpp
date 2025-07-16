#pragma once

#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include "../common/constants.hpp"

/**
 * @brief WSL2/X11 대응 Platform 클래스 (SDL2 기반)
 */
class Platform {
public:
    Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height);
    bool Initialize();
    bool ProcessInput(std::array<uint8_t, 16>& keypad);
    void Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch);
    ~Platform();

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;

    int window_width_;
    int window_height_;
    int texture_width_;
    int texture_height_;
};
