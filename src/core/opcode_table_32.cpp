#include "opcode_table_32.hpp"
#include "chip8_32.hpp"
#include <stdexcept>
#include <iostream>
#include <cstring> // memset, memcpy
#include <random>  // for random operations, CXNN 
#include <fstream>
#include <vector>
#include <map>

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
        
        // 원본 CHIP-8과 같은 비교 방식: 하위 16비트만 비교
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
            case 0x05:     // SUB with borrow -  수정: 플래그 반전
                chip8_32.set_R(15, rx >= ry ? 1 : 0);  // borrow 플래그 (원본과 반대)
                chip8_32.set_R(x, rx - ry);
                break;
            case 0x06:     // SHR (Shift Right)
                chip8_32.set_R(15, rx & 0x1);  // LSB를 R15에 저장
                chip8_32.set_R(x, rx >> 1);
                break;
            case 0x07:     // SUBN (Ry - Rx)
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
        uint32_t rand_val = static_cast<uint32_t>(rand() & 0xFFFF);  //  수정: 16비트 랜덤값

        chip8_32.set_R(x, rand_val & mask);
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    /// @brief 스프라이트 그리기 (0DXXYYNN) -  주요 수정
    void OP_0DXXYYNN(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t reg_x = (opcode & 0x00FF0000) >> 16;
        uint8_t reg_y = (opcode & 0x0000FF00) >> 8;

        //  수정: 하위 8비트만 사용 (원본 CHIP-8과 동일)
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
                    //  수정: 화면 경계 처리 개선
                    uint8_t pixel_x = (x + col) % VIDEO_WIDTH;
                    uint8_t pixel_y = (y + row) % VIDEO_HEIGHT;
                    uint32_t video_idx = pixel_y * VIDEO_WIDTH + pixel_x;

                    //  수정: 충돌 감지 로직 개선
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
            case 0x010E:   // Fx1E -> 0FXX010E (Add to I) -  수정
            {
                uint32_t sum = chip8_32.get_I() + (chip8_32.get_R(x) & 0xFFFF);
                //  오버플로우 플래그 설정 (16비트 주소 공간에서)
                chip8_32.set_R(15, (sum > 0xFFFF) ? 1 : 0);
                chip8_32.set_I(sum & 0xFFFF);  // 16비트로 제한
                break;
            }
            case 0x0209:     // FX29 -> 0FXX0209 (Set I to Font Address)
                //  수정: 0x50부터 시작
                chip8_32.set_I(0x50 + ((chip8_32.get_R(x) & 0xF) * 5));
                break;
            case 0x0303: {  // FX33 -> 0FXX0303 (BCD 변환)
                uint32_t value = chip8_32.get_R(x) & 0xFF;  //  수정: 하위 8비트만 사용
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
            case 0x0505:  // FX55 -> 0FXX0505 (Registers 값들 저장) -  수정
                for (int i = 0; i <= x && i < 16; ++i) {  // 원본과 호환을 위해 16개 레지스터만 사용
                    chip8_32.set_memory(chip8_32.get_I() + i, chip8_32.get_R(i) & 0xFF);  // 8비트만 저장
                } 
                break;
            case 0x0605:  // FX65 -> 0FXX0605 (레지스터 로드) -  수정
                for (int i = 0; i <= x && i < 16; ++i) {  // 원본과 호환을 위해 16개 레지스터만 사용
                    chip8_32.set_R(i, chip8_32.get_memory(chip8_32.get_I() + i));  // 8비트 값을 32비트 레지스터에
                }
                break;
        }
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

    // 32비트 CHIP-8 레지스터 사용 규칙 : 
    // R0 ~ R15 : 8비트 CHIP-8 호환 (V0 ~ VF와 대응)
    // R16 ~ R31 : 32비트 확장 전용 레지스터

    // 시스템콜 규약 : 
    // R16 : 반환 값 / 오류 코드 (성공 : 0 이상, 실패 : 0xFFFFFFFF)
    // R17 : 크기 / 길이 매개 변수
    // R18 : 추가 매개 변수 1 
    // R19 : 추가 매개 변수 2
    // R20 ~ R23 : 범용 시스템 레지스터 
    // R24 ~ R31 : 사용자 정의 

    /// @brief SYSCALL 처리 (10SAAAAF) - 수정된 버전
    void OP_10SAAAAF(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t syscall_num = (opcode & 0x00F00000) >> 20;  // S (4비트)
        uint16_t buffer_addr = (opcode & 0x000FFFF0) >> 4;  // AAAAA (16비트)
        uint8_t fd = opcode & 0x0000000F;                   // F (4비트)
        
        std::cout << "\n=== SYSCALL ===" << std::endl;
        std::cout << "Syscall: " << static_cast<int>(syscall_num) 
                << ", Buffer: 0x" << std::hex << buffer_addr 
                << ", FD: " << std::dec << static_cast<int>(fd) << std::endl; 
        
        // 주소 범위 체크 (오타 수정)
        if (buffer_addr >= MEMORY_SIZE_32) {
            std::cerr << "Invalid buffer address: 0x" << std::hex << buffer_addr << std::endl;
            chip8_32.set_R(16, 0xFFFFFFFF); // R16에 오류 코드
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        switch (syscall_num) {
            case 0x0: {  // READ syscall
                std::cout << "[READ] Reading from fd " << static_cast<int>(fd) << std::endl;
                
                if (fd == 0) {  // stdin
                    std::cout << "Enter text: ";
                    std::string input;
                    std::getline(std::cin, input);

                    // R17에서 최대 읽기 크기 가져오기 (설정되지 않았으면 기본값 256)
                    size_t max_size = chip8_32.get_R(17);
                    if (max_size == 0 || max_size > 1024) {
                        max_size = 256;  // 기본값
                    }
                    
                    // 입력받은 문자열을 메모리에 저장 (괄호 수정)
                    size_t bytes_to_write = std::min(input.length(), max_size - 1);    // null terminator 공간 확보
                    for (size_t i = 0; i < bytes_to_write; ++i) {
                        if (buffer_addr + i < MEMORY_SIZE_32) {  // 오타 수정
                            chip8_32.set_memory(buffer_addr + i, static_cast<uint8_t>(input[i]));  // 괄호 수정
                        }
                    }
                    
                    // null terminator 추가
                    if (buffer_addr + bytes_to_write < MEMORY_SIZE_32) {
                        chip8_32.set_memory(buffer_addr + bytes_to_write, 0); 
                    }

                    // 반환값: R16에 읽은 바이트 수 
                    chip8_32.set_R(16, static_cast<uint32_t>(bytes_to_write));

                    std::cout << "[read] Read " << bytes_to_write << " bytes: \"" 
                            << input.substr(0, 20) << (input.length() > 20 ? "..." : "") << "\"" << std::endl;
                    std::cout << "[read] Return value in R16: " << bytes_to_write << std::endl;
                    
                } else {
                    std::cerr << "[read] Unsupported file descriptor: " << static_cast<int>(fd) << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);  // R16에 오류 코드
                }
                break;
            }
            
            case 0x1: {  // WRITE syscall
                std::cout << "[write] Writing to fd " << static_cast<int>(fd) << std::endl;
                
                if (fd == 1 || fd == 2) {  // stdout, stderr
                    // R17에서 쓸 크기 가져오기 (0이면 null-terminated 문자열로 처리)
                    size_t write_size = chip8_32.get_R(17);
                    
                    std::string output;
                    if (write_size == 0) {
                        // null-terminated 문자열 처리
                        for (size_t i = 0; i < 1024; ++i) {  // 최대 1KB
                            if (buffer_addr + i >= MEMORY_SIZE_32) break;
                            
                            uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                            if (byte == 0) break;  // null terminator
                            
                            output += static_cast<char>(byte);
                        }
                    } else {
                        // 고정 크기 데이터 처리
                        write_size = std::min(write_size, static_cast<size_t>(1024));  // 최대 1KB 제한
                        for (size_t i = 0; i < write_size; ++i) {
                            if (buffer_addr + i >= MEMORY_SIZE_32) break;
                            
                            uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                            output += static_cast<char>(byte);
                        }
                    }
                    
                    // 출력
                    if (fd == 1) {
                        std::cout << "[stdout] " << output << std::endl;
                    } else {
                        std::cerr << "[stderr] " << output << std::endl;
                    }
                    
                    // 반환값: R16에 출력한 바이트 수
                    chip8_32.set_R(16, static_cast<uint32_t>(output.length()));
                    
                    std::cout << "[write] Wrote " << output.length() << " bytes" << std::endl;
                    std::cout << "[write] Return value in R16: " << output.length() << std::endl;
                    
                } else {
                    std::cerr << "[write] Unsupported file descriptor: " << static_cast<int>(fd) << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);  // R16에 오류 코드
                }
                break;
            }
            
            case 0x2: {  // GETPID syscall (예시)
                std::cout << "[getpid] Returning fake PID" << std::endl;
                chip8_32.set_R(16, 1234);  // 가짜 PID 반환
                break;
            }
            
            case 0x3: {  // EXIT syscall
                uint32_t exit_code = chip8_32.get_R(17);
                std::cout << "[exit] Exiting with code " << exit_code << std::endl;
                // 실제로는 에뮬레이터 종료 플래그 설정
                chip8_32.set_R(16, 0);  // 성공
                break;
            }
            
            case 0x4: {   // READ_FILE syscall
                std::cout << "[read_file] Reading file from buffer" << std::endl;

                // buffer_addr에서 파일명 읽기 (null-terminated)
                std::string filename;
                for (size_t i = 0; i < 256; ++i) {
                    if (buffer_addr + i >= MEMORY_SIZE_32) break;
                    uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                    if (byte == 0) break;  // null terminator
                    filename += static_cast<char>(byte);
                }

                std::cout << "[read_file] Attempting to read: " << filename << std::endl;
            
                // 파일 읽어서 주소 0x200에 ROM 파일 로드 (기존 CHIP-8 게임 시작 위치 유지)
                std::ifstream file(filename, std::ios::binary);
                if (file.is_open()) {
                    file.seekg(0, std::ios::end);
                    size_t file_size = file.tellg();
                    file.seekg(0, std::ios::beg);

                    std::cout << "[read_file] File size: " << file_size << " bytes" << std::endl;

                    size_t bytes_loaded = 0;
                    for (size_t i = 0; i < file_size && (0x200 + i) < MEMORY_SIZE_32; ++i) {  //메모리 0x200부터 실제 실행 파일 적재
                        char byte;
                        file.read(&byte, 1);
                        chip8_32.set_memory(0x200 + i, static_cast<uint8_t>(byte));
                        bytes_loaded++;
                    }

                    file.close();
                    chip8_32.set_R(16, bytes_loaded);  // R16에 읽은 바이트 수 저장
                    std::cout << "[read_file] Successfully loaded" << bytes_loaded << "bytes to 0x200" << std::endl; //0x800 -> 0x200
                }  else {
                    std::cerr << "[read_file] Failed to open file: " << filename << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);  // 실패
                }
                break;
            }
            default:
                std::cerr << "[syscall] Unknown syscall: " << static_cast<int>(syscall_num) << std::endl;
                chip8_32.set_R(16, 0xFFFFFFFF);  // R16에 오류 코드
                break;
        
        }
        // 디버그 출력: 시스템 레지스터 상태
        std::cout << "System registers after syscall:" << std::endl;
        std::cout << "  R16 (return): 0x" << std::hex << chip8_32.get_R(16) << std::dec << std::endl;
        std::cout << "  R17 (size):   0x" << std::hex << chip8_32.get_R(17) << std::dec << std::endl;
        std::cout << "  R18 (param1): 0x" << std::hex << chip8_32.get_R(18) << std::dec << std::endl;
        std::cout << "  R19 (param2): 0x" << std::hex << chip8_32.get_R(19) << std::dec << std::endl;

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
        primary_table_32[0x10] = OP_10SAAAAF;  // SYSCALL 처리
    }

    /// @brief opcode를 상위 8비트로 분기하여 실행
    void Execute(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t index = (opcode & 0xFF000000) >> 24;
    
        // 실제 구현된 명령어만 처리 (0x00~0x0F, 총 16개)
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

}   // namespace OpcodeTable_32 
 