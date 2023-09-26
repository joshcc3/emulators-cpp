//
// Created by jc on 24/09/23.
//

#ifndef GBA_EMULATOR_VIDEO_TEST_H
#define GBA_EMULATOR_VIDEO_TEST_H


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

using namespace std;

struct LCDControl {
    bool lcdEnabled: 1;
    bool windowTileMapDisplaySelect: 1;
    bool windowDispEnabled: 1;
    bool bgWindowTileDataSelect: 1;
    bool bgTileMapDisplaySelect: 1;
    bool objSpriteSize: 1;
    bool objSpriteDisplayEnable: 1;
    bool bgDisplayEnabled: 1;

};

struct LCDStatus {
    bool unusued: 1;
    bool coincidenceInterrupt: 1;
    bool oamInterrupt: 1;
    bool vblankInterrupt: 1;
    bool hblankInterrupt: 1;
    bool coincidenceFlag: 1;
    uint8_t modeFlag: 2;
};

struct FlagReg {
    bool zf: 1;
    bool n: 1;
    bool h: 1;
    bool cy: 1;
    bool unused: 4;
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
        uint8_t r[7] = {3, 2, 5, 4, 7, 6, 1};
        uint8_t opcode = vram[pc];
        switch (opcode) {
            case 0x01: // ld sp 0x
                bc = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;
            case 0x11:
                de = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;
            case 0x21:
                hl = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;
            case 0x31:
                sp = (uint16_t &) (vram[pc + 1]);
                pc += 3;
                break;

            case 0x32:
                vram[hl--] = a;
                ++pc;
                break;

            case 0x20:
                if (f.zf != 0) {
                    int8_t relativeOffset = vram[pc + 1];
                    pc = pc + relativeOffset;
                } else {
                    pc += 2;
                }
                break;
            case 0xCB:
                switch (vram[pc + 1]) {
                    case 0x7C:
                        f.zf = h >> 7;
                        f.h = 1;
                        pc += 2;
                        break;
                    case 0x10:
                    case 0x11:
                    case 0x12:
                    case 0x13:
                    case 0x14:
                    case 0x15:
                    case 0x17:
                        uint8_t &reg = r[opcode - 0x10];
                        bool carryFlag = reg >> 7;
                        reg = (reg << 1) | f.cy;
                        f.zf = reg == 0;
                        f.n = 0;
                        f.h = 0;
                        f.cy = carryFlag;
                        pc += 2;
                        break;
                }
                break;

            case 0xA0:
                a = a & b;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA1:
                a = a & c;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA2:
                a = a & d;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA3:
                a = a & e;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA4:
                a = a & h;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA5:
                a = a & l;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA6:
                a = a & vram[hl];
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA7:
                a = a & a;
                f.h = 1;
                f.zf = a == 0;
                ++pc;
                break;

            case 0xA8:
                a = a ^ b;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xA9:
                a = a ^ c;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAA:
                a = a ^ d;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAB:
                a = a ^ e;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAC:
                a = a ^ h;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAD:
                a = a ^ l;
                f.zf = a == 0;
                ++pc;
                break;
            case 0xAE:
                a = a ^ vram[hl];
                f.zf = a == 0;
                ++pc;
                break;

            case 0xAF:
                a = a ^ a;
                f.zf = a == 0;
                ++pc;
                break;

            case 0x0E: {
                uint8_t d8 = vram[pc + 1];
                c = d8;
                pc += 2;
                break;
            }
            case 0x1E: {
                uint8_t d8 = vram[pc + 1];
                e = d8;
                pc += 2;
                break;
            }
            case 0x2E: {
                uint8_t d8 = vram[pc + 1];
                l = d8;
                pc += 2;
                break;
            }
            case 0x3E: {
                uint8_t d8 = vram[pc + 1];
                a = d8;
                pc += 2;
                break;
            }
            case 0xE0: {
                vram[0xFF00 + vram[pc + 1]] = a;
                pc += 2;
                break;
            }

            case 0xE2: {
                vram[0xFF00 + c] = a;
                pc += 1;
                break;
            }
            case 0x0C: {
                f.zf = (c + 1) == 0;
                f.n = 0;
                f.h = (c & 0xf) == 0xf;
                f.cy = (c + 1) == 0;
                ++c;
                pc += 1;
                break;
            }
            case 0x70: {
                vram[hl] = b;
                ++pc;
                break;
            }
            case 0x71: {
                vram[hl] = c;
                ++pc;
                break;
            }
            case 0x72: {
                vram[hl] = d;
                ++pc;
                break;
            }
            case 0x73: {
                vram[hl] = e;
                ++pc;
                break;
            }
            case 0x74: {
                vram[hl] = h;
                ++pc;
                break;
            }
            case 0x75: {
                vram[hl] = l;
                ++pc;
                break;
            }
            case 0x77: {
                vram[hl] = a;
                ++pc;
                break;
            }

            case 0x0A: {
                a = vram[bc];
                ++pc;
                break;
            }
            case 0x1A: {
                a = vram[de];
                ++pc;
                break;
            }
            case 0xCD: {
                uint16_t jumpAddr = ((uint16_t) (vram[pc + 1]) << 8) | (vram[pc + 2]);
                vram[--sp] = pc >> 8;
                vram[--sp] = pc & 0xff;
                pc = jumpAddr;
                break;
            }

            case 0x03: {
                ++bc;
                ++pc;
                break;
            }
            case 0x13: {
                ++de;
                ++pc;
                break;
            }
            case 0x23: {
                ++hl;
                ++pc;
                break;
            }
            case 0x33: {
                ++sp;
                ++pc;
                break;
            }

            case 0x40: {
                b = b;
                ++pc;
                break;
            }
            case 0x41: {
                b = c;
                ++pc;
                break;
            }
            case 0x42: {
                b = d;
                ++pc;
                break;
            }
            case 0x43: {
                b = e;
                ++pc;
                break;
            }
            case 0x44: {
                b = h;
                ++pc;
                break;
            }
            case 0x45: {
                b = l;
                ++pc;
                break;
            }
            case 0x50: {
                d = b;
                ++pc;
                break;
            }
            case 0x51: {
                d = c;
                ++pc;
                break;
            }
            case 0x52: {
                d = d;
                ++pc;
                break;
            }
            case 0x53: {
                d = e;
                ++pc;
                break;
            }
            case 0x54: {
                d = h;
                ++pc;
                break;
            }
            case 0x55: {
                d = l;
                ++pc;
                break;
            }
            case 0x60: {
                h = b;
                ++pc;
                break;
            }
            case 0x61: {
                h = c;
                ++pc;
                break;
            }
            case 0x62: {
                h = d;
                ++pc;
                break;
            }
            case 0x63: {
                h = e;
                ++pc;
                break;
            }
            case 0x64: {
                h = h;
                ++pc;
                break;
            }
            case 0x65: {
                h = l;
                ++pc;
                break;
            }
            case 0x47: {
                b = a;
                ++pc;
                break;
            }
            case 0x57: {
                d = a;
                ++pc;
                break;
            }
            case 0x67: {
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
                uint8_t o[4] = {2, 4, 6, 1};
                registers[o[(opcode >> 4) - 4]] = registers[r[opcode - 0x48]];
                ++pc;
                break;
            }

            case 0xFE: {
                int res = a - vram[pc + 1];
                f.zf = res == 0;
                f.n = 1;
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
                uint8_t r[7] = {3, 2, 5, 4, 7, 6, 1};

                uint8_t &opReg = registers[r[opcode - 0x48]];
                uint8_t result = a - opReg;

                f.zf = a == 0;
                f.n = 1;
                f.h = (a & 0xf) < (opReg & 0xf);
                f.cy = a < opReg;
                a = result;
                ++pc;
                break;
            }

            case 0xF0: {
                a = vram[0xFF00 | (pc + 1)];
                pc += 2;
                break;
            }
            case 0x05: {
                uint8_t &reg = b;
                f.zf = (reg - 1) == 0;
                f.n = 1;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x15: {
                uint8_t &reg = d;
                f.zf = (reg - 1) == 0;
                f.n = 1;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }
            case 0x25: {
                uint8_t &reg = h;
                f.zf = (reg - 1) == 0;
                f.n = 1;
                f.h = (reg & 0xf) == 0;
                --reg;
                ++pc;
                break;
            }

            case 0x06: {
                b = vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x16: {
                d = vram[pc + 1];
                pc += 2;
                break;
            }
            case 0x26: {
                h = vram[pc + 1];
                pc += 2;
                break;
            }

            case 0x18: {
                pc = pc + (int8_t) (vram[pc + 1]);
                break;
            }

            case 0xC5: {
                vram[--sp] = bc >> 8;
                vram[--sp] = bc & 0xff;
                ++pc;
                break;
            }
            case 0xD5: {
                vram[--sp] = de >> 8;
                vram[--sp] = de & 0xff;
                ++pc;
                break;
            }
            case 0xE5: {
                vram[--sp] = hl >> 8;
                vram[--sp] = hl & 0xff;
                ++pc;
                break;
            }
            case 0xF5: {
                vram[--sp] = af >> 8;
                vram[--sp] = af & 0xff;
                ++pc;
                break;
            }
            case 0x17: {
                uint8_t &reg = a;
                bool carryFlag = reg >> 7;
                reg = (reg << 1) | f.cy;
                f.zf = 0;
                f.n = 0;
                f.h = 0;
                f.cy = carryFlag;
                pc += 2;
                break;

            }
            case 0xC1:
            case 0xD1:
            case 0xE1:
            case 0xF1: {
                bc = ((uint16_t) (vram[sp + 1]) << 8) | vram[sp];
                sp += 2;
                ++pc;
                break;
            }
            case 0x22: {
                vram[hl++] = a;
                ++pc;
                break;
            }
            case 0xC9: {
                pc = ((uint16_t) vram[sp + 1] << 8) | vram[sp];
                sp += 2;
                break;
            }
            case 0xBE: {
                uint8_t data = vram[hl];
                bool result = a - data;
                f.zf = result == 0;
                f.n = 1;
                f.h = (a & 0xf) < (data & 0xf);
                f.cy = a < data;
                ++pc;
                break;
            }
            case 0x86: {
                uint8_t data = vram[hl];
                uint8_t result = a + data;
                f.zf = result == 0;
                f.n = 0;
                f.h = (0xf - (a & 0xf)) > (data & 0xf);
                f.cy = 0xff - a > data;
                a = result;
                ++pc;
                break;
            }
        }
    }

};

class PPU {
public:
    constexpr static int PIXEL_COLUMNS = 160;
    constexpr static int PIXEL_ROWS = 144;

    constexpr static int DEVICE_RESOLUTION_X = 10;
    constexpr static int DEVICE_RESOLUTION_Y = 10;

    constexpr static int DEVICE_WIDTH = PIXEL_COLUMNS * DEVICE_RESOLUTION_X;

    vector<sf::Uint8>& pixels;
    vector<uint8_t> vram; // reserve 8KB

    uint8_t scx;
    uint8_t scy;
    uint8_t wx;
    uint8_t wy;
    uint8_t dma;
    uint8_t bgp;

    LCDControl lcdControl;
    LCDStatus lcdStatus;

    uint64_t clock;

    PPU(const string &bootROM, vector<sf::Uint8>& pixels): pixels{pixels} {
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
                int tile = (pixelY * 256 + pixelX) / 8;
                uint16_t color = getBackgroundTileMapDataRow(tile, y % 8);
                int c = (color >> (2 * (pixelX % 8))) & 3;
                const uint8_t *outputColor = colorisePixel(bgp, c);
                drawColorToScreen(pixelX, pixelY, outputColor);
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

    const uint8_t pixelColor[4][4] = {{0xff, 0xff, 0xff, 0x88},
                                      {0xbb, 0xbb, 0xbb, 0x88},
                                      {0x88, 0x88, 0x88, 0x88},
                                      {0x00, 0x00, 0x00, 0x88}};

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
        uint8_t objToBGPrio: 1;
        uint8_t yFlip: 1;
        uint8_t xFlip: 1;
        uint8_t palette: 1;
        uint8_t unused: 4;
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

    uint16_t getBackgroundTileMapDataRow(uint8_t ix, int row) {
        assert(lcdControl.bgDisplayEnabled);
        if (lcdControl.bgWindowTileDataSelect) {
            uint8_t tile = lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix];
            return getTileData(tile, row, 0x8000);
        } else {
            int8_t tile = lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix];
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

        array<uint8_t, 48> nintendoHeaderBmp = {
                0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
                0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
                0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
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


    bool isPixelTransfer() {
        return lcdStatus.modeFlag == 3;
    }

    bool isHblank() {
        return lcdStatus.modeFlag == 0;
    }

    bool isVblank() {
        return lcdStatus.modeFlag == 1;
    }

    bool vramAccessLegal() {
        return !isPixelTransfer();
    }

    bool oamAccessLegal() {
        return isHblank() || isVblank();
    }

    void oamInterrupt() {
        throw "Not implemeneted";
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
    PPU ppu;
    CPU cpu;

    gb_emu(const string &bootRom) : ppu{bootRom}, cpu{ppu.vram} {
    }

    void run() {
        uint32_t clockCount = 0;
        while (true) {
            // need to set the status registers:
            for (int i = 0; i < ppu.PIXEL_ROWS; ++i) {
                // all following clock cycles in 4MHz
                // should take 1/60th of a second at 1Mhz

                // must update lcdc status, and trigger interrupts

                ppu.lcdStatus.modeFlag = 2;
                if (ppu.lcdStatus.oamInterrupt) {
                    ppu.oamInterrupt();
                }


                ppu.oamSearch();
                while(cpu.clock < ppu.clock) {
                    cpu.fetchDecodeExecute();
                }

                ppu.lcdStatus.modeFlag = 3;
                ppu.pixelTransfer(i);
                while(cpu.clock < ppu.clock) {
                    cpu.fetchDecodeExecute();
                }

                ppu.lcdStatus.modeFlag = 0;
                if (ppu.lcdStatus.hblankInterrupt) {
                    ppu.hblankInterrupt();
                }
                ppu.hBlank();
                while(cpu.clock < ppu.clock) {
                    cpu.fetchDecodeExecute();
                }
            }
            ppu.lcdStatus.modeFlag = 1;
            if (ppu.lcdStatus.vblankInterrupt) {
                ppu.vBlankInterrupt();
            }
            ppu.vblank();
            while(cpu.clock < ppu.clock) {
                cpu.fetchDecodeExecute();
            }

        }
    }
};

int main() {

    cout << '\a' << endl;
    constexpr uint32_t RANDOM_GEN_SEED = 0x7645387a;
    constexpr int WIDTH = 640;
    constexpr int HEIGHT = 320;

    srand(RANDOM_GEN_SEED);

    sf::RenderWindow w{sf::VideoMode(WIDTH, HEIGHT), "Test", sf::Style::Default};

    vector<sf::Uint8> pixels(WIDTH * HEIGHT * 4, 0);

    //    chip8 emu{"C:\\Users\\jerem\\CLionProjects\\gba_emulator\\ibm_logo.ch8", HEIGHT, WIDTH, pixels};
//    chip8 emu{"/home/jc/projects/cpp/emulators-cpp/tetris.ch8", HEIGHT, WIDTH, pixels};


    sf::Texture texture;
    if (!texture.create(WIDTH, HEIGHT)) {
        cerr << "Could not create texture, quitting." << endl;
        exit(1);
    }

    vector<sf::Event> events;
    events.reserve(100);

    sf::Sprite sprite;
    sprite.setTexture(texture);

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



