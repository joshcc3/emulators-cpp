//
// Created by jc on 24/09/23.
//

#define DEBUG
//#define VERBOSE

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


class gb_emu {
public:

    MemoryRef ram;
    PPU ppu;
    CPU cpu;
    AudioDriver ad;
    Timer timer;
    Joypad jp;
    InterruptFlag &ifReg;

    gb_emu(vector<u8> &pixels, MemoryRef ram) :
            ram{ram}, ppu{pixels, ram}, cpu{ram},
            ad{MUT(ram)}, timer{ram}, ifReg{*reinterpret_cast<InterruptFlag *>(&MUT8(ram[0xFF0F]))},
            jp{ram} {}

    void run(vector<sf::Event> &es) {

        // need to set the status registers:
#ifdef VERBOSE
        auto p1 = chrono::high_resolution_clock::now();
        uint64_t startingClock[3] = {ppu.clock, cpu.clock, ad.clock};
#endif
        jp.processKeyEvents(es);
        runDevices();
        for (int i = 0; i < PPU::PIXEL_ROWS; ++i) {
            uint64_t startClock = ppu.clock;
            ppu.ly = i;
            ppu.lcdStatus.coincidenceFlag = ppu.ly == ppu.lyc;

            ifReg.lcdStat = ppu.lcdStatus.coincidenceFlag && ppu.lcdStatus.coincidenceInterrupt;

            // all following clockx %x %x cycles in 4MHz
            // should take 1/60th of a second at 1Mhz
            // must update lcdc status, and trigger interrupts

            ppu.lcdStatus.modeFlag = 2;
            cpu.ifReg.lcdStat = ppu.lcdStatus.oamInterrupt;
            ppu.oamSearch();
            runDevices();

            ppu.lcdStatus.modeFlag = 3;
            ppu.pixelTransfer(i);
            runDevices();

            ppu.lcdStatus.modeFlag = 0;
            cpu.ifReg.lcdStat = ppu.lcdStatus.hblankInterrupt;
            ppu.hBlank();
            runDevices();
#ifndef DEBUG

            // usleep(1e6 * (ppu.clock - startClock) / 4 / (1 << 20) / 2);

#endif
        }

        ppu.lcdStatus.modeFlag = 1;
        cpu.ifReg.vBlank = true;
        cpu.ifReg.lcdStat = ppu.lcdStatus.vblankInterrupt;

        for (int i = 0; i < 10; ++i) {
            ppu.vblankRow();
            ppu.ly = ppu.PIXEL_ROWS + i;
            runDevices();
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

    void runDevices() {

        while (cpu.clock <= ppu.clock || ad.clock <= ppu.clock) {
            jp.refresh();

            if (timer.clock <= ppu.clock) {
                timer.run();
            }
            if (cpu.clock <= ppu.clock) {
                cpu.processInterrupts();
                MUT(ram).flushWrites();
                cpu.fetchDecodeExecute();
                MUT(ram).flushWrites();
            }
            if (ad.clock <= ppu.clock) {
                ad.run(cpu.clock);
            }
            if (ppu.dma != 0) {
                ppu.dmaTransfer(); // should take 160 microseconds of 600 cycles
                ppu.dma = 0;
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


    MBC ram(bootROM, cartridgeROM);
    gb_emu emu{pixels, ram};
//    gb_emu emu{"/home/jc/projects/cpp/emulators-cpp/gameboy/PokemonReg.gb", pixels};

    int instructionCount = 0;

    while (w.isOpen()) {
        sf::Event e{};
        events.clear();
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


