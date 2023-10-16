//
// Created by jc on 14/10/23.
//

#include "debug_code.h"
#include <iostream>
#include <vector>
#include <fstream>
#include "gameboy/debug_utils.h"

using namespace std;

class A {
public:
    A() {

    }

    bool b;
    void f() {
        b = false;
    }

    void f() const {
        const_cast<bool&>(b) = true;
    }

};


int main() {

    vector<u8> data;
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

    bool a = 1 - 1;
    cout << (a == 0) << endl;
}