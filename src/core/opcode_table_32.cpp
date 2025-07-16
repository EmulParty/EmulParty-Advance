#include "../../include/core/opcode_table_32.hpp"
#include "../../include/core/chip8_32.hpp"
#include "../../include/core/stack_frame.hpp"

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
        chip8_32.set_pc(chip8_32.stack_at(chip8_32.get_sp()));  // +4 제거 - 스택에서 정확한 반환 주소 사용
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

        // 현재 PC 저장 (다음 명령어 주소)
        chip8_32.stack_at(chip8_32.get_sp()) = chip8_32.get_pc() + 4;
        chip8_32.set_sp(chip8_32.get_sp() + 1);
        chip8_32.set_pc(opcode & 0x00FFFFFF);
    }

    /// @brief Rx == KKKK(상수)면 다음 명령어 건너뜀 (03XXKKKK)
    void OP_03XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스 수정
    
        if (x >= 32) {
            std::cerr << "Register index out of bounds: " << static_cast<int>(x) << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
    
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF);    // 16비트 상수
        uint32_t reg_val = chip8_32.get_R(x);                        // 32비트 레지스터 값
        
        // 💥 원본 CHIP-8과 같은 비교 방식: 하위 16비트만 비교
        bool equal = ((reg_val & 0xFFFF) == kk);
        
        std::cout << "[DEBUG] OP_03XXKKKK: R[" << static_cast<int>(x) << "]="
                  << std::hex << reg_val << " (lower 16: " << (reg_val & 0xFFFF)
                  << "), KK=" << kk
                  << ", Equal=" << equal
                  << std::dec << std::endl;
    
        chip8_32.set_pc(chip8_32.get_pc() + (equal ? 8 : 4));
    }

    /// @brief Rx != KKKK (상수)면 다음 명령어 건너뜀 (04XXKKKK)
    void OP_04XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스
        uint32_t kk  = static_cast<uint32_t>(opcode & 0x0000FFFF); // 16비트 상수
        uint32_t reg_val = chip8_32.get_R(x);   // 32비트 레지스터 값
        
        // 하위 16비트만 비교
        bool not_equal = ((reg_val & 0xFFFF) != kk);
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
        uint8_t x = (opcode & 0x00FF0000) >> 16;    // 레지스터 인덱스
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF); // 16비트 상수
        chip8_32.set_R(x, kk);  // 레지스터에 값 저장
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Rx에 상수 kkkk 더하기 (07XXKKKK)
    void OP_07XXKKKK(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;
        uint32_t kk = static_cast<uint32_t>(opcode & 0x0000FFFF);  // 16비트 상수
        chip8_32.set_R(x, chip8_32.get_R(x) + kk);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief Rx와 Ry 간 다양한 연산 수행 (08XXYYZZ)
    void OP_08XXYYZZ(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;    // 첫 번째 레지스터
        uint8_t y = (opcode & 0x0000FF00) >> 8;     // 두 번째 레지스터
        uint8_t zz  = opcode & 0x000000FF;          // 연산 타입
        uint32_t rx = chip8_32.get_R(x);
        uint32_t ry = chip8_32.get_R(y);

        switch (zz) {
            case 0x00: chip8_32.set_R(x, ry); break;        // Rx = Ry (복사)
            case 0x01: chip8_32.set_R(x, rx | ry); break;   // Rx = Rx OR Ry  
            case 0x02: chip8_32.set_R(x, rx & ry); break;   // Rx = Rx AND Ry
            case 0x03: chip8_32.set_R(x, rx ^ ry); break;   // Rx = Rx XOR Ry
            case 0x04: {    // ADD with carry
                uint64_t sum = static_cast<uint64_t>(rx) + ry;
                chip8_32.set_R(15, sum > 0xFFFFFFFF ? 1 : 0);  // R15에 캐리 플래그
                chip8_32.set_R(x, static_cast<uint32_t>(sum));
                break;
            }
            case 0x05:     // SUB with borrow - 💥 수정: 플래그 반전
                chip8_32.set_R(15, rx >= ry ? 1 : 0);  // borrow 플래그 (원본과 반대)
                chip8_32.set_R(x, rx - ry);
                break;
            case 0x06:     // SHR (Shift Right)
                chip8_32.set_R(15, rx & 0x1);  // LSB를 R15에 저장
                chip8_32.set_R(x, rx >> 1);
                break;
            case 0x07:     // SUBN (Ry - Rx) - 💥 수정: 플래그 반전
                chip8_32.set_R(15, ry >= rx ? 1 : 0);  // borrow flag (원본과 반대)
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
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스
        uint32_t mask = static_cast<uint32_t>(opcode & 0x0000FFFF);      // 16비트 마스크
        uint32_t rand_val = static_cast<uint32_t>(rand() & 0xFFFF);  // 💥 수정: 16비트 랜덤값

        chip8_32.set_R(x, rand_val & mask);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief 스프라이트 그리기 (0DXXYYNN) - 💥 주요 수정
    void OP_0DXXYYNN(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
        uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

        // 💥 수정: 하위 8비트만 사용 (원본 CHIP-8과 동일)
        uint8_t x = static_cast<uint8_t>(chip8_32.get_R(reg_x) & 0xFF) % VIDEO_WIDTH;   
        uint8_t y = static_cast<uint8_t>(chip8_32.get_R(reg_y) & 0xFF) % VIDEO_HEIGHT;  

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
                    // 💥 수정: 화면 경계 처리 개선
                    uint8_t pixel_x = (x + col) % VIDEO_WIDTH;
                    uint8_t pixel_y = (y + row) % VIDEO_HEIGHT;
                    uint32_t video_idx = pixel_y * VIDEO_WIDTH + pixel_x;

                    // 💥 수정: 충돌 감지 로직 개선
                    if (chip8_32.get_video(video_idx) != 0) {
                        chip8_32.set_R(15, 1); // 충돌 감지
                    }
                    chip8_32.set_video(video_idx, chip8_32.get_video(video_idx) ^ 1);
                }
            }
        }

        std::cout << "\n=== DRW DEBUG ===" << std::endl;
        std::cout << "Opcode = 0x" << std::hex << opcode << std::dec << std::endl;
        std::cout << "reg_x = " << static_cast<int>(reg_x)
                  << " reg_y = " << static_cast<int>(reg_y) << std::endl;
        std::cout << "Draw position: (" << static_cast<int>(x) 
                  << ", " << static_cast<int>(y) << ")" << std::endl;
        std::cout << "R[15] (Collision Flag) = " << chip8_32.get_R(15) << std::endl;
        std::cout << "I = 0x" << std::hex << chip8_32.get_I() << std::dec << std::endl;

        chip8_32.set_draw_flag(true);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief 키 입력 조건 분기 (0EXX090E, 0EXX0A01)
    void OP_0EXXCCCC(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;  // 레지스터 인덱스
        uint8_t key = chip8_32.get_R(x) & 0xFF;   // R 레지스터에서 키 값 (하위 8비트)
        
        if (key >= 16) {
            std::cerr << "Invalid key index: " << static_cast<int>(key) << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        uint16_t code = opcode & 0x0000FFFF;      // 16비트 조건 코드

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
            case 0x010E:   // Fx1E -> 0FXX010E (Add to I) - 💥 수정
            {
                uint32_t sum = chip8_32.get_I() + (chip8_32.get_R(x) & 0xFFFF);
                // 💥 오버플로우 플래그 설정 (16비트 주소 공간에서)
                chip8_32.set_R(15, (sum > 0xFFFF) ? 1 : 0);
                chip8_32.set_I(sum & 0xFFFF);  // 16비트로 제한
                break;
            }
            case 0x0209:     // FX29 -> 0FXX0209 (Set I to Font Address)
                // 💥 수정: 0x50부터 시작
                chip8_32.set_I(0x50 + ((chip8_32.get_R(x) & 0xF) * 5));
                break;
            case 0x0303: {  // FX33 -> 0FXX0303 (BCD 변환)
                uint32_t value = chip8_32.get_R(x) & 0xFF;  // 💥 수정: 하위 8비트만 사용
                chip8_32.set_memory(chip8_32.get_I(), value / 100);
                chip8_32.set_memory(chip8_32.get_I() + 1, (value / 10) % 10);
                chip8_32.set_memory(chip8_32.get_I() + 2, value % 10);
                
                std::cout << "[BCD] R[" << (int)x << "]=" << value
                          << " → MEM[" << std::hex << chip8_32.get_I()
                          << "]=" << (value / 100)
                          << ", MEM[" << chip8_32.get_I() + 1
                          << "]=" << ((value / 10) % 10)
                          << ", MEM[" << chip8_32.get_I() + 2
                          << "]=" << (value % 10) << std::dec << std::endl;
                break;
            }
            case 0x0505:  // FX55 -> 0FXX0505 (Registers 값들 저장) - 💥 수정
                for (int i = 0; i <= x && i < 16; ++i) {  // 원본과 호환을 위해 16개 레지스터만 사용
                    chip8_32.set_memory(chip8_32.get_I() + i, chip8_32.get_R(i) & 0xFF);  // 8비트만 저장
                } 
                break;
            case 0x0605:  // FX65 -> 0FXX0605 (레지스터 로드) - 💥 수정
                for (int i = 0; i <= x && i < 16; ++i) {  // 원본과 호환을 위해 16개 레지스터만 사용
                    chip8_32.set_R(i, chip8_32.get_memory(chip8_32.get_I() + i));  // 8비트 값을 32비트 레지스터에
                }
                break;
        }
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief CALL_FRAME - 스택 프레임 생성 (11RRAAAA)
    /// R = 레지스터 인덱스, AAAA = 함수 주소
    void OP_11RRAAAA(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t reg = (opcode & 0x00FF0000) >> 16;    // 레지스터 인덱스
        uint32_t func_addr = opcode & 0x0000FFFF;     // 함수 주소 (16비트)
        
        std::cout << "\n=== CALL_FRAME ===" << std::endl;
        std::cout << "Target register: R" << static_cast<int>(reg) << std::endl;
        std::cout << "Function address: 0x" << std::hex << func_addr << std::dec << std::endl;
        
        // 현재 PC + 4를 반환 주소로 설정
        uint32_t return_addr = chip8_32.get_pc() + 4;
        uint32_t current_fp = chip8_32.get_current_frame_pointer();
        
        // 새 스택 프레임 생성
        if (!chip8_32.get_frame_manager().create_frame(return_addr, current_fp)) {
            std::cerr << "[CALL_FRAME] Failed to create stack frame!" << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
        
        // 새 프레임 포인터 설정
        chip8_32.set_current_frame_pointer(chip8_32.get_frame_manager().get_current_fp());
        
        // 함수 주소로 점프
        chip8_32.set_pc(func_addr);
        
        std::cout << "Frame created successfully, jumping to 0x" << std::hex << func_addr << std::dec << std::endl;
        std::cout << "===================" << std::endl;
    }

    /// @brief RET_FRAME - 스택 프레임 복원 (12000000)
    void OP_12000000(Chip8_32& chip8_32, uint32_t opcode) {
        std::cout << "\n=== RET_FRAME ===" << std::endl;
        
        uint32_t return_addr;
        uint32_t prev_fp;
        
        // 스택 프레임 복원 (카나리 검증 포함)
        if (!chip8_32.get_frame_manager().restore_frame(return_addr, prev_fp)) {
            std::cerr << "[RET_FRAME] Failed to restore stack frame!" << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
        
        // 프레임 포인터 복원
        chip8_32.set_current_frame_pointer(prev_fp);
        
        // 반환 주소로 점프
        chip8_32.set_pc(return_addr);
        
        std::cout << "Frame restored successfully, returning to 0x" << std::hex << return_addr << std::dec << std::endl;
        std::cout << "==================" << std::endl;
    }

    /// @brief PUSH_LOCAL - 지역변수에 값 저장 (13XXVVVV)
    /// XX = 지역변수 인덱스, VVVV = 값
    void OP_13XXVVVV(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t local_index = (opcode & 0x00FF0000) >> 16;  // 지역변수 인덱스
        uint32_t value = opcode & 0x0000FFFF;               // 값 (16비트)
        
        std::cout << "\n=== PUSH_LOCAL ===" << std::endl;
        std::cout << "Local index: " << static_cast<int>(local_index) << std::endl;
        std::cout << "Value: 0x" << std::hex << value << std::dec << std::endl;
        
        // 지역변수에 값 저장
        if (!chip8_32.get_frame_manager().set_current_local(local_index, value)) {
            std::cerr << "[PUSH_LOCAL] Failed to set local variable!" << std::endl;
        }
        
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        std::cout << "===================" << std::endl;
    }

    /// @brief POP_LOCAL - 지역변수에서 값 로드 (14XXRRRR)
    /// XX = 지역변수 인덱스, RRRR = 대상 레지스터 인덱스
    void OP_14XXRRRR(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t local_index = (opcode & 0x00FF0000) >> 16;  // 지역변수 인덱스
        uint8_t reg_index = (opcode & 0x0000FFFF);          // 대상 레지스터 인덱스
        
        std::cout << "\n=== POP_LOCAL ===" << std::endl;
        std::cout << "Local index: " << static_cast<int>(local_index) << std::endl;
        std::cout << "Target register: R" << static_cast<int>(reg_index) << std::endl;
        
        uint32_t value;
        // 지역변수에서 값 로드
        if (!chip8_32.get_frame_manager().get_current_local(local_index, value)) {
            std::cerr << "[POP_LOCAL] Failed to get local variable!" << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
        
        // 레지스터에 값 저장
        if (reg_index < NUM_REGISTERS_32) {
            chip8_32.set_R(reg_index, value);
            std::cout << "Loaded value 0x" << std::hex << value << std::dec 
                      << " into R" << static_cast<int>(reg_index) << std::endl;
        } else {
            std::cerr << "[POP_LOCAL] Invalid register index: " << static_cast<int>(reg_index) << std::endl;
        }
        
        chip8_32.set_pc(chip8_32.get_pc() + 4);
        std::cout << "==================" << std::endl;
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
        primary_table_32[0x11] = OP_11RRAAAA;  // CALL_FRAME - 스택 프레임 생성
        primary_table_32[0x12] = OP_12000000;  // RET_FRAME - 스택 프레임 복원
        primary_table_32[0x13] = OP_13XXVVVV;  // PUSH_LOCAL - 지역변수에 값 저장
        primary_table_32[0x14] = OP_14XXRRRR;  // POP_LOCAL - 지역변수에서 값 로드
    }

    /// @brief opcode를 상위 8비트로 분기하여 실행
    void Execute(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t index = (opcode & 0xFF000000) >> 24;
    
        // 실제 구현된 명령어만 처리 (0x00~0x14, 총 21개)
        if (index >= IMPLEMENTED_OPCODES) {
            std::cerr << "Unimplemented 32-bit opcode: " << std::hex << opcode << "\n";
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }
        
        OpcodeHandler32 handler = primary_table_32[index];
        if (handler)
            handler(chip8_32, opcode);
        else {
            std::cerr << "Unknown 32-bit opcode: " << std::hex << opcode << "\n";
            chip8_32.set_pc(chip8_32.get_pc() + 4);
        }
    }

} // namespace OpcodeTable_32