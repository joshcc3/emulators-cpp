//
// Created by jc on 21/10/23.
//

#ifndef AUDIO_DRIVER2_H
#define AUDIO_DRIVER2_H

#include "Memory.h"
#include <iostream>
#include <cstdint>
#include <alsa/asoundlib.h>
#include <cmath>
#include <chrono>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using namespace std;


struct PulseA {
    u8 sweepNum: 3; // r/w
    u8 sweepDir: 1;
    u8 sweepTime: 3;
    [[maybe_unused]] u8 _unused: 1;
    u8 len: 6; // r/w
    u8 dutyPattern: 2;
    u8 volEnvelopeNum: 3; // r/w
    u8 envelopeDir: 1;
    u8 initialVol: 4;
    u8 freqLo: 8; // wo
    u8 freqHi: 3; // r/w
    [[maybe_unused]] u8 _unused2: 3;
    u8 counter: 1;
    u8 restart: 1;

    void clear() {
        sweepNum = 0; // r/w
        sweepDir = 0;
        sweepTime = 0;
        _unused = 0;
        len = 0; // r/w
        dutyPattern = 0;
        volEnvelopeNum = 0; // r/w
        envelopeDir = 0;
        initialVol = 0;
        freqLo = 0; // wo
        freqHi = 0; // r/w
        _unused2 = 0;
        counter = 0;
        restart = 0;
    }

    bool operator==(const PulseA &a) const {
        return sweepNum == a.sweepNum && // r/w
               sweepDir == a.sweepDir &&
               sweepTime == a.sweepTime &&
               len == a.len && // r/w
               dutyPattern == a.dutyPattern &&
               volEnvelopeNum == a.volEnvelopeNum && // r/w
               envelopeDir == a.envelopeDir &&
               initialVol == a.initialVol &&
               freqLo == a.freqLo && // wo
               freqHi == a.freqHi && // r/w
               counter == a.counter &&
               restart == a.restart;
    }

    bool operator!=(const PulseA &a) const {
        return !(*this == a);
    }

    int getFreq() const {
        return (u16(freqHi) << 8) | freqLo;
    }
};

struct PulseB {
    u8 len: 6;
    u8 dutyPattern: 2;
    u8 volEnvelopeNum: 3;
    u8 envelopeDir: 1;
    u8 initialVol: 4;
    u8 freqLo: 8; // wo
    u8 freqHi: 3;
    [[maybe_unused]] u8 _unused2: 3;
    u8 counter: 1;
    u8 restart: 1;

    int getFreq() const {
        return (u16(freqHi) << 8) | freqLo;
    }

    void clear() {
        *reinterpret_cast<uint32_t *>(this) = 0;
    }

    bool operator!=(const PulseB &b) const {
        return *reinterpret_cast<const uint32_t *>(this) != *reinterpret_cast<const uint32_t *>(&b);
    }
};

struct Wave {
    // FF1A
    [[maybe_unused]] u8 _unused0: 7;
    u8 soundOnOff: 1;
    u8 len: 8; // r/w
    [[maybe_unused]] u8 _unused1: 5;
    u8 outputLevel: 2;
    [[maybe_unused]] u8 _unusued2: 1;
    u8 freqLo: 8; // wo
    u8 freqHi: 3; // rw
    [[maybe_unused]] u8 _unused2: 3;
    u8 counter: 1;
    u8 restart: 1;

    void clear() {
        *reinterpret_cast<uint32_t *>(this) = 0;
    }

    bool operator!=(const Wave &b) const {
        return *reinterpret_cast<const uint32_t *>(this) != *reinterpret_cast<const uint32_t *>(&b);
    }

    u16 getFreq() {
        return (u16(freqHi) << 8) | freqLo;
    }

};

struct WaveData {
    u8 upper: 4;
    u8 lower: 4;

    void clear() {
        upper = 0;
        lower = 0;
    }
};

struct Noise {
    u8 soundLen: 6;
    u8 unused1: 2;
    u8 volEnvelopeNum: 3;
    u8 volEnvelopeDir: 1;
    u8 volEnvelopeInitialVolume: 4;
    u8 divFreq: 3;
    u8 counterStep: 1;
    u8 shiftClock: 4;
    u8 unused2: 6;
    u8 counter: 1;
    u8 restart: 1;

    void clear() {
        *reinterpret_cast<uint32_t *>(this) = 0;
    }


    bool operator!=(const Noise &b) const {
        return *reinterpret_cast<const uint32_t *>(this) != *reinterpret_cast<const uint32_t *>(&b);
    }

};

struct ChannelControl {
    u8 so1Output: 3;
    u8 vinSO1: 1;
    u8 so2Output: 3;
    u8 vinSO2: 1;

    void clear() {
        *reinterpret_cast<uint8_t *>(this) = 0;
    }

};

struct SoundOutputSelection {
    u8 so11: 1;
    u8 so12: 1;
    u8 so13: 1;
    u8 so14: 1;
    u8 so21: 1;
    u8 so22: 1;
    u8 so23: 1;
    u8 so24: 1;

    void clear() {
        *reinterpret_cast<uint8_t *>(this) = 0;
    }

};

struct SoundOnOff {
    u8 sound1: 1; // these bits are readonly
    u8 sound2: 1;
    u8 sound3: 1;
    u8 sound4: 1;
    u8 unused: 3;
    u8 allSound: 1;

    void clear() {
        *reinterpret_cast<uint8_t *>(this) = 0;
    }

};


class AlsaSM {
public:

    constexpr static int SAMPLES_PER_SECOND = (1 << 14);
    constexpr static int LATENCY_US = 20000;
    constexpr static int NS_PER_SAMPLE = 1e9 / AlsaSM::SAMPLES_PER_SECOND;
    snd_pcm_t *handle;
    unsigned long delay;
    unsigned long available;
    u64 bytesWritten;

    AlsaSM() : delay{0}, available{0} {

        int err;

        // TODO - double check that the frame size is actual 1 byte.
        // TODO - need to check that specifying in frames is fine.

        if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s \n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_set_params(handle,
                                      SND_PCM_FORMAT_U8,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      SAMPLES_PER_SECOND,
                                      1,
                                      LATENCY_US)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }


    }

    ~AlsaSM() {
        /* pass the remaining samples, otherwise they're dropped in close */
        int err = snd_pcm_drain(handle);
        if (err < 0)
            printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
        snd_pcm_close(handle);
    }

    void syncState() {
        snd_pcm_status_t *status;
        snd_pcm_status_alloca(&status);

        int err = snd_pcm_status(handle, status);
        if (err) {
            cerr << "Bad: [" << err << "]." << endl;
        }

        snd_pcm_state_t state = snd_pcm_status_get_state(status);
        if ((state & (snd_pcm_state_t::SND_PCM_STATE_RUNNING | snd_pcm_state_t::SND_PCM_STATE_PREPARED |
                      snd_pcm_state_t::SND_PCM_STATE_XRUN)) == 0) {
            cerr << "In bad state: [" << state << "]." << endl;
        }

        delay = snd_pcm_status_get_delay(status);
        available = snd_pcm_status_get_avail(status);
//        cout << delay << " " << available << endl;
    }

    size_t getBytesToWrite() {

        return available;
//        long delayUS = (delay * 1000000) / SAMPLES_PER_SECOND;
//
//        if (delayUS < 1000) {
//            return min(available, bytes10ms);
//        } else {
//            return 0;
//        }

    }

    void write(u8 *buffer, size_t sz) {


        snd_pcm_sframes_t frames;
        frames = snd_pcm_writei(handle, buffer, sz);
        if (frames < 0)
            frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) {
            printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
        }
        if (frames > 0 && frames < (long) sz) {
            printf("Short write (expected %li, wrote %li)\n", (long) sz, frames);
        }
    }

};

class PulseASM {
public:

    const u8 waveForm[4][2] = {{1, 7},
                               {2, 6},
                               {4, 4},
                               {6, 2}};
    PulseA &pa;


    int sweepTime;
    int sweepDir;
    int sweepShiftN;
    int duty;
    int len;
    int initialVol;
    int volStep;
    int volSweep;
    int rawFreq;

    int initialFreq;
    int sweepTimeNs;
    int volStepCounterNs;
    int soundLengthNs;

    long long timePassedNs;

    bool enabled;


    PulseASM(PulseA &reg) :
            pa{reg},
            enabled{false},
            timePassedNs{0},
            sweepTimeNs{0},
            sweepTime{},
            sweepDir{},
            sweepShiftN{},
            duty{},
            len{},
            initialVol{},
            volStep{},
            volSweep{},
            rawFreq{},
            initialFreq{},
            volStepCounterNs{},
            soundLengthNs{} {
    }

    void syncState() {

        if (pa.restart == 1 || pa.counter == 1) {

            sweepTime = pa.sweepTime;
            sweepDir = pa.sweepDir ? 1 : -1;
            sweepShiftN = pa.sweepNum;
            duty = pa.dutyPattern;
            len = pa.len;
            initialVol = pa.initialVol;
            volStep = pa.envelopeDir ? 1 : -1;
            volSweep = pa.volEnvelopeNum;
            rawFreq = pa.getFreq();

            initialFreq = 131072 / (2048 - rawFreq);
            sweepTimeNs = (double) (7.8 * 1e6 * sweepTime);
            volStepCounterNs = volSweep == 0 ? 2e9 : 1e9 / 64.0 * volStep * volSweep;
            soundLengthNs = 1e9 / 256.0 * (64 - len);

            pa.restart = 0;
            pa.counter = 0;
            enabled = true;

        }

        if (pa.counter && timePassedNs >= soundLengthNs) {
            enabled = false;
            timePassedNs = 0;
        }


    }

    u8 step() {

        if (enabled) {
            const u8 low = waveForm[duty][0];
            const u8 high = waveForm[duty][1];

            int sweeps = sweepTimeNs == 0 ? 0 : timePassedNs / sweepTimeNs;
            double newFreq = initialFreq * pow((1 + sweepDir / pow(2, sweepShiftN)), sweeps);
            double finalFreq = std::min((double) 2047, std::max((double) 20, newFreq));
            int oneClock = 1e9 / finalFreq / 8;

            int volume = std::min(std::max(initialVol + timePassedNs / volStepCounterNs, 0LL), 15LL);

            bool outputHigh = (timePassedNs % (8 * oneClock)) >= (low * oneClock);
            timePassedNs += AlsaSM::NS_PER_SAMPLE;

            return outputHigh ? volume : 0;
        } else {
            return 0;
        }
    }

};

class PulseBSM {
public:
    const u8 waveForm[4][2] = {{1, 7},
                               {2, 6},
                               {4, 4},
                               {6, 2}};
    PulseB &pb;


    int duty;
    int len;
    int initialVol;
    int volStep;
    int volSweep;
    int rawFreq;

    int initialFreq;
    int volStepCounterNs;
    int soundLengthNs;

    long long timePassedNs;

    bool enabled;


    PulseBSM(PulseB &reg) :
            pb{reg},
            enabled{false},
            timePassedNs{0},
            duty{},
            len{},
            initialVol{},
            volStep{},
            volSweep{},
            rawFreq{},
            initialFreq{},
            volStepCounterNs{},
            soundLengthNs{} {}

    void syncState() {

        if (pb.restart == 1 || pb.counter == 1) {

            duty = pb.dutyPattern;
            len = pb.len;
            initialVol = pb.initialVol;
            volStep = pb.envelopeDir ? 1 : -1;
            volSweep = pb.volEnvelopeNum;
            rawFreq = pb.getFreq();

            initialFreq = 131072 / (2048 - rawFreq);
            volStepCounterNs = volSweep == 0 ? 2e9 : 1e9 / 64.0 * volStep * volSweep;
            soundLengthNs = 1e9 / 256.0 * (64 - len);

            pb.restart = 0;
            pb.counter = 0;
            enabled = true;

        }

        if (pb.counter && timePassedNs >= soundLengthNs) {
            enabled = false;
            timePassedNs = 0;
        }

    }

    u8 step() {

        if (enabled) {
            const u8 low = waveForm[duty][0];
            const u8 high = waveForm[duty][1];

            int oneClock = 1e9 / initialFreq / 8;

            int volume = std::min(std::max(initialVol + timePassedNs / volStepCounterNs, 0LL), 15LL);

            bool outputHigh = (timePassedNs % (8 * oneClock)) >= (low * oneClock);
            timePassedNs += AlsaSM::NS_PER_SAMPLE;

            return outputHigh ? volume : 0;
        } else {
            return 0;
        }
    }
};


class WaveSM {
public:
    /*
     * This channel can be used to output digital sound, the length of the sample buffer (Wave RAM) is limited to 32 digits.
     * This sound channel can be also used to output normal tones when initializing the Wave RAM by a square wave.
     * This channel doesn't have a volume envelope register.

FF1A - NR30 - Channel 3 Sound on/off (R/W)
  Bit 7 - Sound Channel 3 Off  (0=Stop, 1=Playback)  (Read/Write)

FF1B - NR31 - Channel 3 Sound Length
  Bit 7-0 - Sound length (t1: 0 - 255)
Sound Length = (256-t1)*(1/256) seconds
This value is used only if Bit 6 in NR34 is set.

FF1C - NR32 - Channel 3 Select output level (R/W)
  Bit 6-5 - Select output level (Read/Write)
Possible Output levels are:
  0: Mute (No sound)
  1: 100% Volume (Produce Wave Pattern RAM Data as it is)
  2:  50% Volume (Produce Wave Pattern RAM data shifted once to the right)
  3:  25% Volume (Produce Wave Pattern RAM data shifted twice to the right)

FF1D - NR33 - Channel 3 Frequency's lower data (W)
Lower 8 bits of an 11 bit frequency (x).

FF1E - NR34 - Channel 3 Frequency's higher data (R/W)
  Bit 7   - Initial (1=Restart Sound)     (Write Only)
  Bit 6   - Counter/consecutive selection (Read/Write)
            (1=Stop output when length in NR31 expires)
  Bit 2-0 - Frequency's higher 3 bits (x) (Write Only)
Frequency = 4194304/(64*(2048-x)) Hz = 65536/(2048-x) Hz

FF30-FF3F - Wave Pattern RAM
Contents - Waveform storage for arbitrary sound data

This storage area holds 32 4-bit samples that are played back upper 4 bits first.
     */

    Wave &w;
    WaveData *waveData; // 16 bytes

    u64 soundLenNs;
    std::array<u8, 32> waveForm;
    u32 freq;
    int nsPerSample;
    int waveLenInNs;

    u64 timePassedNs;
    bool enabled;

    // TODO intiialize
    WaveSM(Wave &w, WaveData *waveData) :
            w{w},
            waveData{waveData},
            soundLenNs{0},
            waveForm{},
            freq{0},
            nsPerSample{0},
            waveLenInNs{0},
            timePassedNs{0},
            enabled{false} {

    }

    void syncState() {

        if (w.counter == 1 || w.restart == 1) {

            timePassedNs = 0;

            u8 soundLenFmt = w.len;
            u8 outputLevel = w.outputLevel;
            uint32_t freqFmt = w.getFreq();
            uint16_t soundScale = (outputLevel - 1);

            freq = 65536 / (2048 - freqFmt);
            soundLenNs = 1e9 / 256.0 * (256 - soundLenFmt);

            for (int i = 0; i < 16; ++i) {
                waveForm[2 * i] = (waveData[i].upper >> soundScale) * 8;
                waveForm[2 * i + 1] = (waveData[i].lower >> soundScale) * 8;
            }

            nsPerSample = 1e9 / (freq * waveForm.size());
            waveLenInNs = nsPerSample * waveForm.size();

            w.counter = 0;
            w.restart = 0;
            enabled = true;

        }

        if (w.counter && timePassedNs >= soundLenNs) {
            enabled = false;
        }

        assert(w.restart == 0 && w.counter == 0);

    }

    u8 step() {

        u8 output;
        if (enabled && w.soundOnOff == 1) {

            assert(freq != 0);
            assert(!w.counter || soundLenNs > 0);

            int ix = (timePassedNs % waveLenInNs) / nsPerSample;
            output = waveForm[ix];

        } else {
            output = 0;
        }

        timePassedNs += AlsaSM::NS_PER_SAMPLE;
        return output;

    }


};

class NoiseSM {
public:
    void syncState() {}

    u8 step() {
        return 0;
    }

};

PulseA p = PulseA{
        .sweepNum = 7,
        .sweepDir = 1,
        .sweepTime = 3,
        ._unused = 1,
        .len = 0, // r/w
        .dutyPattern = 2,
        .volEnvelopeNum = 30, // r/w
        .envelopeDir = 1,
        .initialVol = 12,
        .freqLo = 0, // wo
        .freqHi = 0, // r/w
        ._unused2 = 3,
        .counter = 1,
        .restart = 1
};

PulseB p2 = PulseB{
        .len = 0,
        .dutyPattern = 0,
        .volEnvelopeNum = 0,
        .envelopeDir = 0,
        .initialVol = 15,
        .freqLo = 255, // wo
        .freqHi = 3,
        .counter = 0,
        .restart = 1
};

class audio_driver2 {
    PulseASM paSM;
    PulseBSM pbSM;
    WaveSM wSM;
    NoiseSM nSM;
    AlsaSM aSM;
    atomic_flag &isRunning;
    atomic<u64> &clockVar;

public:

    /*
     * FF26 - NR52 - Sound on/off
If your GB programs don't use sound then write 00h to this register to save 16% or more on GB power consumption.
Disabeling the sound controller by clearing Bit 7 destroys the contents of all sound registers.
Also, it is not possible to access any sound registers (execpt FF26) while the sound controller is disabled.
  Bit 7 - All sound on/off  (0: stop all sound circuits) (Read/Write)
  Bit 3 - Sound 4 ON flag (Read Only)
  Bit 2 - Sound 3 ON flag (Read Only)
  Bit 1 - Sound 2 ON flag (Read Only)
  Bit 0 - Sound 1 ON flag (Read Only)
Bits 0-3 of this register are read only status bits, writing to these bits does NOT enable/disable sound.
The flags get set when sound output is restarted by setting the Initial flag (Bit 7 in NR14-NR44),
the flag remains set until the sound length has expired (if enabled). A volume envelopes which has decreased
to zero volume will NOT cause the sound flag to go off.
     */

    //*reinterpret_cast<PulseA *>(&ram[0xFF10])

    audio_driver2(_MBC &ram, atomic<u64> &clockVar, atomic_flag &isRunning) :
            isRunning{isRunning},
            clockVar{clockVar},
            paSM{*reinterpret_cast<PulseA *>(&ram[0xFF10])},
            wSM{*reinterpret_cast<Wave *>(&ram[0xFF1A]), reinterpret_cast<WaveData *>(&ram[0xFF30])},
            pbSM{*reinterpret_cast<PulseB *>(&ram[0xFF16])},
            nSM{},
            aSM{} {
    }

    void run() {
        while (isRunning.test(std::memory_order_relaxed)) {
            step();
            usleep(1);
        }
    }


    void step() {

#ifdef DEBUG
        constexpr int SZ = 100;
        static array<u8, SZ> adat{};
        static array<u8, SZ> bdat{};
        static array<u8, SZ> wdat{};
        int aC = 0;
        int bC = 0;
        int wC = 0;
#endif

        aSM.syncState();
        paSM.syncState();
        pbSM.syncState();
        wSM.syncState();
        nSM.syncState();

        int bufferSize = aSM.getBytesToWrite();
        u8 buffer[bufferSize];

        // TODO look at the global sound control registers.
        for (int i = 0; i < bufferSize; ++i) {
            u8 ch1 = paSM.step();
            u8 ch2 = pbSM.step();
            u8 ch3 = wSM.step();
            u8 ch4 = nSM.step();
            u8 output = ch1 + ch2 + ch3 + ch4;
            assert(output <= 255);

#ifdef DEBUG
            adat[aC] = ch1;
            bdat[bC] = ch1;
            wdat[wC] = ch1;
            aC = (aC - 1 + SZ) % SZ;
            bC = (bC - 1 + SZ) % SZ;
            wC = (wC - 1 + SZ) % SZ;
#endif
            buffer[i] = output * 4;
        }

        aSM.write(buffer, bufferSize);

    }

};


#endif //AUDIO_DRIVER2_H
