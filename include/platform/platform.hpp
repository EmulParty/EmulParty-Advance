#pragma once

#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include "common/constants.hpp"

/**
 * @brief Platform 클래스는 SDL2 기반 입출력을 담당하는 플랫폼 추상화 계층입니다.
 * - 화면 렌더링 (video buffer -> 화면)
 * - 키 입력 처리 (PC 키보드 -> CHIP-8 키패드 배열)
 * - SDL 초기화 및 종료 관리
 */
class Platform {
public:
    /**
     * @brief Platform 생성자
     * @param title 창 제목
     * @param window_width 윈도우 너비 (픽셀 기준)
     * @param window_height 윈도우 높이
     * @param texture_width 내부 텍스처 너비 (CHIP-8 원본 해상도)
     * @param texture_height 내부 텍스처 높이
     */

    Platform(const char* title, int window_width, int window_height, int texture_width, int texture_height);

    /**
     * @brief SDL 초기화 및 창/렌더러/텍스처 준비
     * @return 초기화 성공 여부
    */
    bool Initialize();

    /**
     * @brief 사용자 입력 처리
     * @param keypad 키보드 배열 (CHIP-8 16키)
     * @return SDL 종료 이벤트 발생 여부
    */
    bool ProcessInput(std::array<uint8_t, 16>& keypad);

    /**
     * @brief CHIP-8 화면 출력
     * @param video 비디오 메모리 배열 (흑백 64x32)
     * @param pitch 한 줄당 바이트 수 (ex: VIDEO_WIDTH * sizeof(uint32_t))
    */
   void Update(const std::array<uint8_t, VIDEO_WIDTH * VIDEO_HEIGHT>& video, int pitch);

   /**
    * * @brief 소멸자: SDL 리소스 해제
   */
  ~Platform();

private:
    SDL_Window* window_;      // SDL 윈도우 객체
    SDL_Renderer* renderer_;  // SDL 렌더러 객체
    SDL_Texture* texture_;    // SDL 텍스처 객체

    int window_width_;        // 윈도우 너비
    int window_height_;       // 윈도우 높이
    int texture_width_;       // 텍스처 너비
    int texture_height_;      // 텍스처 높이
};
