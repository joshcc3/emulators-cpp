//
// Created by jc on 09/10/23.
//

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
#include <cmath>
#include <SFML/Audio.hpp>
#include <queue>
#include <array>
#include <cassert>
#include <fstream>
#include <unistd.h>
#include "audio_driver.h"
#include "structs.h"


#ifndef GBA_EMULATOR_PPU_H
#define GBA_EMULATOR_PPU_H

class PPU {
public:
    constexpr static int PIXEL_COLUMNS = 160;
    constexpr static int PIXEL_ROWS = 144;

    constexpr static int DEVICE_RESOLUTION_X = 3;
    constexpr static int DEVICE_RESOLUTION_Y = 3;

    constexpr static int DEVICE_WIDTH = PIXEL_COLUMNS * DEVICE_RESOLUTION_X;
    constexpr static int DEVICE_HEIGHT = PIXEL_ROWS * DEVICE_RESOLUTION_Y;

    std::vector<sf::Uint8> &pixels;
    Memory vram; // reserve 8KB

    u8 &scx;
    u8 &scy;
    u8 &ly;
    u8 &lyc;
    u8 &wx;
    u8 &wy;
    u8 &dma;
    u8 &bgp;
    u8 &obp0;
    u8 &obp1;

    LCDControl &lcdControl;
    LCDStatus &lcdStatus;

    OAMEntry *oamEntries;

    uint64_t clock;

    PPU(const std::string &bootROM, const std::string &cartridgeROM, std::vector<sf::Uint8> &pixels, Memory &ram)
            : pixels{pixels}, scx{vram[0xFF43]}, scy{vram[0xFF42]}, ly{vram[0xFF44]}, lyc{vram[0xFF45]},
              wx{vram[0xFF4B]}, wy{vram[0xFF4A]}, dma{vram[0xFF46]}, bgp{vram[0xFF47]},
              obp0{vram[0xFF48]}, obp1{vram[0xFF49]}, lcdControl{*reinterpret_cast<LCDControl *>(&vram[0xFF40])},
              lcdStatus{*reinterpret_cast<LCDStatus *>(&vram[0xFF41])},
              vram(ram),
              oamEntries{reinterpret_cast<OAMEntry *>(&vram[OAM_ADDR_START])},
              clock{0} {

        debugInitializeCartridgeHeader();
        std::ifstream input(cartridgeROM, std::ios::binary);
        std::copy(std::istreambuf_iterator(input), {}, vram.begin());
//        std::ifstream input2(bootROM, std::ios::binary);
//        std::copy(std::istreambuf_iterator(input2), {}, vram.begin());
    }


    void pixelTransfer(int y) {

        // screen dimensions: 166 x 143 (166 wide and 143 long)

        // at ly flush to display and generate interrupt?
        // lyc is a counter that gets incremented. when ly == lyc then an interrupt is generated

        if (lcdControl.lcdEnabled) {


            std::vector<uint8_t> visibleSprites{};
            visibleSprites.reserve(10);

            for (int i = 0; i < 40 && visibleSprites.size() < 10; ++i) {
                OAMEntry &e = oamEntries[i];
                u8 spriteHeight = lcdControl.objSpriteSize ? 16 : 8;
                if (e.xPos != 0 && e.xPos < 168 && e.yPos < y + spriteHeight && e.yPos >= y) {
                    visibleSprites.push_back(i);
                }
            }
            std::vector<PixelColor> v;
            v.reserve(12);
            for (int x = 0; x < PIXEL_COLUMNS; ++x) {
                v.clear();

                if (lcdControl.bgDisplayEnabled) {

                    int pixelY = ((y + scy) % 256);
                    int pixelX = (x + scx) % 256;
                    int tile = (pixelY / 8 * 32 + pixelX / 8);

                    u16 color = getBackgroundTileMapDataRow(tile, pixelY % 8);
                    u8 colorShade;
                    const u8 *col = getPixelColor(pixelX, color, bgp, colorShade);
                    v.emplace_back(3 * colorShade + 1, colorShade, col);
                }

                if (lcdControl.windowDispEnabled) {

                    // wy;
                    // wx;
                    // wx = 7, wy = 0 is upper left

                    if (x >= wx && y >= wy) {

                        int pixelY = y - wy;
                        int pixelX = x - wx + 7;
                        int tile = (pixelY / 8 * 32 + pixelX / 8);

                        u16 color = getWindowTileMapDataRow(tile, pixelY % 8);
                        u8 colorShade;
                        auto col = getPixelColor(pixelX, color, bgp, colorShade);
                        v.emplace_back(10 * colorShade + 2, colorShade, col);
                    }
                }


                if (lcdControl.objSpriteDisplayEnable) {
                    u8 spriteHeight = lcdControl.objSpriteSize ? 16 : 8;
                    for (auto s: visibleSprites) {
                        OAMEntry &e = oamEntries[s];
                        if (e.xPos < x + 8 && x <= e.xPos) {
                            u8 row = !e.flags.yFlip ? y - e.yPos : spriteHeight - (y - e.yPos);

                            u16 color = getTileData((e.tileNumber & (~lcdControl.objSpriteSize)) + (row >> 3),
                                                    row % 8,
                                                    0x8000);
                            u8 colorShade;
                            u8 &palette = e.flags.palette == 0 ? obp0 : obp1;
                            u8 column = !e.flags.xFlip ? x - e.xPos : 8 - (x - e.xPos);
                            auto col = getPixelColor(column, color, palette, colorShade);
                            uint32_t priority;
                            if (colorShade == 0) {
                                priority = 0;
                            } else if (e.flags.objToBGPrio == 1) {
                                priority = 3;
                            } else {
                                priority = (((255 - x) << 8) | (255 - e.tileNumber));
                            }
                            v.emplace_back(priority, colorShade, col);
                        }
                    }
                }

                if (!v.empty()) {
                    auto visible = *min_element(v.begin(), v.end(), PixelComparator());
                    drawColorToScreen(x, y, visible.color);
                }

            }
        }
        clock += 172;

    }

    const u8 *getPixelColor(int pixelX, u16 color, u8 &reg, u8 &colorShade) const {
        const u8 *topColor;
        int xoffs = pixelX & 7;
        u8 upper = ((color >> 8) >> (7 - xoffs)) & 1;
        u8 lower = (color >> (7 - xoffs)) & 1;
        colorShade = upper * 2 + lower;
        const u8 *outputColor = colorisePixel(reg, colorShade);
        topColor = outputColor;
        return topColor;
    }

    void drawColorToScreen(int pixelX, int pixelY, const u8 *col) {

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

    const u8 pixelColor[4][4] = {{0xff, 0xff, 0xff, 0xff},
                                 {0xbb, 0xbb, 0xbb, 0xff},
                                 {0x88, 0x88, 0x88, 0xff},
                                 {0x00, 0x00, 0x00, 0xff}};

    [[nodiscard]] const u8 *colorisePixel(u8 reg, u8 pixel) const {
        // for some regs, 00 is transparent
        u8 pixelCode = (reg >> (2 * pixel)) & 3;
        auto x = pixelColor[pixelCode];
        return x;
    }

    constexpr static u16 OAM_ADDR_START = 0xFE00;

    void dmaTransfer() {
        // make sure code is executin gin hram and copying data only from HRAM.
        u16 startAddress = (u16) dma << 8;
        u16 endAddress = startAddress | 0xa0;
        copy(vram.begin() + startAddress, vram.begin() + endAddress, vram.begin() + OAM_ADDR_START);
    }


    u16 getTileData(int tile, int row, u16 addrStart) {

        int rowStride = 2;
        u16 rowStart = addrStart + tile * (rowStride * 8) + row * rowStride;
        u16 colorData = (((u16) vram[rowStart]) << 8) | vram[rowStart + 1];

        return colorData;
    }

    u16 getBackgroundTileMapDataRow(int ix, int row) {
        assert(lcdControl.bgDisplayEnabled);
        if (lcdControl.bgWindowTileDataSelect) {
            u8 tile = lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix];
            return getTileData(tile, row, 0x8000);
        } else {
            auto tile = static_cast<int8_t>(lcdControl.bgTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix]);
            return getTileData(tile, row, 0x9000);
        }
    }

    u16 getWindowTileMapDataRow(int ix, int row) {
        assert(lcdControl.windowDispEnabled);
        if (lcdControl.bgWindowTileDataSelect) {
            u8 tile = lcdControl.windowTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 + ix];
            return getTileData(tile, row, 0x8000);
        } else {
            auto tile = static_cast<int8_t>(lcdControl.windowTileMapDisplaySelect ? vram[0x9C00 + ix] : vram[0x9800 +
                                                                                                             ix]);
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
#ifdef VERBOSE
        dumpCartridgeHeader(vram);
#endif

//        vram[0x100] = 0;
//        vram[0x102] = 0;
//
//        array<u8, 56> nintendoHeaderBmp = {
//                0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
//                0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
//                0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
//                0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C
//        };
//
//        for (int i = 0; i < nintendoHeaderBmp.size(); ++i) {
//            vram[0x104 + i] = nintendoHeaderBmp[i];
//        }
//
//        array<u8, 16> title = {'J', 'O', 'S', 'H', 'B', 'O', 'Y', 'A', 'D', 'V', 'E', 'N', 'T', 'U', 'R', 'E'};
//        for (int i = 0; i < title.size(); ++i) {
//            vram[0x134 + i] = title[i];
//        }
//
//        vram[0x0147] = 0x0;
//        vram[0x0148] = 0;
//        vram[0x149] = 0;
//
//        assert(vram[0x0146] == 0);
//        std::cout << "Cartridge memory type: " << vram[0x147] << std::endl;
//        assert(vram[0x149] == 0);

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

    void vblankRow() {
        clock += 456;
    }
};

#endif //GBA_EMULATOR_PPU_H
