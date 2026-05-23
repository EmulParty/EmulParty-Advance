// include/syscall/syscall_policy.hpp
//
// SYSCALL 정책 (최소권한) — 게스트가 호출 가능한 syscall을 허용목록으로 제한.
//
// 두 가지 프로파일:
//   - kBootRom : BootROM 자신은 모든 syscall 허용 (LOAD_ROM도 BootROM만 호출).
//   - kGuest   : 일반 ROM은 READ/WRITE/GETPID/EXIT/CALC만 허용.
//                LOAD_ROM·DEBUG 등 호스트 상태를 바꿀 수 있는 syscall은 차단.
//
// 결정 결과는 audit_log에 기록되고, 호출자(opcode_table_32)는 거부 시
// 에러 코드(0xFFFFFFFF)를 R16에 채우고 PC만 진행한다.

#pragma once

#include <cstdint>
#include <string>

namespace security {

enum class SyscallProfile {
    kBootRom,  // BootROM 영역(0x0000~0x01FF)에서 실행 중 — 신뢰 코드
    kGuest     // 사용자 ROM 영역(0x0200~) — 신뢰 X
};

struct SyscallDecision {
    bool allowed;
    std::string reason;       // 거부 시 사유 (감사 로그용)
    std::string syscall_name; // 가독성용
};

class SyscallPolicy {
public:
    // PC 값으로 BootROM/Guest 프로파일 자동 판정.
    // (BootROM은 0x0000~0x01FF 영역에 로드됨)
    static SyscallProfile classify_by_pc(uint32_t pc);

    // syscall_num: 0=READ, 1=WRITE, 2=GETPID, 3=LOAD_ROM, 4=EXIT, 5=CALC 등.
    // 정책 결정 + 사람이 읽을 수 있는 이름 반환.
    static SyscallDecision evaluate(SyscallProfile profile, uint8_t syscall_num);

    // syscall 번호 → 이름 (감사 로그·디버깅용)
    static const char* name_of(uint8_t syscall_num);
};

const char* to_string(SyscallProfile p);

}  // namespace security
