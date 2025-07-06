#include "platform.hpp"
#include "chip8_32.hpp"
#include "timer.hpp"
#include "opcode_table_32.hpp"


#include <iostream>

/**
 * @file main.cpp
 * @brief CHIP-8 32비트 확장 에뮬레이터 실행 진입점
 */

int main(int argc, char* argv[]) {

    std::cout << "CHIP-8 32-bit Emulator 시작" << std::endl;

    // 명령줄 인자가 정확히 2개여야 함 (실행 파일 + ROM 파일)
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    // 32비트 Opcode 테이블 초기화 (명령어 처리용 디스패처 준비)
    OpcodeTable_32::Initialize();

    // ROM 경로 저장
    const char* rom_path = argv[1];

    // CHIP-8 32비트 시스템 초기화
    Chip8_32 chip8_32;

    // SDL 기반의 화면 출력 플랫폼 초기화 (화면 해상도는 기존과 동일)
    Platform platform(
        "CHIP-8 32-bit Emulator",   // 윈도우 제목
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
    if (!chip8_32.load_rom(rom_path)) {
        std::cerr << "Failed to load ROM: " << rom_path << std::endl;
        return 1;
    }

    // 메인 루프 종료 조건
    bool quit = false;

    std::cout << "32비트 CHIP-8 에뮬레이터 실행 중..." << std::endl;
    std::cout << "메모리 크기: " << MEMORY_SIZE_32 << " 바이트 (64KB)" << std::endl;
    std::cout << "레지스터 개수: " << NUM_REGISTERS_32 << "개 (32비트)" << std::endl;
    std::cout << "스택 크기: " << STACK_SIZE_32 << "단계" << std::endl;
    
   // 메인 루프: 키 입력 → 명령 실행 → 타이머 업데이트 → 화면 출력
    while (!quit) {  //20 사이클만 수행
        // 1. 사용자 입력 처리 (종료 키 포함)
        quit = platform.ProcessInput(chip8_32.keypad);

        // 2. 하나의 명령어 사이클 수행 (Fetch → Decode → Execute)
        chip8_32.cycle();

        // 4. 화면 갱신 요청이 있으면 렌더링 수행
        if (chip8_32.needs_redraw()) {
            
            // 비디오 버퍼 상태 확인
            const auto& video = chip8_32.get_video();
            int pixel_count = 0;
            for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; ++i) {
                if (video[i]) pixel_count++;
            }
            std::cout << "Active pixels: " << pixel_count << " / " << (VIDEO_WIDTH * VIDEO_HEIGHT) << std::endl;
            
            // 첫 몇 개 픽셀 상태 출력
            std::cout << "First 64 pixels: ";
            for (int i = 0; i < 64; ++i) {
                std::cout << (video[i] ? "█" : "·");
            }
            std::cout << std::endl;
            
            int video_pitch = VIDEO_WIDTH * sizeof(uint32_t);
            platform.Update(chip8_32.video, video_pitch);
            chip8_32.clear_draw_flag();
            
            std::cout << "Screen update completed." << std::endl;
        }
        
        // 5. (선택) CPU 속도 조절: 너무 빠른 실행 방지용 딜레이
        timer::delay(2); // 약 500Hz 실행
    }
    std::cout << "32비트 CHIP-8 에뮬레이터 종료" << std::endl;
    return 0;
}
