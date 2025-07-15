// platform.hpp - 수정된 헤더
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "common/constants.hpp"

class Platform {
private:
    // 멤버 변수 선언 순서를 생성자 초기화 순서와 맞춤
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    TTF_Font* font_;

    int window_width_;
    int window_height_;
    int texture_width_;
    int texture_height_;

    enum class InputMode {
        FILE_INPUT,
        GAME
    };

    InputMode current_mode_;
    std::string input_buffer_;
    bool file_selected_;

    bool ProcessFileInput(SDL_Event& event);
    bool ProcessGameInput(SDL_Event& event, std::array<uint8_t, 16>& keypad);
    
    // TTF 기반 텍스트 렌더링 함수들
    void RenderFileInputUI();
    void RenderText(const std::string& text, int x, int y, SDL_Color color);
    void RenderTextCentered(const std::string& text, int y, SDL_Color color);

public:
    Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height);
    bool Initialize();
    bool ProcessInput(std::array<uint8_t, 16>& keypad);
    void Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch);
    void UpdateFileInput();
    ~Platform();

    std::string GetSelectedFile();
    bool IsFileSelected() const;
    void ResetFileInput();
    void SwitchToGameMode();
};
