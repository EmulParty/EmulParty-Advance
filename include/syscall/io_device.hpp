#pragma once

#include <cstdint>
#include <cstddef>

/**
 * @brief I/O 장치 추상 인터페이스
 * 
 * CHIP-8 32비트 확장에서 SYSCALL이 사용할 I/O 장치들의 공통 인터페이스입니다.
 * stdin, stdout, stderr, file 등 다양한 I/O 장치를 통일된 방식으로 처리합니다.
 */
class IODevice {
public:
    virtual ~IODevice() = default;

    /**
     * @brief 장치에서 데이터를 읽습니다
     * @param buffer 읽은 데이터를 저장할 버퍼
     * @param size 읽을 최대 바이트 수
     * @return 실제로 읽은 바이트 수 (실패 시 0)
     */
    virtual size_t read(char* buffer, size_t size) = 0;

    /**
     * @brief 장치에 데이터를 씁니다
     * @param buffer 쓸 데이터가 담긴 버퍼
     * @param size 쓸 바이트 수
     * @return 실제로 쓴 바이트 수 (실패 시 0)
     */
    virtual size_t write(const char* buffer, size_t size) = 0;

    /**
     * @brief 장치가 읽기 가능한지 확인
     * @return true if readable, false otherwise
     */
    virtual bool is_readable() const = 0;

    /**
     * @brief 장치가 쓰기 가능한지 확인
     * @return true if writable, false otherwise
     */
    virtual bool is_writable() const = 0;

    /**
     * @brief 장치 타입을 반환 (디버깅용)
     * @return 장치 타입 문자열
     */
    virtual const char* get_device_type() const = 0;
};