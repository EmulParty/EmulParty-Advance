// sdl_console_io.cpp - 스택 프레임용 콘솔 I/O (완전 수정 버전)
#include "sdl_console_io.hpp"
#include "platform.hpp"
#include <iostream>
#include <cstring>

SDLConsoleIO::SDLConsoleIO(Platform* platform) 
    : platform_(platform), input_ready_(false) {
    if (!platform_) {
        std::cerr << "[SDLConsoleIO] Warning: Platform pointer is null!" << std::endl;
    }
    std::cout << "[SDLConsoleIO] Initialized with platform: " << platform_ << std::endl;
}

size_t SDLConsoleIO::read(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        std::cout << "[SDLConsoleIO] Invalid buffer or size" << std::endl;
        return 0;
    }

    std::cout << "[SDLConsoleIO] READ syscall - switching to CONSOLE mode for stack frame operations" << std::endl;

    if (platform_) {
        // 대기 중인 입력 확인
        if (!pending_input_.empty()) {
            std::cout << "[SDLConsoleIO] Using pending input: " << pending_input_ << std::endl;

            size_t bytes_to_copy = std::min(pending_input_.length(), size - 1);
            std::memcpy(buffer, pending_input_.c_str(), bytes_to_copy);
            buffer[bytes_to_copy] = '\0';

            pending_input_.clear();

            std::cout << "[SDLConsoleIO] Read: \"" << std::string(buffer, bytes_to_copy) << "\"" << std::endl;
            return bytes_to_copy;
        }

        // 🔧 **핵심 수정: 콘솔 모드로 전환 (스택 프레임 작업용)**
        std::cout << "[SDLConsoleIO] Switching to console mode for stack frame input..." << std::endl;
        platform_->SwitchToConsoleMode();


        // 🔧 **수정: 콘솔 입력이 들어올 때까지 SDL 이벤트 처리 루프**
        while (!platform_->IsConsoleInputReady()) {
            platform_->ProcessEvents();
            platform_->UpdateConsoleInput();
            SDL_Delay(16); // 약 60FPS 대기
        }

        // 🔧 **수정: 콘솔 입력 가져오기**
        std::string input = platform_->GetConsoleInput();
        if (!input.empty()) {
            std::cout << "[SDLConsoleIO] Got console input: " << input << std::endl;

            size_t bytes_to_copy = std::min(input.length(), size - 1);
            std::memcpy(buffer, input.c_str(), bytes_to_copy);
            buffer[bytes_to_copy] = '\0';

            // 🎯 입력 처리 후 게임 모드 복귀
            platform_->SwitchToGameMode();
            platform_->ProcessEvents();     // 모드 전환 적용 보장
            SDL_Delay(16);                  // 한 프레임 딜레이로 UI 전환 완료

            return bytes_to_copy;
        }

        std::cout << "[SDLConsoleIO] No console input received" << std::endl;
        return 0;
    }

    // Fallback: 터미널 입력 처리
    std::cout << "[SDL Console] Enter input: ";
    std::string input;
    std::getline(std::cin, input);

    if (input.empty()) {
        return 0;
    }

    size_t bytes_to_copy = std::min(input.length(), size - 1);
    std::memcpy(buffer, input.c_str(), bytes_to_copy);
    buffer[bytes_to_copy] = '\0';

    std::cout << "[SDLConsoleIO] Console Read: \"" << std::string(buffer, bytes_to_copy) << "\"" << std::endl;
    return bytes_to_copy;
}

size_t SDLConsoleIO::write(const char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return 0;
    }

    std::string output(buffer, size);
    output_queue_.push(output);
    std::cout << "[SDL Console Output] " << output << std::endl;

    if (platform_) {
        platform_->RenderTextQueue(output);
    }

    return size;
}

void SDLConsoleIO::update() {
    // 현재는 Platform에서 입력 처리하므로 비워둠
}

void SDLConsoleIO::render() {
    if (!platform_) return;

    while (!output_queue_.empty()) {
        std::string message = output_queue_.front();
        output_queue_.pop();
        platform_->RenderTextQueue(message);
    }
}

void SDLConsoleIO::clearInput() {
    input_buffer_.clear();
    input_ready_ = false;
    pending_input_.clear();
    while (!output_queue_.empty()){
        output_queue_.pop();
    }
    if(platform_) {
        platform_->ClearConsoleOutput();
    }
}

void SDLConsoleIO::setPendingInput(const std::string& input) {
    pending_input_ = input;
    std::cout << "[SDLConsoleIO] Pending input set: " << input << std::endl;
}

bool SDLConsoleIO::hasInput() const {
    return !pending_input_.empty();
}