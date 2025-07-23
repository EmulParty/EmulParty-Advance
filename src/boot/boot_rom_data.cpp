// boot_rom_data.cpp - 자동 크기 조정 버전
#include "boot_rom_data.hpp"

// 🔧 **수정: 자동 크기 조정 + 크기 상수 정의**
const uint32_t BOOT_ROM[] = {
    // 0x0000: R17에 메시지 길이 설정 (부팅 메시지)
    0x06110030,  // LD R17, 48 ("NeoCHIP-8 BootROM v2.0 - Enter ROM filename: ")
    
    // 0x0004: WRITE syscall로 부팅 메시지 출력 (fd=1, buffer=0x0100)  
    0x10101000 | (0x0100 << 4) | 1,  // SYSCALL WRITE fd=1, buffer=0x0100, size=R17
    
    // 0x0008: R17에 입력 버퍼 크기 설정 (256바이트)
    0x06110100,  // LD R17, 256
    
    // 0x000C: READ syscall로 파일명 입력받기 (fd=0, buffer=0x0200)
    0x10002000 | (0x0200 << 4) | 0,  // SYSCALL READ fd=0, buffer=0x0200, size=R17
    
    // 0x0010: R17 초기화 (LOAD_ROM syscall용)
    0x06110000,  // LD R17, 0
    
    // 0x0014: LOAD_ROM syscall로 파일 로드 (syscall=3, buffer=0x0200)
    0x10302000 | (0x0200 << 4) | 0,  // SYSCALL LOAD_ROM syscall=3, buffer=0x0200
    
    // 0x0018: 무한 루프 (ROM 로드 실패 시)
    0x01000018,  // JMP 0x0018
    
    // 0x001C: 예비 (사용 안 함)
    0x00000000   // NOP
};

// 🔧 **배열 크기 상수 정의**
const size_t BOOT_ROM_SIZE = sizeof(BOOT_ROM) / sizeof(BOOT_ROM[0]);