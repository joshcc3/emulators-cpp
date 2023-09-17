#include <iostream>

#include "chip8.h"
#include <cstdint>
#include <bitset>
#include <vector>
#include <bit>

using namespace std;


int main() {

    chip8 emu{"C:\\Users\\jerem\\CLionProjects\\gba_emulator\\ibm_logo.ch8"};
    string footer(64, '-');
    while (true) {
        uint16_t instr = emu.fetch();
        emu.decodeAndExecute(instr);
        cout << footer << endl;
    }

}
