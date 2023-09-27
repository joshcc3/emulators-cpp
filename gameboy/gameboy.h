////
//// Created by jc on 19/09/23.
////
//
//#ifndef GBA_EMULATOR_GAMEBOY_H
//#define GBA_EMULATOR_GAMEBOY_H
//
//#include <cstdint>
//#include <array>
//#include <vector>
//#include <cassert>
//#include <memory>
//#include <bitset>
//
//namespace gb {
//
//    using addr_t = uint16_t;
//    using cspeed_t = uint8_t;
//
//    struct flag_t {
//        uint8_t z: 1;
//        uint8_t n: 1;
//        uint8_t h: 1;
//        uint8_t c: 1;
//        uint8_t _rem: 4;
//    };
//
//
//    class reg_t {
//
//        uint8_t a;
//        flag_t f;
//        uint8_t b;
//        uint8_t c;
//        uint8_t d;
//        uint8_t e;
//        uint8_t h;
//        uint8_t l;
//        addr_t sp;
//        addr_t pc;
//
//        addr_t hlA() {
//            return (h << 8) | l;
//        }
//
//        uint16_t bc() {
//            return (b << 8) | c;
//        }
//
//        uint16_t de() {
//            return (d << 8) | e;
//        }
//
//        uint16_t hl() {
//            return (h << 8) | l;
//        }
//
//    };
//
//    enum class memsection {
//        BOOT,
//        ROM,
//        VRAM,
//        EXT_RAM,
//        RAM,
//        OAM,
//        IO,
//        HRAM,
//        UNMAPPED
//    };
//
//    struct SPECS {
//    };
//
//    struct memsec_t {
//        std::vector<uint8_t> data{};
//        memsection sect;
//        uint16_t size{};
//        bool zeroPage{};
//        addr_t base{};
//        addr_t end{};
//        cspeed_t CLOCK_SPEED{};
//
//        memsec_t() : data{}, sect{memsection::UNMAPPED} {};
//
//        memsec_t(cspeed_t c, addr_t base, addr_t end, memsection m, bool zp) :
//                CLOCK_SPEED{c},
//                sect{m},
//                zeroPage{zp},
//                size{static_cast<uint16_t>(end - base)},
//                base{base},
//                end{end},
//                data(0, end - base) {
//            assert(end >= base);
//            data.reserve(size);
//        }
//
//        memsec_t &operator=(const memsec_t &m) = default;
//
//        void set(const std::vector<uint8_t> &r) {
//            assert(r.size() == data.size());
//            std::copy(r.begin(), r.end(), data.begin());
//        }
//    };
//
//    class memorycontroller_t {
//        constexpr static int MAX_MEM = 0xffff;
//
//        memsec_t bootROM;
//        memsec_t romBank0;
//        memsec_t romBankN;
//        memsec_t vram;
//        memsec_t extRam;
//        memsec_t ram;
//        memsec_t oam;
//        memsec_t io;
//        memsec_t hram;
//        memsec_t unmapped1;
//        memsec_t unmapped2;
//
//    public:
//
//        std::array<memsec_t, (MAX_MEM + 1) / 0x80> memoryMap;
//
//        memorycontroller_t(const std::vector<uint8_t> &rom) :
//                bootROM(memsec_t{1, 0x0000, 0x100, memsection::BOOT, false}),
//                romBank0(memsec_t{1, 0x0000, 0x4000, memsection::ROM, false}),
//                romBankN(memsec_t{1, 0x4000, 0x4000, memsection::ROM, false}),
//                vram(memsec_t{2, 0x8000, 0x2000, memsection::VRAM, false}),
//                extRam(memsec_t{1, 0xA000, 0x2000, memsection::EXT_RAM, false}),
//                ram(memsec_t{1, 0xC000, 0x2000, memsection::RAM, false}),
//                unmapped1(memsec_t{1, 0xE000, 0x1E00, memsection::UNMAPPED, false}),
//                oam(memsec_t{1, 0xFE00, 0xA0, memsection::OAM, false}),
//                unmapped2(memsec_t{1, 0xFEA0, 0x60, memsection::UNMAPPED, false}),
//                io(memsec_t{1, 0xFF00, 0x80, memsection::IO, false}),
//                hram(memsec_t{1, 0xFF80, 0x80, memsection::HRAM, false}) {
//
//            std::array<memsec_t, 11> sections{bootROM, romBank0, romBankN, vram, extRam, ram, unmapped1, oam, unmapped2,
//                                              io, hram};
//            auto it = sections.begin();
//
//            for (std::size_t addr = 0; addr <= MAX_MEM; addr += 0x80) {
//                if (it->base + it->size <= addr) {
//                    ++it;
//                }
//                assert(it->base + it->size > addr && it->base <= addr);
//
//                memoryMap[addr / 0x80] = *it;
//            }
//
//            bootROM.set(rom);
//        }
//
//    };
//
//    class interruptcontroller_t {
//    public:
//        uint8_t &IF; // interrupt flag
//        uint8_t &IE; // enable
//
//
//        interruptcontroller_t(memorycontroller_t &m) : IF{m[0xFF0F]}, IE{m[0xFFFF]} {
//            // RST $40, 48.. 60
//            // v-blank, lcd stat, timer, serial, joypad
//        }
//
//        void timerInterrupt() {
//            IF = IF | (IE & 0x4);
//        }
//
//    };
//
//    class soundcontroller_t {
//        struct PulseA {
//            uint8_t T: 1;
//            uint8_t L: 1;
//            uint8_t unused: 3;
//            uint8_t _F1: 3;
//            uint8_t _F2: 8;
//            uint8_t V: 4;
//            uint8_t A: 1;
//            uint8_t P1: 3;
//            uint8_t D: 2;
//            uint8_t Len: 6;
//            uint8_t unused2: 1;
//            uint8_t P2: 3;
//            uint8_t N: 1;
//            uint8_t S: 3;
//        };
//        struct PulseB {
//
//        };
//        struct Wave {
//            uint8_t T: 1;
//            uint8_t L: 1;
//            uint8_t unused1: 3;
//            uint8_t _F1: 3;
//            uint8_t _F2: 8;
//            uint8_t unused2: 1;
//            uint8_t V: 2;
//            uint8_t unused3: 5;
//            uint8_t Len: 8;
//            uint8_t unusued4: 8;
//        };
//
//        struct Noise {
//
//        };
//
//        struct PulseA pA;
//        struct PulseB pB;
//        struct Wave wave;
//        uint8_t *wave_data;
//        struct Noise noise;
//
//        uint8_t &NR50;
//        uint8_t &NR51;
//        uint8_t &NR52;
//
//
//        soundcontroller_t(memorycontroller_t &mem) : NR50{mem[0xFF24]}, NR51{mem[0xFF25]}, NR52{mem[0xFF26]},
//                                                     wave_data{mem[0xFF30]} {
//
//            pA = *(reinterpret_cast<PulseA>(&(mem[0xFF10])));
//            pB = *(reinterpret_cast<PulseB>(&(mem[0xFF15])));
//            wave = *(reinterpret_cast<Wave>(&(mem[0xFFA])));
//            noise = *(reinterpret_cast<Noise>(&(mem[0xFF1F])));
//
//        }
//
//
//    };
//
//    class joypad_t {
//    public:
//        enum class jpbutton_t {
//            DOWN,
//            UP,
//            LEFT,
//            RIGHT,
//            START,
//            SELECT,
//            B,
//            A
//        };
//        jpbutton_t buttonMap[2][4];
//        uint8_t &P1; // joypad
//        std::bitset<8> keysPressed;
//        using btn = jpbutton_t;
//
//        joypad_t(memorycontroller_t &m) : P1{m[0xFF00]},
//                                          buttonMap{
//                                                  {btn::RIGHT, btn::LEFT, btn::UP,     btn::DOWN},
//                                                  {btn::A,     btn::B,    btn::SELECT, btn::START}
//                                          } {
//
//        }
//
//        void readP1() {
//            uint8_t P15 = (P1 >> 5) & 1;
//            uint8_t P14 = (P1 >> 4) & 1;
//            uint8_t P13 = (P1 >> 3) & 1;
//            uint8_t P12 = (P1 >> 2) & 1;
//            uint8_t P11 = (P1 >> 1) & 1;
//            uint8_t P10 = P1 & 1;
//            for (auto ix1: {P14, P15}) {
//                for (auto ix2: {P10, P11, P12, P13}) {
//                    keysPressed[buttonMap[ix1][ix2]] = ix1 & ix2
//                }
//            }
//        }
//
//    };
//
//    class serialdata_t {
//    public:
//        // OUT lcock 8KHz
//        uint8_t &SB; // data
//        uint8_t &SC; // ctrl
//
//        serialdata_t(memorycontroller_t &m) : data{m[0xFF01]}, ctrl{m[0xFF02]} {}
//
//        bool isTransferStart() {
//            return (SB >> 7) & 1;
//        }
//
//        bool isInternal() {
//            return SB & 1;
//        }
//    };
//
//
//    class ppu_t {
//        constexpr static int CLOCK_SPEED = 4;
//    public:
//        uint8_t &LCDControl;
//        uint8_t &STAT;
//        uint8_t &SCY;
//        uint8_t &SCX;
//        uint8_t &LY;
//        uint8_t &LYC;
//        uint8_t &DMA;
//        uint8_t &BGP;
//        uint8_t &OBP0;
//        uint8_t &OBP1;
//        uint8_t &WY;
//        uint8_t &WX;
//
//        ppu_t(memorycontroller_t &m) :
//                LCDControl{m[0xFF40]},
//                STAT{m[0xFF40]},
//                SCY{m[0xFF40]},
//                SCX{m[0xFF40]},
//                LY{m[0xFF40]},
//                LYC{m[0xFF40]},
//                DMA{m[0xFF40]},
//                BGP{m[0xFF40]},
//                OBP0{m[0xFF40]},
//                OBP1{m[0xFF40]},
//                WY{m[0xFF40]},
//                WX{m[0xFF40]} {}
//    };
//
//    class cpu_t {
//    public:
//        const int CLOCK_SPEED = 4; // 4MHz1
//        addr_t programCounter;
//        uint64_t clockNumber1Mhz;
//        alu_t alu;
//        interruptcontroller_t ic;
//        timer_t t;
//        memorycontroller_t mem;
//        joypad_t jp;
//        serialdata_t serial;
//        soundcontroller_t sound;
//        ppu_t ppu;
//
//        reg_t regs;
//
//        cpu_t() : programCounter{0}, clockNumber1Mhz{0} {}
//    };
//
//
//    class timer_t {
//    public:
//
//        uint8_t &DIV;
//        uint8_t &TIMA;
//        uint8_t &TMA;
//        uint8_t &TAC;
//
//        interruptcontroller_t &ic;
//
//        std::array<uint32_t, 4> timerTable;
//
//        timer_t(memorycontroller_t &m, interruptcontroller_t &ic) : DIV{m[0xFF04]}, TIMA{m[0xFF05]}, TMA{m[0xFF06]},
//                                                                    TAC{m[0xFF07]}, ic{ic} {
//            timerTable[0] = 256;
//            timerTable[1] = 64;
//            timerTable[2] = 16;
//            timerTable[3] = 4;
//        }
//
//        // what'ringBufferSize the div register for
//        // how do you generate
//        void run(uint32_t clockNumber1Mhz) {
//            uint32_t freq = timerTable[TAC % 0x3];
//            bool started = (TAC >> 2) & 1;
//            if ((clockNumber1Mhz & (freq - 1)) == 0 && started) {
//                ++TIMA;
//                if (TIMA == 0) {
//                    TIMA = TMA;
//                    ic.timerInterrupt();
//                }
//            }
//        }
//    };
//
//    class gameboy {
//
//    };
//
//}
//
//#endif //GBA_EMULATOR_GAMEBOY_H
