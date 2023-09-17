#include <iostream>

#include "chip8.h"
#include <cstdint>
#include <bitset>
#include <vector>
#include <bit>


#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>


using namespace std;


int main() {

    constexpr int WIDTH = 640;
    constexpr int HEIGHT = 320;

    sf::RenderWindow w{sf::VideoMode(WIDTH, HEIGHT), "Test", sf::Style::Default};

    sf::Uint8 pixels[WIDTH * HEIGHT * 4];

    chip8 emu{"C:\\Users\\jerem\\CLionProjects\\gba_emulator\\ibm_logo.ch8", HEIGHT, WIDTH, pixels};


    sf::Texture texture;
    if (!texture.create(WIDTH, HEIGHT)) {
        cerr << "Could not create texture, quitting." << endl;
        exit(1);
    }

    sf::Sprite sprite;
    sprite.setTexture(texture);
    int instructionCount = 0;
    while (w.isOpen()) {

        sf::Event e{};
        while (w.pollEvent(e)) {
            if (e.type == sf::Event::EventType::Closed) {
                w.close();
            }
        }
        w.clear(sf::Color::Black);

        uint16_t instr = emu.fetch();
        emu.decodeAndExecute(instr);
//        emu.draw();

        texture.update(pixels);
        w.draw(sprite);
        w.display();
        ++instructionCount;
        if(instructionCount % 700 == 0) {
            _sleep(1000);
        }
    }


}
