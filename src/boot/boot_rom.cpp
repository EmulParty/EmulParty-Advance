// boot_rom.cpp - 자동 크기 사용 버전
#include "boot_rom.hpp"
#include "boot_rom_data.hpp"
#include "chip8_32.hpp"
#include <cstring>
#include <iostream>

void BootROM::load_into_memory(Chip8_32& chip8) {
    // 1. Boot ROM 명령어들을 메모리 0x0000부터 로드 (자동 크기 사용)
    for (size_t i = 0; i < BOOT_ROM_SIZE; ++i) {  // 🔧 BOOT_ROM_SIZE 사용
        uint32_t opcode = BOOT_ROM[i];
        chip8.set_memory(0x0000 + i*4 + 0, (opcode >> 24) & 0xFF);
        chip8.set_memory(0x0000 + i*4 + 1, (opcode >> 16) & 0xFF);
        chip8.set_memory(0x0000 + i*4 + 2, (opcode >> 8) & 0xFF);
        chip8.set_memory(0x0000 + i*4 + 3, opcode & 0xFF);
    }
    
    // 2. 부팅 메시지를 메모리 0x0100에 저장
    const char* boot_message = "";
    size_t msg_len = strlen(boot_message);
    
    for (size_t i = 0; i < msg_len; ++i) {
        chip8.set_memory(0x0100 + i, static_cast<uint8_t>(boot_message[i]));
    }
    
    // null terminator 추가
    chip8.set_memory(0x0100 + msg_len, 0);
    
    // 3. 입력 버퍼 영역 초기화 (0x0200~0x02FF)
    for (int i = 0; i < 256; ++i) {
        chip8.set_memory(0x0200 + i, 0);
    }
    
    std::cout << "[BootROM] Loaded " << BOOT_ROM_SIZE << " instructions" << std::endl;
    std::cout << "[BootROM] Boot message: \"" << boot_message << "\"" << std::endl;
    std::cout << "[BootROM] Input buffer: 0x0200-0x02FF" << std::endl;
}