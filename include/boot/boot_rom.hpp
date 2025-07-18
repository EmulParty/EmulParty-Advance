#pragma once
class Chip8_32;

class BootROM {
public:
    static void load_into_memory(Chip8_32& chip8);
};
