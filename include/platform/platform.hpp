// platform.hpp - 키패드 참조로 수정
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <queue>
#include "common/constants.hpp"

class Platform {
private:
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
        GAME,
        CONSOLE_INPUT
    };

    InputMode current_mode_;
    std::string input_buffer_;
    bool file_selected_;
    
    std::queue<std::string> console_input_queue_;
    std::string current_console_input_;
    bool console_input_ready_;
    bool input_ready_;

    bool ProcessFileInput(SDL_Event& event);
    bool ProcessGameInput(SDL_Event& event, std::array<uint8_t, 16>& keypad); // 참조로 유지
    bool ProcessConsoleInput(SDL_Event& event);
    
    void RenderFileInputUI();
    void RenderConsoleInputUI();
    void RenderText(const std::string& text, int x, int y, SDL_Color color);
    void RenderTextCentered(const std::string& text, int y, SDL_Color color);
    std::vector<std::string> console_output_;

public:
    Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height);
    bool Initialize();
    
    // 🔧 **핵심 수정: 키패드를 실제로 전달받고 업데이트**
    bool ProcessInput(std::array<uint8_t, 16>& keypad);
    
    void Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch);
    void UpdateFileInput();
    void UpdateConsoleInput();
    ~Platform();
    void RenderConsoleOutput();
    void RenderTextQueue(const std::string& text);
    std::string GetSelectedFile();
    bool IsFileSelected() const;
    void ResetFileInput();
    void SwitchToGameMode();
    void SwitchToConsoleMode();
    void ForceConsoleMode();
    void ProcessEvents();
    bool IsConsoleInputReady() const;
    std::string GetConsoleInput();
    void ClearConsoleInput();
    void RequestConsoleInput(const std::string& prompt = "Enter input: ");
};