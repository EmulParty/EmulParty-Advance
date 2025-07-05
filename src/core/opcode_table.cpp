#include "opcode_table.hpp"
#include "chip8.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring> // memset, memcpy
#include <random>  // for CXNN
#include <fstream>
#include <vector>

namespace OpcodeTable {

    // 16개의 주요 명령 그룹(상위 4비트로 구분)을 처리하기 위한 함수 테이블
    std::array<OpcodeTable::OpcodeHandler32, 256> primary_table;

    /// @brief 화면을 지우는 명령 (00E0)
    void OP_00E0(Chip8& chip8, uint32_t) {
        chip8.get_video().fill(0);
        chip8.set_draw_flag(true);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief 서브루틴 반환 명령 (00EE)
    void OP_00EE(Chip8& chip8, uint32_t) {
        chip8.set_sp(chip8.get_sp() - 1);
        chip8.set_pc(chip8.stack_at(chip8.get_sp()));
    }

    /// @brief 절대 주소로 점프 (1NNN)
    void OP_1NNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_pc(opcode & 0x00FFFFFF);
    }

    /// @brief 서브루틴 호출 (2NNN)
    void OP_2NNN(Chip8& chip8, uint32_t opcode) {
        chip8.stack_at(chip8.get_sp()) = chip8.get_pc();
        chip8.set_sp(chip8.get_sp() + 1); // 스택 포인터 증가
        chip8.set_pc(opcode & 0x00FFFFFF);
    }

    /// @brief Vx가 NN과 같으면 다음 명령어 건너뜀 (3XNN)
    void OP_3XNN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 8비트 레지스터
        uint16_t nn = opcode & 0x0000FFFF;        // 16비트 값
        chip8.set_pc(chip8.get_pc() + (chip8.get_V(x) == nn ? 8 : 4));
    }

    /// @brief Vx가 NN과 다르면 다음 명령어 건너뜀 (4XNN)
    void OP_4XNN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint16_t nn = opcode & 0x0000FFFF;
        chip8.set_pc(chip8.get_pc() + (chip8.get_V(x) != nn ? 8 : 4));
    }

    /// @brief Vx == Vy면 다음 명령어 건너뜀 (5XY0)
    void OP_5XY0(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint8_t y = (opcode & 0x0000FF00) >> 8;
        uint8_t subcode = opcode & 0x000000FF;
        chip8.set_pc(chip8.get_pc() + (subcode == 0 && chip8.get_V(x) == chip8.get_V(y) ? 8 : 4));
    }

    /// @brief Vx에 NN 저장 (6XNN)
    void OP_6XNN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        chip8.set_V(x, opcode & 0x0000FFFF);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief Vx에 NN 더하기 (7XNN)
    void OP_7XNN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint16_t nn = opcode & 0x0000FFFF; // 16비트 값
        chip8.set_V(x, chip8.get_V(x) + nn);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief Vx와 Vy 간 다양한 연산 수행 (8XYN)
    void OP_8XYN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16; // 8비트
        uint8_t y = (opcode & 0x0000FF00) >> 8;  // 8비트
        uint8_t n = opcode & 0x000000FF;         // 8비트
        uint8_t vx = chip8.get_V(x);
        uint8_t vy = chip8.get_V(y);

        switch (n) {
            case 0x0: chip8.set_V(x, vy); break;
            case 0x1: chip8.set_V(x, vx | vy); break;
            case 0x2: chip8.set_V(x, vx & vy); break;
            case 0x3: chip8.set_V(x, vx ^ vy); break;
            case 0x4: {
                uint16_t sum = vx + vy;
                chip8.set_V(0xF, sum > 0xFF);  // carry flag
                chip8.set_V(x, sum & 0xFF);
                break;
            }
            case 0x5:
                chip8.set_V(0xF, vx > vy);  // borrow flag
                chip8.set_V(x, vx - vy);
                break;
            case 0x6:
                chip8.set_V(0xF, vx & 0x1);  // LSB 저장
                chip8.set_V(x, vx >> 1);
                break;
            case 0x7:
                chip8.set_V(0xF, vy > vx);  // borrow flag
                chip8.set_V(x, vy - vx);
                break;
            case 0xE:
                chip8.set_V(0xF, (vx & 0x80) >> 7);  // MSB 저장
                chip8.set_V(x, vx << 1);
                break;
        }
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief Vx != Vy면 다음 명령어 건너뜀 (9XY0)
    void OP_9XY0(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint8_t y = (opcode & 0x0000FF00) >> 8;
        uint8_t subcode = opcode & 0x000000FF;
        chip8.set_pc(chip8.get_pc() + (subcode == 0 && chip8.get_V(x) != chip8.get_V(y) ? 8 : 4));
    }

    /// @brief I에 NNN 저장 (ANNN)
    void OP_ANNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_I(opcode & 0x00FFFFFF);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief PC = NNN + V0 (BNNN)
    void OP_BNNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_pc((opcode & 0x00FFFFFF) + chip8.get_V(0));
    }

    /// @brief Vx에 rand() & NN 저장 (CXNN)
    void OP_CXNN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        chip8.set_V(x, (rand() % 256) & (opcode & 0x0000FFFF));
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief 스프라이트 그리기 (DXYN)
    void OP_DXYN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = chip8.get_V((opcode & 0x00FF0000) >> 16);
        uint8_t y = chip8.get_V((opcode & 0x0000FF00) >> 8);
        uint8_t height = opcode & 0x000000FF;
        chip8.set_V(0xF, 0);  // 충돌 감지 플래그

        for (int row = 0; row < height; ++row) {
            uint8_t sprite = chip8.get_memory(chip8.get_I() + row);
            for (int col = 0; col < 8; ++col) {
                if (sprite & (0x80 >> col)) {
                    uint32_t index = ((y + row) % VIDEO_HEIGHT) * VIDEO_WIDTH + ((x + col) % VIDEO_WIDTH);
                    if (chip8.get_video(index)) chip8.set_V(0xF, 1);
                    chip8.set_video(index, chip8.get_video(index) ^ 1);
                }
            }
        }
        chip8.set_draw_flag(true);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief 키 입력 조건 분기 (EX9E, EXA1)
    void OP_EX(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint8_t subcode = opcode & 0x000000FF; // 하위 8비트
        uint8_t key = chip8.get_V(x);
        if (subcode == 0x9E)
            chip8.set_pc(chip8.get_pc() + (chip8.get_key(key) ? 8 : 4));
        else if (subcode == 0xA1)
            chip8.set_pc(chip8.get_pc() + (!chip8.get_key(key) ? 8 : 4));
        else
            chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief Fx 계열 확장 명령들 처리
    void OP_FX(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint8_t nn = opcode & 0x000000FF;
        switch (nn) {
            case 0x07: chip8.set_V(x, chip8.get_delay_timer()); break;
            case 0x0A: {
                for (int i = 0; i < 16; ++i) {
                    if (chip8.get_key(i)) {
                        chip8.set_V(x, i);
                        chip8.set_pc(chip8.get_pc() + 4);
                        return;
                    }
                }
                return;  // 키가 눌릴 때까지 대기
            }
            case 0x15: chip8.set_delay_timer(chip8.get_V(x)); break;
            case 0x18: chip8.set_sound_timer(chip8.get_V(x)); break;
            case 0x1E: chip8.set_I(chip8.get_I() + chip8.get_V(x)); break;
            case 0x29: chip8.set_I(chip8.get_V(x) * 5); break;  // 폰트 주소
            case 0x33: {  // BCD 변환
                uint8_t vx = chip8.get_V(x);
                chip8.set_memory(chip8.get_I(), vx / 100);
                chip8.set_memory(chip8.get_I() + 1, (vx / 10) % 10);
                chip8.set_memory(chip8.get_I() + 2, vx % 10);
                break;
            }
            case 0x55:
                for (int i = 0; i <= x; ++i)
                    chip8.set_memory(chip8.get_I() + i, chip8.get_V(i));
                break;
            case 0x65:
                for (int i = 0; i <= x; ++i)
                    chip8.set_V(i, chip8.get_memory(chip8.get_I() + i));
                break;
        }
        chip8.set_pc(chip8.get_pc() + 4);
    }

    /// @brief opcode 상위 4비트 기반으로 핸들러 함수 등록
    void Initialize() {
        primary_table.fill(nullptr);

        primary_table[0x00] = [](Chip8& chip8, uint32_t opcode) {
            switch (opcode & 0x000000FF) { // 하위 8비트 확인
                case 0xE0: OP_00E0(chip8, opcode); break;
                case 0xEE: OP_00EE(chip8, opcode); break;
                default:
                    std::cerr << "Unknown 0x00 opcode: " << std::hex << opcode << "\n";
                    chip8.set_pc(chip8.get_pc() + 4);
                    break;
            }
        };

        // 32비트 방식: 기존 0x1 -> 0x01, 0x2 -> 0x02로 매핑
        primary_table[0x01] = OP_1NNN;
        primary_table[0x02] = OP_2NNN;
        primary_table[0x03] = OP_3XNN;
        primary_table[0x04] = OP_4XNN;
        primary_table[0x05] = OP_5XY0;
        primary_table[0x06] = OP_6XNN;
        primary_table[0x07] = OP_7XNN;
        primary_table[0x08] = OP_8XYN;
        primary_table[0x09] = OP_9XY0;
        primary_table[0x0A] = OP_ANNN;
        primary_table[0x0B] = OP_BNNN;
        primary_table[0x0C] = OP_CXNN;
        primary_table[0x0D] = OP_DXYN;
        primary_table[0x0E] = OP_EX;
        primary_table[0x0F] = OP_FX;
    }

    /// @brief opcode를 상위 8비트로 분기하여 실행
    void Execute(Chip8& chip8, uint32_t opcode) {
        uint8_t index = (opcode & 0xFF000000) >> 24; // 상위 8비트만 추출
        OpcodeTable::OpcodeHandler32 handler = primary_table[index];

        if (handler)
            handler(chip8, opcode);
        else {
            std::cerr << "Unknown opcode: " << std::hex << opcode << "\n";
            chip8.set_pc(chip8.get_pc() + 4);
        }
    }

} // namespace opcode_table