//
// Created by jc on 24/09/23.
//

#define PERF_DEBUG
//#define VERBOSE

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <bitset>
#include <vector>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <atomic>
#include <thread>

#include "audio_driver2.h"
#include "structs.h"
#include "PPU.h"
#include "CPU.h"
#include "Joypad.h"
#include "Timer.h"

using namespace std;


class gb_emu {
public:

    constexpr static int FREQ = (1 << 22);
    constexpr static int SPEEDUP = 1;
    constexpr static double TIME_QUANTUM = 1000000.0 / FREQ / SPEEDUP;

    MemoryRef ram;
    PPU ppu;
    CPU cpu;
    Timer timer;
    Joypad jp;
    InterruptFlag &ifReg;
    atomic<u64> &clockVar;

    gb_emu(vector<u8> &pixels, MemoryRef ram, atomic<u64> &clockVar) :
            ram{ram}, ppu{pixels, ram}, cpu{ram}, timer{MUT(ram)},
            ifReg{*reinterpret_cast<InterruptFlag *>(&MUT(ram)[0xFF0F])},
            jp{ram}, clockVar{clockVar} {}

    void run(vector<sf::Event> &es) {

        // need to set the status registers:
#ifdef VERBOSE
        auto p1 = chrono::high_resolution_clock::now();
        uint64_t startingClock[3] = {ppu.clock, cpu.clock, ad.clock};
#endif

#ifdef PERF_DEBUG
        static u64 counter = 0;
        static double rowDisplayTime = 0;
#endif
        jp.processKeyEvents(es);
        runDevices();
        for (int i = 0; i < PPU::PIXEL_ROWS; ++i) {
#ifdef PERF_DEBUG
            const chrono::time_point startTime = chrono::system_clock::now();
#endif
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

#ifdef PERF_DEBUG
            const chrono::time_point endTime = chrono::system_clock::now();
            rowDisplayTime = rowDisplayTime / 4.0 + 3 / 4.0 *
                                                    chrono::duration_cast<chrono::nanoseconds>(
                                                            endTime - startTime).count();
#endif
        }
#ifdef PERF_DEBUG
        ++counter;
        if ((counter & 0x1F) == 0) {
            cout << "-- RDT: [" << rowDisplayTime / 1000.0 << "us]" << endl;
        }
#endif


        ppu.lcdStatus.modeFlag = 1;
        cpu.ifReg.vBlank = true;
        cpu.ifReg.lcdStat = ppu.lcdStatus.vblankInterrupt;

        for (int i = 0; i < 10; ++i) {
            ppu.vblankRow();
            ppu.ly = ppu.PIXEL_ROWS + i;
            runDevices();
        }
        ifReg.vBlank = false;

#ifdef VERBOSE
        auto p2 = chrono::high_resolution_clock::now();
        auto pd = p2 - p1;
        cout << "Screen Render Time taken: " << pd.count() / 1000000.0 << endl;
#endif

    }

    void runDevices() {
#ifdef PERF_DEBUG
        static int counter = 0;
        static u64 runDeviceTime = 0;
#endif

        while (cpu.clock <= ppu.clock) {
#ifdef PERF_DEBUG
            const chrono::time_point start = chrono::system_clock::now();
#endif
            jp.refresh();

            if (timer.clock <= ppu.clock) {
                timer.run();
            }
            if (cpu.clock <= ppu.clock) {
                lock_guard lg(ram.memoryAccess);
                cpu.processInterrupts();
                MUT(ram).flushWrites();
                cpu.fetchDecodeExecute();
                MUT(ram).flushWrites();
            }
            if (ppu.dma != 0) {
                ppu.dmaTransfer(); // should take 160 microseconds of 600 cycles
                ppu.dma = 0;
            }

#ifdef PERF_DEBUG
            const chrono::time_point end = chrono::system_clock::now();
            u64 elapsedTime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
            if (runDeviceTime == 0) {
                runDeviceTime = elapsedTime;
            } else {
                runDeviceTime = runDeviceTime / 4 + elapsedTime * 3 / 4;
            }

#endif
        }
#ifdef PERF_DEBUG
        ++counter;
        if ((counter & 0x3FFF) == 0) {
            cout << "DRT: [" << runDeviceTime / 1000.0 << "us]" << endl;
        }
#endif

    }
};

void clockThread(std::atomic<u64> &clockVar, std::atomic_flag &isRunning) {
    while (isRunning.test(memory_order_relaxed)) {
        usleep(gb_emu::TIME_QUANTUM);
        clockVar.fetch_add(4, memory_order_seq_cst);
    }
}

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

    atomic<u64> clockVar(0);
    atomic_flag isRunning(true);

    MBC ram(bootROM, cartridgeROM);
    gb_emu emu{pixels, ram, clockVar};

    audio_driver2 audioDriver{MUT(ram), clockVar, isRunning};

    std::thread clockT([&clockVar, &isRunning]() { clockThread(clockVar, isRunning); });
    std::thread audioT([&audioDriver]() {
        audioDriver.run();
    });

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
    isRunning.clear(memory_order_relaxed);
    clockT.join();
    audioT.join();
}

/*
 *     vector<u8> data;
    data.reserve(0xFFFF0);
//    std::ifstream input("/home/jc/CLionProjects/gb_emulator/gameboy/PokemonRed.gb", std::ios::binary);
    {
        std::ifstream input("/home/jc/CLionProjects/gb_emulator/gameboy/streetFighter.gb", std::ios::binary);
        std::copy(std::istreambuf_iterator(input), {}, data.begin());

        dumpCartridgeHeader(data);

    }
    cout << "----------------------------" << endl;
    {
        std::ifstream input("/home/jc/CLionProjects/gb_emulator/gameboy/tetris.gb", std::ios::binary);
        std::copy(std::istreambuf_iterator(input), {}, data.begin());

        dumpCartridgeHeader(data);
    }

 */



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


