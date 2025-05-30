// instruments.h
// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp, songgen.h, instruments.h
// The other linesplus code is free and cannot be resold.
// Interested parties can find my contact information at https://github.com/ZacGeurts

#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <cmath>
#include <random>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <functional>
#include <map>
#include <algorithm>
#include <string>
#include <set>
#include <memory>
#include <iostream>
#include <SDL2/SDL.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

struct AutomationPoint {
    float time;
    float value;
    AutomationPoint(float t, float v) : time(t), value(v) {}
};

class SampleManager {
public:
    std::vector<float> getSample(const std::string& sampleName, float pitch, float volume, float duration) {
        std::vector<float> samples(static_cast<size_t>(AudioUtils::DEFAULT_SAMPLE_RATE * duration));
        for (size_t i = 0; i < samples.size(); ++i) {
            float t = i / AudioUtils::DEFAULT_SAMPLE_RATE;
            samples[i] = generateInstrumentWave(sampleName, t, pitch, duration) * volume;
        }
        return samples;
    }
};

// some reason
namespace AudioUtils {
    constexpr float DEFAULT_SAMPLE_RATE = 44100.0f; // max SDL2 supports
    constexpr int BUFFER_SIZE = 128;
    constexpr int RING_BUFFER_COUNT = 4;

    class RandomGenerator {
        thread_local static std::vector<uint8_t> buffer;
        thread_local static size_t buffer_pos;
        static constexpr size_t BUFFER_SIZE = 1024;
        std::random_device rd;
        std::mt19937 gen;
        std::uniform_real_distribution<float> dist;

        static void fill_buffer() {
            buffer.resize(BUFFER_SIZE);
            buffer_pos = 0;
            #ifdef _WIN32
            HCRYPTPROV hCryptProv = 0;
            if (!CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
                throw std::runtime_error("Failed to acquire cryptographic context");
            }
            if (!CryptGenRandom(hCryptProv, BUFFER_SIZE, buffer.data())) {
                CryptReleaseContext(hCryptProv, 0);
                throw std::runtime_error("Failed to generate random bytes");
            }
            CryptReleaseContext(hCryptProv, 0);
            #else
            int fd = open("/dev/urandom", O_RDONLY);
            if (fd == -1) {
                throw std::runtime_error("Failed to open /dev/urandom");
            }
            ssize_t bytes_read = read(fd, buffer.data(), BUFFER_SIZE);
            close(fd);
            if (bytes_read != static_cast<ssize_t>(BUFFER_SIZE)) {
                throw std::runtime_error("Failed to read random bytes");
            }
            #endif
        }

        uint32_t get_random_uint32() {
            if (buffer_pos + 4 > buffer.size()) {
                fill_buffer();
            }
            uint32_t result = 0;
            for (int i = 0; i < 4; ++i) {
                result = (result << 8) | buffer[buffer_pos++];
            }
            return result;
        }

        float random_float() {
            constexpr uint64_t max_val = 1ULL << 53;
            uint64_t x = get_random_uint32();
            x = (x << 32) | get_random_uint32();
            x &= (max_val - 1);
            return static_cast<float>(x) / max_val;
        }

    public: // AI says latest best rng
        RandomGenerator() : rd(), gen(rd()), dist(-1.0f, 1.0f) {
            fill_buffer();
        }

        float generateWhiteNoise() { return dist(gen); }
        float generatePinkNoise() {
            thread_local static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
            float white = dist(gen);
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            return 0.2f * (b0 + b1 + b2 + white * 0.1848f);
        }
        float generateUniform(float min, float max) {
            std::uniform_real_distribution<float> uniform_dist(min, max);
            return uniform_dist(gen);
        }
    };

    thread_local std::vector<uint8_t> RandomGenerator::buffer;
    thread_local size_t RandomGenerator::buffer_pos = 0;

    class Distortion {
        float drive, threshold;
    public:
        Distortion(float driveFactor = 2.0f, float clipThreshold = 0.7f) : drive(driveFactor), threshold(clipThreshold) {}
        float process(float input) {
            float x = input * drive;
            return std::max(std::min(x, threshold), -threshold) / threshold;
        }
    };

    class LowPassFilter {
        float cutoffFreq, x1, y1;
    public:
        LowPassFilter(float cutoff = 1000.0f) : cutoffFreq(cutoff), x1(0.0f), y1(0.0f) {}
        float process(float input) {
            // Assume DEFAULT_SAMPLE_RATE at playback
            float alpha = 1.0f / (1.0f + 2.0f * M_PI * cutoffFreq / DEFAULT_SAMPLE_RATE);
            float output = alpha * input + (1.0f - alpha) * y1;
            x1 = input; y1 = output;
            return output;
        }
        void setCutoff(float cutoff) { cutoffFreq = cutoff; }
    };

    class BandPassFilter {
        float centerFreq, bandwidth, x1, x2, y1, y2;
    public:
        BandPassFilter(float center = 1000.0f, float bw = 0.5f)
            : centerFreq(center), bandwidth(bw), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
        float process(float input) {
            float w0 = 2.0f * M_PI * centerFreq / DEFAULT_SAMPLE_RATE;
            float alpha = std::sin(w0) * std::sinh(std::log(2.0f) / 2. * bandwidth * w0 / std::sin(w0));
            float b0 = alpha, b1 = 0.0f, b2 = -alpha;
            float a0 = 1.0f + alpha, a1 = -2.0f * std::cos(w0), a2 = 1.0f - alpha;
            float output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return output;
        }
    };

    class HighPassFilter {
        float cutoffFreq, q, x1, x2, y1, y2;
    public:
        HighPassFilter(float cutoff = 100.0f, float qFactor = 0.707f)
            : cutoffFreq(cutoff), q(qFactor), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
        float process(float input) {
            float omega = 2.0f * M_PI * cutoffFreq / DEFAULT_SAMPLE_RATE;
            float alpha = std::sin(omega) / (2.0f * q);
            float cosOmega = std::cos(omega);
            float a0 = 1.0f + alpha;
            float b0 = (1.0f + cosOmega) / 2.0f;
            float b1 = -(1.0f + cosOmega);
            float b2 = (1.0f + cosOmega) / 2.0f;
            float a1 = -2.0f * cosOmega;
            float a2 = 1.0f - alpha;
            float output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return output;
        }
    };

    class Reverb {
        std::vector<float> delayBuffer;
        size_t bufferSize;
        size_t writePos;
        float decay, mix;
        float delayTime;

    public:
        Reverb(float delayTime = 0.1f, float decayFactor = 0.5f, float mixFactor = 0.3f)
            : bufferSize(static_cast<size_t>(delayTime * DEFAULT_SAMPLE_RATE)), writePos(0),
              decay(decayFactor), mix(mixFactor), delayTime(delayTime) {
            delayBuffer.resize(bufferSize, 0.0f);
        }
        float process(float input) {
            if (bufferSize == 0) return input;
            size_t readPos = (writePos + bufferSize - bufferSize / 2) % bufferSize;
            float output = input + decay * delayBuffer[readPos];
            delayBuffer[writePos] = input + decay * delayBuffer[readPos];
            writePos = (writePos + 1) % bufferSize;
            return input * (1.0f - mix) + output * mix;
        }
    };

    class AudioProtector {
        HighPassFilter dcBlocker;
        float fadeOutTime, maxGain;

    public:
        AudioProtector(float fadeTime = 0.005f, float gain = 0.9f)
            : dcBlocker(20.0f, 0.707f), fadeOutTime(fadeTime), maxGain(gain) {}
        float process(float input, float t, float dur) {
            float output = dcBlocker.process(input);
            if (t > dur - fadeOutTime) {
                float fade = 1.0f - (t - (dur - fadeOutTime)) / fadeOutTime;
                output *= std::max(0.0f, std::min(1.0f, fade));
            }
            output = std::tanh(output * 1.2f) / 1.2f;
            float absOutput = std::abs(output);
            if (absOutput > maxGain) {
                output *= maxGain / absOutput;
            }
            return output;
        }
    };
}

// Include instrument headers
#include "../instruments/kick.h"
#include "../instruments/hihat.h"
#include "../instruments/snare.h"
#include "../instruments/clap.h"
#include "../instruments/tom.h"
#include "../instruments/subbass.h"
#include "../instruments/syntharp.h"
#include "../instruments/leadsynth.h"
#include "../instruments/pad.h"
#include "../instruments/cymbal.h"
#include "../instruments/vocal.h"
#include "../instruments/flute.h"
#include "../instruments/trumpet.h"
#include "../instruments/bass.h"
#include "../instruments/guitar.h"
#include "../instruments/saxophone.h"
#include "../instruments/piano.h"
#include "../instruments/violin.h"
#include "../instruments/organ.h"
#include "../instruments/cello.h"
#include "../instruments/steelguitar.h"
#include "../instruments/sitar.h"

#define DEBUG_LOG 0

namespace Instruments {
    struct FormantFilter {
        float centerFreq, bandwidth;
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        FormantFilter(float freq, float bw)
            : centerFreq(freq), bandwidth(bw), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void updateCoefficients() {
            float r = std::exp(-M_PI * bandwidth / AudioUtils::DEFAULT_SAMPLE_RATE);
            float theta = 2.0f * M_PI * centerFreq / AudioUtils::DEFAULT_SAMPLE_RATE;
            b0 = 1.0f - r; b1 = 0.0f; b2 = 0.0f;
            a1 = -2.0f * r * std::cos(theta); a2 = r * r;
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return output;
        }
        void setParameters(float freq, float bw) { centerFreq = freq; bandwidth = bw; updateCoefficients(); }
    };

    struct WaveguideState {
        std::vector<float> forwardWave, backwardWave;
        size_t delayLineSize = 0;
        size_t writePos = 0;
        float lastFreq = 0.0f;
        float pressure = 0.0f;
    };

    struct Note {
        float startTime, duration, freq, volume, velocity;
        int phoneme;
        bool open;
    };

    struct Section {
        std::string name;
        float startTime, endTime;
    };

    struct Part {
        std::string instrument;
        std::vector<Note> notes;
        std::vector<AutomationPoint> panAutomation, volumeAutomation, reverbMixAutomation;
        float pan, reverbMix;
        bool useDistortion, useReverb;
    };

    struct Song {
        float duration;
        int channels;
        std::vector<Section> sections;
        std::vector<Part> parts;
    };

    struct ActiveNote {
        size_t noteIndex;
        float startTime, endTime;
    };

    struct PlaybackState {
        Song song;
        float currentTime;
        size_t currentSectionIdx;
        bool playing;
        std::vector<size_t> nextNoteIndices;
        std::vector<std::vector<ActiveNote>> activeNotes;
        std::vector<AudioUtils::Reverb> reverbs;
        std::vector<AudioUtils::Distortion> distortions;
        PlaybackState() : currentTime(0.0f), currentSectionIdx(0), playing(false) {}
    };

    // Starts an instrument from their folder /instruments/ function for dynamic instantiation
    float generateInstrumentWave(const std::string& instrument, float t, float freq, float dur, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE) {
        if (instrument == "cello") {
            Cello inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "tom") {
            Tom inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "kick") {
            Kick inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "hihat") {
            HiHat inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "snare") {
            Snare inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "clap") {
            Clap inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "subbass") {
            SubBass inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "syntharp") {
            SynthArp inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "leadsynth") {
            LeadSynth inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "pad") {
            Pad inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "cymbal") {
            Cymbal inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "vocal_0") {
            Vocal inst(1.0f);
            return inst.generateWave(t, freq, 0, dur, 0); // Default phoneme=0, depth=1 (male voice)
		} else if (instrument == "vocal_1") {
            Vocal inst(1.0f);
            return inst.generateWave(t, freq, 0, dur, 1); // Default phoneme=0, depth=1 (female voice)
        } else if (instrument == "flute") {
            Flute inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "trumpet") {
            Trumpet inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "bass") {
            Bass inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "guitar") {
            Guitar inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "saxophone") {
            Saxophone inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "piano") {
            Piano inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "violin") {
            Violin inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "organ") {
            Organ inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "steelguitar") {
            SteelGuitar inst(1.0f);
            return inst.generateWave(t, freq, dur);
        } else if (instrument == "sitar") {
            Sitar inst(1.0f);
            return inst.generateWave(t, freq, dur);
        }
        return 0.0f;
    }

    float interpolateAutomation(float t, const std::vector<AutomationPoint>& points, float defaultValue) {
        if (points.empty()) return defaultValue;
        if (t <= points.front().time) return points.front().value;
        if (t >= points.back().time) return points.back().value;
        for (size_t i = 1; i < points.size(); ++i) {
            if (t >= points[i-1].time && t < points[i].time) {
                float t0 = points[i-1].time, t1 = points[i].time;
                float v0 = points[i-1].value, v1 = points[i].value;
                return v0 + (v1 - v0) * (t - t0) / (t1 - t0);
            }
        }
    	return defaultValue;
    }
    size_t countNotesInSection(const Song& song, const Section& section);
    std::string getInstrumentsInSection(const Song& song, const Section& section);
}

#endif // INSTRUMENTS_H