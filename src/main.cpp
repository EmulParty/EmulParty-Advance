#include "platform.hpp"
#include "chip8.hpp"
#include "timer.hpp"
#include "opcode_table.hpp"


#include <iostream>

/**
 * @file main.cpp
 * @brief CHIP-8 에뮬레이터 실행 진입점
 */

int main(int argc, char* argv[]) {

    std::cout << "main 시작" << std::endl;

    // 명령줄 인자가 정확히 2개여야 함 (실행 파일 + ROM 파일)
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    // Opcode 테이블 초기화 (명령어 처리용 디스패처 준비)
    OpcodeTable::Initialize();

    // ROM 경로 저장
    const char* rom_path = argv[1];

    // CHIP-8 시스템 초기화
    Chip8 chip8;

    // SDL 기반의 화면 출력 플랫폼 초기화
    Platform platform(
        "CHIP-8 Emulator",          // 윈도우 제목
        VIDEO_WIDTH * SCALE,        // 창 너비 (픽셀 * 배율)
        VIDEO_HEIGHT * SCALE,       // 창 높이
        VIDEO_WIDTH,                // 텍스처 너비 (64)
        VIDEO_HEIGHT                // 텍스처 높이 (32)
    );

    // SDL 플랫폼 초기화 실패 시 종료
    if (!platform.Initialize()) {
        std::cerr << "Platform initialization failed!" << std::endl;
        return 1;
    }

    // ROM 파일 로딩 실패 시 종료
    if (!chip8.load_rom(rom_path)) {
        std::cerr << "Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }

    // 메인 루프 종료 조건
    bool quit = false;

    // 60Hz 타이머 조절을 위한 시간 기준값
    uint32_t last_timer_update = timer::get_ticks();
    const uint32_t timer_interval = 1000 / 60; // 60Hz = 16.67ms

    // 메인 루프: 키 입력 → 명령 실행 → 타이머 업데이트 → 화면 출력
    while (!quit) {
        // 1. 사용자 입력 처리 (종료 키 포함)
        quit = platform.ProcessInput(chip8.keypad);

        // 2. 하나의 명령어 사이클 수행 (Fetch → Decode → Execute)
        chip8.cycle();

        // 3. 60Hz 타이머 값 감소 (Delay / Sound Timer)
        uint32_t current_time = timer::get_ticks();
        if (current_time - last_timer_update >= timer_interval) {
            if (chip8.delay_timer > 0) --chip8.delay_timer;
            if (chip8.sound_timer > 0) --chip8.sound_timer;
            last_timer_update = current_time;
        }

        // 4. 화면 갱신 요청이 있으면 렌더링 수행
        if (chip8.needs_redraw()) {
            int video_pitch = VIDEO_WIDTH * sizeof(uint32_t);
            platform.Update(chip8.video, video_pitch);
            chip8.clear_draw_flag();
        }

        // 5. (선택) CPU 속도 조절: 너무 빠른 실행 방지용 딜레이
        timer::delay(2); // 약 500Hz 실행
    }

    return 0;
}
