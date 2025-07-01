#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../include/core/chip8.hpp"

/**
 * @file test_chip8.cpp
 * @brief CHIP-8의 핵심 기능에 대한 유닛 테스트 코드 (Catch2 기반)
 */

TEST_CASE("00E0: Clear screen", "[opcode]") {
    Chip8 chip8;
    // 화면을 채운 다음 00E0 명령어를 실행해서 클리어 되는지 확인
    std::fill(chip8.video.begin(), chip8.video.end(), 1);
    chip8.memory[0x200] = 0x00;
    chip8.memory[0x201] = 0xE0;
    chip8.pc = 0x200;
    chip8.cycle();

    REQUIRE(std::all_of(chip8.video.begin(), chip8.video.end(), [](uint8_t px) { return px == 0; }));
}

TEST_CASE("6XNN: Set Vx", "[opcode]") {
    Chip8 chip8;
    chip8.memory[0x200] = 0x60;  // 6XNN: Set VX
    chip8.memory[0x201] = 0x0A;  // V0 = 0x0A
    chip8.pc = 0x200;
    chip8.cycle();

    REQUIRE(chip8.V[0] == 0x0A);
}

TEST_CASE("7XNN: Add NN to Vx", "[opcode]") {
    Chip8 chip8;
    chip8.V[1] = 0x05;
    chip8.memory[0x200] = 0x71;  // 7XNN: Add to V1
    chip8.memory[0x201] = 0x03;  // V1 += 0x03
    chip8.pc = 0x200;
    chip8.cycle();

    REQUIRE(chip8.V[1] == 0x08);
}

TEST_CASE("ANNN: Set I", "[opcode]") {
    Chip8 chip8;
    chip8.memory[0x200] = 0xA2;  // ANNN
    chip8.memory[0x201] = 0xF0;  // I = 0x2F0
    chip8.pc = 0x200;
    chip8.cycle();

    REQUIRE(chip8.I == 0x2F0);
}

TEST_CASE("2NNN & 00EE: Call & Return", "[opcode]") {
    Chip8 chip8;
    // 서브루틴 호출
    chip8.memory[0x200] = 0x22;
    chip8.memory[0x201] = 0x10;  // CALL 0x210
    chip8.memory[0x210] = 0x00;
    chip8.memory[0x211] = 0xEE;  // RET

    chip8.pc = 0x200;
    chip8.cycle();
    REQUIRE(chip8.pc == 0x210);
    REQUIRE(chip8.sp == 1);

    chip8.cycle();  // RET 실행
    REQUIRE(chip8.pc == 0x202);
    REQUIRE(chip8.sp == 0);
}
