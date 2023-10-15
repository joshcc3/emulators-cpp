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


struct DebugWindow {

    constexpr static int WIDTH = PPU::DEVICE_WIDTH * 2;
    constexpr static int HEIGHT = PPU::DEVICE_HEIGHT * 2.5;

    constexpr static pair<int, int> WINDOW_MAP = {0, 0};

    constexpr static pair<int, int> TILE_MAP = {0, PPU::DEVICE_HEIGHT / 2 + 10};

    constexpr static int CHARACTER_SIZE = 12;

    constexpr static tuple<int, int, int> OAM_MAP = {WIDTH / 2, PPU::DEVICE_HEIGHT / 2 + 10, CHARACTER_SIZE*1.3};
};

class vram_viz {
public:

    MemoryRef ram;
    vector<u8> &pixels;

    vram_viz(vector<u8> &pixels, MemoryRef ram) :
            ram{ram}, pixels{pixels} {

    }


    void bgMap() {
//        static void drawColorToScreen(std::vector<sf::Uint8> &pixels, int pixelX, int pixelY, const u8 *col);
//
//        static u16 getTileData(MemoryRef vram, int tile, int row, u16 addrStart);
//
//        static const u8 pixelColor[4][4];
//
//        static const u8 *getPixelColor(int pixelX, u16 color, u8 &reg, u8 &colorShade);
        fill(pixels.begin(), pixels.end(), 0);
        const u8 &bgpReg = ram[PPU::BGP_ADDR];
        for (int t = 0; t < 256; ++t) {
            for (int x = 0; x < 8; ++x) {
                for (int y = 0; y < 8; ++y) {
                    u16 color = PPU::getTileData(ram, t, y, 0x8000);
                    u8 colorShade;
                    const u8 *col = PPU::getPixelColor(x, color, bgpReg, colorShade);
                    int pixelX = (t % 16) * 8 + x;
                    int pixelY = (t / 16) * 8 + y;
                    PPU::drawColorToScreen(pixels, pixelX, pixelY, col);
                }
            }
        }
    }

    void tileMap(sf::RenderWindow &window, const sf::Font &f) {

        sf::Text text("", f);
        text.setFillColor(sf::Color::White);
        text.setStyle(sf::Text::Regular);
        text.setCharacterSize(DebugWindow::CHARACTER_SIZE);
        text.setPosition(DebugWindow::TILE_MAP.first, DebugWindow::TILE_MAP.second);

        int width = 5;
        char s[width*33];
        s[width*32] = 0;
        char *iter = s;
        for(int i = 0; i < 32; ++i) {
            if(i < 10) {
                std::sprintf(iter, "    %d", i);
            } else {
                std::sprintf(iter, "   %d", i);
            }
            iter += width;
        }
        string s_{s};
        drawString(window, text, 0, s_);

        for (int i = 0; i < 32; ++i) {
            char s[width * 33];
            s[width * 32] = 0;
            char *iter = s;

            for (int j = 0; j < 32; ++j) {
                int n = ram[0x9800 + i * 32 + j];
                if(n < 10) {
                    std::sprintf(iter, "    %d", n);
                } else if(n < 100) {
                    std::sprintf(iter, "   %d", n);
                } else {
                    std::sprintf(iter, "  %d", n);
                }
                iter += width;
            }
            string s_{s};
            drawString(window, text, i + 1, s_);
        }
    }

    void drawString(sf::RenderWindow &window, sf::Text &text, int i, const string &s_) const {
        text.setString(s_);
        text.setPosition(DebugWindow::TILE_MAP.first,
                         DebugWindow::TILE_MAP.second + DebugWindow::CHARACTER_SIZE * i);
        window.draw(text);
    }

    void oamMap(sf::RenderWindow &window, const sf::Font &f) {
        sf::Text text("", f);
        text.setFillColor(sf::Color::White);
        text.setStyle(sf::Text::Regular);
        text.setCharacterSize(get<2>(DebugWindow::OAM_MAP));
        text.setPosition(get<0>(DebugWindow::OAM_MAP), get<1>(DebugWindow::OAM_MAP));

        const OAMEntry *oamEntries{reinterpret_cast<const OAMEntry *>(&ram[PPU::OAM_ADDR_START])};

        int disp = 0;
        for (int i = 0; i < 40;) {
            const OAMEntry &e1 = oamEntries[i++];
            const OAMEntry &e2 = oamEntries[i++];
            stringstream s;
            if (e1.xPos > 0) {
                s << i << ": " << e1;
            }
            if (e2.xPos > 0) {
                s << "   |    " << e2;
            }
            if(!s.str().empty()) {
                text.setString(s.str());
                text.setPosition(get<0>(DebugWindow::OAM_MAP),
                                 get<1>(DebugWindow::OAM_MAP) + get<2>(DebugWindow::OAM_MAP) *  disp++);
                window.draw(text);
            }
        }
    }

};


int main() {

    printf("Starting\n");

    constexpr uint32_t RANDOM_GEN_SEED = 0x7645387a;

    srand(RANDOM_GEN_SEED);

    sf::RenderWindow w{sf::VideoMode(DebugWindow::WIDTH, DebugWindow::HEIGHT), "Test", sf::Style::Default};

    vector<sf::Uint8> pixels(DebugWindow::WIDTH * DebugWindow::HEIGHT * 4, 0);


    sf::Texture texture;
    if (!texture.create(DebugWindow::WIDTH, DebugWindow::HEIGHT)) {
        cerr << "Could not create texture, quitting." << endl;
        exit(1);
    }


    vector<sf::Event> events;
    events.reserve(100);

    sf::Sprite sprite;
    sprite.setTexture(texture);

    MBC ram;

    vram_viz vz{pixels, ram};

    int instructionCount = 0;

    sf::Font f;
//    if(!f.loadFromFile("/home/jc/CLionProjects/gb_emulator/fonts/Sarcastic.otf")) {
    if (!f.loadFromFile("/home/jc/CLionProjects/gb_emulator/fonts/OpenSans-Regular.ttf")) {
        cerr << "Unable to load font" << endl;
        exit(1);
    }

    while (w.isOpen()) {
        sf::Event e{};

        while (w.pollEvent(e)) {
            if (e.type == sf::Event::EventType::Closed) {
                w.close();
            }
            events.push_back(e);
        }

        ++instructionCount;

        w.clear(sf::Color::Black);

        vz.bgMap();
        texture.update(&pixels[0]);
        w.draw(sprite);

        vz.tileMap(w, f);
        vz.oamMap(w, f);

//        vz.tileMap(w, f);


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


