// mode_selector.cpp - 완전 통합 BootROM 아키텍처 (깔끔한 최종 버전)
#include "mode_selector.hpp"
#include "chip8.hpp"
#include "chip8_32.hpp"
#include "opcode_table.hpp"
#include "opcode_table_32.hpp"
#include "platform.hpp"
#include "timer.hpp"
#include "debugger/debugger.hpp"
#include "sdl_console_io.hpp"
#include "security/path_jail.hpp"
#include "security/audit_log.hpp"
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

static bool g_debug_mode = false;
static bool g_switched_to_8bit = false;  // 8비트 모드 전환 플래그
static std::vector<uint8_t> g_rom_data;  // ROM 데이터 저장용 전역 변수
static std::string g_loaded_filename;    // 로드된 파일명 저장
static std::string g_startup_rom;        // CLI에서 주입할 ROM 파일명
static int g_max_frames = -1;            // headless/CI 실행 종료 조건

// 게스트 ROM 파일 크기 상한.
// 32비트 모드 가용 영역(0x0200~0x7FFF, 약 32KB)을 넘는 ROM은 의미가 없고,
// 무제한 resize()는 호스트 메모리 고갈(DoS) 벡터가 된다.
// 보수적으로 64KB로 자른다 — 32KB 코드 + 여유.
constexpr std::streamsize kMaxRomBytes = 64 * 1024;

// jail 루트 후보: 현재 작업 디렉터리 기준 roms/ 또는 ../roms/.
// (build/ 안에서 실행되면 ../roms/, 프로젝트 루트에서 실행되면 roms/.)
static std::filesystem::path pick_jail_root() {
    namespace fs = std::filesystem;
    fs::path c1 = fs::current_path() / "roms";
    fs::path c2 = fs::current_path().parent_path() / "roms";
    std::error_code ec;
    if (fs::is_directory(c1, ec)) return c1;
    if (fs::is_directory(c2, ec)) return c2;
    return c1;  // 둘 다 없으면 c1 — PathJail이 RootMissing으로 거부.
}

void ModeSelector::set_debug_mode(bool enable) {
    g_debug_mode = enable;
}

void ModeSelector::set_startup_rom(const std::string& filename) {
    g_startup_rom = filename;
}

void ModeSelector::set_max_frames(int max_frames) {
    g_max_frames = max_frames;
}

// 핵심: 항상 32비트 BootROM으로 시작
int ModeSelector::select_and_run() {
    std::cout << "[INFO] Starting Unified BootROM Architecture\n";
    std::cout << "[INFO] BootROM will handle file selection and mode switching\n";

    // 전역 변수 초기화
    g_switched_to_8bit = false;
    g_rom_data.clear();
    g_loaded_filename.clear();

    // 보안 서브시스템 초기화:
    //   - PathJail 루트를 roms/로 봉쇄
    //   - 감사 로그 파일 오픈 (실행 디렉터리 기준 epa_audit.log)
    auto jail_root = pick_jail_root();
    security::PathJail::set_root(jail_root);
    security::AuditLog::init("epa_audit.log");
    security::AuditLog::info("vmm_start",
        { {"jail_root", jail_root.string()},
          {"max_rom_bytes", static_cast<long long>(kMaxRomBytes)} });

    return run_unified_bootrom_mode();
}

int ModeSelector::run_unified_bootrom_mode() {
    std::cout << "\n=== Unified BootROM-Driven CHIP-8 System ===" << std::endl;
    
    OpcodeTable_32::Initialize();
    Chip8_32 chip8_32;
    
    chip8emu::Debugger32 debugger32(chip8_32);
    if (g_debug_mode) {
        debugger32.enable(true);
        debugger32.setStepMode(true);
        std::cout << "Debug mode enabled\n";
    }
    
    Platform platform("NeoCHIP-8 Unified System", 
                      VIDEO_WIDTH * SCALE, VIDEO_HEIGHT * SCALE,
                      VIDEO_WIDTH, VIDEO_HEIGHT);
    
    if (!platform.Initialize()) {
        std::cerr << "[ERROR] Platform initialization failed!" << std::endl;
        return 1;
    }

    chip8_32.setPlatform(&platform);
    if (!g_startup_rom.empty()) {
        auto console_io = chip8_32.get_console_io();
        if (console_io) {
            console_io->setPendingInput(g_startup_rom);
            std::cout << "[INFO] Startup ROM queued: " << g_startup_rom << std::endl;
        }
    }
    platform.SwitchToGameMode();

    std::cout << "[INFO] BootROM System Active" << std::endl;
    
    bool quit = false;
    
    while (!quit) {
        uint32_t frame_start = timer::get_ticks();
        
        // 핵심 수정: 실제 키패드 전달
        quit = platform.ProcessInput(chip8_32.keypad);
        
        if (debugger32.isEnabled()) {
            uint32_t current_opcode = chip8_32.getCurrentOpcode();
            debugger32.printState(current_opcode);
            
            if (!debugger32.isEnabled()) {
                break;
            }
        }
        
        static int frame_counter_32 = 0;
        if (frame_counter_32 % 10 == 0) {  // 60fps 기준 6cycle/sec
            chip8_32.cycle();
        }
        frame_counter_32++;

        if (g_max_frames >= 0 && frame_counter_32 >= g_max_frames) {
            std::cout << "[INFO] Max frame limit reached; exiting 32-bit loop" << std::endl;
            break;
        }

        // 게스트 폴트 처리: VMM은 죽지 않고 메인 루프가 게스트만 정지시킨다.
        if (chip8_32.is_halted()) {
            std::cout << "[VMM] 32-bit guest halted ("
                      << chip8_32.halt_reason() << ") — exiting loop" << std::endl;
            break;
        }

        if (g_switched_to_8bit) {
            std::cout << "[MODE SWITCH] Transitioning from 32-bit BootROM to 8-bit CHIP-8..." << std::endl;
            quit = true;
            break;
        }
        
        // 타이머 업데이트
        static uint32_t last_timer_update = 0;
        uint32_t current_time = timer::get_ticks();
        if (current_time - last_timer_update >= 16) {
            if (chip8_32.delay_timer > 0) chip8_32.delay_timer--;
            if (chip8_32.sound_timer > 0) chip8_32.sound_timer--;
            last_timer_update = current_time;
        }
        
        // needs_redraw() 체크 없이 매 프레임마다 업데이트
        platform.Update(chip8_32.video, VIDEO_WIDTH * sizeof(uint32_t));
        if (chip8_32.needs_redraw()) {
            chip8_32.clear_draw_flag();
        }
    
        
        // 60Hz 속도 제어
        uint32_t frame_end = timer::get_ticks();
        uint32_t elapsed = frame_end - frame_start;
        if (elapsed < 16) {
            timer::delay(16 - elapsed);
        }
    }
    
    platform.SwitchToGameMode();
    
    if (g_switched_to_8bit) {
        return run_8bit_mode_after_bootrom(platform);
    }
    
    std::cout << "[INFO] BootROM system terminated" << std::endl;
    return 0;
}

int ModeSelector::run_8bit_mode_after_bootrom(Platform& platform) {
    std::cout << "\n=== Switching to 8-bit CHIP-8 Mode ===" << std::endl;
    
    OpcodeTable::Initialize();
    Chip8 chip8;
    
    chip8emu::Debugger8 debugger8(chip8);
    if (g_debug_mode) {
        debugger8.enable(true);
        debugger8.setStepMode(true);
        std::cout << "Debug mode enabled for 8-bit CHIP-8\n";
    }
    
    // ROM 데이터 로드
    if (!g_rom_data.empty()) {
        std::cout << "[8-bit] Loading ROM data (" << g_rom_data.size() << " bytes)" << std::endl;
        
        for (size_t i = 0; i < g_rom_data.size() && i < 4096 - 0x200; ++i) {
            chip8.set_memory(0x200 + i, g_rom_data[i]);
        }
        
        chip8.set_pc(0x200);
        
        std::cout << "[8-bit] ROM \"" << g_loaded_filename << "\" loaded successfully" << std::endl;
    } else {
        std::cerr << "[8-bit] ERROR: No ROM data available!" << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] 8-bit CHIP-8 System Ready" << std::endl;
    
    bool quit = false;
    
    while (!quit) {
        uint32_t frame_start = timer::get_ticks();
        
        // 핵심 수정: 실제 키패드 전달
        quit = platform.ProcessInput(chip8.keypad);
        
        if (debugger8.isEnabled()) {
            uint32_t current_opcode = chip8.getCurrentOpcode();
            debugger8.printState(current_opcode);
            
            if (!debugger8.isEnabled()) {
                break;
            }
        }
        
        static int frame_counter_8 = 0;
        if (frame_counter_8 % 10 == 0) {
            chip8.cycle();
        }
        frame_counter_8++;

        if (g_max_frames >= 0 && frame_counter_8 >= g_max_frames) {
            std::cout << "[INFO] Max frame limit reached; exiting 8-bit loop" << std::endl;
            break;
        }
        
        // 타이머 업데이트
        static uint32_t last_timer_update = 0;
        uint32_t current_time = timer::get_ticks();
        if (current_time - last_timer_update >= 16) {
            if (chip8.delay_timer > 0) chip8.delay_timer--;
            if (chip8.sound_timer > 0) chip8.sound_timer--;
            last_timer_update = current_time;
        }
        
        if (chip8.needs_redraw()) {
            platform.Update(chip8.video, VIDEO_WIDTH * sizeof(uint32_t));
            chip8.clear_draw_flag();
        }
        
        // 60Hz 속도 제어
        uint32_t frame_end = timer::get_ticks();
        uint32_t elapsed = frame_end - frame_start;
        if (elapsed < 16) {
            timer::delay(16 - elapsed);
        }
    }
    
    std::cout << "[INFO] 8-bit CHIP-8 emulator terminated" << std::endl;
    return 0;
}


// **SYSCALL에서 호출할 모드 전환 함수**
//
// 보안 흐름:
//   1) PathJail로 게스트 입력 파일명을 jail 내부 절대경로로 봉쇄.
//      탈출 시도(`..`, 절대경로, symlink 경유)는 여기서 거부 + 감사 로그.
//   2) ROM 파일 크기 상한(kMaxRomBytes) 검사 — 호스트 메모리 고갈 방지.
//   3) 통과한 경로만 ifstream으로 읽어 g_rom_data에 적재.
bool ModeSelector::load_and_switch_mode(Chip8_32& chip8_32, const std::string& filename) {
    std::string extension = get_file_extension(filename);

    // ── 벡터 1 차단: FS Jail ────────────────────────────────────────────────
    auto jail = security::PathJail::resolve(filename);
    if (jail.error != security::PathJailError::Ok) {
        security::AuditLog::record("path_jail", security::Decision::Deny,
            { {"input",  filename},
              {"reason", security::to_string(jail.error)},
              {"detail", jail.reason},
              {"caller", "load_and_switch_mode"} });
        std::cerr << "[BootROM] path_jail DENY: " << jail.reason << std::endl;
        return false;
    }
    const std::string full_path = jail.resolved.string();

    security::AuditLog::record("path_jail", security::Decision::Allow,
        { {"input",    filename},
          {"resolved", full_path} });

    std::cout << "[BootROM] Loading: " << full_path << std::endl;
    std::cout << "[BootROM] Extension: " << extension << std::endl;

    // 파일을 직접 읽어서 전역 변수에 저장
    std::ifstream rom_file(full_path, std::ios::binary | std::ios::ate);
    if (!rom_file.is_open()) {
        std::cerr << "[BootROM] Failed to open: " << full_path << std::endl;
        return false;
    }

    std::streamsize size = rom_file.tellg();
    rom_file.seekg(0, std::ios::beg);

    // ── 벡터 2 차단(전반부): ROM 크기 상한 ──────────────────────────────────
    if (size < 0) {
        security::AuditLog::record("rom_size", security::Decision::Deny,
            { {"resolved", full_path},
              {"reason",   "tellg returned negative"} });
        std::cerr << "[BootROM] invalid file size" << std::endl;
        return false;
    }
    if (size > kMaxRomBytes) {
        security::AuditLog::record("rom_size", security::Decision::Deny,
            { {"resolved", full_path},
              {"size",     static_cast<long long>(size)},
              {"limit",    static_cast<long long>(kMaxRomBytes)},
              {"reason",   "exceeds_max_rom_bytes"} });
        std::cerr << "[BootROM] ROM too large: " << size
                  << " > " << kMaxRomBytes << std::endl;
        return false;
    }

    g_rom_data.resize(static_cast<size_t>(size));
    if (!rom_file.read(reinterpret_cast<char*>(g_rom_data.data()), size)) {
        std::cerr << "[BootROM] Failed to read ROM data" << std::endl;
        return false;
    }

    g_loaded_filename = filename;
    std::cout << "[BootROM] ROM data loaded: " << size << " bytes" << std::endl;
    security::AuditLog::record("rom_load", security::Decision::Allow,
        { {"resolved", full_path},
          {"size",     static_cast<long long>(size)},
          {"ext",      extension} });

    if (extension == ".ch8" || extension == ".c8") {
        std::cout << "[BootROM] → Switching to 8-bit CHIP-8 mode" << std::endl;

        // 8비트 모드 전환 플래그 설정 (ROM 데이터는 전역변수에 저장됨)
        g_switched_to_8bit = true;
        return true;
    } else if (extension == ".ch32" || extension == ".c32") {
        std::cout << "[BootROM] → Continuing in 32-bit Extended mode" << std::endl;
        return chip8_32.load_rom(full_path.c_str());

    } else {
        std::cout << "[BootROM] → Unknown extension, trying 8-bit mode" << std::endl;
        g_switched_to_8bit = true;
        return true;
    }
}

std::string ModeSelector::get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    
    std::string extension = filename.substr(dot_pos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
}
