# CHIP-8 Extended Emulator (EmulPartyAdvance : E.P.A) 문서

E.P.A의 상세 문서 모음입니다. 이 디렉토리는 프로젝트의 핵심 기능과 아키텍처에 대한 심층적인 설명을 제공합니다.

## 문서 목록

### 설치 및 시작

- [**installation.md**](https://claude.ai/chat/installation.md) - 설치 가이드 및 빌드 방법
    - 의존성 설치 (SDL2, CMake 등)
    - 단계별 빌드 과정
    - 문제 해결 및 트러블슈팅

### 핵심 기능 문서

- [**stack-frames.md**](https://claude.ai/chat/stack-frames.md) - x86 스타일 스택 프레임 시스템
    - EBP/ESP/EIP 레지스터 상세 설명
    - 스택 프레임 생성 및 해제 과정
    - 함수 호출 규약 및 실제 사용 예제
    - 스택 시각화 디버깅 기능
- [**syscall.md**](https://claude.ai/chat/syscall.md) - BootROM 아키텍처 및 SYSCALL 인터페이스
    - BootROM 시스템 동작 원리
    - 자동 모드 감지 및 전환 메커니즘
    - SYSCALL 인터페이스 실제 구현 및 사용법
    - I/O 리다이렉션 시스템
- [**instruction-reference.md**](https://claude.ai/chat/instruction-reference.md) - 8비트에서 32비트로의 명령어 진화
    - 기존 CHIP-8 명령어 세트 분석
    - 32비트 확장 명령어 상세 설명
    - 명령어 크기 및 형식 변화 (2바이트 → 4바이트)
    - 레지스터 확장 (V0-VF → R0-R31)
    - 실제 코드 비교 및 마이그레이션 가이드

## 빠른 시작

1. **처음 사용자라면**: [installation.md](https://claude.ai/chat/installation.md)부터 시작하세요
2. **핵심 기능이 궁금하다면**: [stack-frames.md](https://claude.ai/chat/stack-frames.md)와 [syscall.md](https://claude.ai/chat/syscall.md)를 확인하세요
3. **개발자라면**: [instruction-reference.md](https://claude.ai/chat/instruction-reference.md)에서 명령어 세트를 학습하세요

## 문서 구성 원칙

각 문서는 다음 구조를 따릅니다:

- **개요**: 해당 기능의 목적과 배경
- **상세 설명**: 기술적 구현 내용
- **실제 예제**: 코드와 함께하는 사용법
- **참고 자료**: 관련 링크 및 추가 정보

## 기여 방법

문서 개선에 기여하고 싶다면:

1. 오타나 부정확한 내용 발견 시 이슈 등록
2. 새로운 예제나 설명 추가 제안
3. 번역이나 다국어 지원 기여

## 프로젝트 개요

**CHIP-8 Extended Emulator**는 클래식 8비트 CHIP-8과 혁신적인 32비트 확장을 모두 지원하는 현대적인 에뮬레이터입니다:

- **듀얼 아키텍처**: 8비트 호환성 + 32비트 확장
- **BootROM 시스템**: 지능적 모드 감지 및 전환
- **전문적인 스택 관리**: x86-64 스타일 스택 프레임
- **고급 디버깅**: 시각적 스택 프레임 및 단계별 실행

---

더 자세한 정보는 개별 문서를 참조하거나 [메인 README](https://claude.ai/README.md)를 확인하세요.
