// include/core/mode_selector.hpp
#pragma once
#include <string>
#include "common/constants.hpp"

/**
 * @brief 모드 선택기 클래스
 * 파일 확장자를 기반으로 적절한 에뮬레이터 모드를 선택하고 실행
 */
class ModeSelector {
public:
    /**
     * @brief ROM 파일을 분석하여 적절한 모드로 실행
     * @param rom_path ROM 파일 경로
     * @return 실행 결과 (0: 성공, 1: 실패)
     */
    static int select_and_run();

    /**
     * @brief 디버그 모드 설정
     * @param enable true면 디버그 모드 활성화
     */
    static void set_debug_mode(bool enable);
   

private:
    /**
     * @brief 파일 확장자 추출
     * @param filename 파일명
     * @return 소문자로 변환된 확장자 (예: ".ch8", ".ch32")
     */
    static std::string get_file_extension(const std::string& filename);
    static int run_8bit_mode_with_file(const std::string& filename);
    static int run_32bit_mode_with_file(const std::string& filename);
};