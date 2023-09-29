//
// Created by jc on 24/09/23.
//

#ifndef GBA_EMULATOR_AUDIO_TEST_H
#define GBA_EMULATOR_AUDIO_TEST_H

//
// Created by jc on 21/09/23.
//
/*
 *  This extra small demo sends a random samples to your speakers.
 */

//#define ANW

#include <alsa/asoundlib.h>
#include <vector>
#include <valarray>
#include <iostream>
#include <memory>
#include <functional>

static char *device = "default";            /* playback device */
//unsigned char buffer[16*1024];              /* some random data */
using namespace std;


struct PulseAS {
    uint8_t T: 1;
    uint8_t L: 1;
    uint8_t unused: 3;
    uint8_t _F1: 3;
    uint8_t _F2: 8;
    uint8_t V: 4;
    uint8_t A: 1;
    uint8_t P1: 3;
    uint8_t D: 2;
    uint8_t Len: 6;
    uint8_t unused2: 1;
    uint8_t P2: 3;
    uint8_t N: 1;
    uint8_t S: 3;
};

struct PulseBS {
    uint8_t T: 1;
    uint8_t L: 1;
    uint8_t unused: 3;
    uint8_t _F1: 3;
    uint8_t _F2: 8;
    uint8_t V: 4;
    uint8_t A: 1;
    uint8_t P1: 3;
    uint8_t D: 2;
    uint8_t Len: 6;

    uint16_t getF() const {
        return (_F1 << 8) | _F2;
    }

} typedef PulseBS;

struct WaveS {
    uint8_t T: 1;
    uint8_t L: 1;
    uint8_t unused1: 3;
    uint8_t _F1: 3;
    uint8_t _F2: 8;
    uint8_t unused2: 1;
    uint8_t V: 2;
    uint8_t unused3: 5;
    uint8_t Len: 8;
    uint8_t unused4: 8;
};


void channel2(::uint64_t nr21, ::uint64_t nr22, ::uint64_t nr23, ::uint64_t nr24, vector<char> &samples) {

    uint32_t reg = (nr21 << 24) | (nr22 << 16) | (nr23 << 8) | nr24;
    PulseBS pb = *reinterpret_cast<PulseBS *>(&reg);

    // writing pb.T to 1
    if (pb.T == 1) {
        // channel enabled
        // if length counter is 0, then set to 64.
        // all timers reloaded,
        // channel vol reloaded
        //

    } else {

    }

}

class Timer {
    const std::function<void()> f;
    uint16_t timer;
    const uint16_t FREQ;
public:
    Timer(const std::function<void()> &f2, ::uint16_t freq) : f{f2}, FREQ{freq} {}

    void systemClock() {
        if (--timer <= 0) {
            f();
            timer = FREQ;
        }
    }
};

class SampleBuffer {

public:
    vector<uint8_t> sampleBuf;
    static constexpr size_t SAMPLES_PER_SECOND = 16384;

    SampleBuffer() {
        sampleBuf.reserve(100000);
    }

    void generateWave(long long microseconds, long freqHz, long volume, int offset) {

        size_t samplesInWaveForm = SAMPLES_PER_SECOND / freqHz;
        long long samples = microseconds * freqHz * samplesInWaveForm / 1000000;

        constexpr double PI = 3.14159265;
        for (int i = 0; i < samples; ++i) {
            double sinValue = sin(2 * PI * i / samplesInWaveForm) * volume / 2 + volume / 2 + offset;
            sampleBuf.push_back((uint8_t) sinValue);
        }
    }

    void generateGB(long long nanoseconds, long soundLevel) {
        int samples = SAMPLES_PER_SECOND * nanoseconds / 1000000000;
        sampleBuf.insert(sampleBuf.end(), samples, soundLevel);
    }
};

class Channel {
public:
    bool lengthEnabled;
    bool isEnabled;
    bool isDACOn;
    uint16_t volume;
    uint16_t len;
    Timer freqT; // dependent on channel
    Timer lenT; // 256 hz

    int8_t vol;
    uint8_t volInc;
    uint8_t volClock;
    uint8_t volSweepNum;
    Timer volT; // vol 64 hz
    Timer sweepT; // 128 hz

    SampleBuffer samples;

    Channel(SampleBuffer &samples)
            : lengthEnabled{false}, isEnabled{true}, isDACOn{true}, volume{0}, len{0},
              freqT{Timer{[=]() { channelClock(); }, 1}},
              lenT{Timer{[=]() { channelClock(); }, 1}},
              volT{Timer{[=]() { channelClock(); }, 1}},
              sweepT{Timer{[=]() { channelClock(); }, 1}} {}

    void disable() {
        isEnabled = false;
        lengthEnabled = false;
        volume = 0;
        len = 0;
    }

    // writing to nrx1 loads length - can be reloaded at any time.
    void lengthCounterF() {

        assert(len != 0);

        if (--len == 0) {
            disable();
        }
    }

    void volEnvelopeF() {
        if (volClock == volSweepNum - 1) {
            volClock = (volClock + 1) % volSweepNum;
            vol = min((vol + volInc), 15) & 0xF;
        }
    }

    void systemClock() {
//        if (isEnabled) {
//            freqT.systemClock();
//            lenT.systemClock();
//            if (lengthEnabled && --lengthTimer == 0) {
//                if (--len == 0) {
//                    disable();
//                }
//                lengthTimer = LEN_WAVEFORM_GEN;
//            }
//            if (--freqTimer == 0) {
//                channelClock();
//                freqTimer = FREQ_WAVEFORM_GEN;
//            }
//        }
    }

    void channelClock() {
        uint8_t output = 0;
        if (isEnabled) {

        }

        if (--len == 0) {
            isEnabled = false;
        }
    }


};

class PulseB : Channel {
public:

    // frequency timer is
    const static uint8_t wavePattern[4];

    uint16_t lowFreq;
    uint16_t highFreq;
    uint8_t wavePatternIx;
    uint8_t wavePatternData;

    PulseB(const PulseBS data, SampleBuffer &sb) :
            Channel(sb) {
        assert(data.getF() < 2048);
        reinit(data);
    }

    void reinit(const PulseBS data) {
        lowFreq = data.getF();
        highFreq = (uint16_t) (131072 / (2048 - data.getF()));
        wavePatternIx = 0;
        wavePatternData = wavePattern[data.D];
    }

    void systemClock() {
        volT.systemClock();
        sweepT.systemClock();
        freqT.systemClock();
    }

    void freqClock() {

    }


};

class Mixer {
public:
    Channel sq1;
    Channel sq2;
    Channel wave;
    Channel noise;

};

void generatePulseB(SampleBuffer &buffer, int duty, int len, int initialVol, int volStep, int volSweep, int freq) {
    int freqHz = 131072 / (2048 - freq);
    int oneClock = 1e9 / freqHz / 8;

    uint8_t waveForm[4][2] = {{1, 7},
                              {2, 6},
                              {4, 4},
                              {6, 2}};
    uint8_t low = waveForm[duty][0];
    uint8_t high = waveForm[duty][1];
    int volStepCounter = volStep * volSweep / 64.0 * 1e9;
    int soundLength = (64 - len) / 256.0 * 1e9 / (8 * oneClock);
    for (int i = 0; i < soundLength; ++i) {
        int volume = min(max(initialVol + i * 8 * oneClock / volStepCounter, 0), 100);
        buffer.generateGB(low * oneClock, 0);
        buffer.generateGB(high * oneClock, volume);
    }
    cout << "Volume," << min(max(initialVol + soundLength * 8 * oneClock / volStepCounter, 0), 100) << endl;
}


void
generatePulseA(SampleBuffer &buffer, int sweepTime, int sweepDir, int sweepShiftN, int duty, int len, int initialVol,
               int volStep, int volSweep, int freq) {
    const int lenTemporary = 64;

    const int initialFreq = 131072 / (2048 - freq);

    const int sweepTimeNs = sweepTime == 0 ? 2e9 : (double) (7.8 * 1e6 * sweepTime);

    const uint8_t waveForm[4][2] = {{1, 7},
                                    {2, 6},
                                    {4, 4},
                                    {6, 2}};
    const uint8_t low = waveForm[duty][0];
    const uint8_t high = waveForm[duty][1];
    const long long volStepCounterNs = volSweep == 0 ? 10 * 1e9 : volSweep / 64.0 * 1e9;
    const int soundLengthNs = (lenTemporary - len) / 256.0 * 1e9;
    int activeFreq = initialFreq;
    long long timePassedNs = 0;
    long long a = 0;
    while (timePassedNs <= soundLengthNs) {
        int sweeps = timePassedNs / sweepTimeNs;
        double newFreq = initialFreq * pow((1 + sweepDir / pow(2, sweepShiftN)), sweeps);
        if ((int) newFreq != activeFreq) {
            a = timePassedNs;
        }
        activeFreq = min((double) 2047, max((double) 20, newFreq));
        int oneClock = 1e9 / activeFreq / 8;

        int volume = min(max(initialVol + volStep * timePassedNs / volStepCounterNs, 0LL), 100LL);
        cout << "Volume, time-passed: " << volume << " " << (timePassedNs - a) / 1e6 << endl;

        buffer.generateGB(low * oneClock, 0);
        buffer.generateGB(high * oneClock, volume);
        timePassedNs += (8 * oneClock);
    }
}


void generateWave(SampleBuffer &sb, uint8_t soundLenFmt, uint8_t outputLevel, uint32_t freqFmt,
                  array<uint8_t, 16> waveFormCompressed) {

    long long soundLenNs = 1e9 * (256 - soundLenFmt) / 256;
    uint16_t soundScale = (outputLevel - 1);
    uint16_t freq = 65536 / (2048 - freqFmt);
    long long oneClock = 1e9 / freq / 32;

    array<uint8_t, 32> waveForm;
    for (int i = 0; i < waveFormCompressed.size(); ++i) {
        waveForm[2 * i] = ((waveFormCompressed[i] >> 4) >> soundScale) * 8;
        waveForm[2 * i + 1] = ((waveFormCompressed[i] & 0xf) >> soundScale) * 8;
    }


    auto iter = back_inserter(sb.sampleBuf);

    for (int i = 0; i < soundLenNs / oneClock / 32; ++i) {
        int amountToCopy = 32LL; // possible bug over here copying too much
        iter = copy(waveForm.begin(), waveForm.end(), iter);
    }

}


int main() {
    int err;
    unsigned int i;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;


#ifndef ANW
    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_U8,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  1,
                                  SampleBuffer::SAMPLES_PER_SECOND,
                                  1,
                                  280000)) < 0) {   /* 0.5sec */
        printf("Playback open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
#endif


    SampleBuffer buffer1;
    SampleBuffer buffer2;
    SampleBuffer buffer3;
    SampleBuffer buffer4;
//    generatePulseB(buffer1, 2, 0, 50, -1, 1, 1600);
    generatePulseA(buffer2, 1, -1, 5, 2, 0, 100, -1, 1, 1900);
//    generatePulseA(buffer2, 0, 0, 0, 2, 49, 15, -1, 0, 1985);
//    generatePulseA(buffer2, 0, 0, 0, 2, 49, 100, -1, 0, 1923);
//    generatePulseA(buffer2, 0, 0, 0, 2, 0, 100, -5, 1, 1985);
//    generatePulseB(buffer3, 2, 0, 50, -1, 1, 800);
//    generatePulseA(buffer4, 1, -1, 5, 2, 0, 100, -1, 1, 1000);

    array<uint8_t, 16> waveForm1{0x01, 0x24, 0x7f, 0x3c, 0xa2, 0x47, 0x67, 0x67, 0x76, 0x76, 0x74, 0x24, 0x67, 0xff,
                                 0x42, 0x10};
    array<uint8_t, 16> waveForm2{0x00, 0xdd, 0x88, 0x00, 0xcc, 0x99, 0x00, 0xaa, 0xaa, 0x00, 0xff, 0xff, 0xff, 0xaa,
                                 0x44, 0x00};
    vector<uint8_t> silence(buffer1.SAMPLES_PER_SECOND / 4, 0);

    vector<uint8_t> resultBuffer{};
    resultBuffer.reserve(buffer1.sampleBuf.size() + buffer2.sampleBuf.size() + buffer3.sampleBuf.size() +
                         min(min(buffer1.sampleBuf.size(), buffer1.sampleBuf.size()), buffer1.sampleBuf.size()));

    auto iter = back_inserter(resultBuffer);

//    iter = copy(buffer1.sampleBuf.begin(), buffer1.sampleBuf.end(), iter);
//    iter = copy(silence.begin(), silence.end(), iter);
    iter = copy(buffer2.sampleBuf.begin(), buffer2.sampleBuf.end(), iter);
//    iter = copy(silence.begin(), silence.end(), iter);

//    for (int i = 0; i < 6; ++i) {
//        for (int i = 0;
//             i < buffer1.sampleBuf.size() && i < buffer2.sampleBuf.size(); ++i) {
//            *iter = buffer1.sampleBuf[i] + buffer2.sampleBuf[i];
//            ++iter;
//        }
//    }

//    iter = copy(buffer3.sampleBuf.begin(), buffer3.sampleBuf.end(), iter);
//    iter = copy(silence.begin(), silence.end(), iter);
//    iter = copy(buffer4.sampleBuf.begin(), buffer4.sampleBuf.end(), iter);
//    iter = copy(silence.begin(), silence.end(), iter);
//
//    for (int i = 0; i < 6; ++i) {
//        for (int i = 0; i < buffer3.sampleBuf.size() && i < buffer4.sampleBuf.size(); ++i) {
//            *iter = buffer3.sampleBuf[i] + buffer4.sampleBuf[i];
//            ++iter;
//        }
//    }

    auto samplesPtr = &resultBuffer[0];
    auto samplesSize = resultBuffer.size();

//    for(int i = 0; i < 5; ++i) {
//        buffer.generate(1000000, 260, 100, i * 10);
//    }

//    for (i = 0; i < sizeof(buffer); i++)
//        buffer[i] = random() & 0xff;

#ifndef ANW
    for (int i = 0; i < samplesSize / 80; ++i) {
//        frames = snd_pcm_writei(handle, samplesPtr, samplesSize);
        frames = snd_pcm_writei(handle, samplesPtr + i * 80, 80);
        if (frames < 0) {
            frames = snd_pcm_recover(handle, frames, 0);
        }
        if (frames < 0) {
            printf("snd_pcm_writei failed: %s\n", snd_strerror(frames));
        }
        if (frames > 0 && frames < (long) samplesSize) {
            printf("Short write (expected %li, wrote %li)\n", (long) samplesSize, frames);
        }
    }

    usleep(3000000);

    /* pass the remaining samples, otherwise they're dropped in close */
    err = snd_pcm_drain(handle);
    if (err < 0)
        printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
    snd_pcm_close(handle);
    return 0;
#endif
}


#endif //GBA_EMULATOR_AUDIO_TEST_H
