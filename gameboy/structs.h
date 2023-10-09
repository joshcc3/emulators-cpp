//
// Created by jc on 09/10/23.
//

#ifndef GBA_EMULATOR_STRUCTS_H
#define GBA_EMULATOR_STRUCTS_H

#include <cstdint>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using Memory = std::vector<u8>&;


constexpr int KEYPRESS = sf::Event::EventType::KeyPressed;
constexpr int KEYRELEASED = sf::Event::EventType::KeyReleased;



struct InterruptFlag {
    bool vBlank: 1;
    bool lcdStat: 1;
    bool timer: 1;
    bool serial: 1;
    bool joypad: 1;
    u8 _unused: 3;
};

struct InterruptEnable {
    bool vBlank: 1;
    bool lcdStat: 1;
    bool timer: 1;
    bool serial: 1;
    bool joypad: 1;
    u8 _unused: 3;
};

// jr_000_037e, jr_000_03e9, jr_000_045a, jr_000_06c8, jr_000_08a4, jr_000_1062, jr_000_1160, jr_000_12ad
//jr_000_13df, jr_000_147d, jr_000_1577, jr_000_160d, jr_000_1ae0, jr_000_2828
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
    u8 modeFlag: 2;
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

struct OAMFlags {
    u8 unused: 4;
    u8 palette: 1;
    u8 xFlip: 1;
    u8 yFlip: 1;
    u8 objToBGPrio: 1;
};
struct OAMEntry {
    u8 yPos;
    u8 xPos;
    u8 tileNumber;
    OAMFlags flags;
};

struct PixelColor {
    uint32_t priority;
    u16 shade;
    const u8 *color;

    PixelColor(u8 p, u16 s, const u8 *c) : priority{p}, shade{s}, color{c} {

    }
};

struct PixelComparator {
    bool operator()(const PixelColor &p1, const PixelColor &p2) {
        return p1.priority > p2.priority;
    }
};



#endif //GBA_EMULATOR_STRUCTS_H

