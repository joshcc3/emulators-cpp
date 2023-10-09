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
#include <cstdlib>
#include <set>

#include <SFML/Graphics.hpp>

using addr_t = uint16_t;
using regix_t = uint8_t;
using data_t = uint8_t;

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

class reg_t {
public:
    reg_t() {
        std::fill(reg.begin(), reg.end(), 0);
    }

    std::array<data_t, 16> reg;

    data_t &operator[](regix_t ix) {
        // TODO a bunch of checks
        return reg[ix];
    }
};

class stack_t {
    int stackSize;
    std::array<addr_t, STACK_MEMORY> mem;
public:
    stack_t() {
        std::fill(mem.begin(), mem.end(), 0);
    }

    addr_t pop() {
        assert(stackSize > 0);
        return mem[--stackSize];
    }

    void push(addr_t a) {
        assert(a >= USER_SPACE_START);
        mem[stackSize++] = a;
    }
};

class delaytimer_t {
public:

    uint8_t timer;

    void decrement() {
        if (timer > 0) {
            --timer;
        }
    }

};

class soundtimer_t {
public:

    uint8_t timer;

    void decrement() {
        if (timer > 0) {
            std::cout << '\a';
            --timer;
        }
    }
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

constexpr addr_t FONT_ADDRESSES = 0x50;

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
        std::copy(fonts.begin(), fonts.end(), mem.begin() + FONT_ADDRESSES);
        std::ifstream input(romName, std::ios::binary);
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
        std::copy(buffer.begin(), buffer.end(), mem.begin() + USER_SPACE_START);

        // Jump instruction
        mem[0] = 0x1;
        mem[1] = 0x0;
        mem[2] = 0x0;
        mem[3] = 0x0;

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
        return sprite_t{n,
                        std::vector<uint8_t>(mem.begin() + indexRegister, mem.begin() + indexRegister + n)};
    }
};


class display_t {

    int displayHeight;
    int displayWidth;
    int pixelDisplaySizeHeight;
    int pixelDisplaySizeWidth;
    uint32_t pixelDataTable[2];
    sf::Uint32 *deviceBuffer;

public:

    std::vector<std::bitset<64>> disp;

    display_t(int _displayHeight, int _displayWidth, sf::Uint32 *_deviceBuffer) :
            disp(SCREEN_HEIGHT, std::bitset<SCREEN_WIDTH>{0x0}),
            displayHeight{_displayHeight},
            displayWidth{_displayWidth},
            pixelDisplaySizeHeight{
                    _displayHeight /
                    SCREEN_HEIGHT
            },
            pixelDisplaySizeWidth{
                    _displayWidth / SCREEN_WIDTH
            },
            deviceBuffer{_deviceBuffer} {
        pixelDataTable[0] = 0;
        pixelDataTable[1] = 0xffffffff;
    }

    void clear() {
        std::fill(deviceBuffer, deviceBuffer + displayWidth * displayHeight, 0x0);
    }

    unsigned char f(unsigned char b) const {
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        return b;
    }


//    void drawDebug(const sprite_t &sprite, data_t x, data_t y) {
//        // assert on the bounds of x and y and on the sprite.
//        for (int i = 0; i < sprite.n && y + i < SCREEN_HEIGHT; ++i) {
//            int screenY = y + i;
//
//            //            int rightOffset = std::max(0,SCREEN_WIDTH - x - 8);
////            int charactersElided = std::max(0, x + 8 - SCREEN_WIDTH);
////            uint64_t maskL = (((uint64_t)sprite.data[i]) >> charactersElided) << rightOffset;
//
//            uint64_t byte1Bits = 8 * (x / 8 + 1) - x;
//            uint64_t byte1Pos = 8 * (x / 8);
//            uint64_t byte2Bits = 8 - byte1Bits;
//            uint64_t byte2Pos = byte1Pos + 8;
//            uint64_t byte2Mask = (1 << byte2Bits) - 1;
//            uint64_t byte1Mask = ~byte2Mask;
//
//            unsigned char byte1 = (sprite.data[i] & byte1Mask) >> byte2Bits;
//            unsigned char byte2 = (sprite.data[i] & byte2Mask) << byte1Bits;
//            unsigned char _byte1 = f(byte1);
//            unsigned char _byte2 = f(byte2);
//
//            uint64_t maskL = ((uint64_t) _byte1 << byte1Pos) | ((uint64_t) _byte2 << byte2Pos);
//            std::bitset<SCREEN_WIDTH> mask{maskL};
//            disp[screenY] ^= mask;
//        }
//    }

    bool draw(const sprite_t &sprite, data_t x, data_t y) {

        bool vf = false;

        uint64_t byte1Bits = 8 * (x / 8 + 1) - x;
        uint64_t byte1Pos = 8 * (x / 8);
        uint64_t byte2Bits = 8 - byte1Bits;
        uint64_t byte2Pos = byte1Pos + 8;
        uint64_t byte2Mask = (1 << byte2Bits) - 1;
        uint64_t byte1Mask = ~byte2Mask;


        for (int i = 0; i < sprite.n && y + i < SCREEN_HEIGHT; ++i) {
            int screenY = y + i;

            unsigned char byte1 = (sprite.data[i] & byte1Mask) >> byte2Bits;
            unsigned char byte2 = (sprite.data[i] & byte2Mask) << byte1Bits;
            unsigned char _byte1 = f(byte1);
            unsigned char _byte2 = f(byte2);

            uint64_t maskL = ((uint64_t) _byte1 << byte1Pos) | ((uint64_t) _byte2 << byte2Pos);
            std::bitset<SCREEN_WIDTH> mask{maskL};

            vf |= (disp[screenY] & mask).any();
            disp[screenY] ^= mask;

            for (int j = 0; j < 8; ++j) {
                int screenX = x + j;
                bool pixelSet = disp[screenY][screenX];
                uint32_t pixelData = pixelDataTable[pixelSet];
                int deviceX = screenX * pixelDisplaySizeWidth;
                int deviceY = screenY * pixelDisplaySizeHeight;
                int pixelStartIx = deviceY * displayWidth + deviceX;
                for (int dy = 0; dy < pixelDisplaySizeHeight; ++dy) {
                    for (int dx = 0; dx < pixelDisplaySizeHeight; ++dx) {
                        int pixelIx = pixelStartIx + dy * displayWidth + dx;
                        deviceBuffer[pixelIx] = pixelData;
                    }
                }
            }
        }

        return vf;
    }

};

struct instr_typ1 {

    data_t n: 4;
    regix_t y: 4;
    regix_t x: 4;
    data_t typ: 4;
};

struct instr_typ2 {

    data_t _n2: 4;
    data_t _n1: 4;
    regix_t x: 4;
    data_t typ: 4;

    [[nodiscard]] data_t getN() const {
        return (_n1 << 4) | _n2;
    }
};

struct instr_t {
    uint8_t _op2: 8;
    uint8_t _op1: 4;
    uint8_t typ: 4;

    [[nodiscard]] uint16_t getOperand() const {
        return ((uint16_t) _op1 << 8) | (uint16_t) _op2;
    }
};

//constexpr int KEYPRESS = sf::Event::EventType::KeyPressed;
//constexpr int KEYRELEASED = sf::Event::EventType::KeyReleased;
using Scancode = sf::Keyboard::Scancode;

class keyboard_t {
    std::set<Scancode> interestKeys;
    std::map<Scancode , uint8_t> keyMap;
public:

    data_t lastKeyPress;


    keyboard_t(): lastKeyPress{0x10} {
        std::vector<Scancode> events = {Scancode::Num1, Scancode::Num2, Scancode::Num3, Scancode::Num4,
                                        Scancode::Q, Scancode::W, Scancode::E, Scancode::R,
                                        Scancode::A, Scancode::S, Scancode::D, Scancode::F,
                                        Scancode::Z, Scancode::X, Scancode::C, Scancode::V};
        for (auto e: events) {
            interestKeys.insert(e);
        }

        keyMap = std::map<Scancode , uint8_t>{};
        keyMap[Scancode::Num1] = 1;
        keyMap[Scancode::Num2] = 2;
        keyMap[Scancode::Num3] = 3;
        keyMap[Scancode::Num4] = 0xC;
        keyMap[Scancode::Q] = 4;
        keyMap[Scancode::W] = 5;
        keyMap[Scancode::E] = 6;
        keyMap[Scancode::R] = 0xD;
        keyMap[Scancode::A] = 7;
        keyMap[Scancode::S] = 8;
        keyMap[Scancode::D] = 9;
        keyMap[Scancode::F] = 0xE;
        keyMap[Scancode::Z] = 0xA;
        keyMap[Scancode::X] = 0;
        keyMap[Scancode::C] = 0xB;
        keyMap[Scancode::V] = 0xF;


    }

    std::bitset<16> keysPressed;

    void processKeyEvents(const std::vector<sf::Event> &events) {
        for (auto event: events) {
            if ((event.type & (KEYPRESS | KEYRELEASED)) && interestKeys.find(event.key.scancode) != interestKeys.end()) {
                int deviceKeyPad = keyMap[event.key.scancode];
                keysPressed[deviceKeyPad] = (event.type == KEYPRESS);
                lastKeyPress = lastKeyPress == 0x10 ? deviceKeyPad : lastKeyPress;
            }
        }
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
    reg_t registers;
    stack_t stack;
    delaytimer_t delayTimer;
    soundtimer_t soundTimer;
    mainmemory_t mainMemory;
    display_t disp;
    keyboard_t keyboard;


public:

    chip8(const std::string &romName, int displayHeight, int displayWidth, sf::Uint8 *deviceMem) :
            programCounter{USER_SPACE_START},
            indexRegister{0x050},
            mainMemory(romName),
            disp{displayHeight,
                 displayWidth,
                 (sf::Uint32 *) deviceMem} {
    }


    void processKeyboardEvents(const std::vector<sf::Event> &v) {
        keyboard.processKeyEvents(v);
    }

    [[nodiscard]]
    uint16_t fetch() noexcept {
        assert((programCounter.pc >= USER_SPACE_START || programCounter.pc == 0) &&
               programCounter.pc <= MAIN_MEMORY_SIZE_B - 16);
        uint16_t result =
                ((uint8_t) (mainMemory[programCounter.pc]) << 8) | (uint8_t) mainMemory[programCounter.pc + 1];
        programCounter.pc += 2;
        return result;
    }

    void decodeAndExecute(uint16_t instruction) {
        auto i = *reinterpret_cast<instr_t *>(&instruction);
        switch (i.typ) {
            case 0x0: { // clear screen
                switch (i.getOperand()) {
                    case 0x0E0:
                        disp.clear();
                        break;
                    case 0x0EE:
                        programCounter.pc = stack.pop();
                        break;
                }
                break;
            }
            case 0x1: { // jump
                addr_t jmpInstr = i.getOperand();
                programCounter.set(jmpInstr);
                break;
            }
            case 0x2: {
                stack.push(programCounter.pc);
                addr_t jmpInstr = i.getOperand();
                programCounter.set(jmpInstr);
                break;
            }
            case 0x3: {
                auto conditionalSkip = *reinterpret_cast<instr_typ2 *>(&instruction);
                data_t& vx = registers[conditionalSkip.x];
                data_t val = conditionalSkip.getN();
                if (vx == val) {
                    programCounter.pc += 2;
                }
                break;
            }
            case 0x4: {
                auto conditionalSkip = *reinterpret_cast<instr_typ2 *>(&instruction);
                const data_t& vx = registers[conditionalSkip.x];
                data_t val = conditionalSkip.getN();
                if (vx != val) {
                    programCounter.pc += 2;
                }
                break;
            }
            case 0x5: {
                auto conditionalSkipReg = *reinterpret_cast<instr_typ1 *>(&instruction);
                assert(conditionalSkipReg.n == 0);
                regix_t x = conditionalSkipReg.x;
                regix_t y = conditionalSkipReg.y;
                if (registers[x] == registers[y]) {
                    programCounter.pc += 2;
                }
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
            case 0x8: {
                auto arithmetic = *reinterpret_cast<instr_typ1 *>(&instruction);
                data_t &vx = registers[arithmetic.x];
                data_t &vy = registers[arithmetic.y];
                data_t &vf = registers[0xf];
                switch (arithmetic.n) {
                    case 0x0:
                        vx = vy;
                        break;
                    case 0x1:
                        vx |= vy;
                        break;
                    case 0x2:
                        vx &= vy;
                        break;
                    case 0x3:
                        vx ^= vy;
                        break;
                    case 0x4:
                        vf = vy > 255 - vx;
                        vx += vy;
                        break;
                    case 0x5:
                        vf = vx > vy;
                        vx -= vy;
                        break;
                    case 0x6:
                        std::cout << "Ambiguous 8XY6 Instruction - defaulting to super-chip" << std::endl;
                        vf = vx & 1;
                        vx >>= 1;
                        break;
                    case 0x7:
                        vf = vy > vx;
                        vx = vy - vx;
                        break;
                    case 0xE:
                        std::cout << "Ambiguous 8XYE Instruction - defaulting to super-chip" << std::endl;
                        vf = (vx & 0x80) != 0;
                        vx <<= 1;
                        break;
                }
                break;
            }
            case 0x9: {
                auto conditionalSkipReg = *reinterpret_cast<instr_typ1 *>(&instruction);
                assert(conditionalSkipReg.n == 0);
                regix_t x = conditionalSkipReg.x;
                regix_t y = conditionalSkipReg.y;
                if (registers[x] != registers[y]) {
                    programCounter.pc += 2;
                }
                break;
            }
            case 0xA: { // set index register
                indexRegister.set(i.getOperand());
                break;
            }
            case 0xB: {
                std::cout << "Ambiguous BNNN Instruction - defaulting to COSMAC" << std::endl;
                programCounter.pc = i.getOperand() + registers[0];
                break;
            }
            case 0xC: {
                auto randomInstr = *reinterpret_cast<instr_typ2 *>(&instruction);
                data_t &vx = registers[randomInstr.x];
                data_t n = randomInstr.getN();
                uint32_t randomVal = rand();
                vx = n & (randomVal & 0xff);
                break;
            }
            case 0xD: { // display
                auto ops = *reinterpret_cast<instr_typ1 *>(&instruction);
                data_t x = registers[ops.x];
                data_t y = registers[ops.y];
                data_t n = ops.n;
                sprite_t sprite = mainMemory.getSpriteData(indexRegister.reg, n);
                registers[0xF] = disp.draw(sprite, x, y);
                break;
            }
            case 0xE: {
                auto keyboardSkip = *reinterpret_cast<instr_typ2 *>(&instruction);
                data_t &vx = registers[keyboardSkip.x];
                switch (keyboardSkip.getN()) {
                    case 0x9E:
                        if (keyboard.keysPressed[vx]) {
                            programCounter.pc += 2;
                        }
                        break;
                    case 0xA1:
                        if (!keyboard.keysPressed[vx]) {
                            programCounter.pc += 2;
                        }
                        break;
                    default:
                        std::cerr << "Unrecognized Instruction " << keyboardSkip.getN() << std::endl;
                        exit(1);
                }
                break;
            }
            case 0xF: {
                auto timerAndLoad = *reinterpret_cast<instr_typ2 *>(&instruction);
                data_t &vx = registers[timerAndLoad.x];
                data_t &vf = registers[0xF];
                data_t n = timerAndLoad.getN();
                switch (n) {
                    case 0x07:
                        vx = delayTimer.timer;
                        break;
                    case 0x15:
                        delayTimer.timer = vx;
                        break;
                    case 0x18:
                        soundTimer.timer = vx;
                        break;
                    case 0x1E:
                        vf = indexRegister.reg > (0x0fff - vx);
                        indexRegister.reg += vx;
                        break;
                    case 0x0A:
                        if (keyboard.lastKeyPress <= 0xF) {
                            keyboard.lastKeyPress = 0x10;
                            vx = keyboard.lastKeyPress;
                        } else {
                            programCounter.pc -= 2;
                        }
                        break;
                    case 0x29:
                        indexRegister.reg = FONT_ADDRESSES + vx * 5;
                        break;
                    case 0x33: {
                        uint8_t digit1 = vx / 100;
                        uint8_t digit2 = (vx % 100) / 10;
                        uint8_t digit3 = vx % 10;
                        mainMemory[indexRegister.reg] = digit1;
                        mainMemory[indexRegister.reg + 1] = digit2;
                        mainMemory[indexRegister.reg + 2] = digit3;
                        break;
                    }
                    case 0x55:
                        for (int i = 0; i <= timerAndLoad.x; ++i) {
                            mainMemory[indexRegister.reg + i] = registers[i];
                        }
                        break;
                    case 0x65:
                        for (int i = 0; i <= timerAndLoad.x; ++i) {
                            registers[i] = mainMemory[indexRegister.reg + i];
                        }
                        break;
                    default:
                        std::cout << "Unexpected instruction [" << n << "]." << std::endl;
                        exit(1);
                }
                break;
            }
            default: {
                std::cerr << "Unrecognized instruction [" << i.typ << "]" << std::endl;
                programCounter.pc = 0;
            }
        }

    }

    void draw() {
        for (auto str: disp.disp) {
            for (int i = 0; i < str.size(); ++i) {
                std::cout << ((str[i] == 0) ? ' ' : '1');
            }
            std::cout << std::endl;
        }
    }


    void updateTimers() {
        delayTimer.decrement();
        soundTimer.decrement();

    }
};

#endif //GBA_EMULATOR_CHIP8_H
