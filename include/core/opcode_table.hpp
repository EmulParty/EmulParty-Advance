#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <iostream>

class Chip8; // 전방 선언 (헤더에서 Chip8 전체 정의 불필요)

/**
 * @brief 함수 포인터 테이블을 정의하는 클래스 혹은 네임스페이스
 * opcode의 상위 nibble 또는 패턴을 기준으로 실행할 함수를 연결합니다.
 */

namespace OpcodeTable {

    // 32비트 명령어 핸들러 타입 정의
    using OpcodeHandler32 = std::function<void(Chip8&, uint32_t)>;

    // 상위 8비트(0x00 ~ 0xFF)를 키로 사용하여 명령어 핸들러를 매핑하는 테이블    
    extern std::array<OpcodeHandler32, 256> primary_table; 
    /**
     * @brief 테이블을 초기화합니다. (초기 실행 시 한 번만 호출)
     * 각 명령어 패턴에 대해 해당 핸들러 함수를 등록합니다.
     */
    void Initialize();

    // 명령어 실행 (32비트 opcode 사용)
    void Execute(Chip8& chip8, uint32_t opcode);

} // namespace OpcodeTable
