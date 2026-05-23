// include/security/path_jail.hpp
//
// 경로 봉쇄 (FS Jail) — 게스트(=신뢰 X)가 제공한 파일명이
// 호스트의 임의 경로를 가리키지 못하도록 차단한다.
//
// 정책:
//   1) 허용 루트(roms/)를 weakly_canonical로 고정.
//   2) 입력 파일명을 루트 기준으로 합친 뒤 weakly_canonical로 정규화.
//   3) 정규화 결과가 허용 루트의 하위(prefix)인지 검사.
//      통과 못 한 경로 — `..`, 절대경로, symlink 경유 탈출 — 는 모두 거부.
//   4) 파일명 자체에 디렉터리 구분자(`/`, `\`)나 NUL 바이트가 있으면 거부.
//
// 위반 유형은 PathJailError로 호출자에 전달되어 감사 로그에 그대로 기록된다.

#pragma once

#include <filesystem>
#include <string>

namespace security {

enum class PathJailError {
    Ok = 0,
    EmptyName,           // 빈 파일명
    ForbiddenCharacter,  // 경로 구분자·NUL 등 게스트가 직접 줄 수 없는 문자
    AbsolutePath,        // 절대경로 입력
    EscapesRoot,         // `..`·symlink 등으로 jail 루트를 벗어남
    RootMissing,         // 허용 루트 자체가 존재하지 않음 (배포 오류)
    NotARegularFile      // 디렉터리·장치·소켓 등 일반 파일이 아님
};

struct PathJailResult {
    PathJailError error;
    std::filesystem::path resolved;  // 통과 시: 정규화된 절대경로
    std::string reason;              // 거부 시: 사람이 읽을 수 있는 사유
};

class PathJail {
public:
    // 허용 루트를 설정한다. weakly_canonical 적용 후 디렉터리 존재 검증.
    // 보통 ModeSelector 초기화 시 한 번만 호출.
    static void set_root(const std::filesystem::path& root);

    // 현재 허용 루트(canonical) 반환. set_root 호출 전이면 비어 있음.
    static const std::filesystem::path& root();

    // 게스트가 준 파일명을 jail 내부 절대경로로 해석.
    // 통과: { Ok, resolved=<canonical>, reason="" }
    // 실패: { <Error>, resolved={}, reason=<사유> }
    static PathJailResult resolve(const std::string& guest_filename);
};

// 디버그·로그 가독성용
const char* to_string(PathJailError e);

}  // namespace security
