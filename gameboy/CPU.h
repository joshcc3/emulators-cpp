//
// Created by jc on 09/10/23.
//

#ifndef GBA_EMULATOR_CPU_H
#define GBA_EMULATOR_CPU_H

#include "PPU.h"
#include "structs.h"
#include "audio_driver.h"
#include <fstream>
#include <cassert>
#include <array>
#include <queue>
#include <SFML/Audio.hpp>
#include <math.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <unistd.h>
#include <bit>
#include <vector>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <algorithm>

class CPU {
public:

    /*
     *
0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
C000-CFFF   4KB Work RAM Bank 0 (WRAM)
D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
FE00-FE9F   Sprite Attribute Table (OAM)
FEA0-FEFF   Not Usable
FF00-FF7F   I/O Ports
FF80-FFFE   High RAM (HRAM)
FFFF        Interrupt Enable Register


     LCD & PPU enable: 0 = Off; 1 = On
Window tile map area: 0 = 9800–9BFF; 1 = 9C00–9FFF
Window enable: 0 = Off; 1 = On
BG & Window tile data area: 0 = 8800–97FF; 1 = 8000–8FFF
BG tile map area: 0 = 9800–9BFF; 1 = 9C00–9FFF
OBJ size: 0 = 8×8; 1 = 8×16
OBJ enable: 0 = Off; 1 = On
BG & Window enable / priority [Different meaning in CGB Mode]: 0 = Off; 1 = On

     */
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

    MemoryRef vram;

    bool ime;

    InterruptFlag &ifReg;
    InterruptEnable &ieReg;

    CPU(MemoryRef vram) : vram{vram}, clock{0}, ime{false},
                          ifReg{*reinterpret_cast<InterruptFlag *>(&MUT(vram)[0xFF0F])},
                          ieReg{*reinterpret_cast<InterruptEnable *>(&MUT(vram)[0xFFFF])} {
        initializeRegisters();

        _MBC &mbc = MUT(vram);

        mbc[0xFF40] = 0x91;
        mbc[0xFF42] = 0x00;
        mbc[0xFF43] = 0x00;
        mbc[0xFF45] = 0x00;
        mbc[0xFF47] = 0xFC;
        mbc[0xFF48] = 0xFF;
        mbc[0xFF49] = 0xFF;
        mbc[0xFF4A] = 0x00;
        mbc[0xFF4B] = 0x00;
        mbc[0xFFFF] = 0x00;
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
            // if reset
            u16 jumpAddr = 0x0;
            if (ifReg.vBlank && ieReg.vBlank) {
                ifReg.vBlank = false;
                jumpAddr = 0x40;
            } else if (ifReg.lcdStat && ieReg.lcdStat) {
                /*
                 *
                 *   Bit 5 - Mode 2 OAM Interrupt         (1=Enable) (Read/Write)
  Bit 4 - Mode 1 V-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 3 - Mode 0 H-Blank Interrupt     (1=Enable) (Read/Write)
  Bit 2 - Coincidence Flag  (0:LYC<>LY, 1:LYC=LY) (Read Only)
  Bit 1-0 - Mode Flag       (Mode 0-3, see below) (Read Only)
            0: During H-Blank
            1: During V-Blank
            2: During Searching OAM-RAM
            3: During Transfering Data to LCD Driver

The two lower STAT bits show the current status of the LCD controller.
  Mode 0: The LCD controller is in the H-Blank period and
          the CPU can access both the display RAM (8000h-9FFFh)
          and OAM (FE00h-FE9Fh)

  Mode 1: The LCD contoller is in the V-Blank period (or the
          display is disabled) and the CPU can access both the
          display RAM (8000h-9FFFh) and OAM (FE00h-FE9Fh)

  Mode 2: The LCD controller is reading from OAM memory.
          The CPU <cannot> access OAM memory (FE00h-FE9Fh)
          during this period.

  Mode 3: The LCD controller is reading from both OAM and VRAM,
          The CPU <cannot> access OAM and VRAM during this period.
          CGB Mode: Cannot access Palette Data (FF69,FF6B) either.

                 */
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
                ime = false;
                MUT(vram)[--sp] = pc >> 8;
                MUT(vram)[--sp] = pc & 0xff;
                pc = jumpAddr;
                clock += 20;
            } else {
                clock += 4;
            }
        }
    }

    // at instruction 0x26b - turned on devices and stuff
    void fetchDecodeExecute() {
        static int counter = 0;
        ++counter;
        u8 r[8] = {3, 2, 5, 4, 7, 6, 255, 1};

        /*
         * call stack at time of bug:
1a20
2ca
         */
// 12634626, 1a57
#ifdef DEBUG
        {
            int bufferSize = 100;
            static std::vector<u16> pcs(bufferSize, 0);
            static int bufferIx = 0;
            pcs[bufferIx] = pc;
            bufferIx = (bufferIx + 1) % bufferSize;
        }

        if(pc == 0x2b46) {

        }
#endif

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
                MUT(vram)[hl--] = a;
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
                        f.zf = (h >> 7) == 0;
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
                        u8 &reg = MUT(vram)[hl];
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
                        MUT(vram)[hl] |= (1 << ix);
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
                    case 0x27: {
                        u8 prevA = a;
                        a = a << 1;
                        f.zf = a == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = (prevA >> 7) == 0x1;

                        pc += 2;
                        clock += 8;
                        break;
                    }
                    case 0x40:
                    case 0x50:
                    case 0x60:
                    case 0x70: {
                        int shift = 2 * ((vram[pc + 1] >> 4) - 4);
                        pc += 2;
                        clock += 8;
                        f.zf = ((b >> shift) & 1) == 0;
                        f.n = false;
                        f.h = true;
                        break;
                    }
                    case 0x7E: {
                        pc += 2;
                        clock += 16;
                        f.zf = ((vram[hl] >> 7) & 1) == 0;
                        f.n = false;
                        f.h = true;
                        break;
                    }
                    case 0x86: {
                        MUT(vram)[hl] = vram[hl] & 0xFE;
                        pc += 2;
                        clock += 16;
                        break;
                    }
                    case 0x48:
                    case 0x58:
                    case 0x68:
                    case 0x78: {
                        int shift = 2 * ((vram[pc + 1] >> 4) - 4) + 1;
                        f.zf = ((b >> shift) & 1) == 0x0;
                        f.n = false;
                        f.h = true;
                        pc += 2;
                        clock += 8;
                        break;
                    }
                    case 0x3F: {
                        u8 prevA = a;
                        a = a >> 1;
                        pc += 2;
                        clock += 8;
                        f.zf = a == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = prevA & 1;
                        break;
                    }
                    case 0x4F:
                    case 0x5F:
                    case 0x6F:
                    case 0x7F: {
                        int shift = 2 * ((vram[pc + 1] >> 4) - 0x4) + 1;
                        f.zf = ((a >> shift) & 1) == 0;
                        f.n = false;
                        f.h = true;
                        pc += 2;
                        clock += 8;
                        break;
                    }
                    case 0x8E:
                    case 0x9E:
                    case 0xAE:
                    case 0xBE: {
                        u8 opcode = vram[pc + 1];
                        u8 shift = 2 * ((opcode >> 4) - 8) + 1;
                        u8 mask = ~(1 << shift);
                        MUT(vram)[hl] = vram[hl] & mask;
                        pc += 2;
                        clock += 16;
                        break;
                    }
                    case 0x41:
                    case 0x51:
                    case 0x61:
                    case 0x71: {
                        u8 opcode = vram[pc + 1];
                        int shift = 2*((opcode >> 4) - 4);
                        f.zf = ((c >> shift) & 1) == 0;
                        f.n = false;
                        f.h = true;
                        pc += 2;
                        clock += 8;
                        break;
                    }
                    case 0x49:
                    case 0x59:
                    case 0x69:
                    case 0x79: {
                        int shift = 2*((opcode >> 4) - 4) + 1;
                        f.zf = ((c >> shift) & 1) == 0;
                        f.n = false;
                        f.h = true;
                        pc += 2;
                        clock += 8;
                        break;
                    }
                    default:
                        printf("[%d] [%x] CB - Opcode not implemented: [%x]", counter, pc, vram[pc + 1]);
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
                MUT(vram)[0xFF00 + vram[pc + 1]] = a;
                pc += 2;
                break;
            }
            case 0xE2: {
                clock += 8;
                MUT(vram)[0xFF00 + c] = a;
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
                MUT(vram)[hl] = b;
                ++pc;
                break;
            }
            case 0x71: {
                clock += 8;
                MUT(vram)[hl] = c;
                ++pc;
                break;
            }
            case 0x72: {
                clock += 8;
                MUT(vram)[hl] = d;
                ++pc;
                break;
            }
            case 0x73: {
                clock += 8;
                MUT(vram)[hl] = e;
                ++pc;
                break;
            }
            case 0x74: {
                clock += 8;
                MUT(vram)[hl] = h;
                ++pc;
                break;
            }
            case 0x75: {
                clock += 8;
                MUT(vram)[hl] = l;
                ++pc;
                break;
            }
            case 0x77: {
                clock += 8;
                MUT(vram)[hl] = a;
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
                ++pc;
                break;
            }
            case 0xCD: {
                clock += 24;
                u16 jumpAddr = ((u16) (vram[pc + 2]) << 8) | (vram[pc + 1]);
                MUT(vram)[--sp] = (pc + 3) >> 8;
                MUT(vram)[--sp] = (pc + 3) & 0xff;
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
                MUT(vram)[--sp] = bc >> 8;
                MUT(vram)[--sp] = bc & 0xff;
                ++pc;
                break;
            }
            case 0xD5: {
                clock += 16;
                MUT(vram)[--sp] = de >> 8;
                MUT(vram)[--sp] = de & 0xff;
                ++pc;
                break;
            }
            case 0xE5: {
                clock += 16;
                MUT(vram)[--sp] = hl >> 8;
                MUT(vram)[--sp] = hl & 0xff;
                ++pc;
                break;
            }
            case 0xF5: {
                clock += 16;
                MUT(vram)[--sp] = af >> 8;
                MUT(vram)[--sp] = af & 0xff;
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
                MUT(vram)[hl++] = a;
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
                f.h = (0xf - (a & 0xf)) < (data & 0xf);
                f.cy = 0xff - a < data;
                a = result;
                ++pc;
                break;

            }
            case 0xEA: {
                u16 addr = ((u16) (vram[pc + 2]) << 8) | vram[pc + 1];
                MUT(vram)[addr] = a;
                pc += 3;
                clock += 8;
                break;
            }
            case 0x28: {
                auto relJump = static_cast<int8_t>(vram[pc + 1]);
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
                MUT(vram)[hl] = vram[pc + 1];
                pc += 2;
                clock += 12;
                break;
            }
            case 0x2A: {
                a = vram[hl++];
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
                u8 updatedVal = ++MUT(vram)[hl];
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
                std::array<u8, 4> addrs = {0x08, 0x18, 0x28, 0x38};
                u16 jmpAddr = addrs[(opcode >> 4) - 0xC];
                u8 msb = (pc + 1) >> 8;
                u8 lsb = (pc + 1) & 0xFF;
                MUT(vram)[--sp] = msb;
                MUT(vram)[--sp] = lsb;
                pc = jmpAddr;
                clock += 16;
                break;
            }
            case 0x09:
            case 0x19:
            case 0x29:
            case 0x39: {
                std::array<u16 *, 4> regs = {&bc, &de, &hl, &sp};
                u16 reg = *regs[opcode >> 4];
                ++pc;
                clock += 8;
                f.n = false;
                f.h = (hl & 0xFF) > 0xFF - (reg & 0xFF);
                f.cy = hl > 0xFFFF - reg;
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
                f.h = (0xf - (a & 0xf)) < (data & 0xf);
                f.cy = 0xff - a < data;
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
                MUT(vram)[de] = a;
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
            case 0x35: {

                f.h = (vram[hl] & 0xF) == 0x0;
                u8 updatedVal = --MUT(vram)[hl];
                ++pc;
                clock += 12;
                f.zf = updatedVal == 0;
                f.n = false;
                break;
            }
            case 0xC2: {
                //  jp   f,nn      xx nn nn 16;12 ---- conditional jump if nz,z,nc,c
                if (!f.zf) {
                    pc = (u16(vram[pc + 2]) << 8) | vram[pc + 1];
                    clock += 16;
                } else {
                    pc += 3;
                    clock += 12;
                }
                break;
            }
            case 0x3A: {
                a = vram[hl--];
                ++pc;
                clock += 8;
                break;
            }
            case 0xC6: {
                u8 prevA = a;
                a += vram[pc + 1];
                f.zf = a == 0;
                f.n = false;
                f.h = 0xF - (prevA & 0xF) < (vram[pc + 1] & 0xF);
                f.cy = 0xFF - prevA < vram[pc + 1];
                pc += 2;
                clock += 8;
                break;
            }
            case 0xD6: {
                u8 prevA = a;
                u8 d8 = vram[pc + 1];
                f.n = true;
                f.h = (prevA & 0xF) < (d8 & 0xF);
                f.cy = prevA < d8;
                a = a - d8;
                f.zf = a == 0;

                pc += 2;
                clock += 8;
                break;
            }
            case 0x07: {
                f.zf = false;
                f.n = false;
                f.h = false;
                u8 res = (a << 1) | (a >> 7);
                f.cy = (a >> 7);
                a = res;
                ++pc;
                clock += 4;
                break;
            }

            case 0x89: {
                u8 prevA = a;
                a = a + c + f.cy;
                f.zf = a == 0;
                f.n = false;
                f.h = ((a ^ c ^ prevA) & 0x10) != 0;
                f.cy = 0xFF - prevA < c;
                ++pc;
                clock += 4;
                break;
            }

            case 0xEE: {
                a = a ^ vram[pc + 1];
                f.zf = a == 0;
                f.n = false;
                f.h = false;
                f.cy = false;

                pc += 2;
                clock += 8;

                break;
            }
            case 0x96: {
                u8 hlVal = vram[hl];
                f.h = (a & 0xF) < (0xF & hlVal);
                f.cy = a < hlVal;

                a = a - hlVal;

                f.zf = a == 0;
                f.n = true;

                ++pc;
                clock += 8;
                break;
            }
            case 0x38: {
                if(f.cy) {
                    pc = pc + 2 + vram[pc + 1];
                    clock += 12;
                } else {
                    pc += 2;
                    clock += 8;
                }
                break;
            }
            case 0x0B:
            case 0x1B:
            case 0x2B:
            case 0x3B: {
                int ix = (opcode >> 4);
                u16* regs[4] = {&bc, &de, &hl, &sp};
                u16* reg = regs[ix];
                --(*reg);
                clock += 8;
                ++pc;
                break;
            }
            case 0x30: {
                u8 r8 = vram[pc + 1];
                if(!f.cy) {
                    u16 jumpAddr = pc + 2 + r8;
                    pc = jumpAddr;
                    clock += 12;
                } else {
                    pc += 2;
                    clock += 8;
                }
                break;
            }
            case 0xB8:
            case 0xB9:
            case 0xBA:
            case 0xBB:
            case 0xBC:
            case 0xBD:
            case 0xBF: {
                const u8& reg = registers[r[(opcode & 0xF) - 8]];
                f.zf = a == reg;
                f.n = true;
                f.h = (a & 0xF) < (reg & 0xF);
                f.cy = a < reg;
                ++pc;
                clock += 4;
                break;
            }
            case 0x27: {
                /*
                 *  /* When adding/subtracting two numbers in BCD form, this instructions
         * brings the results back to BCD form too. In BCD form the decimals 0-9
         * are encoded in a fixed number of bits (4). E.g., 0x93 actually means
         * 93 decimal. Adding/subtracting such numbers takes them out of this
         * form since they can results in values where each digit is >9.
         * E.g., 0x9 + 0x1 = 0xA, but should be 0x10. The important thing to
         * note here is that per 4 bits we 'skip' 6 values (0xA-0xF), and thus
         * by adding 0x6 we get: 0xA + 0x6 = 0x10, the correct answer. The same
         * works for the upper byte (add 0x60).
         * So: If the lower byte is >9, we need to add 0x6.
         * If the upper byte is >9, we need to add 0x60.
         * Furthermore, if we carried the lower part (HF, 0x9+0x9=0x12) we
         * should also add 0x6 (0x12+0x6=0x18).
         * Similarly for the upper byte (CF, 0x90+0x90=0x120, +0x60=0x180).
         *
         * For subtractions (we know it was a subtraction by looking at the NF
         * flag) we simiarly need to *subtract* 0x06/0x60/0x66 to again skip the
         * unused 6 values in each byte. The GB does this by only looking at the
         * NF and CF flags then.
         */
                /*
                 *         s8 add = 0;
        if ((!NF && (A & 0xf) > 0x9) || HF)
            add |= 0x6;
        if ((!NF && A > 0x99) || CF) {
            add |= 0x60;
            CF = 1;
        }
        A += NF ? -add : add;
        ZF = A == 0;
        HF = 0;
                 */
                u8 add = 0;
                if((!f.n && (a & 0xF)) || f.h) {
                    add |= 0x6;
                }
                if((!f.n && (a > 0x99)) || f.cy) {
                    add |= 0x60;
                    f.cy = 1;
                }
                a += f.n ? -add : add;
                f.zf = a == 0;
                f.h = false;
                ++pc;
                clock += 4;
                break;
            }
            case 0x8E: {
                const u8& reg = vram[hl];
                u16 res = u16(a) + reg + f.cy;
                f.zf = res == 0;
                f.n = false;
                f.h = (a ^ reg ^res) & 0x10  ? 1 : 0;
                f.cy = (res & 0x100) != 0;
                ++pc;
                clock += 4;
                break;
            }
            case 0x02: {
                MUT(vram)[bc] = a;
                ++pc;
                clock += 8;
                break;
            }
            case 0xD0: {
                if (!f.cy) {
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
            case 0xD8: {
                if (f.cy) {
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
            default: {
                printf("[%d] [%x] Opcode not implemented: [%x]", counter, pc, opcode);
                exit(1);
            }
        }
    }

};

#endif //GBA_EMULATOR_CPU_H
