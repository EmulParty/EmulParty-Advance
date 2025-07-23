#pragma once

#include "io_device.hpp"
#include <string>
#include <queue>

class Platform; // 전방 선언

/**
 * @brief SDL2 기반 콘솔 I/O 장치
 * 
 * SDL2 창에서 텍스트 입력을 받고 출력을 화면에 표시합니다.
 */
class SDLConsoleIO : public IODevice {
private:
    Platform* platform_;
    std::queue<std::string> output_queue_;
    std::string input_buffer_;
    bool input_ready_;
    std::string pending_input_; // 대기 중인 입력

public:
    explicit SDLConsoleIO(Platform* platform);
    virtual ~SDLConsoleIO() = default;

    // IODevice 인터페이스 구현
    size_t read(char* buffer, size_t size) override;
    size_t write(const char* buffer, size_t size) override;
    bool is_readable() const override { return true; }
    bool is_writable() const override { return true; }
    const char* get_device_type() const override { return "SDL_Console_IO"; }

    // SDL2 업데이트 함수들
    void update();
    void render();
    void clearInput();
    
    // 입력 설정
    void setPendingInput(const std::string& input);
    bool hasInput() const;
};