//
// Created by jc on 24/09/23.
//

#ifndef GBA_EMULATOR_VIDEO_TEST_H
#define GBA_EMULATOR_VIDEO_TEST_H

#define DEBUG

#include <iostream>
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

using namespace std;

struct LCDControl {
    bool bgDisplayEnabled: 1;
    bool objSpriteDisplayEnable: 1;
    bool objSpriteSize: 1;
    bool bgTileMapDisplaySelect: 1;
    bool bgWindowTileDataSelect: 1;
    bool windowDispEnabled: 1;
    bool windowTileMapDisplaySelect: 1;
    bool lcdEnabled: 1;

};

struct LCDStatus {
    uint8_t modeFlag: 2;
    bool coincidenceFlag: 1;
    bool hblankInterrupt: 1;
    bool vblankInterrupt: 1;
    bool oamInterrupt: 1;
    bool coincidenceInterrupt: 1;
    bool unused: 1;
};

struct FlagReg {
    bool unused: 4;
    bool cy: 1;
    bool h: 1;
    bool n: 1;
    bool zf: 1;
};

class CPU {
public:
    uint8_t registers[8];
    uint8_t &a = registers[1];
    FlagReg &f = (FlagReg &) registers[0];
    uint8_t &b = registers[3];
    uint8_t &c = registers[2];
    uint8_t &d = registers[5];
    uint8_t &e = registers[4];
    uint8_t &h = registers[7];
    uint8_t &l = registers[6];
    uint16_t &af = (uint16_t &) registers;
    uint16_t &bc = (uint16_t &) (registers[2]);
    uint16_t &de = (uint16_t &) (registers[4]);
    uint16_t &hl = (uint16_t &) (registers[6]);

    uint16_t sp;
    uint16_t pc;

    uint64_t clock;

    vector<uint8_t> &vram;

    CPU(vector<uint8_t> &vram) : vram{vram} {
        initializeRegisters();
        clock = 0;
    }

    void initializeRegisters() {
        af = 0x01B0;
        bc = 0x0013;
        de = 0x00D8;
        hl = 0x014D;
        sp = 0xFFFE;
        pc = 0x0000;

    }

    uint32_t fetchDecodeExecute() {
        uint8_t r[8] = {3, 2, 5, 4, 7, 6, 255, 1};
        uint8_t opcode = vram[pc];
        switch (opcode) {

            case 0x01: // ld sp 0x
                clock += 12;
                bc = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x11:
                clock += 12;
                de = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x21:
                clock += 12;
                hl = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x31:
                clock += 12;
                sp = (uint16_t &) (vram[pc + 1]);
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

                        uint8_t &reg = registers[r[vram[pc + 1] - 0x10]];

                        bool carryFlag = reg >> 7;
                        reg = (reg << 1) | f.cy;
                        f.zf = reg == 0;
                        f.n = false;
                        f.h = false;
                        f.cy = carryFlag;
                        pc += 2;
                        break;
                    }
                    default:
                        throw "Not implemented";
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
                uint8_t d8 = vram[pc + 1];
                c = d8;
                pc += 2;
                break;
            }
            case 0x1E: {
                clock += 8;
                uint8_t d8 = vram[pc + 1];
                e = d8;
                pc += 2;
                break;
            }
            case 0x2E: {
                clock += 8;
                uint8_t d8 = vram[pc + 1];
                l = d8;
                pc += 2;
                break;
            }
            case 0x3E: {
                clock += 8;
                uint8_t d8 = vram[pc + 1];
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
                uint8_t m[4] = {2, 4, 6, 1};
                uint8_t &reg = registers[m[(opcode >> 4)]];
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
                uint8_t m[4] = {2, 4, 6, 1};
                uint8_t &reg = registers[m[(opcode >> 4)]];
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
                ++pc;
                break;
            }
            case 0xCD: {
                clock += 24;
                uint16_t jumpAddr = ((uint16_t) (vram[pc + 2]) << 8) | (vram[pc + 1]);
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
                uint8_t o[4] = {2, 4, 6, 1};
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
                uint8_t &opReg = registers[r[(opcode & 0xf)]];
                uint8_t result = a - opReg;
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
                uint8_t &reg = b;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x15: {
                clock += 4;
                uint8_t &reg = d;
                f.zf = (reg - 1) == 0;
                f.n = true;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x25: {
                clock += 4;
                uint8_t &reg = h;
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
                uint8_t &reg = a;
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
            case 0xE1:
            case 0xF1: {
                clock += 12;
                bc = ((uint16_t) (vram[sp + 1]) << 8) | vram[sp];
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
                pc = ((uint16_t) vram[sp + 1] << 8) | vram[sp];
                sp += 2;
                break;
            }
            case 0xBE: {
                clock += 8;
                uint8_t data = vram[hl];
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
                uint8_t data = vram[hl];
                uint8_t result = a + data;
                f.zf = result == 0;
                f.n = false;
                f.h = (0xf - (a & 0xf)) > (data & 0xf);
                f.cy = 0xff - a > data;
                a = result;
                ++pc;
                break;

            }
            case 0xEA: {
                uint16_t addr = ((uint16_t) (vram[pc + 2]) << 8) | vram[pc + 1];
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
                uint8_t ix[3] = {3, 5, 7};
                uint8_t &reg = registers[ix[opcode >> 4]];
                f.zf = (reg + 1 == 0);
                f.n = false;
                f.h = ((reg & 0xf) == 0xf);
                ++reg;
                ++pc;
                clock += 4;
                break;
            }
            default: {
                throw "Not implemented";
            }
        }
    }

};

class PPU {
public:
    constexpr static int PIXEL_COLUMNS = 160;
    constexpr static int PIXEL_ROWS = 144;

    constexpr static int DEVICE_RESOLUTION_X = 2;
    constexpr static int DEVICE_RESOLUTION_Y = 2;

    constexpr static int DEVICE_WIDTH = PIXEL_COLUMNS * DEVICE_RESOLUTION_X;
    constexpr static int DEVICE_HEIGHT = PIXEL_ROWS * DEVICE_RESOLUTION_Y;

    vector<sf::Uint8> &pixels;
    vector<uint8_t> &vram; // reserve 8KB

    uint8_t &scx;
    uint8_t &scy;
    uint8_t &ly;
    uint8_t &lyc;
    uint8_t &wx;
    uint8_t &wy;
    uint8_t &dma;
    uint8_t &bgp;
    uint8_t &obp0;
    uint8_t &obp1;

    LCDControl &lcdControl;
    LCDStatus &lcdStatus;

    uint64_t clock;

    PPU(const string &bootROM, vector<sf::Uint8> &pixels, vector<uint8_t> &ram)
            : pixels{pixels}, scx{vram[0xFF43]}, scy{vram[0xFF42]}, ly{vram[0xFF44]}, lyc{vram[0xFF45]},
              wx{vram[0xFF4B]}, wy{vram[0xFF4A]}, dma{vram[0xFF46]}, bgp{vram[0xFF47]},
              obp0{vram[0xFF48]}, obp1{vram[0xFF49]}, lcdControl{*reinterpret_cast<LCDControl *>(&vram[0xFF40])},
              lcdStatus{*reinterpret_cast<LCDStatus *>(&vram[0xFF41])},
              vram(ram), clock{0} {
#ifdef DEBUG
        debugInitializeCartridgeHeader();
#endif
        std::ifstream input(bootROM, std::ios::binary);
        std::copy(std::istreambuf_iterator(input), {}, vram.begin());
    }


    void pixelTransfer(int y) {
        // screen dimensions: 166 x 143 (166 wide and 143 long)

        // at ly flush to display and generate interrupt?
        // lyc is a counter that gets incremented. when ly == lyc then an interrupt is generated

        if (!lcdControl.lcdEnabled) {
//            draw all 0s to screen;
        } else if (lcdControl.windowDispEnabled) {

            throw "Not supporting window disp enabled yet";
            // wy;
            // wx;
            // wx = 7, wy = 0 is upper left

        } else if (lcdControl.bgDisplayEnabled) {

            for (int x = 0; x < PIXEL_COLUMNS; ++x) {
                int pixelY = ((y + scy) % 256);
                int pixelX = (x + scx) % 256;
                int tile = (pixelY / 8 * 32 + pixelX / 8);
                uint16_t color = getBackgroundTileMapDataRow(tile, y % 8);
                int c = (color >> (2 * (7 - pixelX % 8))) & 3;
                const uint8_t *outputColor = colorisePixel(bgp, c);
                drawColorToScreen(x, y, outputColor);
            }

        }
        clock += 172;

    }

    void drawColorToScreen(int pixelX, int pixelY, const uint8_t *col) {

        int deviceX = pixelX * DEVICE_RESOLUTION_X;
        int deviceY = pixelY * DEVICE_RESOLUTION_Y;
        int pixelStartIx = deviceY * DEVICE_WIDTH + deviceX;
        for (int dy = 0; dy < DEVICE_RESOLUTION_Y; ++dy) {
            for (int dx = 0; dx < DEVICE_RESOLUTION_X; ++dx) {
                int pixelIx = 4 * (pixelStartIx + dy * DEVICE_WIDTH + dx);
                pixels[pixelIx] = *col;
                pixels[pixelIx + 1] = *(col + 1);
                pixels[pixelIx + 2] = *(col + 2);
                pixels[pixelIx + 3] = *(col + 3);
            }
        }

    }

    const uint8_t pixelColor[4][4] = {{0xff, 0xff, 0xff, 0xff},
                                      {0xbb, 0xbb, 0xbb, 0xff},
                                      {0x88, 0x88, 0x88, 0xff},
                                      {0x00, 0x00, 0x00, 0xff}};

    [[nodiscard]] const uint8_t *colorisePixel(uint8_t reg, uint8_t pixel) const {
        // for some regs, 00 is transparent
        uint8_t pixelCode = (reg >> (2 * pixel)) & 3;
        auto x = pixelColor[pixelCode];
        return x;
    }

    constexpr static uint16_t OAM_ADDR_START = 0xFE00;

    void dmaTransfer() {
        // make sure code is executin gin hram and copying data only from HRAM.
        uint16_t startAddress = (uint16_t) dma << 8;
        uint16_t endAddress = startAddress | 0xa0;
        copy(vram.begin() + startAddress, vram.begin() + endAddress, vram.begin() + OAM_ADDR_START);
    }

    struct OAMFlags {
        uint8_t unused: 4;
        uint8_t palette: 1;
        uint8_t xFlip: 1;
        uint8_t yFlip: 1;
        uint8_t objToBGPrio: 1;
    };
    struct OAMEntry {
        uint8_t yPos;
        uint8_t xPos;
        uint8_t tileNumber;
        uint8_t flags;
    };

    OAMEntry accessOAMEntry(int ix) {
        // verify its readable or writable here - check the lcd stat register.
        uint8_t spriteDataStart = 0xFE00 + ix * 4;

        uint8_t yPos = vram[spriteDataStart];
        uint8_t xPos = vram[spriteDataStart + 1];
        uint8_t tileNumber = vram[spriteDataStart + 2];
        uint8_t flags = vram[spriteDataStart + 3];
        return OAMEntry{.yPos = yPos, .xPos = xPos, .tileNumber = tileNumber, .flags = flags};
    }

    uint16_t getTileData(int tile, int row, uint16_t addrStart) {

        int rowStride = 2;
        uint16_t rowStart = addrStart + tile * (rowStride * 8) + row * rowStride;
        uint16_t colorData = (((uint16_t) vram[rowStart + 1]) << 8) | vram[rowStart];

        return colorData;
    }

    uint16_t getBackgroundTileMapDataRow(int ix, int row) {
        assert(lcdControl.bgDisplayEnabled);
        if (lcdControl.bgWindowTileDataSelect) {
            uint8_t tile = lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix];
            return getTileData(tile, row, 0x8000);
        } else {
            auto tile = static_cast<int8_t>(lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix]);
            return getTileData(tile, row, 0x9000);
        }
    }

    void verticalTiming() {

        // oam search (20 clock), pixel search (43 clock), h-blank (51 clock), v-blank

        // oam search:, visible sprites,
        // DAM - copy 160clocks
        // send pixel to lcd every clock cycles:

        // fetch - background tile, data 0, data 1
        // to scroll, discard pixels
        //

    }

    void debugInitializeCartridgeHeader() {
        vram[0x100] = 0;
        vram[0x102] = 0;

        array<uint8_t, 56> nintendoHeaderBmp = {
                0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
                0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C
        };

        for (int i = 0; i < nintendoHeaderBmp.size(); ++i) {
            vram[0x104 + i] = nintendoHeaderBmp[i];
        }

        array<uint8_t, 16> title = {'J', 'O', 'S', 'H', 'B', 'O', 'Y', 'A', 'D', 'V', 'E', 'N', 'T', 'U', 'R', 'E'};
        for (int i = 0; i < title.size(); ++i) {
            vram[0x134 + i] = title[i];
        }

        vram[0x0147] = 0x0;
        vram[0x0148] = 0;
        vram[0x149] = 0;


    }


    [[nodiscard]] bool isPixelTransfer() const noexcept {
        return lcdStatus.modeFlag == 3;
    }

    [[nodiscard]] bool isHblank() const noexcept {
        return lcdStatus.modeFlag == 0;
    }

    [[nodiscard]] bool isVblank() const noexcept {
        return lcdStatus.modeFlag == 1;
    }

    bool vramAccessLegal() const {
        return !isPixelTransfer();
    }

    bool oamAccessLegal() const {
        return isHblank() || isVblank();
    }

    void oamInterrupt() {
        throw "Not implemeneted";
    }

    void coincidenceInterruptt() {
        throw "Not implemented";
    }

    void oamSearch() {
        clock += 80;
    }

    void hblankInterrupt() {
        throw "Not implemented";
    }

    void vBlankInterrupt() {
        throw "Not implemented";
    }


    void hBlank() {
        clock += 204;
    }

    void vblank() {
        clock += 1140;
    }
};

class gb_emu {
public:

    vector<uint8_t> ram;
    PPU ppu;
    CPU cpu;

    gb_emu(const string &bootRom, vector<uint8_t> &pixels) : ram(0x10000, 0), ppu{bootRom, pixels, ram}, cpu{ram} {}

    void run() {

        // need to set the status registers:
        for (int i = 0; i < PPU::PIXEL_ROWS; ++i) {
            ppu.ly = i;
            ppu.lcdStatus.coincidenceFlag = ppu.ly == ppu.lyc;

            if (ppu.lcdStatus.coincidenceFlag && ppu.lcdStatus.coincidenceInterrupt) {
                ppu.coincidenceInterruptt();
            }
            // all following clockx %x %x cycles in 4MHz
            // should take 1/60th of a second at 1Mhz

            // must update lcdc status, and trigger interrupts

            ppu.lcdStatus.modeFlag = 2;
            if (ppu.lcdStatus.oamInterrupt) {
                ppu.oamInterrupt();
            }


            ppu.oamSearch();
            while (cpu.clock <= ppu.clock) {
                cpu.fetchDecodeExecute();
            }

            ppu.lcdStatus.modeFlag = 3;
            ppu.pixelTransfer(i);
            while (cpu.clock <= ppu.clock) {
                cpu.fetchDecodeExecute();
            }

            ppu.lcdStatus.modeFlag = 0;
            if (ppu.lcdStatus.hblankInterrupt) {
                ppu.hblankInterrupt();
            }
            ppu.hBlank();
            while (cpu.clock <= ppu.clock) {
                cpu.fetchDecodeExecute();
            }
        }
        ppu.lcdStatus.modeFlag = 1;
        if (ppu.lcdStatus.vblankInterrupt) {
            ppu.vBlankInterrupt();
        }
        ppu.vblank();
        for (int i = 0; i < 10; ++i) {
            ppu.ly = ppu.PIXEL_ROWS + i;
            while (cpu.clock <= ppu.clock) {
                cpu.fetchDecodeExecute();
            }
        }

        if (cpu.clock > (1 << 22) && ppu.clock > (1 << 22)) {
            cpu.clock = cpu.clock & ((1 << 22) - 1);
            ppu.clock = ppu.clock & ((1 << 22) - 1);
        }


    }
};

int main() {

    printf("Starting\n");

    constexpr uint32_t RANDOM_GEN_SEED = 0x7645387a;

    srand(RANDOM_GEN_SEED);

    sf::RenderWindow w{sf::VideoMode(PPU::DEVICE_WIDTH, PPU::DEVICE_HEIGHT), "Test", sf::Style::Default};

    vector<sf::Uint8> pixels(PPU::DEVICE_WIDTH * PPU::DEVICE_HEIGHT * 4, 0);

    //    chip8 emu{"C:\\Users\\jerem\\CLionProjects\\gba_emulator\\ibm_logo.ch8", HEIGHT, WIDTH, pixels};
//    chip8 emu{"/home/jc/projects/cpp/emulators-cpp/tetris.ch8", HEIGHT, WIDTH, pixels};


    sf::Texture texture;
    if (!texture.create(PPU::DEVICE_WIDTH, PPU::DEVICE_HEIGHT)) {
        cerr << "Could not create texture, quitting." << endl;
        exit(1);
    }


    vector<sf::Event> events;
    events.reserve(100);

    sf::Sprite sprite;
    sprite.setTexture(texture);

    gb_emu emu{"/home/jc/projects/cpp/emulators-cpp/DMG_ROM.bin", pixels};

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

        emu.run();

        texture.update(&pixels[0]);
        w.draw(sprite);
        w.display();

    }

}


#endif //GBA_EMULATOR_VIDEO_TEST_H

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


