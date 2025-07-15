CHIP-8 Dual Mode Emulator
세상에 없던 32비트 확장 CHIP-8 에뮬레이터와 보안 취약점 시뮬레이션 플랫폼
📋 프로젝트 개요
이 프로젝트는 전통적인 8비트 CHIP-8 에뮬레이터를 32비트로 확장하고, 스택 프레임 관리와 메모리 보안 기능을 추가한 교육용 에뮬레이터입니다. 에뮬레이터 상에서 다양한 보안 취약점을 시뮬레이션할 수 있도록 설계되었습니다.
🏗️ 프로젝트 구조
EmulParty-Advance/
├── include/
│   ├── common/
│   │   └── constants.hpp          # 공통 상수 정의
│   ├── core/
│   │   ├── chip8.hpp             # 8비트 CHIP-8 코어
│   │   ├── chip8_32.hpp          # 32비트 CHIP-8 확장 코어
│   │   ├── mode_selector.hpp      # 모드 선택기
│   │   ├── opcode_table.hpp       # 8비트 명령어 테이블
│   │   └── opcode_table_32.hpp    # 32비트 명령어 테이블
│   ├── debugger/
│   │   └── debugger.hpp           # 디버거 헤더
│   ├── platform/
│   │   ├── platform.hpp          # SDL2 플랫폼 계층
│   │   └── timer.hpp              # 타이머 유틸리티
│   └── security/                  # 보안 기능 (추후 확장)
├── src/
│   ├── core/
│   │   ├── chip8.cpp
│   │   ├── chip8_32.cpp
│   │   ├── mode_selector.cpp
│   │   ├── opcode_table.cpp
│   │   └── opcode_table_32.cpp
│   ├── debugger/
│   │   └── debugger.cpp
│   ├── platform/
│   │   ├── platform.cpp
│   │   └── timer.cpp
│   └── main.cpp
├── roms/                          # ROM 파일들
└── build/                         # 빌드 출력 디렉토리
🎯 주요 기능
🔧 기본 기능

듀얼 모드 지원: 8비트 클래식 CHIP-8 + 32비트 확장 모드
파일 확장자 자동 감지: .ch8/.c8 (8비트), .ch32/.c32 (32비트)
SDL2 기반 그래픽: WSL2/X11 환경 지원
실시간 키보드 입력: QWERTY 키보드 매핑

🐛 디버깅 기능

인터랙티브 디버거: 단계별 실행, 브레이크포인트
실시간 상태 모니터링: 레지스터, 메모리, 스택 상태
명령어 디스어셈블: 실행 중인 opcode 해석 표시

🛡️ 보안 기능 (개발 중)

스택 프레임 관리: 함수 호출/반환 추적
메모리 보안 검사: 버퍼 오버플로우 감지
취약점 시뮬레이션: 교육용 보안 시나리오

📦 시스템 요구사항
필수 의존성

CMake: 3.10 이상
C++ 컴파일러: GCC 7+ 또는 Clang 5+
SDL2: 개발 라이브러리 포함

Ubuntu/Debian 설치
bashsudo apt update
sudo apt install build-essential cmake libsdl2-dev
WSL2 환경 (X11 필요)
bash# X11 서버 설치 (Windows에서)
# VcXsrv 또는 X410 사용 권장

# WSL2에서 DISPLAY 환경변수 설정
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
🚀 빌드 및 실행
1. 빌드
bash# 프로젝트 루트에서
mkdir -p build
cd build
cmake ..
make
2. 실행 방법
🎮 일반 모드 (Normal Mode)
bash# 8비트 CHIP-8 ROM 실행
./chip8_dual ../roms/maze.ch8
./chip8_dual ../roms/pong.ch8
./chip8_dual ../roms/tetris.c8

# 32비트 확장 ROM 실행 (추후 지원)
./chip8_dual ../roms/extended_demo.ch32
🐛 디버그 모드 (Debug Mode)
bash# 디버그 모드로 8비트 ROM 실행
./chip8_dual --debug ../roms/maze.ch8
./chip8_dual -d ../roms/pong.ch8

# 디버그 모드로 32비트 ROM 실행
./chip8_dual --debug ../roms/extended_demo.ch32
💡 디버그 모드 특징:

각 명령어마다 실행 일시정지
Enter 키만 누르면 다음 명령어 실행
실시간 상태 모니터링 (레지스터, 메모리, 스택)
브레이크포인트 지원

3. 명령행 옵션
bash# 기본 사용법
./chip8_dual [옵션] <ROM_파일>

# 옵션:
#   --debug, -d    인터랙티브 디버거 활성화
#   --help, -h     도움말 표시 (추후 구현)

# 예시:
./chip8_dual ../roms/space_invaders.ch8           # 일반 실행
./chip8_dual --debug ../roms/breakout.ch8         # 디버그 모드
🎮 조작법
키보드 매핑
CHIP-8의 16진 키패드를 QWERTY 키보드에 매핑:
CHIP-8 키패드     →     QWERTY 키보드
┌─────────────┐         ┌─────────────┐
│ 1 │ 2 │ 3 │ C │       │ 1 │ 2 │ 3 │ 4 │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ D │  →    │ Q │ W │ E │ R │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ E │       │ A │ S │ D │ F │
├───┼───┼───┼───┤       ├───┼───┼───┼───┤
│ A │ 0 │ B │ F │       │ Z │ X │ C │ V │
└───┴───┴───┴───┘       └───┴───┴───┴───┘
디버거 명령어 (디버그 모드에서)
명령어              설명
──────────────────────────────────────
s, step           다음 명령어 실행
c, continue       연속 실행 모드로 전환
q, quit           디버거 종료
bp <주소>         브레이크포인트 설정 (16진수)
help, h           도움말 표시
Enter (빈 입력)   단계 실행 (step과 동일)

예시:
Debug> s                # 단계 실행
Debug> [Enter]          # Enter만 쳐도 단계 실행
Debug> bp 0x200         # 0x200 주소에 브레이크포인트
Debug> c                # 연속 실행
Debug> q                # 종료
🐛 디버그 모드 사용법
디버그 모드에서는 각 명령어가 하나씩 실행되며, 사용자 입력을 기다립니다:

단계별 실행: 프로그램이 한 명령어 실행 후 일시정지
Enter 키: 명령어 입력 없이 Enter만 쳐도 다음 명령어 실행
상태 표시: 매번 레지스터, 메모리, 스택 상태 출력
브레이크포인트: 특정 주소에서 자동 일시정지

bash# 디버그 모드 실행 예시
./chip8_dual --debug ../roms/maze.ch8

# 출력 예시:
# ============================================================
# 🎮 8-bit CHIP-8 Debug State
# ============================================================
# 📍 PC=0x200  Opcode=0xA22A  ➤ LD I
# 
# 📊 V0-V7:  V0=00  V1=00  V2=00  V3=00  V4=00  V5=00  V6=00  V7=00
# 📊 V8-VF:  V8=00  V9=00  VA=00  VB=00  VC=00  VD=00  VE=00  VF=00
# 
# 🎯 I=022A  SP=0  Delay=0  Sound=0
# 
# 🐛 Debug> [여기서 Enter 또는 명령어 입력]
📊 에뮬레이터 비교
기능8비트 모드32비트 확장 모드메모리 크기4KB (4,096바이트)64KB (65,536바이트)레지스터16개 × 8비트 (V0-VF)32개 × 32비트 (R0-R31)스택 크기16 레벨32 레벨명령어 크기2바이트4바이트주소 공간12비트 (4KB)24비트 (16MB)호환성원본 CHIP-8 완전 호환확장 기능 + 보안
🛠️ 개발 로드맵
✅ 완료된 기능

 8비트 CHIP-8 코어 구현
 32비트 확장 아키텍처
 SDL2 플랫폼 계층
 파일 확장자 기반 모드 선택
 기본 디버거 구현
 메인 루프 및 입력 처리

🚧 진행 중

 스택 프레임 관리 시스템
 메모리 보안 검사 opcode
 고급 디버거 기능 (메모리 뷰어, 레지스터 편집)

📅 예정된 기능

 버퍼 오버플로우 시뮬레이션
 Use-after-free 감지
 ROP 체인 분석
 메모리 누수 추적
 취약점 시나리오 라이브러리

🐞 문제 해결
일반적인 문제들
SDL2 초기화 실패
bash# 오류: SDL_Init failed
# 해결: SDL2 개발 라이브러리 설치
sudo apt install libsdl2-dev

# WSL2 환경에서 X11 연결 문제
export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0
빌드 오류
bash# CMake 버전 문제
cmake --version  # 3.10+ 확인

# 헤더 파일 누락
# include 디렉토리 구조 확인
ROM 로드 실패
bash# 파일 경로 확인
ls -la ../roms/

# 권한 문제
chmod +r ../roms/*.ch8
🧪 테스트 ROM 목록
8비트 클래식 ROM

maze.ch8 - 미로 생성기
pong.ch8 - 퐁 게임
tetris.ch8 - 테트리스
space_invaders.ch8 - 스페이스 인베이더
breakout.ch8 - 벽돌깨기

32비트 확장 ROM (개발 예정)

stack_demo.ch32 - 스택 프레임 데모
security_test.ch32 - 보안 기능 테스트
buffer_overflow.ch32 - 버퍼 오버플로우 시나리오

🤝 기여하기
이 프로젝트는 교육 목적으로 개발되고 있습니다. 기여를 환영합니다!
개발 환경 설정

프로젝트 포크
새 브랜치 생성: git checkout -b feature/새기능
변경사항 커밋: git commit -m "새 기능 추가"
브랜치 푸시: git push origin feature/새기능
Pull Request 생성

코딩 스타일

C++17 표준 사용
헤더 가드 대신 #pragma once 사용
명확한 변수명과 주석
RAII 원칙 준수

📜 라이선스
이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 LICENSE 파일을 참조하세요.
🙏 감사의 말

SDL2 라이브러리 개발팀
원본 CHIP-8 아키텍처 설계자들
오픈소스 에뮬레이터 커뮤니티


⚠️ 주의사항: 이 에뮬레이터는 교육 목적으로 개발되었습니다. 실제 보안 취약점 연구나 악의적인 목적으로 사용하지 마세요.



























































































































































































