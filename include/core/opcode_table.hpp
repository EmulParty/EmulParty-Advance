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

    using OpcodeHandler = std::function<void(Chip8&, uint16_t)>;

    // 명령어 0x0000 ~ 0xFFFF 중, 상위 4비트 또는 특정 패턴으로 구분하여 핸들러를 매핑합니다.
    extern std::array<OpcodeHandler, 16> primary_table; // 예: 0x1000 => Jump

    /**
     * @brief 테이블을 초기화합니다. (초기 실행 시 한 번만 호출)
     * 각 명령어 패턴에 대해 해당 핸들러 함수를 등록합니다.
     */
    void Initialize();

    void Execute(Chip8& chip8, uint16_t opcode);

} // namespace OpcodeTable