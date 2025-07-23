#pragma once

#include "io_device.hpp"
#include <unordered_map>
#include <memory>
#include <cstdint>

/**
 * @brief I/O 장치 관리자
 * 
 * 파일 디스크립터(fd)를 기반으로 I/O 장치들을 관리합니다.
 * SYSCALL에서 fd를 통해 적절한 I/O 장치로 라우팅합니다.
 */

class IOManager {
private:
    // fd -> IODevice 매핑
    std::unordered_map<uint8_t, std::shared_ptr<IODevice>> devices_;

public:
    IOManager() = default;
    ~IOManager() = default;

    /**
     * @brief I/O 장치를 파일 디스크립터에 등록
     * @param fd 파일 디스크립터 (0=stdin, 1=stdout, 2=stderr, 3+=file)
     * @param device I/O 장치 포인터
     * @return 성공 시 true, 실패 시 false
     */
    bool registerDevice(uint8_t fd, std::shared_ptr<IODevice> device);

    /**
     * @brief I/O 장치 등록 해제
     * @param fd 파일 디스크립터
     * @return 성공 시 true, 실패 시 false
     */
    bool unregisterDevice(uint8_t fd);

    /**
     * @brief 지정된 fd에서 데이터 읽기
     * @param fd 파일 디스크립터
     * @param buffer 읽은 데이터를 저장할 버퍼
     * @param size 읽을 최대 바이트 수
     * @return 실제로 읽은 바이트 수 (실패 시 0)
     */
    size_t read(uint8_t fd, char* buffer, size_t size);

    /**
     * @brief 지정된 fd에 데이터 쓰기
     * @param fd 파일 디스크립터
     * @param buffer 쓸 데이터가 담긴 버퍼
     * @param size 쓸 바이트 수
     * @return 실제로 쓴 바이트 수 (실패 시 0)
     */
    size_t write(uint8_t fd, const char* buffer, size_t size);

    /**
     * @brief 등록된 장치 확인
     * @param fd 파일 디스크립터
     * @return 장치가 등록되어 있으면 true
     */
    bool hasDevice(uint8_t fd) const;

    /**
     * @brief 장치 정보 출력 (디버깅용)
     */
    void printDevices() const;

    /**
     * @brief 등록된 모든 장치 해제
     */
    void clear();
};