//
// Created by jc on 09/10/23.
//

#ifndef GBA_EMULATOR_JOYPAD_H
#define GBA_EMULATOR_JOYPAD_H

#include "CPU.h"
#include "PPU.h"
#include "structs.h"
#include "audio_driver.h"
#include "../chip8/chip8.h"
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


class Joypad {
public:
    using Scancode = sf::Keyboard::Scancode;

    struct JoypadBit {
        u8 column: 4;
        u8 row: 4;
    };
    MemoryRef ram;
    u8 &jReg;
    InterruptFlag &ifReg;

    std::set<Scancode> interestKeys;
    std::map<Scancode, JoypadBit> keyMap;

    Joypad(MemoryRef ram) : ram{ram}, jReg{ram[0xFF00]},
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

#endif //GBA_EMULATOR_JOYPAD_H
