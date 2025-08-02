# BootROM 아키텍처 및 SYSCALL 인터페이스

E.P.A에서 추가한 기능인 BootROM 기반 아키텍처와 SYSCALL 시스템에 대해 설명합니다. 이 시스템은 8비트와 32비트 모드를 통합하고, 고급 I/O 리다이렉션을 제공합니다.

## 개요

기존 CHIP-8 에뮬레이터들은 ROM 파일을 직접 로드하는 방식이었지만, 본 에뮬레이터는 현대적인 운영체제처럼 **BootROM**으로 시작하여 사용자가 원하는 ROM을 선택하고 적절한 모드로 자동 전환하는 혁신적인 아키텍처를 채택했습니다.

## BootROM 아키텍처

### 부팅 과정

```
에뮬레이터 시작
       ↓
┌─────────────────┐
│   32비트 모드   │ ← 항상 여기서 시작
│   BootROM 로드  │
└─────────────────┘
       ↓
┌─────────────────┐
│  파일 선택 UI   │ ← 사용자 인터페이스
│  (SDL 기반)     │
└─────────────────┘
       ↓
┌─────────────────┐
│  확장자 분석     │ ← .ch8/.c8 vs .ch32/.c32
│  자동 모드 감지  │
└─────────────────┘
       ↓
    ┌─────────┐       ┌──────────┐
    │ 8비트   │  또는  │ 32비트   │
    │ 모드    │       │ 모드     │
    │ 전환    │       │ 유지     │
    └─────────┘       └──────────┘
```

### BootROM 메모리 구조

```cpp
// BootROM은 0x0000에서 시작하여 시스템을 초기화
constexpr uint32_t BOOT_ROM_START = 0x0000;

// src/boot/boot_rom_data.cpp - 실제 BootROM 바이너리
static const uint8_t boot_rom_binary[] = {
    // 시스템 초기화 코드
    0xA0, 0x00, 0x00, 0x10,  // SYSCALL 0x0010 (파일 선택 UI)
    0xA0, 0x00, 0x00, 0x20,  // SYSCALL 0x0020 (파일 로드)
    0xA0, 0x00, 0x00, 0x30,  // SYSCALL 0x0030 (모드 전환)
    // ... 더 많은 초기화 코드
};
```

### 자동 모드 감지 시스템

```cpp
// src/core/mode_selector.cpp
bool ModeSelector::load_and_switch_mode(Chip8_32& chip8_32, const std::string& filename) {
    std::string extension = get_file_extension(filename);
    std::string full_path = "../roms/" + filename;
    
    // 파일 확장자 기반 모드 결정
    if (extension == ".ch8" || extension == ".c8") {
        std::cout << "[BootROM] → 8비트 CHIP-8 모드로 전환" << std::endl;
        g_switched_to_8bit = true;  // 8비트 전환 플래그
        return true;
        
    } else if (extension == ".ch32" || extension == ".c32") {
        std::cout << "[BootROM] → 32비트 확장 모드 유지" << std::endl;
        return chip8_32.load_rom(full_path.c_str());
        
    } else {
        std::cout << "[BootROM] → 알 수 없는 확장자, 8비트 모드 시도" << std::endl;
        g_switched_to_8bit = true;
        return true;
    }
}
```

## SYSCALL 인터페이스

### SYSCALL 명령어 형식

```cpp
// 32비트 SYSCALL 명령어 형식
// 0xA0 XX YY ZZ
// A0: SYSCALL 식별자
// XXYYZZ: 16비트 시스템 호출 번호

void OP_SYSCALL(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t syscall_number = opcode & 0x0000FFFF;
    
    switch (syscall_number) {
        case 0x0010: syscall_file_select(chip8_32); break;
        case 0x0020: syscall_file_load(chip8_32); break;
        case 0x0030: syscall_mode_switch(chip8_32); break;
        case 0x0040: syscall_io_redirect(chip8_32); break;
        // ... 더 많은 시스템 호출
    }
}
```

### 주요 SYSCALL 구현

#### 1. 파일 선택 UI (SYSCALL 0x0010)

```cpp
void syscall_file_select(Chip8_32& chip8_32) {
    std::cout << "[SYSCALL 0x0010] 파일 선택 UI 활성화" << std::endl;
    
    // SDL 기반 파일 브라우저 표시
    auto console_io = chip8_32.get_console_io();
    if (console_io) {
        console_io->displayFileSelector("../roms/");
    }
    
    // 사용자 입력 대기
    std::string selected_file = console_io->getSelectedFile();
    
    // 선택된 파일명을 메모리에 저장
    uint32_t buffer_addr = chip8_32.get_R(1);  // R1에서 버퍼 주소 가져오기
    for (size_t i = 0; i < selected_file.length(); ++i) {
        chip8_32.set_memory(buffer_addr + i, selected_file[i]);
    }
    chip8_32.set_memory(buffer_addr + selected_file.length(), 0);  // NULL 종료
}
```

#### 2. 파일 로드 (SYSCALL 0x0020)

```cpp
void syscall_file_load(Chip8_32& chip8_32) {
    std::cout << "[SYSCALL 0x0020] 파일 로드 시스템 호출" << std::endl;
    
    // R1에서 파일명 주소 가져오기
    uint32_t filename_addr = chip8_32.get_R(1);
    std::string filename = read_string_from_memory(chip8_32, filename_addr);
    
    // ModeSelector를 통한 파일 로드 및 모드 감지
    bool success = ModeSelector::load_and_switch_mode(chip8_32, filename);
    
    // 결과를 R0에 저장 (0: 실패, 1: 성공)
    chip8_32.set_R(0, success ? 1 : 0);
    
    if (success) {
        std::cout << "[BootROM] 파일 '" << filename << "' 로드 성공" << std::endl;
    } else {
        std::cerr << "[BootROM] 파일 '" << filename << "' 로드 실패" << std::endl;
    }
}
```

#### 3. 모드 전환 (SYSCALL 0x0030)

```cpp
void syscall_mode_switch(Chip8_32& chip8_32) {
    std::cout << "[SYSCALL 0x0030] 모드 전환 시스템 호출" << std::endl;
    
    // R1에서 모드 정보 가져오기 (0: 8비트, 1: 32비트)
    uint32_t target_mode = chip8_32.get_R(1);
    
    if (target_mode == 0) {
        // 8비트 모드로 전환 요청
        g_switched_to_8bit = true;
        std::cout << "[BootROM] 8비트 모드 전환 예약" << std::endl;
        
    } else if (target_mode == 1) {
        // 32비트 모드 유지
        std::cout << "[BootROM] 32비트 모드 유지" << std::endl;
        
    } else {
        std::cerr << "[BootROM] 알 수 없는 모드: " << target_mode << std::endl;
        chip8_32.set_R(0, 0);  // 실패
        return;
    }
    
    chip8_32.set_R(0, 1);  // 성공
}
```

## I/O 리다이렉션 시스템

### I/O 장치 관리자

```cpp
// include/syscall/io_manager.hpp
class IOManager {
private:
    std::map<int, std::shared_ptr<IODevice>> devices_;
    
public:
    // 파일 디스크립터에 I/O 장치 등록
    void registerDevice(int fd, std::shared_ptr<IODevice> device);
    
    // 파일 디스크립터를 통한 읽기/쓰기
    ssize_t read(int fd, void* buffer, size_t count);
    ssize_t write(int fd, const void* buffer, size_t count);
};
```

### SDL 콘솔 I/O 구현

```cpp
// src/syscall/sdl_console_io.cpp
class SDLConsoleIO : public IODevice {
private:
    Platform* platform_;
    std::queue<char> input_buffer_;
    std::string output_buffer_;
    
public:
    // 표준 입력 구현
    ssize_t read(void* buffer, size_t count) override {
        if (input_buffer_.empty()) {
            // SDL 이벤트에서 키보드 입력 처리
            pollKeyboardInput();
        }
        
        size_t bytes_read = 0;
        char* char_buffer = static_cast<char*>(buffer);
        
        while (bytes_read < count && !input_buffer_.empty()) {
            char_buffer[bytes_read++] = input_buffer_.front();
            input_buffer_.pop();
        }
        
        return bytes_read;
    }
    
    // 표준 출력 구현
    ssize_t write(const void* buffer, size_t count) override {
        const char* char_buffer = static_cast<const char*>(buffer);
        output_buffer_.append(char_buffer, count);
        
        // SDL 텍스트 렌더링으로 화면에 출력
        if (platform_) {
            platform_->RenderText(output_buffer_);
        }
        
        return count;
    }
};
```

### I/O SYSCALL 구현

#### 읽기 (SYSCALL 0x0040)

```cpp
void syscall_read(Chip8_32& chip8_32) {
    int fd = chip8_32.get_R(1);           // 파일 디스크립터
    uint32_t buffer_addr = chip8_32.get_R(2);  // 버퍼 주소
    size_t count = chip8_32.get_R(3);     // 읽을 바이트 수
    
    IOManager& io_manager = chip8_32.get_io_manager();
    
    // 임시 버퍼에 읽기
    std::vector<char> temp_buffer(count);
    ssize_t bytes_read = io_manager.read(fd, temp_buffer.data(), count);
    
    // 메모리에 복사
    for (ssize_t i = 0; i < bytes_read; ++i) {
        chip8_32.set_memory(buffer_addr + i, temp_buffer[i]);
    }
    
    // 실제 읽은 바이트 수를 R0에 반환
    chip8_32.set_R(0, bytes_read);
}
```

#### 쓰기 (SYSCALL 0x0041)

```cpp
void syscall_write(Chip8_32& chip8_32) {
    int fd = chip8_32.get_R(1);           // 파일 디스크립터
    uint32_t buffer_addr = chip8_32.get_R(2);  // 버퍼 주소
    size_t count = chip8_32.get_R(3);     // 쓸 바이트 수
    
    IOManager& io_manager = chip8_32.get_io_manager();
    
    // 메모리에서 임시 버퍼로 복사
    std::vector<char> temp_buffer(count);
    for (size_t i = 0; i < count; ++i) {
        temp_buffer[i] = chip8_32.get_memory(buffer_addr + i);
    }
    
    // I/O 장치에 쓰기
    ssize_t bytes_written = io_manager.write(fd, temp_buffer.data(), count);
    
    // 실제 쓴 바이트 수를 R0에 반환
    chip8_32.set_R(0, bytes_written);
}
```

## 실제 사용 예제

### BootROM의 파일 선택 시나리오

```assembly
; BootROM 의사 코드
main_boot:
    ; 시스템 초기화
    MOV R28, 0xEFFF     ; EBP 초기화
    MOV R29, 0xEFFF     ; ESP 초기화
    
    ; 파일명 버퍼 준비
    SUB RSP, 256        ; 256바이트 문자열 버퍼
    MOV R1, RSP         ; 버퍼 주소를 R1에
    
    ; 파일 선택 UI 호출
    SYSCALL 0x0010      ; 파일 선택 대화상자
    
    ; 선택된 파일 로드
    SYSCALL 0x0020      ; 파일 로드 및 모드 감지
    CMP R0, 0
    JEQ load_failed
    
    ; 성공 시 타겟 모드로 전환
    SYSCALL 0x0030      ; 모드 전환
    JMP boot_complete
    
load_failed:
    ; 에러 메시지 출력
    MOV R1, 1           ; stdout
    MOV R2, error_msg   ; 에러 메시지 주소
    MOV R3, 20          ; 메시지 길이
    SYSCALL 0x0041      ; write 시스템 호출
    JMP main_boot       ; 다시 시도
    
boot_complete:
    ; 8비트 모드 전환이 예약된 경우 여기서 종료
    ; 32비트 모드 계속인 경우 사용자 프로그램으로 점프
    RET_FUNC
```

### 계산기 프로그램에서의 I/O 사용

```assembly
; CALC.ch32 - 계산기 프로그램 예제
calculator_main:
    PUSH EBP
    MOV EBP, ESP
    SUB ESP, 64         ; 입력 버퍼 공간
    
input_loop:
    ; 프롬프트 출력
    MOV R1, 1           ; stdout
    MOV R2, prompt_msg  ; "계산식을 입력하세요: "
    MOV R3, 20
    SYSCALL 0x0041      ; write
    
    ; 사용자 입력 읽기
    MOV R1, 0           ; stdin
    MOV R2, [EBP-64]    ; 입력 버퍼
    MOV R3, 63          ; 최대 63문자
    SYSCALL 0x0040      ; read
    
    ; 입력 파싱 및 계산
    PUSH [EBP-64]       ; 입력 문자열
    CALL_FUNC parse_expression
    ADD ESP, 4
    
    ; 결과 출력
    PUSH R0             ; 계산 결과
    CALL_FUNC print_result
    ADD ESP, 4
    
    JMP input_loop
```

### 8비트 모드 전환 과정

```cpp
// src/core/mode_selector.cpp
int ModeSelector::run_8bit_mode_after_bootrom(Platform& platform) {
    std::cout << "\n=== 8비트 CHIP-8 모드로 전환 ===" << std::endl;
    
    // 8비트 시스템 초기화
    OpcodeTable::Initialize();  // 8비트 명령어 테이블
    Chip8 chip8;                // 8비트 에뮬레이터 인스턴스
    
    // 전역 변수에 저장된 ROM 데이터 로드
    if (!g_rom_data.empty()) {
        std::cout << "[8비트] ROM 데이터 로드: " << g_rom_data.size() << " 바이트" << std::endl;
        
        // 0x200부터 8비트 메모리에 로드
        for (size_t i = 0; i < g_rom_data.size() && i < 4096 - 0x200; ++i) {
            chip8.set_memory(0x200 + i, g_rom_data[i]);
        }
        
        chip8.set_pc(0x200);  // 8비트 PC 설정
        std::cout << "[8비트] '" << g_loaded_filename << "' 로드 완료" << std::endl;
    }
    
    // 8비트 실행 루프 시작
    bool quit = false;
    while (!quit) {
        quit = platform.ProcessInput(chip8.keypad);
        chip8.cycle();  // 8비트 명령어 실행
        
        if (chip8.needs_redraw()) {
            platform.Update(chip8.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8.clear_draw_flag();
        }
    }
    
    return 0;
}
```

## BootROM의 장점

### 1. 통합된 사용자 경험
- **단일 실행 파일**: 사용자는 `./chip8_dual`만 실행
- **자동 모드 감지**: 파일 확장자로 8비트/32비트 자동 결정
- **에러 처리**: 파일 로드 실패 시 다시 시도 가능

### 2. 확장성
- **새로운 SYSCALL 추가**: 번호만 추가하면 새 기능 구현 가능
- **I/O 장치 확장**: 네트워크, 파일 시스템 등 추가 가능
- **디버깅 지원**: SYSCALL을 통한 디버깅 정보 제공

### 3. 모듈화
- **플랫폼 독립성**: SDL 기반 I/O 추상화
- **코드 분리**: BootROM, 8비트, 32비트 코드 완전 분리
- **테스트 용이성**: 각 모드별 독립적 테스트 가능

## 디버깅 SYSCALL

### SYSCALL 추적

디버그 모드에서는 모든 SYSCALL을 추적할 수 있습니다:

```cpp
void OP_SYSCALL(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t syscall_number = opcode & 0x0000FFFF;
    
    if (debugger32.isEnabled()) {
        std::cout << "[SYSCALL DEBUG] 0x" << std::hex << syscall_number 
                  << " 호출됨" << std::dec << std::endl;
        std::cout << "  R0=" << chip8_32.get_R(0) << std::endl;
        std::cout << "  R1=" << chip8_32.get_R(1) << std::endl;
        std::cout << "  R2=" << chip8_32.get_R(2) << std::endl;
        std::cout << "  R3=" << chip8_32.get_R(3) << std::endl;
    }
    
    // 실제 SYSCALL 실행...
}
```

### I/O 상태 확인

```bash
# 디버그 모드에서 I/O 상태 확인
(debug) io
등록된 I/O 장치:
  FD 0 (stdin):  SDLConsoleIO
  FD 1 (stdout): SDLConsoleIO  
  FD 2 (stderr): SDLConsoleIO

# SYSCALL 히스토리
(debug) syscall_history
최근 SYSCALL 호출:
  1. 0x0010 (파일 선택) - 성공
  2. 0x0020 (파일 로드) - 성공: pong.ch8
  3. 0x0030 (모드 전환) - 8비트 모드로 전환 예약
```

## 성능 고려사항

### SYSCALL 오버헤드

```cpp
// 빠른 SYSCALL을 위한 함수 포인터 테이블
typedef void (*syscall_handler_t)(Chip8_32&);

static syscall_handler_t syscall_table[] = {
    [0x0010] = syscall_file_select,
    [0x0020] = syscall_file_load,
    [0x0030] = syscall_mode_switch,
    [0x0040] = syscall_read,
    [0x0041] = syscall_write,
    // ...
};

void OP_SYSCALL(Chip8_32& chip8_32, uint32_t opcode) {
    uint16_t syscall_number = opcode & 0x0000FFFF;
    
    if (syscall_number < sizeof(syscall_table) / sizeof(syscall_handler_t)) {
        if (syscall_table[syscall_number]) {
            syscall_table[syscall_number](chip8_32);  // 빠른 호출
            return;
        }
    }
    
    // 알 수 없는 SYSCALL
    std::cerr << "알 수 없는 SYSCALL: 0x" << std::hex << syscall_number << std::endl;
}
```

### 메모리 효율성

```cpp
// 메모리 풀을 사용한 I/O 버퍼 관리
class IOBufferPool {
private:
    std::vector<std::unique_ptr<char[]>> free_buffers_;
    std::mutex pool_mutex_;
    
public:
    char* allocate(size_t size) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        
        if (!free_buffers_.empty()) {
            auto buffer = std::move(free_buffers_.back());
            free_buffers_.pop_back();
            return buffer.release();
        }
        
        return new char[size];
    }
    
    void deallocate(char* buffer) {
        std::lock_guard<std::mutex> lock(pool_mutex_);
        free_buffers_.emplace_back(buffer);
    }
};
```

## 향후 확장 계획

### 네트워크 SYSCALL

```cpp
// 계획 중인 네트워크 지원
void syscall_socket_create(Chip8_32& chip8_32);    // 0x0100
void syscall_socket_connect(Chip8_32& chip8_32);   // 0x0101
void syscall_socket_send(Chip8_32& chip8_32);      // 0x0102
void syscall_socket_recv(Chip8_32& chip8_32);      // 0x0103
```

### 파일 시스템 SYSCALL

```cpp
// 계획 중인 파일 시스템 지원
void syscall_file_open(Chip8_32& chip8_32);        // 0x0200
void syscall_file_close(Chip8_32& chip8_32);       // 0x0201
void syscall_file_seek(Chip8_32& chip8_32);        // 0x0202
void syscall_directory_list(Chip8_32& chip8_32);   // 0x0203
```

## 결론

BootROM 아키텍처와 SYSCALL 인터페이스는 CHIP-8 Extended Emulator를 단순한 게임 실행기에서 현대적인 시스템으로 발전시킨 핵심 기술입니다. 이를 통해:

- **사용자 편의성**: 직관적인 파일 선택과 자동 모드 전환
- **확장성**: 새로운 기능을 SYSCALL로 쉽게 추가
- **호환성**: 8비트와 32비트 모드의 완벽한 통합
- **현대성**: Unix 스타일 I/O 추상화 제공

## 다음 단계

BootROM과 SYSCALL 시스템을 이해했다면:

1. **[명령어 레퍼런스](instruction-reference.md)** - 전체 32비트 명령어 세트 학습
2. **실제 예제 분석** - `roms/CALC*.ch32`에서 SYSCALL 사용법 확인
3. **디버깅 실습** - `--debug` 모드에서 SYSCALL 추적해보기

## 참고 자료

- Unix System V Interface Definition
- Linux System Call Interface
- SDL2 Event Handling Documentation
- 프로젝트 소스: `src/boot/`, `src/syscall/`, `src/core/mode_selector.cpp`
