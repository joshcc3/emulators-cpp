//
// Created by jc on 27/09/23.
//

#ifndef GBA_EMULATOR_AUDIO_DRIVER_H
#define GBA_EMULATOR_AUDIO_DRIVER_H


#include <alsa/asoundlib.h>
#include <vector>
#include <valarray>
#include <iostream>
#include <memory>
#include <functional>

//#define AUDIO_NOT_WORKING

using u8 = uint8_t;
using u16 = uint16_t;
using i8 = int8_t;

class RingBuffer {

public:
    std::size_t startIx;
    std::size_t endIx;
    std::vector<u8> sampleBuf;
    static constexpr size_t SAMPLES_PER_SECOND = 16384;
    size_t ringBufferSize;

    RingBuffer() : sampleBuf(SAMPLES_PER_SECOND, 0), startIx{0}, endIx{0}, ringBufferSize{0} {
    }

    size_t size() {
        return ringBufferSize;
    }

    const u8 &operator[](int ix) const {
        return sampleBuf[(ix + startIx) % sampleBuf.capacity()];
    }

//    void generateWave(long long microseconds, long freqHz, long volume, int offset) {
//
//        size_t samplesInWaveForm = SAMPLES_PER_SECOND / freqHz;
//        long long samples = microseconds * freqHz * samplesInWaveForm / 1000000;
//
//        constexpr double PI = 3.14159265;
//        for (int i = 0; i < samples; ++i) {
//            double sinValue = sin(2 * PI * i / samplesInWaveForm) * volume / 2 + volume / 2 + offset;
//            sampleBuf.push_back((u8) sinValue);
//        }
//    }

    void generateGB(long long nanoseconds, long soundLevel) {
        int samples = SAMPLES_PER_SECOND * nanoseconds / 1000000000;

        assert(samples < sampleBuf.capacity() - size());

        auto s1 = sampleBuf.begin() + endIx;
        auto s2 = sampleBuf.begin();
        auto e1 = sampleBuf.begin() + std::min(sampleBuf.capacity(), endIx + samples);
        auto e2 = sampleBuf.begin() +
                  std::max(0LL, (long long) endIx + (long long) samples - (long long) sampleBuf.capacity());
        fill(s1, e1, soundLevel);
        fill(s2, e2, soundLevel);
        ringBufferSize += samples;
        endIx = (samples + endIx) % sampleBuf.capacity();
        sampleBuf.insert(sampleBuf.end(), samples, soundLevel);
    }

    void drain(unsigned long i) {
        for (int j = 0; j < std::min(i, size()); ++j) {
            sampleBuf[(j + startIx) % sampleBuf.capacity()] = 0;
        }
        if (i >= size()) {
            ringBufferSize = 0;
            startIx = 0;
            endIx = 0;
        } else {
            ringBufferSize -= i;
            startIx = (startIx + i) % sampleBuf.capacity();
        }
    }
};


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

    int getFreq() {
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

    int getFreq() {
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


class AudioDriver {
public:

    std::vector<u8> &vram;

    constexpr static long long SAMPLES_PER_FLUSH = RingBuffer::SAMPLES_PER_SECOND / 50;
    // send out 20ms of sound per iteration.
    std::array<u8, SAMPLES_PER_FLUSH> mixer;
    constexpr static long long CLOCKS_PER_FLUSH = (double(4 << 20) * double(SAMPLES_PER_FLUSH) /
                                                   double(RingBuffer::SAMPLES_PER_SECOND));

    long long clock;

    PulseA &paReg;
    PulseB &pbReg;
    Wave &wvReg;
    Noise &noReg;
    WaveData *waveData; // 16 bytes
    ChannelControl &ccReg;
    SoundOutputSelection &soundOutputSelection;
    SoundOnOff &soundOnOff;

    PulseA cpPA;
    PulseB cpPB;
    Wave cpW;
    Noise cpN;
    ChannelControl cpCCReg;
    SoundOutputSelection cpSoundOutputSelection;
    SoundOnOff cpSoundOnOff;
    u8 &channel3SoundOnOff;

    RingBuffer ch1;
    RingBuffer ch2;
    RingBuffer ch3;
    RingBuffer ch4;

    snd_pcm_t *handle;

    AudioDriver(std::vector<u8> &vram)
            : vram{vram}, ch1{}, ch2{}, ch3{}, ch4{}, mixer{},
              paReg{*reinterpret_cast<PulseA *>(&vram[0xFF10])},
              pbReg{*reinterpret_cast<PulseB *>(&vram[0xFF16])},
              wvReg{*reinterpret_cast<Wave *>(&vram[0xFF1B])},
              noReg{*reinterpret_cast<Noise *>(&vram[0xFF20])},
              ccReg{*reinterpret_cast<ChannelControl *>(&vram[0xFF24])},
              soundOutputSelection{*reinterpret_cast<SoundOutputSelection *>(&vram[0xFF25])},
              soundOnOff{*reinterpret_cast<SoundOnOff *>(&vram[0xFF26])},
              channel3SoundOnOff{vram[0xFF1A]},
              cpPA{paReg},
              cpPB{pbReg},
              cpW{wvReg},
              cpN{noReg},
              cpCCReg{ccReg}, cpSoundOutputSelection{soundOutputSelection}, cpSoundOnOff{soundOnOff}, clock{0} {

        waveData = reinterpret_cast<WaveData *>(&vram[0xFF30]);

        int err;

#ifndef AUDIO_NOT_WORKING

        if ((err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %ringBufferSize\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }

        if ((err = snd_pcm_set_params(handle,
                                      SND_PCM_FORMAT_U8,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      RingBuffer::SAMPLES_PER_SECOND,
                                      1,
                                      500000)) < 0) {   /* 0.5sec */
            printf("Playback open error: %ringBufferSize\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }
#endif
    }

    void run(long long cpuClock) {
        u8 masterVolume2 = ccReg.so2Output;
        u8 masterVolume1 = ccReg.so1Output;

        // can choose sending sound to different terminals as well

        if (soundOnOff.allSound == 0) {
            paReg.clear();
            pbReg.clear();
            wvReg.clear();
            ccReg.clear();
            soundOutputSelection.clear();
            soundOnOff.clear();
        } else {
            if (paReg.counter && cpPA != paReg || paReg.restart) {
                paReg.restart = 0;
                generatePulseA(paReg, 0);
                flush();
            }
//            else if (paReg.counter + paReg.restart == 0) {
//                ch1.drain(ch1.size());
//            }

            if (pbReg.counter && cpPB != pbReg || pbReg.restart > 0) {
                pbReg.restart = 0;
                generatePulseB(pbReg, 0);
                flush();
            }
//            else if (pbReg.counter + pbReg.restart == 0) {
//                ch2.drain(ch2.size());
//            }
            if (wvReg.counter && wvReg != cpW || wvReg.restart) {
                wvReg.restart = 0;
                generateWave(wvReg, 0);
            }
//            else if ((channel3SoundOnOff >> 7) == 0) {
//                ch3.drain(ch3.size());
//            }
            if (cpN != noReg) {

            }
        }

        if (clock % CLOCKS_PER_FLUSH > cpuClock % CLOCKS_PER_FLUSH) {
            flush();
        }

        clock = cpuClock;

        cpPA = paReg;
        cpPB = pbReg;
        cpW = wvReg;
        cpN = noReg;
        cpCCReg = ccReg;
        cpSoundOutputSelection = soundOutputSelection;
        cpSoundOnOff = soundOnOff;
    }

    long long flush() {
        for (int i = 0; i < mixer.size(); ++i) {
            // volume scaling info
            mixer[i] = ch1[i] + ch2[i] + ch3[i] + ch4[i];
            if (mixer[i] != 0) {
                std::cout << (int) mixer[i] << " ";
            }
        }
        if (mixer[0] != 0) {
            std::cout << std::endl;
        }
        ch1.drain(mixer.size());
        ch2.drain(mixer.size());
        ch3.drain(mixer.size());
        ch4.drain(mixer.size());
        soundOnOff.sound1 = ch1.size() == 0;
        soundOnOff.sound2 = ch2.size() == 0;
        soundOnOff.sound3 = ch3.size() == 0;
        soundOnOff.sound4 = ch4.size() == 0;

        int bufferSize = mixer.size();
        u8 *samplesPtr = &mixer[0];

#ifndef AUDIO_NOT_WORKING
        snd_pcm_sframes_t frames;
        frames = snd_pcm_writei(handle, samplesPtr, bufferSize);
        if (frames < 0)
            frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) {
            printf("snd_pcm_writei failed: %ringBufferSize\n", snd_strerror(frames));
        }
        if (frames > 0 && frames < (long) bufferSize) {
            printf("Short write (expected %li, wrote %li)\n", (long) bufferSize, frames);
        }
#endif
        return bufferSize;

    }


    ~AudioDriver() {
#ifndef AUDIO_NOT_WORKING
        /* pass the remaining samples, otherwise they're dropped in close */
        int err = snd_pcm_drain(handle);
        if (err < 0)
            printf("snd_pcm_drain failed: %ringBufferSize\n", snd_strerror(err));
        snd_pcm_close(handle);
#endif
    }

    void generatePulseB(PulseB &b, int pulseLen) {
        int duty = b.dutyPattern;
        int len = b.counter ? b.len : pulseLen;
        int initialVol = b.initialVol;
        int volStep = b.envelopeDir ? 1 : -1;
        int volSweep = b.volEnvelopeNum;
        int freq = b.getFreq();
        int freqHz = 131072 / (2048 - freq);
        int oneClock = 1e9 / freqHz / 8;

        u8 waveForm[4][2] = {{1, 7},
                             {2, 6},
                             {4, 4},
                             {6, 2}};
        u8 low = waveForm[duty][0];
        u8 high = waveForm[duty][1];
        int volStepCounter = volStep * volSweep / 64.0 * 1e9;
        int soundLength = (64 - len) / 256.0 * 1e9 / (8 * oneClock);
        std::cout << "TimeB " << soundLength << std::endl;
        for (int i = 0; i < soundLength; ++i) {
            int volume = std::min(std::max(initialVol + i * 8 * oneClock / volStepCounter, 0), 100);
            ch2.generateGB(low * oneClock, 0);
            ch2.generateGB(high * oneClock, volume);
        }
    }


    void generatePulseA(PulseA &pa, int pulseLen) {
        int sweepTime = pa.sweepTime;
        int sweepDir = pa.sweepDir ? 1 : -1;
        int sweepShiftN = pa.sweepNum;
        int duty = pa.dutyPattern;
        int len = pa.counter ? pa.len : pulseLen;
        int initialVol = pa.initialVol;
        int volStep = pa.envelopeDir ? 1 : -1;
        int volSweep = pa.volEnvelopeNum;
        int freq = pa.getFreq();


        const int initialFreq = 131072 / (2048 - freq);

        const int sweepTimeNs = sweepTime == 0 ? 2e9 : (double) (7.8 * 1e6 * sweepTime);

        const u8 waveForm[4][2] = {{1, 7},
                                   {2, 6},
                                   {4, 4},
                                   {6, 2}};
        const u8 low = waveForm[duty][0];
        const u8 high = waveForm[duty][1];
        const long long volStepCounterNs = volStep * volSweep / 64.0 * 1e9;
        const int soundLengthNs = (64 - len) / 256.0 * 1e9;
        std::cout << "TimeA " << soundLengthNs << std::endl;
        int activeFreq = initialFreq;
        long long timePassedNs = 0;
        long long a = 0;
        while (timePassedNs <= soundLengthNs) {
            int sweeps = timePassedNs / sweepTimeNs;
            double newFreq = initialFreq * pow((1 + sweepDir / pow(2, sweepShiftN)), sweeps);
            if ((int) newFreq != activeFreq) {
                a = timePassedNs;
            }
            activeFreq = std::min((double) 2047, std::max((double) 20, newFreq));
            int oneClock = 1e9 / activeFreq / 8;

            int volume = std::min(std::max(initialVol + timePassedNs / volStepCounterNs, 0LL), 100LL);

            ch1.generateGB(low * oneClock, 1);
            ch1.generateGB(high * oneClock, volume);
            timePassedNs += (8 * oneClock);
        }
    }


    void generateWave(Wave &w, int soundLen) {

        u8 soundLenFmt = w.counter ? w.len : soundLen;
        u8 outputLevel = w.outputLevel;
        uint32_t freqFmt = w.getFreq();


        long long soundLenNs = 1e9 * (256 - soundLenFmt) / 256;
        uint16_t soundScale = (outputLevel - 1);
        uint16_t freq = 65536 / (2048 - freqFmt);
        long long oneClock = 1e9 / freq / 32;

        std::array<u8, 32> waveForm;
        for (int i = 0; i < 16; ++i) {
            waveForm[2 * i] = (waveData[i].upper >> soundScale) * 8;
            waveForm[2 * i + 1] = (waveData[i].lower >> soundScale) * 8;
        }

        auto iter = back_inserter(ch3.sampleBuf);

        for (int i = 0; i < soundLenNs / oneClock / 32; ++i) {
            int amountToCopy = 32LL; // possible bug over here copying too much
            iter = copy(waveForm.begin(), waveForm.end(), iter);
        }

    }
};


#endif //GBA_EMULATOR_AUDIO_DRIVER_H
