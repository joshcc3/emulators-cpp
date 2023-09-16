//
// Created by jerem on 15/09/2023.
//

#ifndef GBA_EMULATOR_CHIP8_H
#define GBA_EMULATOR_CHIP8_H

#include <cstdint>
#include <cassert>
#include <array>
#include <vector>
#include <memory>
#include <bitset>
#include <algorithm>
#include <iostream>
#include <fstream>

using addr_t = uint16_t;
using regix_t = uint8_t;
using data_t = int8_t;

constexpr uint16_t STACK_MEMORY = 512 / sizeof(uint16_t);
constexpr uint16_t MAIN_MEMORY_SIZE_B = 4096;
constexpr addr_t USER_SPACE_START = 0x200;


constexpr static int SCREEN_HEIGHT = 32;
constexpr static int SCREEN_WIDTH = 64;

class pc_t {
public:
    addr_t pc;

    explicit pc_t(addr_t a) : pc{a} {}

    pc_t() = delete;

    void set(addr_t addr) {
        assert(addr >= USER_SPACE_START && addr < MAIN_MEMORY_SIZE_B - 2);

        pc = addr;
    }
};

class ir_t {
public:
    addr_t reg;

    explicit ir_t(addr_t a) : reg{a} {}

    ir_t() = delete;

    void set(addr_t addr) {
        assert(addr >= USER_SPACE_START && addr < MAIN_MEMORY_SIZE_B - 2);
        reg = addr;
    }
};

class register_t {
public:
    register_t() {
        std::fill(reg.begin(), reg.end(), 0);
    }

    std::array<data_t, 16> reg;

    data_t &operator[](regix_t ix) {
        // TODO a bunch of checks
        return reg[ix];
    }
};

class stack_t {
    addr_t base_addr;
    std::array<addr_t, STACK_MEMORY> mem;
public:
    stack_t() : base_addr{0x0A00} {
        std::fill(mem.begin(), mem.end(), 0);
    }
};

class delaytimer_t {
    uint8_t timer;

};

class soundtimer_t {
    uint8_t timer;

};


class sprite_t {
public:

    sprite_t(uint8_t _n, const std::vector<uint8_t> &&_data) : n{_n}, data{_data} {
        assert(data.size() == n);
        assert(n >= 1 && n <= SCREEN_HEIGHT);
    }

    uint8_t n;
    const std::vector<uint8_t> data;
};

class mainmemory_t {
public:
    std::array<data_t, MAIN_MEMORY_SIZE_B> mem;

    explicit mainmemory_t(const std::string &romName) : mem{} {
        std::vector<uint8_t> fonts{
                0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                0x20, 0x60, 0x20, 0x20, 0x70, // 1
                0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                0xF0, 0x80, 0xF0, 0x80, 0x80  // F
        };
        std::copy(fonts.begin(), fonts.end(), mem.begin() + 0x50);
        std::ifstream input(romName, std::ios::binary);
        std::copy(
                std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>(),
                mem.begin() + USER_SPACE_START);
    }

    data_t &operator[](addr_t addr) {
        // addr is not out of bounds, addr is not special memory
        // addr is the code section and is executable??
        return mem[addr];
    }

    const data_t &operator[](addr_t addr) const {
        // addr is not out of bounds, addr is not special memory
        // addr is the code section and is executable??
        return mem[addr];
    }

    sprite_t getSpriteData(addr_t indexRegister, data_t n) {
        assert(n >= 1 && n <= SCREEN_HEIGHT);
        assert(mem[indexRegister] >= 0 && mem[indexRegister] < MAIN_MEMORY_SIZE_B);
        return sprite_t{(uint8_t) n, std::vector<uint8_t>{}};
    }
};


class display_t {

public:
    std::vector<std::bitset<64>> disp;

    display_t() : disp{SCREEN_HEIGHT, std::bitset<SCREEN_WIDTH>{0x0}} {}

    void clear() {
        for (auto &it: disp) {
            it &= 0x0;
        }
    }

    void draw(const sprite_t &sprite, data_t x, data_t y) {
        // assert on the bounds of x and y and on the sprite.
        for (int i = 0; i < sprite.n && y + i < SCREEN_HEIGHT; ++i) {
            int screenY = y + i;
            uint64_t maskL = (sprite.data[i] >> std::max(0, x + 8 - SCREEN_WIDTH)) << (SCREEN_WIDTH - x);
            std::bitset<SCREEN_WIDTH> mask{maskL};
            disp[screenY] ^= mask;
        }
    }
};

struct instr_typ1 {

    data_t typ: 4;
    regix_t x: 4;
    regix_t y: 4;
    data_t n: 4;
};

struct instr_typ2 {

    data_t typ: 4;
    regix_t x: 4;
    data_t _n1: 4;
    data_t _n2: 4;

    [[nodiscard]] data_t getN() const {
        return (_n1 << 4) | _n2;
    }
};

struct instr_t {
    uint8_t typ: 4;
    uint8_t _op1: 4;
    uint8_t _op2: 8;

    [[nodiscard]] uint16_t getOperand() const {
        return ((uint16_t) _op1 << 8) | (uint16_t) _op2;
    }
};

class chip8 {

    // all the resources
    // program counter: 2 bytes
    // instruction register 2B
    // 16 1 byte registers
    // Stack - 256 bytes, 16 bit addressses
    // delay timer - incremented 60 times per second
    // sound timer - incremented 60 times per second
    // Main memory: 4KB - 4096 bytes
    // Display: 64 by 32 pixels

    // Assume that interpreter code is laid out from 000 - 1FF and program code is laid out from
    // 200 onwards.

    /*
     * lay things out based on which are most accessed: - good class design
     * memory alignment for classes
     */

    pc_t programCounter;
    ir_t indexRegister;
    register_t registers;
    stack_t stack;
    delaytimer_t delayTimer;
    soundtimer_t soundTimer;
    mainmemory_t mainMemory;
    display_t disp;


public:

    chip8(const std::string &romName) : programCounter{USER_SPACE_START}, indexRegister{0x050}, mainMemory(romName), disp{} {

    }

    [[nodiscard]]

    uint16_t fetch() const noexcept {
        assert(programCounter.pc >= USER_SPACE_START && programCounter.pc <= MAIN_MEMORY_SIZE_B - 16);
        return (mainMemory[programCounter.pc] << 8) | mainMemory[programCounter.pc + 1];
    }

    void decodeAndExecute(uint16_t instruction) {
        auto i = *reinterpret_cast<instr_t *>(&instruction);
        switch (i.typ) {
            case 0x0: { // clear screen
                switch (i.getOperand()) {
                    case 0x0E0:
                        disp.clear();
                        break;
                }
                break;
            }
            case 0x1: { // jump
                addr_t jmpInstr = i.getOperand();
                programCounter.set(jmpInstr);
                break;
            }
            case 0x6: { // set register
                auto regSet = *reinterpret_cast<instr_typ2 *>(&instruction);
                regix_t x = regSet.x;
                data_t nn = regSet.getN();
                registers[x] = nn;
                break;
            }
            case 0x7: { // add value to register
                auto regSet = *reinterpret_cast<instr_typ2 *>(&instruction);
                regix_t x = regSet.x;
                data_t nn = regSet.getN();

                registers[x] += nn;
                break;
            }
            case 0xA: { // set index register
                indexRegister.set(i.getOperand());
                break;
            }
            case 0xD: { // display
                auto ops = *reinterpret_cast<instr_typ1 *>(&instruction);
                data_t x = registers[ops.x];
                data_t y = registers[ops.y];
                data_t n = ops.n;
                sprite_t sprite = mainMemory.getSpriteData(indexRegister.reg, n);
                disp.draw(sprite, x, y);
                break;
            }
            default: {
                std::cerr << "Unrecognized instruction [" << i.typ << "]" << std::endl;
                assert(false);
            }
        }
    }

    void draw() {
        for (auto str: disp.disp) {
            std::cout << str << std::endl;
        }
    }

};

#endif //GBA_EMULATOR_CHIP8_H
