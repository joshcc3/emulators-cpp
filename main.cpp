#include <iostream>
#include "chip8/chip8.h"
#include <cstdint>
#include <bitset>
#include <vector>
#include <bit>
#include <unistd.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <math.h>
#include <SFML/Audio.hpp>
#include <AL/alc.h>

using namespace std;


int main() {
    cout << '\a' << endl;
    constexpr uint32_t RANDOM_GEN_SEED = 0x7645387a;
    constexpr int WIDTH = 640;
    constexpr int HEIGHT = 320;

    srand(RANDOM_GEN_SEED);

    sf::RenderWindow w{sf::VideoMode(WIDTH, HEIGHT), "Test", sf::Style::Default};

    sf::Uint8 pixels[WIDTH * HEIGHT * 4];

//    chip8 emu{"C:\\Users\\jerem\\CLionProjects\\gba_emulator\\ibm_logo.ch8", HEIGHT, WIDTH, pixels};
//    chip8 emu{"/home/jc/projects/cpp/emulators-cpp/tetris.ch8", HEIGHT, WIDTH, pixels};
    chip8 emu{"/home/jc/projects/cpp/emulators-cpp/trip.ch8", HEIGHT, WIDTH, pixels};


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

    constexpr int INSTRUCTIONS_PER_SECOND = 600;

    while (w.isOpen()) {

        sf::Event e{};
        while (w.pollEvent(e)) {
            if (e.type == sf::Event::EventType::Closed) {
                w.close();
            }
            events.push_back(e);
        }
        w.clear(sf::Color::Black);

        emu.processKeyboardEvents(events);
        uint16_t instr = emu.fetch();
        emu.decodeAndExecute(instr);
//        emu.draw();

        ++instructionCount;
        if(instructionCount % (INSTRUCTIONS_PER_SECOND/60) == 0) {
            usleep(1000000/60);
            emu.updateTimers();
        }

        texture.update(pixels);
        w.draw(sprite);
        w.display();
    }


}
