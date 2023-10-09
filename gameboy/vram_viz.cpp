//
// Created by jc on 09/10/23.
//


#include <algorithm>
#include <iostream>
#include <filesystem>
#include <bitset>
#include <vector>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "audio_driver.h"

#include "structs.h"
#include "PPU.h"
#include "CPU.h"
#include "Joypad.h"
#include "Timer.h"

using namespace std;


class vram_viz {
public:

    MemoryRef ram;
    vector<u8> &pixels;

    vram_viz(vector<u8> &pixels, MemoryRef ram) :
            ram{ram}, pixels{pixels} {

    }

    void readRam() {
//        static void drawColorToScreen(std::vector<sf::Uint8> &pixels, int pixelX, int pixelY, const u8 *col);
//
//        static u16 getTileData(MemoryRef vram, int tile, int row, u16 addrStart);
//
//        static const u8 pixelColor[4][4];
//
//        static const u8 *getPixelColor(int pixelX, u16 color, u8 &reg, u8 &colorShade);
        u8 &bgpReg = ram[PPU::BGP_ADDR];
        for(int t = 0; t < 256; ++t) {
            for (int x = 0; x < 8; ++x) {
                for (int y = 0; y < 8; ++y) {
                    u16 color = PPU::getTileData(ram, t, y, 0x8000);
                    u8 colorShade;
                    const u8 *col = PPU::getPixelColor(x, color, bgpReg, colorShade);
                    int pixelX = t%16 * 8 + x;
                    int pixelY = t/16 * 8 + y;
                    PPU::drawColorToScreen(pixels, pixelX, pixelY, col);
                }
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

    if (!std::filesystem::exists(base_dir)) {
        cerr << "Base dir does not exist [" << base_dir << "]" << endl;
    }
    if (!std::filesystem::exists(bootROM)) {
        cerr << "Boot rom does not exist [" << bootROM << "]" << endl;
    }
    if (!std::filesystem::exists(cartridgeROM)) {
        cerr << "Cartridge ROM does not exist [" << cartridgeROM << "]" << endl;
    }


    Memory ram;
    ram.reserve(0x10000);

    vram_viz vz{pixels, ram};
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

        vz.readRam();

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


