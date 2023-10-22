//
// Created by jc on 21/10/23.
//

#ifndef AUDIO_DRIVER2_H
#define AUDIO_DRIVER2_H

#include <iostream>
#include <cstdint>
#include <alsa/asoundlib.h>
#include <cmath>
#include <chrono>

using u8 = uint8_t;
using u16 = uint8_t;
using u32 = uint8_t;
using u64 = uint8_t;

using namespace std;


struct PulseA {
    u8 sweepNum: 3; // r/w
    u8 sweepDir: 1;
    u8 sweepTime: 3;
    u8 _unused: 1;
    u8 len: 6; // r/w
    u8 dutyPattern: 2;
    u8 volEnvelopeNum: 3; // r/w
    u8 envelopeDir: 1;
    u8 initialVol: 4;
    u8 freqLo: 8; // wo
    u8 freqHi: 3; // r/w
    u8 _unused2: 3;
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
    u8 _unused2: 3;
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
    u8 len: 8; // r/w
    u8 _unused1: 5;
    u8 outputLevel: 2;
    u8 _unusued2: 1;
    u8 freqLo: 8; // wo
    u8 freqHi: 3; // rw
    u8 _unused2: 3;
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
    constexpr static snd_pcm_uframes_t bytes10ms = SAMPLES_PER_SECOND / 100;

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

    chrono::time_point<chrono::system_clock> cp;
    long long timePassedNs;

    bool enabled;


    PulseASM(PulseA &reg) : pa{reg}, enabled{true}, timePassedNs{0}, cp{chrono::system_clock::now()} {
    }

    void syncState() {

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
        soundLengthNs = pa.counter ? 1e9 / 256.0 * (64 - len) : 20000000;


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

            chrono::time_point now = chrono::system_clock::now();
            long long elapsedTime = chrono::duration_cast<chrono::nanoseconds>(now - cp).count();
            cp = now;
            bool outputHigh = (timePassedNs % (8 * oneClock)) >= (low * oneClock);
            timePassedNs += 1e9/AlsaSM::SAMPLES_PER_SECOND;

            return outputHigh ? volume : 0;
        } else {
            return 0;
        }
    }

};

class PulseBSM {
public:
    void syncState() {}

    u8 step() {
        return 0;
    }
};

class WaveSM {
public:
    void syncState() {}

    u8 step() {
        return 0;
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

class audio_driver2 {
public:

    audio_driver2() : paSM{p} {
    }

    void run() {

        aSM.syncState();
        paSM.syncState();
        pbSM.syncState();
        wSM.syncState();
        nSM.syncState();

        int bufferSize = aSM.getBytesToWrite();
        u8 buffer[bufferSize];

        for (int i = 0; i < bufferSize; ++i) {
            u8 ch1 = paSM.step();
            u8 ch2 = pbSM.step();
            u8 ch3 = wSM.step();
            u8 ch4 = nSM.step();
            u8 output = ch1 + ch2 + ch3 + ch4;
            cout << int(ch1) << endl;
            buffer[i] = output * 4;
        }

        aSM.write(buffer, bufferSize);

    }

private:

    PulseASM paSM;
    PulseBSM pbSM;
    WaveSM wSM;
    NoiseSM nSM;
    AlsaSM aSM;
};


#endif //AUDIO_DRIVER2_H
