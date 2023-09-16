#include <iostream>

#include "chip8.h"
#include <cstdint>
#include <bitset>
#include <vector>

using namespace std;


int main() {


    chip8 emu{};
    while (true) {
        uint16_t instr = emu.fetch();
        emu.decodeAndExecute(instr);
        emu.draw();
    }
}
