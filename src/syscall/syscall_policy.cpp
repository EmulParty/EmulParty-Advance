// src/syscall/syscall_policy.cpp
//
// 정책 표 — 두 프로파일에 대한 syscall 허용/거부 매트릭스.
// 향후 .policy 파일 로딩으로 확장 가능하지만, 시연용으론 하드코딩이 충분.

#include "syscall/syscall_policy.hpp"

namespace security {

// BootROM 영역은 boot_rom_data.cpp가 0x0000부터 로드.
// 사용자 ROM은 0x0200 이후. constants.hpp의 START_ADDRESS_32 = 0x0200.
constexpr uint32_t kBootRomEndExclusive = 0x0200;

SyscallProfile SyscallPolicy::classify_by_pc(uint32_t pc) {
    return pc < kBootRomEndExclusive ? SyscallProfile::kBootRom
                                     : SyscallProfile::kGuest;
}

const char* SyscallPolicy::name_of(uint8_t syscall_num) {
    switch (syscall_num) {
        case 0x0: return "READ";
        case 0x1: return "WRITE";
        case 0x2: return "GETPID";
        case 0x3: return "LOAD_ROM";
        case 0x4: return "EXIT";
        case 0x5: return "CALC";
        case 0x6: return "DEBUG";
        default:  return "UNKNOWN";
    }
}

SyscallDecision SyscallPolicy::evaluate(SyscallProfile profile, uint8_t syscall_num) {
    SyscallDecision d;
    d.syscall_name = name_of(syscall_num);

    if (profile == SyscallProfile::kBootRom) {
        // BootROM은 모든 syscall 허용 (자체가 신뢰 코드).
        d.allowed = true;
        return d;
    }

    // Guest 프로파일: 기본 허용목록.
    switch (syscall_num) {
        case 0x0:  // READ
        case 0x1:  // WRITE
        case 0x2:  // GETPID
        case 0x4:  // EXIT
        case 0x5:  // CALC
            d.allowed = true;
            return d;

        case 0x3:  // LOAD_ROM — 게스트가 호스트 파일을 읽도록 만드는 통로. 차단.
            d.allowed = false;
            d.reason = "LOAD_ROM is BootROM-only; guest cannot mount host files";
            return d;

        case 0x6:  // DEBUG — 호스트 디버거 진입. 시연 외 금지.
            d.allowed = false;
            d.reason = "DEBUG syscall is not in guest allowlist";
            return d;

        default:
            d.allowed = false;
            d.reason = "syscall not in guest allowlist";
            return d;
    }
}

const char* to_string(SyscallProfile p) {
    return p == SyscallProfile::kBootRom ? "bootrom" : "guest";
}

}  // namespace security
