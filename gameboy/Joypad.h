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

    u8 dirPressed;
    u8 buttonsPressed;

    Joypad(MemoryRef ram) : ram{ram}, jReg{MUT8(ram[0xFF00])},
                            ifReg{*reinterpret_cast<InterruptFlag *>(&MUT8(ram[0xFF0F]))}, dirPressed{0xf}, buttonsPressed{0xf} {
        std::vector<Scancode> events = {Scancode::A, Scancode::B, Scancode::P, Scancode::L, Scancode::Left,
                                        Scancode::Right,
                                        Scancode::Up, Scancode::Down};

        jReg = 0;

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

    void refresh() {
        if(((jReg >> 4) & 1) == 0) {
            jReg = (jReg | 0x0f) & (0xf0 | dirPressed);
        } else if(((jReg >> 5) & 1) == 0) {
            jReg = (jReg | 0x0f) & (0xf0 | buttonsPressed);
        } else {
            jReg |= 0x0f;
        }
    }

    void processKeyEvents(const std::vector<sf::Event> &events) {
        for (auto event: events) {
            if ((event.type == KEYPRESS || event.type == KEYRELEASED) &&
                interestKeys.find(event.key.scancode) != interestKeys.end()) {
                auto key = keyMap[event.key.scancode];
                if (event.type == sf::Event::KeyPressed) {
                    if(key.column == 4) {
                        dirPressed &= ~(1 << key.row);
                    } else {
                        buttonsPressed &= ~(1 << key.row);
                    }
                } else {
                    if(key.column == 4) {
                        dirPressed |= (1 << key.row);
                    } else {
                        buttonsPressed |= (1 << key.row);
                    }
                }
                ifReg.joypad = true;
            }
        }
    }

};

#endif //GBA_EMULATOR_JOYPAD_H
