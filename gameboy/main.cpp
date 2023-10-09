//
// Created by jc on 24/09/23.
//

#define DEBUG
//#define VERBOSE

#include <algorithm>

#include <chrono>
#include <iostream>
#include <filesystem>
#include <cstdint>
#include <bitset>
#include <vector>
#include <bit>
#include <unistd.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#include <SFML/Audio.hpp>
#include <queue>
#include <array>
#include <cassert>
#include <fstream>
#include <unistd.h>
#include "audio_driver.h"

#include "structs.h"
#include "PPU.h"

using namespace std;


constexpr int KEYPRESS = sf::Event::EventType::KeyPressed;
constexpr int KEYRELEASED = sf::Event::EventType::KeyReleased;

class Joypad {
public:
    using Scancode = sf::Keyboard::Scancode;

    struct JoypadBit {
        u8 column: 4;
        u8 row: 4;
    };
    vector<u8> &ram;
    u8 &jReg;
    InterruptFlag &ifReg;

    std::set<Scancode> interestKeys;
    std::map<Scancode, JoypadBit> keyMap;

    Joypad(vector<u8> &ram) : ram{ram}, jReg{ram[0xFF00]},
                              ifReg{*reinterpret_cast<InterruptFlag *>(ram[0xFF0F])} {
        std::vector<Scancode> events = {Scancode::A, Scancode::B, Scancode::P, Scancode::L, Scancode::Left,
                                        Scancode::Right,
                                        Scancode::Up, Scancode::Down};
        for (auto e: events) {
            interestKeys.insert(e);
        }

        keyMap = std::map<Scancode, JoypadBit>{};
        keyMap[Scancode::A] = {.column = 5, .row = 0};
        keyMap[Scancode::B] = {.column = 5, .row = 1};
        keyMap[Scancode::L] = {.column = 5, .row = 2}; // select
        keyMap[Scancode::P] = {.column = 5, .row = 3}; // start
        keyMap[Scancode::Left] = {.column = 4, .row = 1};
        keyMap[Scancode::Right] = {.column = 4, .row = 0};
        keyMap[Scancode::Up] = {.column = 4, .row = 2};
        keyMap[Scancode::Down] = {.column = 4, .row = 3};

    }

    void processKeyEvents(const std::vector<sf::Event> &events) {
        for (auto event: events) {
            if ((event.type == KEYPRESS || event.type == KEYRELEASED) &&
                interestKeys.find(event.key.scancode) != interestKeys.end()) {
                auto key = keyMap[event.key.scancode];
                if (event.type & sf::Event::KeyPressed) {
                    jReg = jReg | (1 << key.column) | (1 << key.row);
                } else {
                    jReg = jReg & ~(1 << key.column | 1 << key.row);
                }
                ifReg.joypad = true;
            }
        }

    }

};

class CPU {
public:
    u8 registers[8];
    u8 &a = registers[1];
    FlagReg &f = (FlagReg &) registers[0];
    u8 &b = registers[3];
    u8 &c = registers[2];
    u8 &d = registers[5];
    u8 &e = registers[4];
    u8 &h = registers[7];
    u8 &l = registers[6];
    u16 &af = (u16 &) registers;
    u16 &bc = (u16 &) (registers[2]);
    u16 &de = (u16 &) (registers[4]);
    u16 &hl = (u16 &) (registers[6]);

    u16 sp;
    u16 pc;

    uint64_t clock;

    vector<u8> &vram;

    bool ime;

    InterruptFlag &ifReg;
    InterruptEnable &ieReg;

    CPU(vector<u8> &vram) : vram{vram}, clock{0}, ime{false},
                            ifReg{*reinterpret_cast<InterruptFlag *>(&vram[0xFF0F])},
                            ieReg{*reinterpret_cast<InterruptEnable *>(&vram[0xFFFF])} {
        initializeRegisters();

        vram[0xFF40] = 0x91;
        vram[0xFF42] = 0x00;
        vram[0xFF43] = 0x00;
        vram[0xFF45] = 0x00;
        vram[0xFF47] = 0xFC;
        vram[0xFF48] = 0xFF;
        vram[0xFF49] = 0xFF;
        vram[0xFF4A] = 0x00;
        vram[0xFF4B] = 0x00;
        vram[0xFFFF] = 0x00;
    }

    void initializeRegisters() {
        af = 0x01B0;
        bc = 0x0013;
        de = 0x00D8;
        hl = 0x014D;
        sp = 0xFFFE;
        pc = 0x0100;
    }

    void processInterrupts() {
        if (ime) {
            ime = false;
            // if reset
            u16 jumpAddr = 0x0;
            if (ifReg.vBlank && ieReg.vBlank) {
                ifReg.vBlank = false;
                jumpAddr = 0x40;
            } else if (ifReg.lcdStat && ieReg.lcdStat) {
                ifReg.lcdStat = false;
                jumpAddr = 0x48;
            } else if (ifReg.timer && ieReg.timer) {
                ifReg.timer = false;
                jumpAddr = 0x50;
            } else if (ifReg.serial && ieReg.serial) {
                ifReg.serial = false;
                jumpAddr = 0x58;
            } else if (ifReg.joypad && ieReg.joypad) {
                ifReg.joypad = false;
                jumpAddr = 0x60;
            }

            if (jumpAddr) {
                vram[--sp] = pc >> 8;
                vram[--sp] = pc & 0xff;
                pc = jumpAddr;
            }
        }
    }

    // at instruction 0x26b - turned on devices and stuff
    void fetchDecodeExecute() {
        static int counter = 0;
        ++counter;
        u8 r[8] = {3, 2, 5, 4, 7, 6, 255, 1};
        u8 opcode = vram[pc];
        switch (opcode) {
            case 0x01: // ld sp 0x
                clock += 12;
                bc = (u16 &) (vram[pc + 1]);
                pc += 3;
                break;
            case 0x11:
                clock += 12;
                de = (u16 &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x21:
                clock += 12;
                hl = (u16 &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x31:
                clock += 12;
                sp = (u16 &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x32:
                clock += 8;
                vram[hl--] = a;
                ++pc;
                break;

            case 0x20: {
                if (!f.zf) {
                    int8_t relativeOffset = vram[pc + 1];
                    pc = pc + 2 + relativeOffset;
                    clock += 12;
                } else {
                    pc += 2;
                    clock += 8;
                }
                break;
            }

            case 0xCB:
                switch (vram[pc + 1]) {
                    case 0x7C:
                        clock += 4;
                        f.zf = h >> 7;
                        f.h = true;
                        pc += 2;
                        break;
                    case 0x10:
                    case 0x11:
                    case 0x12:
                    case 0x13:
                    case 0x14:
                    case 0x15:
                    case 0x17: {
                        clock += 4;

                        u8 &reg = registers[r[vram[pc + 1] - 0x10]];

                        bool carryFlag = reg >> 7;
                        reg = (reg << 1) | f.cy;
                        f.zf = reg == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = carryFlag;
                        pc += 2;
                        break;
                    }
                    case 0x30:
                    case 0x31:
                    case 0x32:
                    case 0x33:
                    case 0x34:
                    case 0x35:
                    case 0x37: {
                        u8 op = vram[pc + 1];
                        u8 &reg = registers[r[op & 0xf]];
                        reg = ((reg & 0xf) << 4) | (reg >> 4);
                        f.zf = reg == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = false;
                        clock += 8;
                        pc += 2;
                        break;
                    }
                    case 0x36: {
                        u8 &reg = vram[hl];
                        reg = ((reg & 0xf) << 4) | (reg >> 4);
                        f.zf = reg == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = false;
                        clock += 8;
                        pc += 2;
                        break;

                    }
                    case 0xCE:
                    case 0xDE:
                    case 0xEE:
                    case 0xFE: {
                        u8 ix = ((vram[pc + 1] >> 4) - 0xC) * 2 + 1;
                        vram[hl] |= (1 << ix);
                        pc += 2;
                        clock += 16;
                        break;
                    }
                    case 0x47:
                    case 0x57:
                    case 0x67:
                    case 0x77: {
                        u8 bit = ((vram[pc + 1] >> 4) - 0x4) * 2;
                        u8 bitVal = a >> bit;
                        pc += 2;
                        clock += 8;
                        f.zf = bitVal == 0;
                        f.n = false;
                        f.h = true;
                        break;
                    }
                    case 0x87:
                    case 0x97:
                    case 0xA7:
                    case 0xB7: {
                        u8 bit = ((vram[pc + 1] >> 4) - 0x8) * 2;
                        u8 bitVal = ~(1 << bit);
                        a = a & bitVal;
                        pc += 2;
                        clock += 8;
                        break;
                    }
                    default:
                        printf("CB - Opcode not implemented: [%x]", vram[pc + 1]);
                        exit(1);
                }
                break;
            case 0xA0:
                clock += 4;
                a = a & b;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA1:
                clock += 4;
                a = a & c;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA2:
                clock += 4;
                a = a & d;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA3:
                clock += 4;
                a = a & e;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA4:
                clock += 4;
                a = a & h;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA5:
                clock += 4;
                a = a & l;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA6:
                clock += 8;
                a = a & vram[hl];
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA7:
                clock += 4;
                a = a & a;
                f.h = true;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA8:
                clock += 4;
                a = a ^ b;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA9:
                clock += 4;
                a = a ^ c;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAA:
                clock += 4;
                a = a ^ d;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAB:
                clock += 4;
                a = a ^ e;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAC:
                clock += 4;
                a = a ^ h;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAD:
                clock += 4;
                a = a ^ l;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAE:
                clock += 8;
                a = a ^ vram[hl];
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAF:
                clock += 4;
                a = a ^ a;
                f.zf = a == 0;
                ++pc;
                break;
            case 0x0E: {
                clock += 8;
                u8 d8 = vram[pc + 1];
                c = d8;
                pc += 2;
                break;
            }
            case 0x1E: {
                clock += 8;
                u8 d8 = vram[pc + 1];
                e = d8;
                pc += 2;
                break;
            }
            case 0x2E: {
                clock += 8;
                u8 d8 = vram[pc + 1];
                l = d8;
                pc += 2;
                break;
            }
            case 0x3E: {
                clock += 8;
                u8 d8 = vram[pc + 1];
                a = d8;
                pc += 2;
                break;
            }
            case 0xE0: {
                clock += 12;
                vram[0xFF00 + vram[pc + 1]] = a;
                pc += 2;
                break;
            }
            case 0xE2: {
                clock += 8;
                vram[0xFF00 + c] = a;
                ++pc;
                break;
            }
            case 0x0C:
            case 0x1C:
            case 0x2C:
            case 0x3C: {
                u8 m[4] = {2, 4, 6, 1};
                u8 &reg = registers[m[(opcode >> 4)]];
                clock += 4;
                f.zf = (reg + 1) == 0;
                f.n = false;
                f.h = (reg & 0xf) == 0xf;
                ++reg;
                ++pc;
                break;
            }
            case 0x0D:
            case 0x1D:
            case 0x2D:
            case 0x3D: {
                u8 m[4] = {2, 4, 6, 1};
                u8 &reg = registers[m[(opcode >> 4)]];
                clock += 4;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0x0;
                --reg;
                ++pc;
                break;
            }
            case 0x70: {
                clock += 8;
                vram[hl] = b;
                ++pc;
                break;
            }
            case 0x71: {
                clock += 8;
                vram[hl] = c;
                ++pc;
                break;
            }
            case 0x72: {
                clock += 8;
                vram[hl] = d;
                ++pc;
                break;
            }
            case 0x73: {
                clock += 8;
                vram[hl] = e;
                ++pc;
                break;
            }
            case 0x74: {
                clock += 8;
                vram[hl] = h;
                ++pc;
                break;
            }
            case 0x75: {
                clock += 8;
                vram[hl] = l;
                ++pc;
                break;
            }
            case 0x77: {
                clock += 8;
                vram[hl] = a;
                ++pc;
                break;
            }
            case 0x0A: {
                clock += 8;
                a = vram[bc];
                ++pc;
                break;
            }
            case 0x1A: {
                clock += 8;
                a = vram[de];
                ++pc;//
                break;
            }
            case 0xCD: {
                clock += 24;
                u16 jumpAddr = ((u16) (vram[pc + 2]) << 8) | (vram[pc + 1]);
                vram[--sp] = (pc + 3) >> 8;
                vram[--sp] = (pc + 3) & 0xff;
                pc = jumpAddr;
                break;
            }
            case 0x03: {
                clock += 8;
                ++bc;
                ++pc;
                break;
            }
            case 0x13: {
                clock += 8;
                ++de;
                ++pc;
                break;
            }
            case 0x23: {
                clock += 8;
                ++hl;
                ++pc;
                break;
            }
            case 0x33: {
                clock += 8;
                ++sp;
                ++pc;
                break;
            }
            case 0x40: {
                clock += 4;
                b = b;
                ++pc;
                break;
            }
            case 0x41: {
                clock += 4;
                b = c;
                ++pc;
                break;
            }
            case 0x42: {
                clock += 4;
                b = d;
                ++pc;
                break;
            }
            case 0x43: {
                clock += 4;
                b = e;
                ++pc;
                break;
            }
            case 0x44: {
                clock += 4;
                b = h;
                ++pc;
                break;
            }
            case 0x45: {
                clock += 4;
                b = l;
                ++pc;
                break;
            }
            case 0x50: {
                clock += 4;
                d = b;
                ++pc;
                break;
            }
            case 0x51: {
                clock += 4;
                d = c;
                ++pc;
                break;
            }
            case 0x52: {
                clock += 4;
                d = d;
                ++pc;
                break;
            }
            case 0x53: {
                clock += 4;
                d = e;
                ++pc;
                break;
            }
            case 0x54: {
                clock += 4;
                d = h;
                ++pc;
                break;
            }
            case 0x55: {
                clock += 4;
                d = l;
                ++pc;
                break;
            }
            case 0x60: {
                clock += 4;
                h = b;
                ++pc;
                break;
            }
            case 0x61: {
                clock += 4;
                h = c;
                ++pc;
                break;
            }
            case 0x62: {
                clock += 4;
                h = d;
                ++pc;
                break;
            }
            case 0x63: {
                clock += 4;
                h = e;
                ++pc;
                break;
            }
            case 0x64: {
                clock += 4;
                h = h;
                ++pc;
                break;
            }
            case 0x65: {
                clock += 4;
                h = l;
                ++pc;
                break;
            }
            case 0x47: {
                clock += 4;
                b = a;
                ++pc;
                break;
            }
            case 0x57: {
                clock += 4;
                d = a;
                ++pc;
                break;
            }
            case 0x67: {
                clock += 4;
                h = a;
                ++pc;
                break;
            }
            case 0x48:
            case 0x49:
            case 0x4a:
            case 0x4b:
            case 0x4c:
            case 0x4d:
            case 0x4f:
            case 0x58:
            case 0x59:
            case 0x5a:
            case 0x5b:
            case 0x5c:
            case 0x5d:
            case 0x5f:
            case 0x68:
            case 0x69:
            case 0x6a:
            case 0x6b:
            case 0x6c:
            case 0x6d:
            case 0x6f:
            case 0x78:
            case 0x79:
            case 0x7a:
            case 0x7b:
            case 0x7c:
            case 0x7d:
            case 0x7f: {
                clock += 4;
                u8 o[4] = {2, 4, 6, 1};
                registers[o[(opcode >> 4) - 4]] = registers[r[(opcode & 0xf) - 0x8]];
                ++pc;
                break;
            }
            case 0xFE: {
                clock += 8;
                int res = a - vram[pc + 1];
                f.zf = res == 0;
                f.n = true;
                f.h = (a & 0xf) < (vram[pc + 1] & 0xf);
                f.cy = a < vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x90:
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x97: {
                clock += 4;
                u8 &opReg = registers[r[(opcode & 0xf)]];
                u8 result = a - opReg;
                f.zf = a == 0;
                f.n = true;
                f.h = (a & 0xf) < (opReg & 0xf);
                f.cy = a < opReg;
                a = result;
                ++pc;
                break;
            }
            case 0xF0: {
                clock += 12;
                a = vram[0xFF00 | vram[pc + 1]];
                pc += 2;
                break;
            }
            case 0x05: {
                clock += 4;
                u8 &reg = b;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x15: {
                clock += 4;
                u8 &reg = d;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x25: {
                clock += 4;
                u8 &reg = h;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x06: {
                clock += 8;
                b = vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x16: {
                clock += 8;
                d = vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x26: {
                clock += 8;
                h = vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x18: {
                clock += 12;
                pc = pc + 2 + (int8_t) (vram[pc + 1]);
                break;
            }
            case 0xC5: {
                clock += 16;
                vram[--sp] = bc >> 8;
                vram[--sp] = bc & 0xff;
                ++pc;
                break;
            }
            case 0xD5: {
                clock += 16;
                vram[--sp] = de >> 8;
                vram[--sp] = de & 0xff;
                ++pc;
                break;
            }
            case 0xE5: {
                clock += 16;
                vram[--sp] = hl >> 8;
                vram[--sp] = hl & 0xff;
                ++pc;
                break;
            }
            case 0xF5: {
                clock += 16;
                vram[--sp] = af >> 8;
                vram[--sp] = af & 0xff;
                ++pc;
                break;
            }
            case 0x17: {
                clock += 4;
                u8 &reg = a;
                bool carryFlag = reg >> 7;
                reg = (reg << 1) | f.cy;
                f.zf = false;
                f.n = false;
                f.h = false;
                f.cy = carryFlag;
                ++pc;
                break;
            }
            case 0xC1:
            case 0xD1:
            case 0xE1: {
                u16 *regs[3] = {&bc, &de, &hl};
                u16 *reg = regs[(opcode >> 4) - 0xC];
                clock += 12;
                *reg = ((u16) (vram[sp + 1]) << 8) | vram[sp];
                sp += 2;
                ++pc;
                break;
            }
            case 0xF1: {
                clock += 12;
                af = ((u16) (vram[sp + 1]) << 8) | vram[sp];
                sp += 2;
                ++pc;
                break;
            }
            case 0x22: {
                clock += 8;
                vram[hl++] = a;
                ++pc;
                break;
            }
            case 0xC9: {
                clock += 16;
                pc = ((u16) vram[sp + 1] << 8) | vram[sp];
                sp += 2;
                break;
            }
            case 0xBE: {
                clock += 8;
                u8 data = vram[hl];
                bool result = a - data;
                f.zf = result == 0;
                f.n = true;
                f.h = (a & 0xf) < (data & 0xf);
                f.cy = a < data;
                ++pc;
                break;
            }
            case 0x86: {
                clock += 8;
                u8 data = vram[hl];
                u8 result = a + data;
                f.zf = result == 0;
                f.n = false;
                f.h = (0xf - (a & 0xf)) > (data & 0xf);
                f.cy = 0xff - a > data;
                a = result;
                ++pc;
                break;

            }
            case 0xEA: {
                u16 addr = ((u16) (vram[pc + 2]) << 8) | vram[pc + 1];
                vram[addr] = a;
                pc += 3;
                clock += 8;
                break;
            }
            case 0x28: {
                auto relJump = static_cast<::int8_t>(vram[pc + 1]);
                if (f.zf) {
                    pc += 2 + relJump;
                    clock += 12;
                } else {
                    pc += 2;
                    clock += 12;
                }
                break;
            }
            case 0x04:
            case 0x14:
            case 0x24: {
                u8 ix[3] = {3, 5, 7};
                u8 &reg = registers[ix[opcode >> 4]];
                f.zf = (reg + 1 == 0);
                f.n = false;
                f.h = ((reg & 0xf) == 0xf);
                ++reg;
                ++pc;
                clock += 4;
                break;
            }
            case 0xc3: {
                u16 updatedPC = ((u16) vram[pc + 2] << 8) | vram[pc + 1];
                clock += 16;
                pc = updatedPC;
                break;
            }
            case 0xF3: {
                ime = false;
                ++pc;
                clock += 4;
                break;
            }
            case 0x36: {
                vram[hl] = vram[pc + 1];
                pc += 2;
                clock += 12;
                break;
            }
            case 0x2a: {
                a = vram[hl++];
                ++pc;
                clock += 8;
                break;
            }
            case 0x0B: {
                --bc;
                ++pc;
                clock += 8;
                break;
            }
            case 0xFB: {
                ime = true;
                ++pc;
                clock += 4;
                break;
            }
            case 0xC0: {
                if (!f.zf) {
                    u16 lower = vram[sp++];
                    u16 upper = vram[sp++];
                    u16 updated = (upper << 8) | lower;
                    pc = updated;
                    clock += 20;
                } else {
                    ++pc;
                    clock += 8;
                }
                break;
            }
            case 0xFA: {
                u16 a16 = (u16 &) vram[pc + 1];
                a = vram[a16];
                pc += 3;
                clock += 16;
                break;
            }
            case 0xC8: {
                if (f.zf) {
                    u16 newAddr = (u16(vram[sp + 1]) << 8) | vram[sp];
                    sp += 2;
                    pc = newAddr;
                    clock += 20;
                } else {
                    ++pc;
                    clock += 8;
                }
                break;
            }
            case 0x34: {
                f.h = (vram[hl] & 0xf) == 0xf;
                u8 updatedVal = ++(vram[hl]);
                ++pc;
                clock += 12;
                f.zf = updatedVal == 0;
                f.n = false;
                break;
            }
            case 0xD9: {
                u16 updatedAddr = (u16(vram[sp + 1]) << 8) | vram[sp];
                sp += 2;
                ime = true;
                pc = updatedAddr;
                clock += 16;
                break;
            }
            case 0x2F: {
                a = ~a;
                ++pc;
                f.n = true;
                f.h = true;
                clock += 4;
                break;
            }
            case 0xE6: {
                u8 d8 = vram[pc + 1];
                a = a & d8;
                f.zf = a == 0;
                f.n = false;
                f.h = true;
                f.cy = false;
                pc += 2;
                clock += 8;

                break;
            }
            case 0xB0:
            case 0xB1:
            case 0xB2:
            case 0xB3:
            case 0xB4:
            case 0xB5:
            case 0xB7: {
                u8 &reg = registers[r[opcode & 0xF]];
                a = a | reg;
                f.zf = a == 0;
                f.n = false;
                f.h = false;
                f.cy = false;
                ++pc;
                clock += 4;
                break;
            }
            case 0xCF:
            case 0xDF:
            case 0xEF:
            case 0xFF: {
                array<u8, 4> addrs = {0x08, 0x18, 0x28, 0x38};
                u16 jmpAddr = addrs[(opcode >> 4) - 0xC];
                u8 msb = (pc + 1) >> 8;
                u8 lsb = (pc + 1) & 0xFF;
                vram[--sp] = msb;
                vram[--sp] = lsb;
                pc = jmpAddr;
                clock += 16;
                break;
            }
            case 0x09:
            case 0x19:
            case 0x29:
            case 0x39: {
                array<u16 *, 4> regs = {&bc, &de, &hl, &sp};
                u16 reg = *regs[opcode >> 4];
                ++pc;
                clock += 8;
                f.n = false;
                f.h = (hl & 0xFF) >= 0xFF - (reg & 0xFF);
                f.cy = hl >= 0xFFFF - reg;
                hl += reg;
                break;
            }
            case 0x0: {
                pc += 1;
                clock += 4;
                break;
            }

            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x87: {
                clock += 8;
                u8 &reg = registers[r[opcode & 0xF]];
                u8 data = reg;
                u8 result = a + data;
                f.zf = result == 0;
                f.n = false;
                f.h = (0xf - (a & 0xf)) > (data & 0xf);
                f.cy = 0xff - a > data;
                a = result;
                ++pc;
                break;
            }
            case 0x4E:
            case 0x5E:
            case 0x6E:
            case 0x7E: {
                u8 *arr[4] = {&c, &e, &l, &a};
                u8 &reg = *arr[(opcode >> 4) - 4];
                reg = vram[hl];
                clock += 8;
                ++pc;
                break;
            }
            case 0x46:
            case 0x56:
            case 0x66: {
                u8 *arr[3] = {&b, &d, &h};
                u8 &reg = *arr[(opcode >> 4) - 4];
                reg = vram[hl];
                clock += 8;
                ++pc;
                break;
            }
            case 0xE9: {
                pc = hl;
                clock += 4;
                break;
            }
            case 0xF6: {
                a |= vram[pc + 1];
                pc += 2;
                clock += 8;
                f.zf = a == 0;
                f.n = false;
                f.h = false;
                f.cy = false;
                break;
            }
            case 0x12: {
                vram[de] = a;
                ++pc;
                clock += 8;
                break;
            }
            case 0xca: {
                if (f.zf) {
                    pc = (u16(vram[pc + 2]) << 8) | vram[pc + 1];
                    clock += 16;
                } else {
                    pc += 3;
                    clock += 12;
                }
                break;
            }
            case 0xFC: {
                break;
            }
            default: {
                printf("Opcode not implemented: [%x]", opcode);
                exit(1);
            }
        }
    }

};




class Timer {
public:

    u8 clock;
    vector<u8> &ram;
    u8 &div;
    u8 &tima;
    u8 &tma;
    u8 &tac;
    InterruptFlag &ifReg;

    Timer(vector<u8> &ram) : ram{ram},
                             div{ram[0xFF04]}, tima{ram[0xFF05]}, tma{ram[0xFF06]}, tac{ram[0xFF07]},
                             clock{0},
                             ifReg{*reinterpret_cast<InterruptFlag *>(ram[0xFF0F])} {
        tima = 0x00;
        tma = 0x00;
        tac = 0x00;

    }

    void run() {

        if (div != (clock >> 8)) {
            div = 0;
        } else {
            div = (clock + 4) >> 8;
        }

        clock += 4;
        int updateFreq = 1 << (12 + 2 * (3 - ((tac & 0x3) - 1) % 4));
        bool isInc = (clock & (updateFreq - 1)) == 0;

        if (tima == 0xFF && isInc) {
            tima = tma;
            ifReg.timer = true;
        }

    }

};

class gb_emu {
public:

    vector<u8> ram;
    PPU ppu;
    CPU cpu;
    AudioDriver ad;
    Timer timer;
    Joypad jp;
    InterruptFlag &ifReg;

    gb_emu(const string &bootROM, const string &cartridgeROM, vector<u8> &pixels) :
            ram(0x10000, 0), ppu{bootROM, cartridgeROM, pixels, ram}, cpu{ram},
            ad{ram}, timer{ram}, ifReg{*reinterpret_cast<InterruptFlag *>(&ram[0xFF0F])},
            jp{ram} {

    }

    void run(vector<sf::Event> &es) {

        // need to set the status registers:
#ifdef VERBOSE
        auto p1 = chrono::high_resolution_clock::now();
        uint64_t startingClock[3] = {ppu.clock, cpu.clock, ad.clock};
#endif
        runDevices(es);
        es.clear();
        for (int i = 0; i < PPU::PIXEL_ROWS; ++i) {
            uint64_t startClock = ppu.clock;
            ppu.ly = i;
            ppu.lcdStatus.coincidenceFlag = ppu.ly == ppu.lyc;

            if (ppu.lcdStatus.coincidenceFlag && ppu.lcdStatus.coincidenceInterrupt) {
                ifReg.lcdStat = true;
                runDevices(es);
            } else {
                ifReg.lcdStat = false;
            }

            // all following clockx %x %x cycles in 4MHz
            // should take 1/60th of a second at 1Mhz
            // must update lcdc status, and trigger interrupts

            ppu.lcdStatus.modeFlag = 2;
            if (ppu.lcdStatus.oamInterrupt) {
                ppu.oamInterrupt();
            }

            ppu.oamSearch();
            runDevices(es);

            ppu.lcdStatus.modeFlag = 3;
            ppu.pixelTransfer(i);
            runDevices(es);
            ppu.lcdStatus.modeFlag = 0;
            if (ppu.lcdStatus.hblankInterrupt) {
                ppu.hblankInterrupt();
            }
            ppu.hBlank();
            runDevices(es);

#ifndef DEBUG
            //            usleep(1e6 * (ppu.clock - startClock) / 4 / (1 << 20) / 2);
#endif
        }
        ppu.lcdStatus.modeFlag = 1;
        if (ppu.lcdStatus.vblankInterrupt) {
            ifReg.vBlank = true;
            runDevices(es);
        }

        for (int i = 0; i < 10; ++i) {
            ppu.vblankRow();
            ppu.ly = ppu.PIXEL_ROWS + i;
            runDevices(es);
        }
        ifReg.vBlank = false;

        if (cpu.clock > (1 << 22) && ppu.clock > (1 << 22) && ad.clock > (1 << 22)) {
            cpu.clock = cpu.clock & ((1 << 22) - 1);
            ppu.clock = ppu.clock & ((1 << 22) - 1);
            ad.clock = (ad.clock) & ((1 << 22) - 1);
        }

#ifdef VERBOSE
        auto p2 = chrono::high_resolution_clock::now();
        auto pd = p2 - p1;
        cout << "Screen Render Time taken: " << pd.count() / 1000000.0 << endl;
#endif

    }

    void runDevices(vector<sf::Event> &es) {
        while (cpu.clock <= ppu.clock || ad.clock <= ppu.clock) {
            if (timer.clock <= ppu.clock) {
                timer.run();
            }
            if (cpu.clock <= ppu.clock) {
                cpu.processInterrupts();
                cpu.fetchDecodeExecute();
            }
            if (ad.clock <= ppu.clock) {
                ad.run(cpu.clock);
            }
            if (ppu.dma != 0) {
                ppu.dmaTransfer(); // should take 160 microseconds of 600 cycles
                ppu.dma = 0;
            }
            if (!es.empty()) {
                jp.processKeyEvents(es);
            }
        }
    }
};

int main() {

    printf("Starting\n");

    constexpr uint32_t RANDOM_GEN_SEED = 0x7645387a;

    srand(RANDOM_GEN_SEED);

    sf::RenderWindow w{sf::VideoMode(PPU::DEVICE_WIDTH, PPU::DEVICE_HEIGHT), "Test", sf::Style::Default};

    vector<sf::Uint8> pixels(PPU::DEVICE_WIDTH * PPU::DEVICE_HEIGHT * 4, 0);


    sf::Texture texture;
    if (!texture.create(PPU::DEVICE_WIDTH, PPU::DEVICE_HEIGHT)) {
        cerr << "Could not create texture, quitting." << endl;
        exit(1);
    }


    vector<sf::Event> events;
    events.reserve(100);

    sf::Sprite sprite;
    sprite.setTexture(texture);

    const string base_dir = "/home/jc/CLionProjects/gb_emulator/";
    const string bootROM = base_dir + "DMG_ROM.bin";
    const string cartridgeROM = base_dir + "gameboy/tetris.gb";

    if(!std::filesystem::exists(base_dir)) {
        cerr << "Base dir does not exist [" << base_dir << "]" << endl;
    }
    if(!std::filesystem::exists(bootROM)) {
        cerr << "Boot rom does not exist [" << bootROM << "]" << endl;
    }
    if(!std::filesystem::exists(cartridgeROM)) {
        cerr << "Cartridge ROM does not exist [" << cartridgeROM << "]" << endl;
    }
    gb_emu emu{bootROM, cartridgeROM, pixels};
//    gb_emu emu{"/home/jc/projects/cpp/emulators-cpp/gameboy/PokemonReg.gb", pixels};

    int instructionCount = 0;

    while (w.isOpen()) {
        sf::Event e{};

        while (w.pollEvent(e)) {
            if (e.type == sf::Event::EventType::Closed) {
                w.close();
            }
            events.push_back(e);
        }

        w.clear(sf::Color::Black);

        ++instructionCount;

        emu.run(events);

        texture.update(&pixels[0]);
        w.draw(sprite);
        w.display();

    }

}



/*
 *
 * >>> resp = r.get("https://raw.githubusercontent.com/lmmendes/game-boy-opcodes/master/opcodes.json")

 *json.loads(resp.content)
 *
 * >>> for c in cs:
...   if 'case ' in c and "0xCB" not in c:
...     x = js["unprefixed"][c.split("case ")[1].split(":")[0].lower()]["cycles"]
...     c += "// " + str(x)
...   outputLines.append(c)

 */


