#include "opcode_table_32.hpp"
#include "chip8_32.hpp"

#include <stdexcept>
#include <iostream>
#include <cstring> // memset, memcpy
#include <random>  // for random operations, CXNN 
#include <fstream>
#include <vector>

namespace OpcodeTable_32 {

    // 20개의 주요 명령 그룹(상위 8비트로 구분)을 처리하기 위한 함수 테이블
    std::array<OpcodeHandler32, 20> primary_table_32;

    /// @brief 화면을 지우는 명령 (00000E00)
    void OP_00000E00(Chip8_32& chip8_32, uint32_t) {
        chip8_32.get_video().fill(0);
        chip8_32.set_draw_flag(true);
        chip8_32.set_pc(chip8_32.get_pc() + 4); 
    }

    /// @brief 서브루틴 반환 명령 (00000E0E)
    void OP_00000E0E(Chip8_32& chip8_32, uint32_t) {
        // 스택 언더플로우 체크 필요
        if (chip8_32.get_sp() == 0) {
            std::cerr << "Stack underflow!" << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        chip8_32.set_sp(chip8_32.get_sp() - 1);
        chip8_32.set_pc(chip8_32.stack_at(chip8_32.get_sp()) + 4);
    }

    /// @brief 절대 주소로 점프 (1NNN)
    void OP_01NNNNNN(Chip8_32& chip8_32, uint32_t opcode) {
        chip8_32.set_pc(opcode & 0x00FFFFFF);  //24비트 주소 추출
    }

    /// @brief 서브루틴 호출 (02NNNNNN)
    void OP_02NNNNNN(Chip8_32& chip8_32, uint32_t opcode) {
        if (chip8_32.get_sp() >= STACK_SIZE_32) {
            std::cerr << "Stack overflow!" << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
         }

        // 💥 다음 명령어 주소 저장
        chip8_32.stack_at(chip8_32.get_sp()) = chip8_32.get_pc();
        chip8_32.set_sp(chip8_32.get_sp() + 1);
        chip8_32.set_pc(opcode & 0x00FFFFFF);

    }


    /// @brief Rx == KKKK(상수)면 다음 명령어 건너뜀 (03XXKKKK)
    void OP_03XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스 (XX 필드)
    
        if (x >= 32) {
            std::cerr << "Register index out of bounds: " << static_cast<int>(x) << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
    
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF);    // 16비트 상수 32비트로 확장(KKKK 필드)
        uint32_t reg_val = chip8_32.get_R(x);                        // 32비트 레지스터 값
        bool equal = (reg_val == kk);
        
        // 💥 디버깅 로그
        std::cout << "[DEBUG] OP_03XXKKKK: R[" << static_cast<int>(x) << "]="
                  << std::hex << reg_val
                  << ", KK=" << kk
                  << ", Equal=" << equal
                  << std::dec << std::endl;
    
        chip8_32.set_pc(chip8_32.get_pc() + (equal ? 8 : 4));  // 같으면 8, 아니면 4
    }
    

    /// @brief Rx != KKKK (상수)면 다음 명령어 건너뜀 (04XXKKKK)
    void OP_04XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스 (XX 필드)
        uint32_t kk  = static_cast<uint32_t>(opcode & 0x0000FFFF); // 16비트 상수 32비트로 확장(KKKK 필드)
        uint32_t reg_val = chip8_32.get_R(x);   // 32비트 레지스터 값
        
        bool not_equal = (reg_val != kk);
        chip8_32.set_pc(chip8_32.get_pc() + (not_equal ? 8 : 4));
    }

    /// @brief Rx == Ry면 다음 명령어 건너뜀 (05XXYY00)
    void OP_05XXYY00(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;    // 첫 번째 레지스터
        uint8_t y = (opcode & 0x0000FF00) >> 8;     // 두 번째 레지스터
        bool equal = (chip8_32.get_R(x) == chip8_32.get_R(y)); //전체 32비트 비교
        chip8_32.set_pc(chip8_32.get_pc() + (equal ? 8 : 4));
    }

    /// @brief Rx에 상수 kkkk 저장 (06XXKKKK)
    void OP_06XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;    // 레지스터 인덱스 (XX 필드)
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF); // 16비트 상수 32비트로 확장 (KKKK 필드)
        chip8_32.set_R(x, kk);  // 레지스터에 값 저장
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Rx에 상수 kkkk 더하기 (07XXKKKK)
    void OP_07XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF);  // 16비트 상수 32비트로 확장 (KKKK 필드)
        chip8_32.set_R(x, chip8_32.get_R(x) + kk);   //chip8_32.set_R 함수도 추후 점검 필요
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Rx와 Ry 간 다양한 연산 수행 (08XXYYNN)
    void OP_08XXYYZZ(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;    // 첫 번째 레지스터 (XX 필드)
        uint8_t y = (opcode & 0x0000FF00) >> 8;     // 두 번째 레지스터 (YY 필드)
        uint8_t zz  = opcode & 0x000000FF;          // 연산 타입 (zz 필드)
        uint32_t rx = chip8_32.get_R(x);

        uint32_t ry = chip8_32.get_R(y);

        switch (zz) {
            case 0x00: chip8_32.set_R(x, ry); break;        // Rx = Ry (복사)
            case 0x01: chip8_32.set_R(x, rx | ry); break;   // Rx = Rx OR Ry  
            case 0x02: chip8_32.set_R(x, rx & ry); break;      // Rx = Rx AND Ry
            case 0x03: chip8_32.set_R(x, rx ^ ry); break;      // Rx = Rx XOR Ry
            case 0x04: {    // ADD with carry
                uint64_t sum = static_cast<uint64_t>(rx) + ry;
                chip8_32.set_R(15, sum > 0xFFFFFFFF ? 1 : 0);  // R15에 캐리 플래그
                chip8_32.set_R(x, static_cast<uint32_t>(sum));
                break;
            }
            case 0x05:     // SUB with borrow
                chip8_32.set_R(15, rx >= ry ? 0 : 1);  // R15에 borrow 플래그
                chip8_32.set_R(x, rx - ry);
                break;
            case 0x06:     // SHR (Shift Right)
                chip8_32.set_R(15, rx & 0x1);  // LSB를 R15에 저장
                chip8_32.set_R(x, rx >> 1);
                break;
            case 0x07:     // SUBN (Ry - Rx)
                chip8_32.set_R(15, ry >= rx ? 0 : 1);  // borrow flag
                chip8_32.set_R(x, ry - rx);
                break;
            case 0xE:   // SHL (Shift Left)
                chip8_32.set_R(15, (rx & 0x80000000) ? 1: 0);  // MSB를 R15에 저장
                chip8_32.set_R(x, rx << 1);
                break;
        }
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Rx != Ry면 다음 명령어 건너뜀 (09XXYY00)
    void OP_09XXYY00(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 첫 번째 레지스터
        uint8_t y = (opcode & 0x0000FF00) >> 8;   // 두 번째 레지스터
        bool not_equal = (chip8_32.get_R(x) != chip8_32.get_R(y));
        chip8_32.set_pc(chip8_32.get_pc() + (not_equal ? 8 : 4));
    }

    /// @brief I에 주소 NNNNNN 저장 (0ANNNNNN)
    void OP_0ANNNNNN(Chip8_32& chip8_32, uint32_t opcode) {
        chip8_32.set_I(opcode & 0x00FFFFFF);    // 24비트 주소 추출
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief PC = 주소 NNNNNN + R0 (0BNNNNNN)
    void OP_0BNNNNNN(Chip8_32& chip8_32, uint32_t opcode) {
        chip8_32.set_pc((opcode & 0x00FFFFFF) + chip8_32.get_R(0));
    }

    /// @brief Rx에 rand() & 상수 kkkk 저장 (0CXXKKKK)
    void OP_0CXXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스 (XX 필드)
        uint32_t mask = static_cast<uint32_t>(opcode & 0x0000FFFF);      // 32비트 마스크 (KKKK 필드)
        uint32_t rand_val = static_cast<uint32_t>(rand());

        chip8_32.set_R(x, rand_val & mask); // 0~255 범위의 랜덤 값과 마스크 적용
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief 스프라이트 그리기 (0DXXYYNN)
    void OP_0DXXYYNN(Chip8_32& chip8_32, uint32_t opcode) {
        // 8비트 인덱스로 원복
        uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
        uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

        // 좌표는 여전히 하위 8비트만 사용
        uint8_t x = chip8_32.get_R(reg_x) % VIDEO_WIDTH;   
        uint8_t y = chip8_32.get_R(reg_y) % VIDEO_HEIGHT;  


        uint8_t height = opcode & 0x000000FF;

        chip8_32.set_R(15, 0);  // 충돌 감지 플래그 초기화

        for (int row = 0; row < height; ++row) {
            uint32_t addr = chip8_32.get_I() + row;
            if (addr >= MEMORY_SIZE_32) {
                std::cerr << "Memory access out of bounds: " << addr << std::endl;
                break;
            }

            uint8_t sprite = chip8_32.get_memory(addr);
            for (int col = 0; col < 8; ++col) {
                if (sprite & (0x80 >> col)) {

                    if ((x + col) < VIDEO_WIDTH && (y + row) < VIDEO_HEIGHT) {
                        uint32_t video_idx = ((y + row) % VIDEO_HEIGHT) * VIDEO_WIDTH + ((x + col) % VIDEO_WIDTH);
                        if (chip8_32.get_video(video_idx))
                            chip8_32.set_R(15, 1); // 충돌 감지
                        chip8_32.set_video(video_idx, chip8_32.get_video(video_idx) ^ 1);
                    }
                    
                }
                if ((x + col) >= VIDEO_WIDTH || (y + row) >= VIDEO_HEIGHT) {
                    std::cout << "[WRAP] Sprite pixel (" << (x+col) << ", " << (y+row) << ") wrapped to (" 
                              << ((x+col) % VIDEO_WIDTH) << ", " << ((y+row) % VIDEO_HEIGHT) << ")\n";
                }
            }
        }

        std::cout << "\n=== DRW DEBUG ===" << std::endl;
        std::cout << "Opcode = 0x" << std::hex << opcode << std::dec << std::endl;
        std::cout << "reg_x = " << static_cast<int>(reg_x)
                << " reg_y = " << static_cast<int>(reg_y) << std::endl;
        std::cout << "R[reg_x] = 0x" << std::hex << chip8_32.get_R(reg_x)
                << " R[reg_y] = 0x" << chip8_32.get_R(reg_y) << std::dec << std::endl;
        std::cout << "R[15] (Collision Flag) = 0x" << std::hex << chip8_32.get_R(15) << std::dec << std::endl;
        std::cout << "I = 0x" << std::hex << chip8_32.get_I() << std::dec << std::endl;

        // 🆕 왼쪽/오른쪽 패들 구분
        if (reg_x == 10 || reg_y == 11) {
            std::cout << "[LEFT PADDLE] Draw at (" << static_cast<int>(x)
                    << ", " << static_cast<int>(y) << ") height=" << static_cast<int>(height) << std::endl;
        } else if (reg_x == 12 || reg_y == 13) {
            std::cout << "[RIGHT PADDLE] Draw at (" << static_cast<int>(x)
                    << ", " << static_cast<int>(y) << ") height=" << static_cast<int>(height) << std::endl;
        } else {
            std::cout << "[OTHER SPRITE] Draw at (" << static_cast<int>(x)
                    << ", " << static_cast<int>(y) << ") height=" << static_cast<int>(height) << std::endl;
        }

        chip8_32.set_draw_flag(true);
        chip8_32.set_pc(chip8_32.get_pc() + 4);

    }

    
    

    /// @brief 키 입력 조건 분기 (0EXX090E, 0EXX0A01)
    void OP_0EXXCCCC(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스 (XX 필드)
        uint8_t key = chip8_32.get_R(x) & 0xFF;   // R 레지스터에서 키 값 (하위 8비트)
        
        if (key >= 16) {
            std::cerr << "Invalid key index: " << static_cast<int>(key) << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        uint16_t code = opcode & 0x0000FFFF;      // 16비트 조건 코드 (확장)

        if (code == 0x090E)       // EX9E -> 0EXX090E
            chip8_32.set_pc(chip8_32.get_pc() + (chip8_32.get_key(key) ? 8 : 4));
        else if (code == 0x0A01)  // EXA1 -> 0EXX0A01
            chip8_32.set_pc(chip8_32.get_pc() + (!chip8_32.get_key(key) ? 8 : 4));
        else
            chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Fx 계열 (타이머/메모리 함수) 확장 명령들 처리 (0FXXCCCC)
    void OP_0FXXCCCC(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint16_t code = opcode & 0x0000FFFF;  // 세부 코드 
        
        switch (code) {
            case 0x0007:  // Fx07 -> 0FXX0007 (Get Delay Timer)
                chip8_32.set_R(x, chip8_32.get_delay_timer()); 
                break;
            case 0x000A: {  // Fx0A -> 0FXX000A (Wait for Key Press)
                for (int i = 0; i < 16; ++i) {
                    if (chip8_32.get_key(i)) {
                        chip8_32.set_R(x, i);
                        chip8_32.set_pc(chip8_32.get_pc() + 4);
                        return;
                    }
                }
                return;  // 키가 눌릴 때까지 대기 (PC 증가 안 함)
            }

            case 0x0105: // Fx15 -> 0FXX0105 (Set Delay Timer) 
                chip8_32.set_delay_timer(chip8_32.get_R(x) & 0xFF); //8비트로 제한 
                break;
            case 0x0108:   // Fx18 -> 0FXX0108 (Set Sound Timer) 
                chip8_32.set_sound_timer(chip8_32.get_R(x) & 0xFF);  
                break;
            case 0x010E:   // Fx1E -> 0FXX010E (Add to I)  
                chip8_32.set_I(chip8_32.get_I() + chip8_32.get_R(x));
                break;
            case 0x0209:     // FX29 -> 0FXX0209 (Set I to Font Address)
                // Fx29 명령어 수정
                chip8_32.set_I(0x50 + (chip8_32.get_R(x) * 5));
                break;  // 폰트 주소
            case 0x0303: {  // FX33 -> 0FXX0303 (BCD 변환)
                uint32_t value = chip8_32.get_R(x);
                chip8_32.set_memory(chip8_32.get_I(), value / 100);
                chip8_32.set_memory(chip8_32.get_I() + 1, (value / 10) % 10);
                chip8_32.set_memory(chip8_32.get_I() + 2, value % 10);
                
                // 점수 디버깅 로그 추가
                std::cout << "[BCD] R[" << (int)x << "]=" << value
                          << " → MEM[" << std::hex << chip8_32.get_I()
                          << "]=" << (value / 100)
                          << ", MEM[" << chip8_32.get_I() + 1
                          << "]=" << ((value / 10) % 10)
                          << ", MEM[" << chip8_32.get_I() + 2
                          << "]=" << (value % 10) << std::dec << std::endl;
                break;
            }
            case 0x0505:  // FX55 -> 0FXX0505 (Registers 값들 저장)
                for (int i = 0; i <= x && i < 32; ++i) {
                    // 32비트 레지스터 전체 저장 (4바이트씩)
                    uint32_t val = chip8_32.get_R(i);
                    // Big-endian으로 저장 
                    chip8_32.set_memory(chip8_32.get_I() + i * 4, (val >> 24) & 0xFF);
                    chip8_32.set_memory(chip8_32.get_I() + i * 4 + 1, (val >> 16) & 0xFF);
                    chip8_32.set_memory(chip8_32.get_I() + i * 4 + 2, (val >> 8) & 0xFF);
                    chip8_32.set_memory(chip8_32.get_I() + i * 4 + 3, val & 0xFF);                    
                } 
                break;
            case 0x0605:  // 32비트 레지스터 전체 로드
                for (int i = 0; i <= x && i < 32; ++i) {
                    // Big-endian으로 값 읽기
                    uint32_t val = (chip8_32.get_memory(chip8_32.get_I() + i * 4) << 24) |
                                   (chip8_32.get_memory(chip8_32.get_I() + i * 4 + 1) << 16) |
                                   (chip8_32.get_memory(chip8_32.get_I() + i * 4 + 2) << 8) |
                                   chip8_32.get_memory(chip8_32.get_I() + i * 4 + 3);
                    chip8_32.set_R(i, val);
                }
                break;
        }
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief opcode 상위 8비트 기반으로 핸들러 함수 등록
    void Initialize() {
        primary_table_32.fill(nullptr);

        primary_table_32[0x00] = [](Chip8_32& chip8_32, uint32_t opcode) {
            uint16_t code = opcode & 0x0000FFFF;  // 세부 코드 
            switch (code) {
                case 0x0E00: OP_00000E00(chip8_32, opcode); break;
                case 0x0E0E: OP_00000E0E(chip8_32, opcode); break;
                default:
                    std::cerr << "Unknown 0x00 opcode: 0x" << std::hex << opcode << "\n";
                    chip8_32.set_pc(chip8_32.get_pc() + 4);
                    break;
            }
        };
        

        primary_table_32[0x01] = OP_01NNNNNN;  // 절대 주소로 점프
        primary_table_32[0x02] = OP_02NNNNNN;  // 서브루틴 호출
        primary_table_32[0x03] = OP_03XXKKKK;  //  Rx == KKKK면 다음 명령어 건너뜀
        primary_table_32[0x04] = OP_04XXKKKK;  //  Rx != KKKK면 다음 명령어 건너뜀
        primary_table_32[0x05] = OP_05XXYY00;  // Rx == Ry면 다음 명령어 건너뜀
        primary_table_32[0x06] = OP_06XXKKKK;  // Rx에 상수 kkkk 저장
        primary_table_32[0x07] = OP_07XXKKKK;  // Rx에 상수 kkkk 더하기
        primary_table_32[0x08] = OP_08XXYYZZ;  // Rx와 Ry 간 다양한 연산 수행
        primary_table_32[0x09] = OP_09XXYY00;  // Rx != Ry면 다음 명령어 건너뜀    
        primary_table_32[0x0A] = OP_0ANNNNNN;  // I에 주소 NNNNNN 저장
        primary_table_32[0x0B] = OP_0BNNNNNN;  // PC = 주소 NNNNNN + R0
        primary_table_32[0x0C] = OP_0CXXKKKK;  // Rx에 rand() & 상수 kkkk 저장
        primary_table_32[0x0D] = OP_0DXXYYNN;  // 스프라이트 그리기
        primary_table_32[0x0E] = OP_0EXXCCCC;  // 키 입력 조건 분기
        primary_table_32[0x0F] = OP_0FXXCCCC;  // Fx 계열 (타이머/메모리 함수) 확장 명령들 처리
    
        // 새로운 확장 명령어들 (미래 구현용) 예시
        // primary_table_32[0x10] = OP_10XXXXXX;  // MALLOC operations
        // primary_table_32[0x11] = OP_11XXXXXX;  // FREE operations
        // primary_table_32[0x12] = OP_12XXXXXX;  // FILE I/O operations
        // primary_table_32[0x13] = OP_13XXXXXX;  // SYSCALL operations
    }

    /// @brief opcode를 상위 4비트로 분기하여 실행
    void Execute(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t index = (opcode & 0xFF000000) >> 24;
    
        // 실제 구현된 명령어만 처리 (0x00~0x0F, 총 16개)
        if (index >= IMPLEMENTED_OPCODES) { //일단은 현재 구현된 것들만 체크하고, 추후 확장시 수정
            std::cerr << "Unimplemented 32-bit opcode: " << std::hex << opcode << "\n";
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;  // 추가: 함수 종료
        }
        
        OpcodeHandler32 handler = primary_table_32[index];
        if (handler)
            handler(chip8_32, opcode);
        else {
            std::cerr << "Unknown 32-bit opcode: " << std::hex << opcode << "\n";
            chip8_32.set_pc(chip8_32.get_pc() + 4);
        }
    }

} // namespace opcode_table