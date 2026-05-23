// test/test_hardening.cpp
//
// VMM 하드닝 단위 테스트 — Catch2 기반.
//
// 대상:
//   1. PathJail::resolve  : 정상 파일 / 경로구분자 / 절대경로 / 심링크 탈출 / 존재X
//   2. SyscallPolicy::evaluate : BootROM은 LOAD_ROM 허용, Guest는 거부
//   3. SyscallPolicy::classify_by_pc : 0x0000~0x01FF vs 0x0200+
//
// 빌드:
//   cmake --build build --target test_hardening
//   (CMakeLists.txt에 test_hardening 타깃이 추가되어야 함)
//
// 실행:
//   ./build/test_hardening                       # 모든 케이스
//   ./build/test_hardening "[path_jail]"         # 특정 태그
//
// 본 테스트는 시연을 위한 임시 jail 루트를 std::filesystem::temp_directory_path()
// 아래에 만들어 격리한다. 실제 roms/ 디렉터리는 건드리지 않는다.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "security/path_jail.hpp"
#include "syscall/syscall_policy.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

// ── 헬퍼: 테스트용 임시 jail 디렉터리 + 정상 ROM 파일 1개 + 심링크 탈출 1개 ──
struct JailFixture {
    fs::path root;

    JailFixture() {
        // 고유한 임시 디렉터리 (병렬 실행 안전)
        root = fs::temp_directory_path() /
               ("epa_jail_test_" + std::to_string(std::rand()));
        fs::create_directories(root);

        // 정상 ROM 파일 1개
        std::ofstream(root / "ok.ch32") << "OK";

        // 외부 파일 + 그것을 가리키는 심링크 (탈출 시도)
        auto outside = root.parent_path() / "secret.txt";
        std::ofstream(outside) << "TOPSECRET";
        std::error_code ec;
        fs::create_symlink(outside, root / "sneaky.ch32", ec);
        // 심링크 권한이 없는 환경에서는 sneaky 케이스만 SKIP되도록 ec 무시.

        security::PathJail::set_root(root);
    }

    ~JailFixture() {
        std::error_code ec;
        fs::remove(root.parent_path() / "secret.txt", ec);
        fs::remove_all(root, ec);
    }
};

// =================================================================
//  PathJail
// =================================================================

TEST_CASE_METHOD(JailFixture, "PathJail allows file inside jail root", "[path_jail][allow]") {
    auto r = security::PathJail::resolve("ok.ch32");
    REQUIRE(r.error == security::PathJailError::Ok);
    REQUIRE(r.resolved.filename() == "ok.ch32");
}

TEST_CASE_METHOD(JailFixture, "PathJail denies path separator in filename", "[path_jail][deny]") {
    auto r = security::PathJail::resolve("../../etc/passwd");
    REQUIRE(r.error == security::PathJailError::ForbiddenCharacter);
}

TEST_CASE_METHOD(JailFixture, "PathJail denies absolute path", "[path_jail][deny]") {
    auto r = security::PathJail::resolve("/etc/passwd");
    // 절대경로는 ForbiddenCharacter 검사가 먼저 잡는다 (선두에 `/`).
    // 어느 단계든 deny면 OK.
    REQUIRE(r.error != security::PathJailError::Ok);
}

TEST_CASE_METHOD(JailFixture, "PathJail denies backslash separator", "[path_jail][deny]") {
    auto r = security::PathJail::resolve("..\\..\\etc\\passwd");
    REQUIRE(r.error == security::PathJailError::ForbiddenCharacter);
}

TEST_CASE_METHOD(JailFixture, "PathJail denies symlink escaping root", "[path_jail][deny]") {
    if (!fs::exists(root / "sneaky.ch32")) {
        SUCCEED("symlink creation skipped (platform/permissions)");
        return;
    }
    auto r = security::PathJail::resolve("sneaky.ch32");
    REQUIRE(r.error == security::PathJailError::EscapesRoot);
}

TEST_CASE_METHOD(JailFixture, "PathJail denies empty filename", "[path_jail][deny]") {
    auto r = security::PathJail::resolve("");
    REQUIRE(r.error == security::PathJailError::EmptyName);
}

TEST_CASE_METHOD(JailFixture, "PathJail denies nonexistent file", "[path_jail][deny]") {
    auto r = security::PathJail::resolve("not_there.ch32");
    REQUIRE(r.error == security::PathJailError::NotARegularFile);
}

// =================================================================
//  SyscallPolicy
// =================================================================

TEST_CASE("SyscallPolicy: BootROM profile allows everything", "[syscall_policy][bootrom]") {
    using namespace security;
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kBootRom, 0x0).allowed);  // READ
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kBootRom, 0x1).allowed);  // WRITE
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kBootRom, 0x3).allowed);  // LOAD_ROM
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kBootRom, 0x6).allowed);  // DEBUG
}

TEST_CASE("SyscallPolicy: Guest profile blocks LOAD_ROM", "[syscall_policy][guest]") {
    auto d = security::SyscallPolicy::evaluate(security::SyscallProfile::kGuest, 0x3);
    REQUIRE_FALSE(d.allowed);
    REQUIRE(d.syscall_name == std::string("LOAD_ROM"));
}

TEST_CASE("SyscallPolicy: Guest profile blocks DEBUG", "[syscall_policy][guest]") {
    auto d = security::SyscallPolicy::evaluate(security::SyscallProfile::kGuest, 0x6);
    REQUIRE_FALSE(d.allowed);
}

TEST_CASE("SyscallPolicy: Guest profile allows benign syscalls", "[syscall_policy][guest]") {
    using namespace security;
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kGuest, 0x0).allowed);  // READ
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kGuest, 0x1).allowed);  // WRITE
    REQUIRE(SyscallPolicy::evaluate(SyscallProfile::kGuest, 0x4).allowed);  // EXIT
}

TEST_CASE("SyscallPolicy: classify_by_pc boundary", "[syscall_policy][profile]") {
    using namespace security;
    REQUIRE(SyscallPolicy::classify_by_pc(0x0000) == SyscallProfile::kBootRom);
    REQUIRE(SyscallPolicy::classify_by_pc(0x01FF) == SyscallProfile::kBootRom);
    REQUIRE(SyscallPolicy::classify_by_pc(0x0200) == SyscallProfile::kGuest);
    REQUIRE(SyscallPolicy::classify_by_pc(0xFFFF) == SyscallProfile::kGuest);
}

TEST_CASE("SyscallPolicy: unknown syscall is denied for guest", "[syscall_policy][guest]") {
    auto d = security::SyscallPolicy::evaluate(security::SyscallProfile::kGuest, 0xE);
    REQUIRE_FALSE(d.allowed);
}
