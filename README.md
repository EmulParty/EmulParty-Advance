# 🕹️ CHIP-8 Emulator (C++ & SDL2)

**CHIP-8**은 1970년대에 설계된 간단한 가상 머신 시스템으로, 고전 게임을 실행하기 위한 인터프리터입니다.  
이 프로젝트는 CHIP-8 명세를 기반으로 한 **에뮬레이터**를 C++와 SDL2를 사용하여 구현한 것입니다.

---

## 📁 프로젝트 구조

```
chip8-emulator/
├── include/
│   ├── core/             # Chip8 클래스 및 명령어 해석기 헤더
│   └── platform/         # SDL2 플랫폼 추상화
├── src/
│   ├── core/             # Chip8 로직, opcode_table 등
│   └── platform/         # SDL2 렌더링 및 입력 처리 구현
├── test/                 # Catch2 테스트 코드
├── roms/                 # 테스트용 ROM (예: pong.ch8, Brick.ch8)
├── CMakeLists.txt        # CMake 빌드 설정
└── README.md             # 프로젝트 설명 문서
```

---

## 🚀 실행 방법

### 1. SDL2 설치 (DBus, 오디오 등 비활성화 빌드)

```bash
cd ~/chip8-emulator
git clone https://github.com/libsdl-org/SDL.git
cd SDL
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local -DSDL_DLOPEN=OFF -DSDL_DBUS=OFF -DSDL_PULSEAUDIO=OFF -DSDL_ALSA=OFF
make -j$(nproc)
sudo make install
```

### 2. 프로젝트 빌드

```bash
cd ~/chip8-emulator
mkdir build && cd build
cmake ..
make
```

### 3. 실행

```bash
./chip8 ../roms/pong.ch8
./chip8 ../roms/Brick.ch8
```

---

## ⚙️ 구현 개요

### ✅ 1. CHIP-8 메모리 구조

| 주소 범위        | 설명                          |
|------------------|-------------------------------|
| 0x000 ~ 0x1FF     | 인터프리터 / 폰트셋 저장 공간       |
| 0x200 ~           | 프로그램(ROM)이 로드되는 위치      |
| 0xEA0 ~ 0xEFF     | 스택 영역 (서브루틴 복귀용)         |
| 0xF00 ~ 0xFFF     | 디스플레이 메모리 / 키 입력 상태 저장 |

- 전체 4KB 메모리 (`std::array<uint8_t, 4096>`)
- ROM은 0x200 주소부터 로드됨

### ✅ 2. 명령어 처리 구조 (FDE + 구조 분리)

- 명령어는 2바이트 단위로 구성
- `Fetch → Decode → Execute` 구조의 `cycle()` 함수 구현
- **초기에는 `chip8.cpp`에서 명령어를 처리했지만**, 유지보수와 확장성을 고려해 **명령어 해석 로직을 `opcode_table.cpp`로 분리**
- `cycle()` 함수는 오직 Fetch + Decode만 수행하고, 실제 명령어 실행은 opcode_table이 담당

```cpp
void Chip8::cycle() {
    opcode = (memory[pc] << 8) | memory[pc + 1];  // Fetch
    OpcodeTable::Execute(*this, opcode);          // Decode + Execute
}
```

#### 📌 PC, SP 증감 방식

- `pc`(Program Counter)와 `sp`(Stack Pointer)는 **명령어 처리 함수 내부에서 직접 제어**
  - 예: `chip8.set_pc(chip8.get_pc() + 2);` 또는 `chip8.set_sp(chip8.get_sp() - 1);`
  - 명령어에 따라 `pc`를 건너뛰거나 점프 / `sp`를 push/pop 형태로 조작

```cpp
void OP_2NNN(Chip8& chip8, uint16_t opcode) {
    chip8.stack_at(chip8.get_sp()) = chip8.get_pc();  // 현재 주소 push
    chip8.set_sp(chip8.get_sp() + 1);
    chip8.set_pc(opcode & 0x0FFF);                    // 점프
}
```

### ✅ 3. opcode 함수 포인터 테이블 구조

- 상위 4비트를 인덱스로 사용한 `primary_table`을 통해 각 명령어 처리 함수에 연결

```cpp
primary_table[0x1] = OP_1NNN;
primary_table[0x2] = OP_2NNN;
```

- opcode_table.cpp에서 함수 포인터 기반 테이블 방식으로 구현 완료
- switch-case보다 구조적이고 유지보수에 용이

### ✅ 4. SDL2 기반 그래픽 렌더링

- 화면 해상도: `64 x 32` 픽셀, 확대 배율: `10x`
- 화면 출력은 `draw_flag`를 기준으로 렌더링 시점 제어
- 텍스처 업데이트 방식으로 SDL 화면에 비디오 메모리 출력

#### 키 매핑

| CHIP-8 Key | PC Key |
|------------|--------|
| 1 2 3 C    | 1 2 3 4 |
| 4 5 6 D    | Q W E R |
| 7 8 9 E    | A S D F |
| A 0 B F    | Z X C V |

---

## ✅ 구현된 명령어

> CHIP-8 명세에 따라 **전 opcode 35개 완전 구현 완료**

| 명령어 종류 |
|-------------|
| 00E0, 00EE, 1NNN, 2NNN, 3XNN, 4XNN, 5XY0, 6XNN, 7XNN |
| 8XY0~8XYE (논리연산, 덧셈, 쉬프트, 비교 등) |
| 9XY0, ANNN, BNNN, CXNN, DXYN (스프라이트 그리기) |
| EX9E, EXA1 (키 입력 분기) |
| FX07, FX0A, FX15, FX18, FX1E, FX29, FX33, FX55, FX65 |

---

## ✅ 실행 확인된 ROM

| ROM 이름 | 실행 결과 |
|----------|-----------|
| `pong.ch8` | 정상 실행. 키 입력 처리 및 게임 진행 가능 |
| `Brick.ch8` | 정상 실행. 벽돌 깨기 게임 작동 |

---

## 🐞 Known Issues

| 문제                           | 설명                                                                 |
|--------------------------------|----------------------------------------------------------------------|
| ⚠️ 그래픽 깜빡거림 (flickering) | SDL 렌더링 타이밍 또는 `draw_flag` 처리 최적화 부족으로 일부 ROM에서 화면이 과하게 깜빡임 발생 가능 |
| ✅ 빨간 배경 문제 해결          | 이전 알파 채널 오류는 SDL 포맷 수정으로 해결 완료                         |
| 🟢 명령어 처리 누락 없음         | 모든 명령어 완전 구현 완료됨                                          |

---

## 🔬 테스트 코드

`test/test_chip8.cpp`에 Catch2를 이용한 유닛 테스트 구현 완료

```cpp
TEST_CASE("6XNN: Set Vx", "[opcode]") {
    Chip8 chip8;
    chip8.memory[0x200] = 0x60;
    chip8.memory[0x201] = 0x0A;
    chip8.pc = 0x200;
    chip8.cycle();
    REQUIRE(chip8.V[0] == 0x0A);
}
```

---

## 📚 참고 자료

- 📘 [Cowgod's CHIP-8 Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
- 📗 [Austin Morlan's CHIP-8 Guide](https://austinmorlan.com/posts/chip8_emulator/)
- 📁 [CHIP-8 ROM 테스트 모음](https://github.com/dmatlack/chip8/tree/master/roms)

---

> 개발자: [@daayuun](https://github.com/daayuun)  
> Organization: [EmulParty](https://github.com/EmulParty)