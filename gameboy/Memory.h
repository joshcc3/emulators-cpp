//
// Created by jc on 13/10/23.
//

#ifndef VRAM_VIZ_MEMORY_H
#define VRAM_VIZ_MEMORY_H

#include "ShmAlloc.h"
#include <vector>
#include <fstream>

#define DEBUG

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using vec = std::vector<u8, ShmAlloc<u8>>;


struct Write {
    const u16 addr;
    u8 writeVal;

    Write(u16 a) : addr(a), writeVal{0xFF} {}
};

class _MBC {
public:

    /*
     *
0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
C000-CFFF   4KB Work RAM Bank 0 (WRAM)
D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
FE00-FE9F   Sprite Attribute Table (OAM)
FEA0-FEFF   Not Usable
FF00-FF7F   I/O Ports
FF80-FFFE   High RAM (HRAM)
FFFF        Interrupt Enable Register
     *
     */

    bool ramEnabled;
    bool romMode;


    std::vector<Write> romWrites;

    vec data;

    u8 romBankIx;
    u8 *romBank0;
    std::vector<u8 *> romBankN;

    u8 *vram;

    u8 ramBankIx;
    std::vector<u8 *> ramBankN;

    u8 *remainingRam;


    template<typename T>
    T &findWritable(u16 addr) const {
        if (addr < 0x8000) {
            return romBankN[romBankIx][addr - 0x4000];
        } else if (addr < 0xA000) {
            return vram[addr - 0x8000];
        } else if (addr < 0xC000) {
            assert(ramEnabled);
            return ramBankN[ramBankIx][addr - 0xA000];
        } else {
            return remainingRam[addr - 0xC000];
        }
    }


public:

    _MBC() :
            romBankN(128, nullptr),
            ramBankN(3, nullptr),
            romBankIx{0},
            ramBankIx{0},
            romWrites(),
            ramEnabled{true},
            romMode{true} {
        data.reserve(0x10000);
        romBank0 = &data[0];
        romBankN[0] = &data[0x4000];
        romBankN[1] = &data[0x4000];

        vram = &data[0x8000];
        ramBankN[0] = &data[0xA000];
        remainingRam = &data[0xC000];

    }

    // TODO make sure 2 byte writes dont screw anything up.
    _MBC(const std::string &bootRom, const std::string &cartridgeROM) : _MBC() {

        /*
         * //        std::ifstream input2(bootROM, std::ios::binary);
//        std::copy(std::istreambuf_iterator(input2), {}, vram.begin());
         */

        std::ifstream input(cartridgeROM, std::ios::binary);
        std::copy(std::istreambuf_iterator(input), {}, data.begin());

//        romBankN[0x20] = bank0x21;
//        romBankN[0x21] = bank0x21;
//        romBankN[0x40] = bank0x40;
//        romBankN[0x41] = bank0x41;
//        romBankN[0x60] = bank0x60;
//        romBankN[0x61] = bank0x61;

    }

    void flushWrites() {
        for (const Write &w: romWrites) {
            if (w.addr < 0x2000) {
                ramEnabled = (w.writeVal & 0xF) == 0xA;
            } else if (w.addr < 0x4000) {
                u8 mask = 0x1F;
                romBankIx = (romBankIx & ~mask) | (w.writeVal & mask);
            } else if (w.addr < 0x6000) {
                if (romMode) {
                    u8 mask = 0xE0;
                    romBankIx = (romBankIx & ~mask) | ((w.writeVal << 5) & mask);
                } else {
                    ramBankIx = w.writeVal;
                }
            } else if (w.addr < 0x8000) {
                romMode = w.writeVal == 0;
            } else {
                assert(false);
            }
        }
        romWrites.clear();
    }

    const u8 &operator[](u16 addr) const {
        if (addr < 0x4000) {
            return romBank0[addr];
        } else {
            return findWritable<const u8>(addr);
        }
    }

    u8 &operator[](u16 addr) {
        if (addr < 0x8000) {
            romWrites.emplace_back(addr);
            return romWrites.back().writeVal;
        } else {
            return const_cast<u8 &>(findWritable<const u8>(addr));
        }
    }


};

using MBC = const _MBC;
#define MUT(a) const_cast<_MBC&>(a)

#endif //VRAM_VIZ_MEMORY_H
