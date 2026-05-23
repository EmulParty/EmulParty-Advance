// src/security/audit_log.cpp
//
// JSON-lines 감사 로거. stdout과 파일에 동시 기록 (헤드리스 시연용).

#include "security/audit_log.hpp"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace security {

namespace {
std::mutex g_mutex;
std::ofstream g_file;
bool g_inited = false;
}

Field::Field(std::string k, int v)            : key(std::move(k)), value(std::to_string(v)) {}
Field::Field(std::string k, unsigned int v)   : key(std::move(k)), value(std::to_string(v)) {}
Field::Field(std::string k, unsigned long v)  : key(std::move(k)), value(std::to_string(v)) {}
Field::Field(std::string k, long long v)      : key(std::move(k)), value(std::to_string(v)) {}

const char* to_string(Decision d) {
    return d == Decision::Allow ? "allow" : "deny";
}

static std::string iso8601_utc_now() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t   = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

static std::string json_escape(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // 제어문자는 \u00XX로
                    oss << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                        << (static_cast<unsigned int>(c) & 0xFF) << std::dec;
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

void AuditLog::init(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_inited) return;
    if (!path.empty()) {
        g_file.open(path, std::ios::app);
        if (!g_file.is_open()) {
            std::cerr << "[audit] failed to open " << path
                      << " — falling back to stderr-only" << std::endl;
        }
    }
    g_inited = true;
}

static void write_line(const std::string& line) {
    // 콘솔과 파일에 동시 기록. 콘솔은 [AUDIT] prefix.
    std::cerr << "[AUDIT] " << line << std::endl;
    if (g_file.is_open()) {
        g_file << line << "\n";
        g_file.flush();
    }
}

static std::string build_line(const std::string& event,
                              const char* decision_or_null,
                              const std::vector<Field>& fields) {
    std::ostringstream oss;
    oss << "{\"ts\":\"" << iso8601_utc_now() << "\""
        << ",\"event\":\"" << json_escape(event) << "\"";
    if (decision_or_null) {
        oss << ",\"decision\":\"" << decision_or_null << "\"";
    }
    for (const auto& f : fields) {
        oss << ",\"" << json_escape(f.key) << "\":\""
            << json_escape(f.value) << "\"";
    }
    oss << "}";
    return oss.str();
}

void AuditLog::record(const std::string& event,
                      Decision decision,
                      const std::vector<Field>& fields) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_inited) {
        // init 안 했어도 stderr로는 흘려보냄. 시연 누락 방지.
        g_inited = true;
    }
    write_line(build_line(event, to_string(decision), fields));
}

void AuditLog::info(const std::string& event, const std::vector<Field>& fields) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_inited) g_inited = true;
    write_line(build_line(event, nullptr, fields));
}

void AuditLog::flush() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file.is_open()) g_file.flush();
}

}  // namespace security
