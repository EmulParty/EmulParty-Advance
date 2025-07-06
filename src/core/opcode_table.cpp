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
    std::array<OpcodeTable::OpcodeHandler, 16> primary_table;

// 0x0 - CLS, RET 등 시스템 명령어 (하위 16비트 기준)
    void OP_00E0(Chip8& chip8, uint32_t) {
        chip8.get_video().fill(0);
        chip8.set_draw_flag(true);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    void OP_00EE(Chip8& chip8, uint32_t) {
        chip8.set_sp(chip8.get_sp() - 1);
        chip8.set_pc(chip8.stack_at(chip8.get_sp()) + 4);
    }

    // 1NNN - JP addr (하위 16비트 주소)
    void OP_01NNNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_pc(opcode & 0x0000FFFF);
    }

    // 2NNN - CALL addr (하위 16비트 주소)
    void OP_02NNNN(Chip8& chip8, uint32_t opcode) {
        chip8.stack_at(chip8.get_sp()) = chip8.get_pc();
        chip8.set_sp(chip8.get_sp() + 1);
        chip8.set_pc(opcode & 0x0000FFFF);
    }

    // 3XKK - SE Vx, byte (하위 16비트 상수)
    void OP_03XKKKK(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t kkkk = opcode & 0x0000FFFF;
        chip8.set_pc(chip8.get_pc() + (chip8.get_V(x) == kkkk ? 8 : 4));
    }

    // 4XKK - SNE Vx, byte
    void OP_04XKKKK(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t kkkk = opcode & 0x0000FFFF;
        chip8.set_pc(chip8.get_pc() + (chip8.get_V(x) != kkkk ? 8 : 4));
    }

    // 5XY0 - SE Vx, Vy
    void OP_05XY00(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint8_t y = (opcode >> 20) & 0xF;
        chip8.set_pc(chip8.get_pc() + ((chip8.get_V(x) == chip8.get_V(y)) ? 8 : 4));
    }

    // 6XKK - LD Vx, byte
    void OP_06XKKKK(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t kkkk = opcode & 0x0000FFFF;
        chip8.set_V(x, kkkk);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    // 7XKK - ADD Vx, byte
    void OP_07XKKKK(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t kkkk = opcode & 0x0000FFFF;
        chip8.set_V(x, chip8.get_V(x) + kkkk);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    // 8XYN - 산술/논리 연산 (nibble)
    void OP_08XYN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint8_t y = (opcode >> 20) & 0xF;
        uint8_t n = opcode & 0xF;

        uint8_t vx = chip8.get_V(x);
        uint8_t vy = chip8.get_V(y);

        switch (n) {
            case 0x0: chip8.set_V(x, vy); break;
            case 0x1: chip8.set_V(x, vx | vy); break;
            case 0x2: chip8.set_V(x, vx & vy); break;
            case 0x3: chip8.set_V(x, vx ^ vy); break;
            case 0x4: {
                uint16_t sum = vx + vy;
                chip8.set_V(0xF, sum > 0xFF);
                chip8.set_V(x, sum & 0xFF);
                break;
            }
            case 0x5:
                chip8.set_V(0xF, vx > vy);
                chip8.set_V(x, vx - vy);
                break;
            case 0x6:
                chip8.set_V(0xF, vx & 0x1);
                chip8.set_V(x, vx >> 1);
                break;
            case 0x7:
                chip8.set_V(0xF, vy > vx);
                chip8.set_V(x, vy - vx);
                break;
            case 0xE:
                chip8.set_V(0xF, (vx & 0x80) >> 7);
                chip8.set_V(x, vx << 1);
                break;
            default:
                std::cerr << "Unknown 0x08 opcode nibble: " << std::hex << (int)n << std::endl;
                break;
        }
        chip8.set_pc(chip8.get_pc() + 4);
    }

    // 9XY0 - SNE Vx, Vy
    void OP_09XY00(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint8_t y = (opcode >> 20) & 0xF;
        chip8.set_pc(chip8.get_pc() + ((chip8.get_V(x) != chip8.get_V(y)) ? 8 : 4));
    }

    // ANNN - LD I, addr
    void OP_0ANNNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_I(opcode & 0x0000FFFF);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    // BNNN - JP V0, addr
    void OP_0BNNNN(Chip8& chip8, uint32_t opcode) {
        chip8.set_pc((opcode & 0x0000FFFF) + chip8.get_V(0));
    }

    // CXKK - RND Vx, byte
    void OP_0CXKKKK(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t kkkk = opcode & 0x0000FFFF;
        chip8.set_V(x, (rand() % 65536) & kkkk);
        chip8.set_pc(chip8.get_pc() + 4);
    }

    // DXYN - DRW Vx, Vy, nibble
    void OP_0DXYN(Chip8& chip8, uint32_t opcode) {
        uint8_t x = chip8.get_V((opcode >> 24) & 0xF);
        uint8_t y = chip8.get_V((opcode >> 20) & 0xF);
        uint8_t height = opcode & 0xF;
        chip8.set_V(0xF, 0);

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

    // EX9E, EXA1 - SKP, SKNP Vx
    void OP_0EXXXX(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t subcode = opcode & 0x0000FFFF;
        if (subcode == 0x009E)
            chip8.set_pc(chip8.get_pc() + (chip8.get_key(chip8.get_V(x)) ? 8 : 4));
        else if (subcode == 0x00A1)
            chip8.set_pc(chip8.get_pc() + (!chip8.get_key(chip8.get_V(x)) ? 8 : 4));
        else
            chip8.set_pc(chip8.get_pc() + 4);
    }

    // FX - 여러 시스템 명령
    void OP_0FXXXX(Chip8& chip8, uint32_t opcode) {
        uint8_t x = (opcode >> 24) & 0xF;
        uint16_t subcode = opcode & 0x0000FFFF;

        switch (subcode) {
            case 0x0007: chip8.set_V(x, chip8.get_delay_timer()); break;
            case 0x000A: {
                for (int i = 0; i < 16; ++i) {
                    if (chip8.get_key(i)) {
                        chip8.set_V(x, i);
                        chip8.set_pc(chip8.get_pc() + 4);
                        return;
                    }
                }
                return; // 키 입력 대기, PC 증분 없음
            }
            case 0x0105: chip8.set_delay_timer(chip8.get_V(x)); break;
            case 0x0108: chip8.set_sound_timer(chip8.get_V(x)); break;
            case 0x010E: chip8.set_I(chip8.get_I() + chip8.get_V(x)); break;
            case 0x0209: chip8.set_I(chip8.get_V(x) * 5); break;
            case 0x0303: {
                uint8_t vx = chip8.get_V(x);
                chip8.set_memory(chip8.get_I(), vx / 100);
                chip8.set_memory(chip8.get_I() + 1, (vx / 10) % 10);
                chip8.set_memory(chip8.get_I() + 2, vx % 10);
                break;
            }
            case 0x0505:
                for (int i = 0; i <= x; ++i)
                    chip8.set_memory(chip8.get_I() + i, chip8.get_V(i));
                break;
            case 0x0605:
                for (int i = 0; i <= x; ++i)
                    chip8.set_V(i, chip8.get_memory(chip8.get_I() + i));
                break;
            default:
                std::cerr << "Unknown 0x0F opcode: " << std::hex << subcode << std::endl;
                break;
        }
        chip8.set_pc(chip8.get_pc() + 4);
    }

    void Initialize() {
        primary_table.fill(nullptr);

        primary_table[0x0] = [](Chip8& chip8, uint32_t opcode) {
            uint16_t subcode = opcode & 0x0000FFFF;
            switch (subcode) {
                case 0x00E0: OP_00E0(chip8, opcode); break;
                case 0x00EE: OP_00EE(chip8, opcode); break;
                default:
                    std::cerr << "Unknown 0x0 opcode: " << std::hex << opcode << "\n";
                    chip8.set_pc(chip8.get_pc() + 4);
                    break;
            }
        };

        primary_table[0x1] = OP_01NNNN;
        primary_table[0x2] = OP_02NNNN;
        primary_table[0x3] = OP_03XKKKK;
        primary_table[0x4] = OP_04XKKKK;
        primary_table[0x5] = OP_05XY00;
        primary_table[0x6] = OP_06XKKKK;
        primary_table[0x7] = OP_07XKKKK;
        primary_table[0x8] = OP_08XYN;
        primary_table[0x9] = OP_09XY00;
        primary_table[0xA] = OP_0ANNNN;
        primary_table[0xB] = OP_0BNNNN;
        primary_table[0xC] = OP_0CXKKKK;
        primary_table[0xD] = OP_0DXYN;
        primary_table[0xE] = OP_0EXXXX;
        primary_table[0xF] = OP_0FXXXX;
    }

    void Execute(Chip8& chip8, uint32_t opcode) {
        uint8_t index = (opcode >> 28) & 0xF;
        OpcodeHandler handler = primary_table[index];

        if (handler)
            handler(chip8, opcode);
        else {
            std::cerr << "Unknown opcode: " << std::hex << opcode << "\n";
            chip8.set_pc(chip8.get_pc() + 4);
        }
    }

} // namespace OpcodeTable