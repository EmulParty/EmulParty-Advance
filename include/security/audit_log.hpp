// include/security/audit_log.hpp
//
// 감사 로그 (Audit Log) — 보안 결정 지점에서 일어난 일을 한 줄 JSON으로 남긴다.
// 목적: SOC 탐지·포렌식 / before-after 시연 / 회귀 방지.
//
// 형식 (JSON Lines, 1 event = 1 line):
//   {"ts":"2026-05-22T16:00:00Z","event":"path_jail","decision":"deny",
//    "input":"../../etc/passwd","reason":"escapes_root","caller":"load_rom"}
//
// 스레드 안전 — 현재 single-threaded 에뮬레이터지만, fstream에 mutex를 둠.

#pragma once

#include <string>
#include <vector>
#include <utility>

namespace security {

enum class Decision {
    Allow,
    Deny
};

// key-value 쌍의 가벼운 wrapper. JSON 문자열 이스케이프는 내부에서 처리.
struct Field {
    std::string key;
    std::string value;
    Field(std::string k, std::string v) : key(std::move(k)), value(std::move(v)) {}
    Field(std::string k, int v);
    Field(std::string k, unsigned int v);
    Field(std::string k, unsigned long v);
    Field(std::string k, long long v);
};

class AuditLog {
public:
    // 로그 파일을 연다. 빈 경로면 stderr로만 출력.
    // append 모드로 열리며, 호출은 한 번만 (idempotent).
    static void init(const std::string& path = "");

    // event: path_jail / syscall_policy / dos_guard / guest_fault 등 짧은 분류.
    // decision: Allow / Deny.
    // fields: 임의의 보조 정보.
    static void record(const std::string& event,
                       Decision decision,
                       const std::vector<Field>& fields);

    // 편의: 분류·결정 없이 정보 로그만 남길 때.
    static void info(const std::string& event,
                     const std::vector<Field>& fields);

    // 테스트·시연 종료 시 flush.
    static void flush();
};

const char* to_string(Decision d);

}  // namespace security
