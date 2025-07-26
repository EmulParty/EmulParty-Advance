#include "opcode_table_32.hpp"
#include "chip8_32.hpp"
#include "mode_selector.hpp"  //  ModeSelector 헤더 추가!
#include "timer.hpp"          //  timer::get_ticks() 사용을 위해 추가!
#include "stack_opcodes.hpp"   //  스택 관련 명령어를 사용하기 위해 추가!
#include "stack_frame.hpp"
#include "sdl_console_io.hpp"  
#include "platform.hpp"      
#include <stdexcept>
#include <iostream>
#include <cstring>  
#include <random>   
#include <fstream>  
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sstream>  // std::istringstream 사용을 위해

namespace OpcodeTable_32 {         

    // === 헬퍼 함수들 ===
    struct StackEntry {
        uint32_t addr;
        uint32_t value;
        std::string label;
    };
    // 스택 프레임 시각화 헬퍼 함수들 
    void clear_screen() {
         #ifdef _WIN32
        system("cls");
        #else
            system("clear");
        #endif
    }

    void wait_for_enter() {
        std::cout << "\n🎯 Press ENTER to continue...";
        std::cout.flush();
        std::string dummy;
        std::getline(std::cin, dummy);
    }

    void print_stack_simple(Chip8_32& chip8_32, const std::string& phase) {
        uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
        uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
        
        std::cout << "\n📊 STACK STATE [" << phase << "]:" << std::endl;
        std::cout << "   RBP = 0x" << std::hex << std::setw(8) << std::setfill('0') << rbp << std::endl;
        std::cout << "   RSP = 0x" << std::hex << std::setw(8) << std::setfill('0') << rsp << std::endl;
        std::cout << "   Used: " << std::dec << (0xEFFF - rsp) << " bytes" << std::endl;
    }

    void debug_write_stack_32(Chip8_32& chip8_32, uint32_t addr, uint32_t value) {
        chip8_32.set_memory(addr + 0, (value >> 24) & 0xFF);
        chip8_32.set_memory(addr + 1, (value >> 16) & 0xFF);
        chip8_32.set_memory(addr + 2, (value >> 8) & 0xFF);
        chip8_32.set_memory(addr + 3, value & 0xFF);
        
        std::cout << "   [WRITE] 0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                << " <- 0x" << std::setw(8) << value << " (" << std::dec << value << ")" << std::endl;
    }

    uint32_t debug_read_stack_32(Chip8_32& chip8_32, uint32_t addr) {
        uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                         (chip8_32.get_memory(addr + 1) << 16) |
                         (chip8_32.get_memory(addr + 2) << 8)  |
                         chip8_32.get_memory(addr + 3);
        
        std::cout << "   [READ]  0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                << " -> 0x" << std::setw(8) << value << " (" << std::dec << value << ")" << std::endl;
        return value;
    }

    void debug_print_stack_state(Chip8_32& chip8_32, const std::string& phase) {
        std::cout << "\n📊 STACK STATE [" << phase << "]:" << std::endl;
        std::cout << "   RBP = 0x" << std::hex << std::setw(8) << std::setfill('0') << chip8_32.get_RBP() << std::endl;
        std::cout << "   RSP = 0x" << std::hex << std::setw(8) << std::setfill('0') << chip8_32.get_RSP() << std::endl;
        std::cout << "   Stack Used: " << std::dec << (0xEFFF - chip8_32.get_RSP()) << " bytes" << std::endl;
    }


    // === 수정된 2개 매개변수 스택 프레임 시뮬레이션 함수 ===
    void debug_stack_frame_sum_2param(Chip8_32& chip8_32, uint32_t a, uint32_t b) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔥 STACK FRAME SIMULATION: sum(" << a << ", " << b << ")" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // 초기 상태 저장
        uint32_t original_rbp = chip8_32.get_RBP();
        (void) original_rbp;  // 사용하지 않는 변수 경고 제거
        
        debug_print_stack_state(chip8_32, "INITIAL");
        
        std::cout << "\nPress ENTER to continue...";
        std::cin.ignore();
        std::cin.get();
        
        // === STEP 1: FUNCTION PROLOGUE ===
        std::cout << "\n🚀 STEP 1: FUNCTION PROLOGUE" << std::endl;
        
        // PUSH RBP
        std::cout << "1.1 PUSH RBP:" << std::endl;
        chip8_32.set_RSP(chip8_32.get_RSP() - 4);
        debug_write_stack_32(chip8_32, chip8_32.get_RSP(), original_rbp);
        
        // MOV RBP, RSP
        std::cout << "\n1.2 MOV RBP, RSP:" << std::endl;
        chip8_32.set_RBP(chip8_32.get_RSP());
        std::cout << "   New RBP = 0x" << std::hex << chip8_32.get_RBP() << std::endl;
        
        // SUB RSP, 12 (2개 매개변수 + 1개 결과 = 12바이트)
        std::cout << "\n1.3 SUB RSP, 12:" << std::endl;
        chip8_32.set_RSP(chip8_32.get_RSP() - 12);
        std::cout << "   Allocated 12 bytes, RSP = 0x" << std::hex << chip8_32.get_RSP() << std::endl;
        
        debug_print_stack_state(chip8_32, "AFTER PROLOGUE");
        
        std::cout << "\nPress ENTER to continue...";
        std::cin.get();
        
        // === STEP 2: PARAMETER STORAGE ===
        std::cout << "\n📦 STEP 2: PARAMETER STORAGE" << std::endl;
        
        uint32_t rbp = chip8_32.get_RBP();
        uint32_t addr_a = rbp - 4;
        uint32_t addr_b = rbp - 8;
        
        // 실제 사용자 입력값을 스택에 저장
        std::cout << "2.1 Store parameter 'a' = " << a << ":" << std::endl;
        debug_write_stack_32(chip8_32, addr_a, a);  // 실제 a 값 저장
        
        std::cout << "\n2.2 Store parameter 'b' = " << b << ":" << std::endl;
        debug_write_stack_32(chip8_32, addr_b, b);  // 실제 b 값 저장
        
        debug_print_stack_state(chip8_32, "AFTER PARAM STORAGE");
        
        std::cout << "\nPress ENTER to continue...";
        std::cin.get();
        
        // === STEP 3: CALCULATION ===
        std::cout << "\n🧮 STEP 3: CALCULATION" << std::endl;
        
        std::cout << "3.1 Load parameter 'a':" << std::endl;
        uint32_t val_a = debug_read_stack_32(chip8_32, addr_a);
        
        std::cout << "\n3.2 Load parameter 'b':" << std::endl;
        uint32_t val_b = debug_read_stack_32(chip8_32, addr_b);
        
        std::cout << "\n3.3 Calculate a + b:" << std::endl;
        uint32_t result = val_a + val_b;
        std::cout << "   " << val_a << " + " << val_b << " = " << result << std::endl;
        
        // 결과를 스택에 저장
        uint32_t addr_result = rbp - 12;
        std::cout << "\n3.4 Store result:" << std::endl;
        debug_write_stack_32(chip8_32, addr_result, result);
        
        debug_print_stack_state(chip8_32, "AFTER CALCULATION");
        
        std::cout << "\nPress ENTER to continue...";
        std::cin.get();
        
        // === STEP 4: FUNCTION EPILOGUE ===
        std::cout << "\n🔄 STEP 4: FUNCTION EPILOGUE" << std::endl;
        
        // Load return value
        std::cout << "4.1 Load return value:" << std::endl;
        uint32_t final_result = debug_read_stack_32(chip8_32, addr_result);
        
        // ADD RSP, 12 (stack cleanup)
        std::cout << "\n4.2 ADD RSP, 12 (stack cleanup):" << std::endl;
        chip8_32.set_RSP(chip8_32.get_RBP());
        std::cout << "   RSP restored to 0x" << std::hex << chip8_32.get_RSP() << std::endl;
        
        // POP RBP
        std::cout << "\n4.3 POP RBP:" << std::endl;
        uint32_t restored_rbp = debug_read_stack_32(chip8_32, chip8_32.get_RSP());
        chip8_32.set_RSP(chip8_32.get_RSP() + 4);
        chip8_32.set_RBP(restored_rbp);
        
        debug_print_stack_state(chip8_32, "FINAL");
        
        // === RESULT ===
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🎯 RESULT: sum(" << a << ", " << b << ") = " << final_result << std::endl;
        std::cout << "✅ Stack frame simulation completed successfully!" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        std::cout << "\nPress ENTER to exit...";
        std::cin.get();
    }

    void write_stack_32(Chip8_32& chip8_32, uint32_t addr, uint32_t value) {
        chip8_32.set_memory(addr + 0, (value >> 24) & 0xFF);
        chip8_32.set_memory(addr + 1, (value >> 16) & 0xFF);
        chip8_32.set_memory(addr + 2, (value >> 8) & 0xFF);
        chip8_32.set_memory(addr + 3, value & 0xFF);
        
        std::cout << "   [WRITE] 0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                  << " <- 0x" << std::setw(8) << value << " (" << std::dec << value << ")" << std::endl;
    }

    uint32_t read_stack_32(Chip8_32& chip8_32, uint32_t addr) {
        uint32_t value = (chip8_32.get_memory(addr + 0) << 24) |
                        (chip8_32.get_memory(addr + 1) << 16) |
                        (chip8_32.get_memory(addr + 2) << 8) |
                        chip8_32.get_memory(addr + 3);
        
        std::cout << "   [READ]  0x" << std::hex << std::setw(8) << std::setfill('0') << addr 
                << " -> 0x" << std::setw(8) << value << " (" << std::dec << value << ")" << std::endl;
        return value;
    }

    void print_stack_with_content(Chip8_32& chip8_32, const std::string& phase, 
                             const std::vector<std::pair<uint32_t, std::string>>& highlights) {
        clear_screen();
        
        std::cout << "\n";
        std::cout << "🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥\n";
        std::cout << "🚀        STACK FRAME VISUALIZATION        🚀\n";
        std::cout << "🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥🔥\n";
        std::cout << "\n📋 PHASE: " << phase << "\n\n";
        
        uint32_t rbp = chip8_32.get_R(StackFrame::RBP_INDEX);
        uint32_t rsp = chip8_32.get_R(StackFrame::RSP_INDEX);
        
        // 스택 다이어그램 그리기
        std::cout << "┌─────────────────────────────────┐ ← 0x" << std::hex << std::uppercase << StackFrame::STACK_START << " (STACK_START)" << std::endl;
        
        // 스택 영역을 4바이트씩 시각화
        for (uint32_t addr = StackFrame::STACK_START; addr >= rsp && addr >= StackFrame::STACK_END; addr -= 4) {
            // 메모리에서 32비트 값 읽기
            uint32_t memory_value = 0;
            if (addr + 3 < MEMORY_SIZE_32) {
                memory_value = (chip8_32.get_memory(addr) << 24) |
                            (chip8_32.get_memory(addr + 1) << 16) |
                            (chip8_32.get_memory(addr + 2) << 8) |
                            chip8_32.get_memory(addr + 3);
            }
            
            // 하이라이트 체크
            std::string label;
            std::string emoji = "  ";
            for (const auto& highlight : highlights) {
                if (highlight.first == addr) {
                    label = highlight.second;
                    emoji = "⭐";
                    break;
                }
            }
            
            // 포인터 정보
            std::string pointer_info;
            if (addr == rbp && addr == rsp) {
                pointer_info = " ← RBP & RSP";
                emoji = "🔴";
            } else if (addr == rbp) {
                pointer_info = " ← RBP (Frame Base)";
                emoji = "🟡";
            } else if (addr == rsp) {
                pointer_info = " ← RSP (Stack Top)";
                emoji = "🟢";
            }
            
            // 스택 셀 출력
            std::cout << "├─────────────────────────────────┤" << pointer_info << std::endl;
            
            if (!label.empty()) {
                std::cout << "│ " << emoji << " 0x" << std::hex << std::setw(4) << std::setfill('0') << addr 
                        << ": " << std::left << std::setw(16) << label << " │" << std::endl;
            } else if (memory_value != 0) {
                std::cout << "│ " << emoji << " 0x" << std::hex << std::setw(4) << std::setfill('0') << addr 
                        << ": 0x" << std::setw(8) << memory_value 
                        << " (" << std::dec << memory_value << ")" << std::setw(3) << " │" << std::endl;
            } else {
                std::cout << "│ " << emoji << " 0x" << std::hex << std::setw(4) << std::setfill('0') << addr 
                        << ": " << std::left << std::setw(16) << "[EMPTY]" << " │" << std::endl;
            }
            
            // 스택 끝에 도달하면 중단
            if (addr == StackFrame::STACK_END || addr <= rsp + 16) break;
        }
        
        std::cout << "└─────────────────────────────────┘ ← 0x" << std::hex << StackFrame::STACK_END << " (STACK_END)" << std::endl;
        
        // 상태 정보
        uint32_t used_bytes = StackFrame::STACK_START - rsp;
        double usage_percent = (double)used_bytes / (StackFrame::STACK_START - StackFrame::STACK_END) * 100.0;
        
        std::cout << "\n🎯 STACK INFO:" << std::endl;
        std::cout << "   Used: " << std::dec << used_bytes << " bytes ";
        std::cout << "(" << std::fixed << std::setprecision(1) << usage_percent << "%)" << std::endl;
        
        if (usage_percent > 75.0) {
            std::cout << "   ⚠️  WARNING: Stack usage > 75%" << std::endl;
        } else {
            std::cout << "   ✅ Stack usage healthy" << std::endl;
        }
    }

    void debug_stack_frame_sum_realistic(Chip8_32& chip8_32, uint32_t a, uint32_t b) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔥 MEGA REALISTIC STACK FRAME: sum(" << a << ", " << b << ")" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // 초기 상태 저장
        uint32_t original_rbp = chip8_32.get_RBP();
        uint32_t original_rsp = chip8_32.get_RSP();
        
        print_stack_with_content(chip8_32, "BOOT ROM READY", {});
        wait_for_enter();
        
        // === STEP 1: FUNCTION PROLOGUE ===
        uint32_t boot_ret_addr = 0x001C;
        chip8_32.set_RSP(chip8_32.get_RSP() - 4);
        write_stack_32(chip8_32, chip8_32.get_RSP(), boot_ret_addr);

        chip8_32.set_RSP(chip8_32.get_RSP() - 4);
        write_stack_32(chip8_32, chip8_32.get_RSP(), original_rbp);

        chip8_32.set_RBP(chip8_32.get_RSP());
        chip8_32.set_RSP(chip8_32.get_RSP() - 12);  // param1, param2, result

        print_stack_with_content(chip8_32, "AFTER PROLOGUE", {
            {chip8_32.get_RBP() + 4, "Return to BootROM"},
            {chip8_32.get_RBP(), "Saved Main RBP"}
        });
        wait_for_enter();

        // === STEP 2: PARAMETER STORAGE ===
        uint32_t rbp = chip8_32.get_RBP();
        uint32_t addr_a = rbp - 4;
        uint32_t addr_b = rbp - 8;

        write_stack_32(chip8_32, addr_a, a);
        write_stack_32(chip8_32, addr_b, b);

        print_stack_with_content(chip8_32, "Stored parameters", {
            {rbp + 4, "Return to BootROM"},
            {rbp, "Saved Main RBP"},
            {addr_a, "param a = " + std::to_string(a)},
            {addr_b, "param b = " + std::to_string(b)}
        });
        wait_for_enter();

        // === STEP 3: CALCULATION ===
        uint32_t val_a = read_stack_32(chip8_32, addr_a);
        uint32_t val_b = read_stack_32(chip8_32, addr_b);
        uint32_t result = val_a + val_b;

        uint32_t addr_result = rbp - 12;
        write_stack_32(chip8_32, addr_result, result);

        print_stack_with_content(chip8_32, "Calculated a + b = " + std::to_string(result), {
            {rbp + 4, "Return to BootROM"},
            {rbp, "Saved Main RBP"},
            {addr_a, "param a = " + std::to_string(val_a)},
            {addr_b, "param b = " + std::to_string(val_b)},
            {addr_result, "result = " + std::to_string(result)}
        });
        wait_for_enter();

        // === STEP 4: FUNCTION EPILOGUE ===
        chip8_32.set_RSP(rbp);
        uint32_t restored_rbp = read_stack_32(chip8_32, chip8_32.get_RSP());
        chip8_32.set_RSP(chip8_32.get_RSP() + 4);
        chip8_32.set_RBP(restored_rbp);

        print_stack_with_content(chip8_32, "EPILOGUE: RBP Restored", {
            {chip8_32.get_RSP(), "Return to BootROM"}
        });
        wait_for_enter();

        
        // RET
        uint32_t return_addr = read_stack_32(chip8_32, chip8_32.get_RSP());
        chip8_32.set_RSP(original_rsp);
        
        clear_screen();
        std::cout << "\n🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉\n";
        std::cout << "🔥        STACK FRAME SIMULATION COMPLETE!        🔥\n";
        std::cout << "🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉🎉\n\n";
        std::cout << "✅ RESULT: sum(" << a << ", " << b << ") = " << result << std::endl;
        std::cout << "✅ Stack frame properly created and destroyed" << std::endl;
        std::cout << "✅ All parameters correctly passed and computed" << std::endl;
        std::cout << "✅ Return address correctly managed" << std::endl;
        
        wait_for_enter();
    }
    

    // 20개의 주요 명령 그룹(상위 8비트로 구분)을 처리하기 위한 함수 테이블
    std::array<OpcodeHandler32, 20> primary_table_32;

    // === 기존 OP_STACK_FRAME_DEBUG 함수를 이것으로 교체 ===
    bool get_two_numbers_from_user(Chip8_32& chip8_32, uint32_t& a, uint32_t& b) {
        std::cout << "Enter two numbers (space separated): ";
        std::cout.flush();  // 출력 강제 플러시
    
        // 간단하게 std::cin으로 직접 받기
        if (std::cin >> a >> b) {
            std::cout << "✅ Parsed: a=" << a << ", b=" << b << std::endl;
            return true;
        }
    
        std::cout << "❌ Failed to parse input" << std::endl;
        return false;
    }



    void OP_STACK_FRAME_DEBUG(Chip8_32& chip8_32, uint32_t opcode) {
        std::cout << "\n🔥 INTERACTIVE STACK FRAME DEMO!" << std::endl;
        std::cout << "🎯 This will demonstrate stack frame operations with your input" << std::endl;
        
        // 🆕 사용자 입력 받기 두 개만 받도록 수정
        uint32_t a, b;
        if (get_two_numbers_from_user(chip8_32, a, b)) {
            std::cout << "\n🚀 Executing add(" << a << ", " << b << ") = " << (a+b) << std::endl;
            debug_stack_frame_sum_realistic(chip8_32, a, b);  // 기존 realistic 함수 사용
        }
        
        chip8_32.set_pc(chip8_32.get_pc() + 4);
    }

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

    /// @brief SYSCALL 처리 (10SAAAAF) - ModeSelector 호출 수정 버전
    void OP_10SAAAAF(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t syscall_num = (opcode & 0x00F00000) >> 20;  
        uint16_t buffer_addr = (opcode & 0x000FFFF0) >> 4;  
        uint8_t fd = opcode & 0x0000000F;                   
        
        std::cout << "\n=== SYSCALL ===" << std::endl;
        std::cout << "Syscall: " << static_cast<int>(syscall_num) 
                << ", Buffer: 0x" << std::hex << buffer_addr 
                << ", FD: " << std::dec << static_cast<int>(fd) << std::endl; 
        
        // 주소 범위 체크
        if (buffer_addr >= MEMORY_SIZE_32) {
            std::cerr << "Invalid buffer address: 0x" << std::hex << buffer_addr << std::endl;
            chip8_32.set_R(16, 0xFFFFFFFF);
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        switch (syscall_num) {
            case 0x0: {  // READ syscall - 스택 프레임 지원 추가
                std::cout << "[read] Reading from fd " << static_cast<int>(fd) << std::endl;
                
                if (fd == 0) {  // stdin
                    IOManager& io_manager = chip8_32.get_io_manager();
                    
                    size_t max_size = chip8_32.get_R(17);
                    if (max_size == 0 || max_size > 1024) {
                        max_size = 256;
                    }
                    
                    char temp_buffer[1024];
                    size_t bytes_read = io_manager.read(fd, temp_buffer, max_size - 1);
                    
                    if (bytes_read > 0) {
                        uint32_t target_addr;
                
                        // 핵심 개선: 스택 기반 주소 지원
                        if (buffer_addr >= 0xFF00) {
                            // 0xFFxx 패턴을 RBP-xx로 해석
                            uint8_t offset = buffer_addr & 0xFF;
                            target_addr = chip8_32.get_RBP() - offset;
                            
                            std::cout << "[read] 스택 주소 변환: 0x" << std::hex << buffer_addr 
                                    << " → RBP(0x" << chip8_32.get_RBP()
                                    << ") - " << std::dec << static_cast<int>(offset) 
                                    << " = 0x" << std::hex << target_addr << std::dec << std::endl;
                            
                            // BOF 실험: 스택 쓰기 시 경계 검사 완화
                            for (size_t i = 0; i < bytes_read; ++i) {
                                if (target_addr + i < MEMORY_SIZE_32) {
                                    chip8_32.set_memory(target_addr + i, static_cast<uint8_t>(temp_buffer[i]));
                                }
                            }
                    
                        } else {
                            // 기존 방식: 일반 메모리 주소
                            target_addr = buffer_addr;
                            
                            for (size_t i = 0; i < bytes_read; ++i) {
                                if (target_addr + i < MEMORY_SIZE_32) {
                                    chip8_32.set_memory(target_addr + i, static_cast<uint8_t>(temp_buffer[i]));
                                }
                            }
                        }

                        // null terminator 추가
                        if (target_addr + bytes_read < MEMORY_SIZE_32) {
                            chip8_32.set_memory(target_addr + bytes_read, 0);
                        }

                        chip8_32.set_R(16, static_cast<uint32_t>(bytes_read));
                        std::cout << "[read] Successfully read " << bytes_read << " bytes to 0x" 
                                << std::hex << target_addr << std::dec << std::endl;
                        chip8_32.set_pc(chip8_32.get_pc() + 4);
                        
                    } else {
                        std::cout << "[read] No input available, retrying..." << std::endl;
                        
                        static uint32_t last_read_attempt = 0;
                        uint32_t current_time = timer::get_ticks();
                        
                        if (current_time - last_read_attempt > 100) {
                            last_read_attempt = current_time;
                            std::cout << "[read] Waiting for SDL2 input... (Press F1 in game to enter input)" << std::endl;
                        } else {
                            chip8_32.set_pc(chip8_32.get_pc() + 4);
                            chip8_32.set_R(16, 0);
                        }
                    }
                    
                } else {
                    std::cerr << "[read] Unsupported file descriptor: " << static_cast<int>(fd) << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);
                    chip8_32.set_pc(chip8_32.get_pc() + 4);
                }
                break;
            }
            case 0x1: {  // WRITE syscall
                std::cout << "[write] Writing to fd " << static_cast<int>(fd) << std::endl;
                
                if (fd == 1 || fd == 2) {  // stdout, stderr
                    size_t write_size = chip8_32.get_R(17);
                    
                    std::string output;
                    if (write_size == 0) {
                        for (size_t i = 0; i < 1024; ++i) {
                            if (buffer_addr + i >= MEMORY_SIZE_32) break;
                            
                            uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                            if (byte == 0) break;
                            
                            output += static_cast<char>(byte);
                        }
                    } else {
                        write_size = std::min(write_size, static_cast<size_t>(1024));
                        for (size_t i = 0; i < write_size; ++i) {
                            if (buffer_addr + i >= MEMORY_SIZE_32) break;
                            
                            uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                            output += static_cast<char>(byte);
                        }
                    }
                    
                    IOManager& io_manager = chip8_32.get_io_manager();
                    size_t bytes_written = io_manager.write(fd, output.c_str(), output.length());
                    
                    chip8_32.set_R(16, static_cast<uint32_t>(bytes_written));
                    std::cout << "[write] Wrote " << bytes_written << " bytes: " << output << std::endl;
                    
                } else {
                    std::cerr << "[write] Unsupported file descriptor: " << static_cast<int>(fd) << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);
                }
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
            }
            
            case 0x2: {  // GETPID syscall
                std::cout << "[getpid] Returning fake PID" << std::endl;
                chip8_32.set_R(16, 1234);
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
            }
            
            case 0x3: {  //  **LOAD_ROM syscall - 수정된 버전**
                std::cout << "[load_rom] Loading ROM with auto-mode detection" << std::endl;
                
                // 메모리에서 파일명 읽기
                std::string filename;
                for (size_t i = 0; i < 256; ++i) {
                    if (buffer_addr + i >= MEMORY_SIZE_32) break;
                    
                    uint8_t byte = chip8_32.get_memory(buffer_addr + i);
                    if (byte == 0) break;
                    
                    filename += static_cast<char>(byte);
                }
                
                if (!filename.empty()) {
                    std::cout << "[load_rom] File: " << filename << std::endl;
                    
                    // 🔧 **수정: ModeSelector::load_and_switch_mode 사용**
                    bool success = ModeSelector::load_and_switch_mode(chip8_32, filename);
                    
                    if (success) {
                        chip8_32.set_R(16, 0); // 성공
                        
                        // 모드별 분기 처리
                        std::string extension = ModeSelector::get_file_extension(filename);
                        
                        if (extension == ".ch8" || extension == ".c8") {
                            std::cout << "[load_rom] 8-bit ROM loaded, mode switch will occur" << std::endl;
                            chip8_32.set_pc(chip8_32.get_pc() + 4);
                            
                        } else {
                            std::cout << "[load_rom] 32-bit ROM loaded, jumping to 0x200" << std::endl;
                            chip8_32.set_pc(0x200);
                        }
                        
                    } else {
                        chip8_32.set_R(16, 0xFFFFFFFF);
                        chip8_32.set_pc(chip8_32.get_pc() + 4);
                        std::cout << "[load_rom] Failed to load ROM" << std::endl;
                    }
                    
                } else {
                    std::cout << "[load_rom] Empty filename" << std::endl;
                    chip8_32.set_R(16, 0xFFFFFFFF);
                    chip8_32.set_pc(chip8_32.get_pc() + 4);
                }
                break;
            }
            
            case 0x4: {  // EXIT syscall
                uint32_t exit_code = chip8_32.get_R(17);
                std::cout << "[exit] Exiting with code " << exit_code << std::endl;
                chip8_32.set_R(16, 0);
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
            }

            case 0x5: {
                std::cout << "[syscall] Entering calculator mode" << std::endl;
                // IOManager → SDLConsoleIO → Platform 경로로 접근
                // 계산기 기능 제거됨
                chip8_32.set_R(16, 0xFFFFFFFF);  // 실패 코드
                std::cerr << "[Calculator] Calculator functionality removed" << std::endl;
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
            }

            default:
                std::cerr << "[syscall] Unknown syscall: " << static_cast<int>(syscall_num) << std::endl;
                chip8_32.set_R(16, 0xFFFFFFFF);
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
        }
        
        std::cout << "System registers after syscall:" << std::endl;
        std::cout << "  R16 (return): 0x" << std::hex << chip8_32.get_R(16) << std::dec << std::endl;
        std::cout << "  R17 (size):   0x" << std::hex << chip8_32.get_R(17) << std::dec << std::endl;
    }

    void OP_11XXXXXX(Chip8_32& chip8_32, uint32_t opcode) {

        // 스택 프레임 디버그 모드 체크 :: 7월 24일경 수정 내용
        if (opcode == 0x11111111) {
            OP_STACK_FRAME_DEBUG(chip8_32, opcode);
            return;
        }

        uint8_t sub_opcode = (opcode & 0x00FF0000) >> 16;  // 상위 8비트에서 세부 명령어 구분
        
        switch (sub_opcode) {
            // 기본 스택 조작 (0x1100xxxx)
            case 0x00: {
                uint8_t detail = (opcode & 0x0000FF00) >> 8;
                if (detail == 0x00) {
                    StackOpcodes::OP_PUSH_RBP(chip8_32, opcode);  // 0x11000000
                } else {
                    StackOpcodes::OP_PUSH_RX(chip8_32, opcode);   // 0x1100RRXX
                }
                break;
            }
            case 0x01: {
                uint8_t detail = (opcode & 0x0000FF00) >> 8;
                if (detail == 0x00) {
                    StackOpcodes::OP_POP_RBP(chip8_32, opcode);   // 0x11010000
                } else {
                    StackOpcodes::OP_POP_RX(chip8_32, opcode);    // 0x1101RRXX
                }
                break;
            }
            
            // 프레임 포인터 조작 (0x1102xxxx, 0x1103xxxx)
            case 0x02:
                StackOpcodes::OP_MOV_RBP_RSP(chip8_32, opcode);   // 0x11020000
                break;
            case 0x03:
                StackOpcodes::OP_MOV_RSP_RBP(chip8_32, opcode);   // 0x11030000
                break;
            
            // 스택 포인터 조작 (0x1104xxxx, 0x1105xxxx)
            case 0x04:
                StackOpcodes::OP_SUB_RSP(chip8_32, opcode);       // 0x1104NNNN
                break;
            case 0x05:
                StackOpcodes::OP_ADD_RSP(chip8_32, opcode);       // 0x1105NNNN
                break;
            
            // 함수 호출/반환 (0x1106xxxx, 0x1107xxxx)
            case 0x06:
                StackOpcodes::OP_CALL_FUNC(chip8_32, opcode);     // 0x1106NNNN
                break;
            case 0x07:
                StackOpcodes::OP_RET_FUNC(chip8_32, opcode);      // 0x11070000
                break;
            
            // 스택 메모리 접근 (0x1108xxxx - 0x110Bxxxx)
            case 0x08:
                StackOpcodes::OP_MOV_RBP_MINUS_RX(chip8_32, opcode);  // 0x1108RRNN
                break;
            case 0x09:
                StackOpcodes::OP_MOV_RX_RBP_MINUS(chip8_32, opcode);  // 0x1109RRNN
                break;
            case 0x0A:
                StackOpcodes::OP_MOV_RBP_PLUS_RX(chip8_32, opcode);   // 0x110ARRNN
                break;
            case 0x0B:
                StackOpcodes::OP_MOV_RX_RBP_PLUS(chip8_32, opcode);   // 0x110BRRNN
                break;
                
            default:
                std::cerr << "Unknown stack opcode: 0x" << std::hex << opcode << "\n";
                chip8_32.set_pc(chip8_32.get_pc() + 4);
                break;
        }
    };

    /// @brief Vx의 하위 8비트를 메모리[Vy]에 저장 (20XXYY00)
    void OP_20XXYY00(Chip8_32& chip8_32, uint32_t opcode) {
        uint8_t x = (opcode & 0x00FF0000) >> 16;   // 값이 있는 레지스터
        uint8_t y = (opcode & 0x0000FF00) >> 8;    // 주소가 있는 레지스터

        uint32_t address = chip8_32.get_R(y);

        if (address >= MEMORY_SIZE_32) {
            std::cerr << "[OP_20XXYY00] Memory access out of bounds: 0x"
                      << std::hex << address << std::dec << std::endl;
            chip8_32.set_pc(chip8_32.get_pc() + 4);
            return;
        }

        // Vx의 하위 8비트(1바이트)만 저장
        uint8_t low_byte = static_cast<uint8_t>(chip8_32.get_R(x) & 0xFF);
        chip8_32.set_memory(address, low_byte);

        // 디버그 출력 (선택)
        std::cout << "[OP_20XXYY00] MEM[0x" << std::hex << address << "] <- 0x"
                  << std::setw(2) << std::setfill('0') << static_cast<int>(low_byte)
                  << " (R" << std::dec << static_cast<int>(x) << " 하위 바이트)" << std::endl;

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
        primary_table_32[0x11] = OP_11XXXXXX;  // 스택 프레임 명령어 핸들러
        primary_table_32[0x20] = OP_20XXYY00;  // 스택 오버 플로우 테스트를 위한 새로운 명령어 체계 추가
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
 