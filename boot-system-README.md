# Boot System Feature Branch

## 개요
CHIP-8 32비트 확장 에뮬레이터의 부팅 시스템을 구현하는 기능 브랜치입니다.

## 주요 기능

### Boot ROM System
- **부팅 ROM 로딩**: 시스템 시작 시 필수 명령어들을 메모리에 자동 로드
- **메모리 초기화**: 입력 버퍼 및 시스템 메모리 영역 초기화
- **부팅 메시지**: 시스템 상태 메시지 출력

## 구현 파일

### 헤더 파일
- `include/boot/boot_rom.hpp`: Boot ROM 클래스 정의
- `include/boot/boot_rom_data.hpp`: Boot ROM 데이터 정의

### 소스 파일
- `src/boot/boot_rom.cpp`: Boot ROM 로딩 로직 구현
- `src/boot/boot_rom_data.cpp`: Boot ROM 데이터 실제 구현

## 메모리 맵

| 주소 범위 | 용도 |
|----------|------|
| 0x0000-0x00FF | Boot ROM 명령어 영역 |
| 0x0100-0x01FF | 부팅 메시지 저장 영역 |
| 0x0200-0x02FF | 입력 버퍼 영역 |

## 사용법

```cpp
#include "boot_rom.hpp"
#include "chip8_32.hpp"

Chip8_32 chip8;
BootROM::load_into_memory(chip8);
```

## 특징
- 자동 크기 계산을 통한 효율적인 메모리 사용
- 32비트 명령어를 4바이트로 분할하여 메모리에 저장
- 시스템 초기화 완료 메시지 출력