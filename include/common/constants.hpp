#pragma once

// CHIP-8은 64x32 해상도의 흑백 화면을 가집니다.
constexpr unsigned int VIDEO_WIDTH = 64;
constexpr unsigned int VIDEO_HEIGHT = 32;

// CHIP-8은 16개의 키를 가짐 (0x0 ~ 0xF)
constexpr unsigned int NUM_KEYS = 16;

// 화면 확대 배율 (64x32 화면을 크게 보이게 하기 위한 배수)
constexpr unsigned int SCALE = 20;