// src/security/path_jail.cpp
//
// PathJail 구현. weakly_canonical로 절대경로화한 뒤 prefix 비교로 봉쇄.
//
// 주의:
//   - weakly_canonical은 일부 컴포넌트가 존재하지 않아도 해석한다(존재 검증과는 별개).
//   - prefix 비교는 path::lexically_relative와 ".."로 시작하는지 확인하는 방식이 가장 견고.
//     문자열 비교(starts_with)는 /roms_evil 같은 경계 모호성에 취약하기 때문.

#include "security/path_jail.hpp"

#include <cstring>
#include <sstream>
#include <system_error>

namespace security {

namespace fs = std::filesystem;

namespace {
fs::path g_root;  // canonical 허용 루트
}

void PathJail::set_root(const fs::path& root) {
    std::error_code ec;
    fs::path canonical = fs::weakly_canonical(root, ec);
    if (ec) {
        // canonical 실패해도 입력 자체는 보관 — resolve()에서 RootMissing으로 거부됨.
        canonical = root;
    }
    g_root = canonical;
}

const fs::path& PathJail::root() {
    return g_root;
}

const char* to_string(PathJailError e) {
    switch (e) {
        case PathJailError::Ok: return "ok";
        case PathJailError::EmptyName: return "empty_name";
        case PathJailError::ForbiddenCharacter: return "forbidden_character";
        case PathJailError::AbsolutePath: return "absolute_path";
        case PathJailError::EscapesRoot: return "escapes_root";
        case PathJailError::RootMissing: return "root_missing";
        case PathJailError::NotARegularFile: return "not_regular_file";
    }
    return "unknown";
}

static bool contains_forbidden_chars(const std::string& s) {
    // 게스트가 직접 입력하는 파일명에 디렉터리 구분자나 NUL이 들어오면
    // 의도적 탈출 시도로 간주. 같은 디렉터리 내 파일명만 허용.
    for (char c : s) {
        if (c == '/' || c == '\\' || c == '\0') return true;
    }
    return false;
}

PathJailResult PathJail::resolve(const std::string& guest_filename) {
    PathJailResult out;
    out.error = PathJailError::Ok;

    if (guest_filename.empty()) {
        out.error = PathJailError::EmptyName;
        out.reason = "filename is empty";
        return out;
    }

    if (contains_forbidden_chars(guest_filename)) {
        out.error = PathJailError::ForbiddenCharacter;
        out.reason = "filename contains path separator or NUL";
        return out;
    }

    // 입력 자체가 절대경로면 즉시 거부 (path("roms")/"/etc/passwd" 가 roms를 무시하는 버그도 차단).
    fs::path input(guest_filename);
    if (input.is_absolute()) {
        out.error = PathJailError::AbsolutePath;
        out.reason = "absolute path is not allowed";
        return out;
    }

    if (g_root.empty()) {
        out.error = PathJailError::RootMissing;
        out.reason = "jail root is not configured";
        return out;
    }

    std::error_code ec;
    fs::path root_canonical = fs::weakly_canonical(g_root, ec);
    if (ec || !fs::exists(root_canonical)) {
        out.error = PathJailError::RootMissing;
        out.reason = "jail root does not exist: " + g_root.string();
        return out;
    }

    fs::path combined = root_canonical / input;
    fs::path resolved = fs::weakly_canonical(combined, ec);
    if (ec) {
        // canonical 자체가 실패하면 보수적으로 거부.
        out.error = PathJailError::EscapesRoot;
        out.reason = "failed to canonicalize: " + combined.string();
        return out;
    }

    // prefix 검사: resolved가 root_canonical의 하위인지 lexically_relative로 판정.
    // "../foo"로 시작하면 탈출.
    fs::path rel = resolved.lexically_relative(root_canonical);
    if (rel.empty() || rel.native().compare(0, 2, fs::path("..").native()) == 0) {
        out.error = PathJailError::EscapesRoot;
        std::ostringstream oss;
        oss << "resolved path escapes jail: " << resolved.string()
            << " (root=" << root_canonical.string() << ")";
        out.reason = oss.str();
        return out;
    }

    // 실제 파일이 존재하고 일반 파일인지 확인.
    if (!fs::exists(resolved, ec) || ec) {
        out.error = PathJailError::NotARegularFile;
        out.reason = "file does not exist: " + resolved.string();
        return out;
    }
    if (!fs::is_regular_file(resolved, ec) || ec) {
        out.error = PathJailError::NotARegularFile;
        out.reason = "not a regular file: " + resolved.string();
        return out;
    }

    out.resolved = resolved;
    return out;
}

}  // namespace security
