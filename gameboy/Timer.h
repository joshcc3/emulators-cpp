//
// Created by jc on 09/10/23.
//

#ifndef GBA_EMULATOR_TIMER_H
#define GBA_EMULATOR_TIMER_H

#include "Joypad.h"
#include "CPU.h"
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

class Timer {
public:

    u8 clock;
    std::vector<u8> &ram;
    u8 &div;
    u8 &tima;
    u8 &tma;
    u8 &tac;
    InterruptFlag &ifReg;

    Timer(std::vector<u8> &ram) : ram{ram},
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

#endif //GBA_EMULATOR_TIMER_H
