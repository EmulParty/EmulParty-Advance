#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <iostream>

constexpr uint8_t IMPLEMENTED_OPCODES = 16;  // 현재 구현된 opcode 수
constexpr uint8_t MAX_OPCODES = 20;          // 최대 확장 가능한 opcode 수

class Chip8_32; // 전방 선언 

/**
 * @brief 32비트 확장 CHIP-8_32 명령어 테이블
 * 32비트 opcode의 상위 8비트 (0x00 ~ 0xFF)를 기준으로 실행할 함수를 연결한다. 
 */

namespace OpcodeTable_32 {

    using OpcodeHandler32 = std::function<void(Chip8_32&, uint32_t)>;

    // 명령어 0x00000000 ~ 0xFFFFFFFF 중, 상위 4비트 또는 특정 패턴으로 구분하여 핸들러를 매핑합니다.
    extern std::array<OpcodeHandler32, 20> primary_table_32; 

    /**
     * @brief 테이블을 초기화합니다. (초기 실행 시 한 번만 호출)
     * 각 명령어 패턴에 대해 해당 핸들러 함수를 등록합니다.
     */
    void Initialize();

    void Execute(Chip8_32& chip8_32, uint32_t opcode);

} // namespace OpcodeTable_32
