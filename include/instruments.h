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
#include <SDL2/SDL.h>

#define DEBUG_LOG 0 // Set to 1 for debug logging
#define M_PI 3.14159265358979323846

// Forward declarations for wave generation functions
static float generateKickWave(float t, float freq, float dur);
static float generateHiHatWave(float t, float freq, bool open, float dur);
static float generateSnareWave(float t, float dur);
static float generateClapWave(float t, float dur);
static float generateTomWave(float t, float freq, float dur);
static float generateSubBassWave(float t, float freq, float dur);
static float generateSynthArpWave(float t, float freq, float dur);
static float generateLeadSynthWave(float t, float freq, float dur);
static float generatePadWave(float t, float freq, float dur);
static float generateCymbalWave(float t, float freq, float dur);
static float generateVocalWave(float t, float freq, int phoneme, float dur, int depth);
static float generateFluteWave(float t, float freq, float dur);
static float generateTrumpetWave(float t, float freq, float dur);
static float generateGuitarWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateOrganWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateBassWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generatePianoWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateViolinWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateCelloWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateMarimbaWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateSteelGuitarWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateSitarWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);
static float generateSaxophoneWave(float sampleRate, float freq, float time, float dur, Instruments::KarplusStrongState& state1, Instruments::KarplusStrongState& state2);

// getTailDuration
static float getTailDuration(const std::string& instrument) {
    if (instrument == "cymbal") return 2.0f;
    if (instrument == "guitar") return 1.5f;
    if (instrument == "syntharp") return 1.2f;
    if (instrument == "subbass") return 0.8f;
    if (instrument == "kick") return 0.5f;
    if (instrument == "snare") return 0.6f;
    if (instrument == "piano") return 2.0f;
    if (instrument == "violin") return 2.5f;
    if (instrument == "cello") return 3.0f;
    if (instrument == "marimba") return 1.0f;
    if (instrument == "steelguitar") return 1.8f;
    if (instrument == "sitar") return 2.0f;
    return 1.5f; // default tail
}

namespace AudioUtils {
const float SAMPLE_RATE = 44100.0f;
const int CHANNELS = 8;
const int BUFFER_SIZE = 128; // 2.9ms latency
const int RING_BUFFER_COUNT = 4;

// RandomGenerator
class RandomGenerator {
    thread_local static std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
public:
    RandomGenerator() : dist(-1.0f, 1.0f) {}
    float generateWhiteNoise() { return dist(rng); }
    float generatePinkNoise() {
        static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
        float white = dist(rng);
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        return 0.2f * (b0 + b1 + b2 + white * 0.1848f);
    }
};
thread_local std::mt19937 RandomGenerator::rng(std::random_device{}());

// Distortion
class Distortion {
    float drive, threshold;
public:
    Distortion(float driveFactor = 2.0f, float clipThreshold = 0.7f) : drive(driveFactor), threshold(clipThreshold) {}
    float process(float input) {
        float x = input * drive;
        return std::max(std::min(x, threshold), -threshold) / threshold;
    }
};

// LowPassFilter
class LowPassFilter {
    float cutoffFreq, sampleRate, x1, y1;
public:
    LowPassFilter(float cutoff, float sr) : cutoffFreq(cutoff), sampleRate(sr), x1(0.0f), y1(0.0f) {}
    float process(float input) {
        float alpha = 1.0f / (1.0f + 2.0f * M_PI * cutoffFreq / sampleRate);
        float output = alpha * input + (1.0f - alpha) * y1;
        x1 = input; y1 = output;
        return output;
    }
};

// BandPassFilter
class BandPassFilter {
    float centerFreq, bandwidth, sampleRate, x1, x2, y1, y2;
public:
    BandPassFilter(float center, float bw, float sr) : centerFreq(center), bandwidth(bw), sampleRate(sr), 
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
    float process(float input) {
        float w0 = 2.0f * M_PI * centerFreq / sampleRate;
        float alpha = std::sin(w0) * std::sinh(std::log(2.0f) / 2.0f * bandwidth * w0 / std::sin(w0));
        float b0 = alpha, b1 = 0.0f, b2 = -alpha;
        float a0 = 1.0f + alpha, a1 = -2.0f * std::cos(w0), a2 = 1.0f - alpha;
        float output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;
        x2 = x1; x1 = input; y2 = y1; y1 = output;
        return output;
    }
};
} // namespace AudioUtils

namespace Instruments {
// Structures
struct KarplusStrongState {
    float lastFreq;
    size_t delayLineSize;
    size_t writePos;
    std::vector<float> delayLine;
    KarplusStrongState() : lastFreq(0.0f), delayLineSize(0), writePos(0), delayLine() {}
};

struct FormantFilter {
    float centerFreq, bandwidth, sampleRate;
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
    FormantFilter(float freq, float bw, float sr) : centerFreq(freq), bandwidth(bw), sampleRate(sr),
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) { updateCoefficients(); }
    void updateCoefficients() {
        float r = std::exp(-M_PI * bandwidth / sampleRate);
        float theta = 2.0f * M_PI * centerFreq / sampleRate;
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

struct InstrumentSample {
    float freq;
    float dur;
    int phoneme;
    bool open;
    std::vector<float> samples;
    InstrumentSample(float f = 0.0f, float d = 0.0f, int p = -1, bool o = false, std::vector<float> s = {})
        : freq(f), dur(d), phoneme(p), open(o), samples(std::move(s)) {}
};

// Wave generation functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static float generateKickWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.005f;
    float decay = 0.15f;
    float sustain = 0.4f;
    float release = 0.1f;
    float env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(t - dur) / release);
    }
    float baseFreq = freq * 0.8f;
    float pitchDecay = std::exp(-15.0f * t / dur);
    float pitchMod = baseFreq * (2.0f * pitchDecay + 0.5f);
    float sine = std::sin(2.0f * M_PI * pitchMod * t);
    float subSine = 0.3f * std::sin(2.0f * M_PI * (baseFreq * 0.5f) * t);
    float clickEnv = std::exp(-80.0f * t);
    float whiteNoise = rng.generateWhiteNoise();
    float pinkNoise = rng.generatePinkNoise();
    AudioUtils::BandPassFilter clickFilter(2500.0f, 1.0f, 44100.0f);
    float click = clickFilter.process(0.6f * whiteNoise + 0.4f * pinkNoise) * clickEnv * 0.25f;
    float output = env * (0.6f * sine + 0.2f * subSine + 0.2f * click);
    float filterCutoff = 150.0f + 1000.0f * pitchDecay;
    AudioUtils::LowPassFilter filter(filterCutoff, 44100.0f);
    output = filter.process(output);
    AudioUtils::Distortion dist(1.8f, 0.8f);
    output = dist.process(output);
    output = std::tanh(output * 1.2f);
    output = std::max(-1.0f, std::min(1.0f, output * 0.9f));
    return output;
}

namespace Instruments {
// Structures
struct KarplusStrongState {
    float lastFreq;
    size_t delayLineSize;
    size_t writePos;
    std::vector<float> delayLine;
    KarplusStrongState() : lastFreq(0.0f), delayLineSize(0), writePos(0), delayLine() {}
};

struct FormantFilter {
    float centerFreq, bandwidth, sampleRate;
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
    FormantFilter(float freq, float bw, float sr) : centerFreq(freq), bandwidth(bw), sampleRate(sr),
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) { updateCoefficients(); }
    void updateCoefficients() {
        float r = expf(-M_PI * bandwidth / sampleRate);
        float theta = 2.0f * M_PI * centerFreq / sampleRate;
        b0 = 1.0f - r; b1 = 0.0f; b2 = 0.0f;
        a1 = -2.0f * r * cosf(theta); a2 = r * r;
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

struct InstrumentSample {
    float freq;
    float dur;
    int phoneme;
    bool open;
    std::vector<float> samples;
    InstrumentSample(float f = 0.0f, float d = 0.0f, int p = -1, bool o = false, std::vector<float> s = {})
        : freq(f), dur(d), phoneme(p), open(o), samples(std::move(s)) {}
};

// Wave generation functions (your implementations + corrected/missing ones)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static float generateKickWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    // Envelope parameters for punchy, dynamic kick
    float attack = 0.005f; // Sharper attack for immediate impact
    float decay = 0.15f;   // Shorter decay for tight body
    float sustain = 0.4f;  // Lower sustain to avoid muddiness
    float release = 0.1f;  // Quick release for clean tail
    float env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(t - dur) / release);
    }

    // Pitch-modulated body (sine + subharmonic)
    float baseFreq = freq * 0.8f; // Lower base frequency for deeper kick
    float pitchDecay = std::exp(-15.0f * t / dur); // Faster pitch drop
    float pitchMod = baseFreq * (2.0f * pitchDecay + 0.5f); // Dynamic pitch sweep
    float sine = std::sin(2.0f * M_PI * pitchMod * t); // Main body
    float subSine = 0.3f * std::sin(2.0f * M_PI * (baseFreq * 0.5f) * t); // Subharmonic for depth

    // Noise component for click/snap
    float clickEnv = std::exp(-80.0f * t); // Sharp transient
    float whiteNoise = rng.generateWhiteNoise();
    float pinkNoise = rng.generatePinkNoise();
    AudioUtils::BandPassFilter clickFilter(2500.0f, 1.0f, 44100.0f); // Center at 2.5kHz for crispness
    float click = clickFilter.process(0.6f * whiteNoise + 0.4f * pinkNoise) * clickEnv * 0.25f;

    // Combine components
    float output = env * (0.6f * sine + 0.2f * subSine + 0.2f * click);

    // Dynamic low-pass filter to shape tone
    float filterCutoff = 150.0f + 1000.0f * pitchDecay; // Sweep from 1150Hz to 150Hz
    AudioUtils::LowPassFilter filter(filterCutoff, 44100.0f);
    output = filter.process(output);

    // Saturation and distortion for warmth and grit
    AudioUtils::Distortion dist(1.8f, 0.8f); // Moderate distortion
    output = dist.process(output);
    output = std::tanh(output * 1.2f); // Soft clipping for warmth

    // Final amplitude adjustment
    output = std::max(-1.0f, std::min(1.0f, output * 0.9f)); // Prevent clipping
    return output;
}

static float generateHiHatWave(float t, float freq, bool open, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::HighPassFilter openFilter(6000.0f, 0.707f, 44100.0f);
    static AudioUtils::HighPassFilter closedFilter(10000.0f, 0.707f, 44100.0f);
    static AudioUtils::Reverb reverb(0.02f, 0.2f, 0.15f, 44100.0f);
    static AudioUtils::Distortion dist(1.2f, 0.8f);
    float release = open ? 1.5f : 0.2f;
    if (t > dur + release) return 0.0f;
    float decayTime = open ? 1.2f : 0.1f;
    float env = std::exp(-t / decayTime);
    float transient = rng.generateWhiteNoise() * std::exp(-100.0f * t);
    float whiteNoise = rng.generateWhiteNoise();
    float pinkNoise = rng.generatePinkNoise();
    float noise = 0.7f * whiteNoise + 0.3f * pinkNoise;
    AudioUtils::HighPassFilter& filter = open ? openFilter : closedFilter;
    float body = filter.process(noise) * env;
    float baseFreq = 2500.0f;
    float tonal = 0.0f;
    float freqs[3] = {baseFreq, baseFreq * 1.618f, baseFreq * 2.618f};
    for (int i = 0; i < 3; i++) tonal += 0.3f * std::sin(2.0f * M_PI * freqs[i] * t);
    if (open) {
        float lowFreqNoise = rng.generatePinkNoise() * 0.1f;
        tonal *= (1.0f + 0.2f * lowFreqNoise);
    }
    tonal *= env;
    float output = transient + 0.5f * body + 0.5f * tonal;
    output = reverb.process(output);
    output = dist.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.3f;
    return output;
}

static float generateSnareWave(float t, float dur) {
    static AudioUtils::RandomGenerator rng;
    float velocity = 0.7f + rng.generateUniform(-0.3f, 0.3f);
    velocity = std::max(0.4f, std::min(1.0f, velocity));
    if (dur < 0.05f) velocity *= 0.7f;
    float attack = 0.002f * (1.0f - 0.3f * velocity);
    float decay = 0.05f;
    float sustain = 0.2f;
    float release = 0.08f;
    float env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain * std::exp(-10.0f * (t - attack - decay));
    else env = sustain * std::exp(-(t - dur) / release);
    float noise = rng.generatePinkNoise() * 0.4f;
    static AudioUtils::BandPassFilter crackFilter(2000.0f, 1.5f, 44100.0f);
    float crack = rng.generateWhiteNoise() * std::exp(-50.0f * t) * 0.3f * velocity;
    crack = crackFilter.process(crack);
    float toneFreq = 220.0f + rng.generateUniform(-20.0f, 20.0f);
    float phase = 2.0f * M_PI * toneFreq * t;
    float tone = (1.0f - 2.0f * std::fmod(phase / M_PI, 1.0f)) * std::exp(-20.0f * t) * 0.2f * velocity;
    static AudioUtils::BandPassFilter rattleFilter(4000.0f, 1.0f, 44100.0f);
    float rattleDecay = 0.1f + rng.generateUniform(-0.02f, 0.02f);
    float rattle = rng.generateWhiteNoise() * std::exp(-t / rattleDecay) * 0.35f * velocity;
    rattle = rattleFilter.process(rattle);
    float output = env * (noise + crack + tone + rattle);
    AudioUtils::Distortion dist(1.5f, 0.5f);
    AudioUtils::Reverb reverb(0.1f, 0.5f, 0.25f);
    output = dist.process(output);
    output = reverb.process(output);
    output *= 0.6f * velocity;
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 1.0f;
    return output;
}

static float generateClapWave(float t, float dur) {
    static AudioUtils::RandomGenerator rng;
    dur = std::clamp(dur, 0.08f, 0.15f);
    float attack = 0.002f, decay = 0.03f, sustain = 0.2f, release = 0.05f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float burst1 = (t < 0.002f) ? rng.generateWhiteNoise() * 1.0f : 0.0f;
    float burst2 = (t >= 0.002f && t < 0.004f) ? rng.generateWhiteNoise() * 0.8f : 0.0f;
    float burst3 = (t >= 0.004f && t < 0.006f) ? rng.generateWhiteNoise() * 0.6f : 0.0f;
    float noise = rng.generatePinkNoise() * 0.4f;
    float tonal = rng.generateWhiteNoise() * std::sin(2.0f * M_PI * 800.0f * t) * 0.3f;
    float output = env * (burst1 + burst2 + burst3 + noise + tonal);
    static AudioUtils::Distortion dist(1.4f, 0.6f);
    static AudioUtils::Reverb reverb(0.03f, 0.3f, 0.2f);
    output = dist.process(output);
    output = reverb.process(output);
    output *= 1.0f;
    return output;
}

static float generateTomWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.01f, decay = 0.15f, sustain = 0.6f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float pitchMod = freq * (1.0f + 0.4f * std::exp(-10.0f * t / dur));
    float sine = std::sin(2.0f * M_PI * pitchMod * t) * 0.7f;
    float saw = 0.2f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);
    float noise = rng.generatePinkNoise() * std::exp(-20.0f * t / dur) * 0.1f;
    float output = env * (sine + saw + noise);
    AudioUtils::Reverb reverb(0.05f, 0.4f, 0.2f);
    AudioUtils::LowPassFilter filter(300.0f, 44100.0f);
    output = reverb.process(output);
    output = filter.process(output);
    output *= 1.0f;
    return output;
}

static float generateSubBassWave(float t, float freq, float dur) {
    freq = std::clamp(freq, 20.0f, 80.0f);
    float attack = 0.005f, decay = 0.1f, sustain = 0.6f, release = 0.25f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float sine = std::sin(2.0f * M_PI * freq * t) * 0.85f;
    float triangle = (2.0f / M_PI) * std::asin(std::sin(2.0f * M_PI * freq * 0.99f * t)) * 0.15f;
    float output = env * (sine + triangle);
    static AudioUtils::LowPassFilter filter(80.0f, 44100.0f);
    output = filter.process(output);
    output = std::tanh(output * 1.2f);
    output *= 1.0f;
    return output;
}

static float generateSynthArpWave(float t, float freq, float dur) {
    // Corrected to produce arpeggio synth sound
    static AudioUtils::RandomGenerator rng;
    float attack = 0.01f, decay = 0.1f, sustain = 0.7f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float saw = (std::fmod(freq * t, 1.0f) - 0.5f) * 0.6f;
    float square = (std::sin(2.0f * M_PI * freq * t) > 0.0f ? 1.0f : -1.0f) * 0.4f;
    float output = env * (saw + square);
    static AudioUtils::LowPassFilter filter(4000.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.1f, 0.5f, 0.3f);
    output = filter.process(output);
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.8f;
    return output;
}

static float generateLeadSynthWave(float t, float freq, float dur) {
    float attack = 0.02f, decay = 0.1f, sustain = 0.7f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float modFreq = freq * 2.5f;
    float modIndex = 0.8f + 0.4f * std::sin(2.0f * M_PI * t / dur);
    float carrier = std::sin(2.0f * M_PI * freq * t + modIndex * std::sin(2.0f * M_PI * modFreq * t));
    float saw = (std::fmod(freq * t, 1.0f) - 0.5f) * 0.3f;
    float vibrato = 1.0f + 0.02f * std::sin(2.0f * M_PI * 6.0f * t);
    float output = env * (carrier * 0.7f + saw) * vibrato;
    AudioUtils::Distortion dist(1.4f, 0.85f);
    AudioUtils::Reverb reverb(0.08f, 0.45f, 0.25f);
    AudioUtils::LowPassFilter filter(5000.0f, 44100.0f);
    output = dist.process(output);
    output = reverb.process(output);
    output = filter.process(output);
    output *= 1.0f;
    return output;
}

static float generatePadWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter filter(800.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.8f, 0.8f, 0.6f, 44100.0f);
    if (!std::isfinite(t) || t < 0.0f || !std::isfinite(freq) || freq <= 0.0f || !std::isfinite(dur)) return 0.0f;
    freq = std::max(32.7f, std::min(2093.0f, freq));
    float phase = 2.0f * M_PI * freq * t;
    float detune1 = 1.005f;
    float detune2 = 0.995f;
    float osc1 = std::sin(phase);
    float osc2 = std::sin(phase * detune1);
    float osc3 = std::sin(phase * detune2);
    float output = (osc1 + osc2 * 0.7f + osc3 * 0.7f) / 2.4f;
    float harmonic2 = 0.5f * std::sin(2.0f * phase);
    float harmonic3 = 0.3f * std::sin(3.0f * phase);
    float harmonic4 = 0.2f * std::sin(4.0f * phase);
    output += (harmonic2 + harmonic3 + harmonic4) * 0.4f;
    float noise = rng.generatePinkNoise() * 0.05f;
    output += noise;
    output = filter.process(output);
    float attack = 0.5f, decay = 0.2f, sustain = 0.8f, release = 1.0f;
    float env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else if (t < dur + release) env = sustain * std::exp(-(t - dur) / release);
    else env = 0.0f;
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
    return output;
}

static float generateCymbalWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    dur = std::clamp(dur, 0.1f, 1.5f);
    freq = (freq > 0.0f) ? std::clamp(freq, 2000.0f, 10000.0f) : 6000.0f;
    float env = std::exp(-6.0f * t / dur) * (1.0f + 0.4f * std::sin(8.0f * M_PI * t / dur));
    env = std::max(0.0f, env);
    float whiteNoise = rng.generateWhiteNoise() * 0.7f;
    float pinkNoise = rng.generatePinkNoise() * 0.3f;
    float metallic1 = std::sin(2.0f * M_PI * freq * t) * 0.2f * std::exp(-4.0f * t / dur);
    float metallic2 = std::sin(2.0f * M_PI * (freq * 1.5f) * t) * 0.15f * std::exp(-5.0f * t / dur);
    float metallic3 = std::sin(2.0f * M_PI * (freq * 2.0f) * t) * 0.1f * std::exp(-6.0f * t / dur);
    float filterMod = 0.5f + 0.5f * std::sin(2.0f * M_PI * (8000.0f + 6000.0f * std::exp(-5.0f * t / dur)) * t);
    float noise = (whiteNoise + pinkNoise) * filterMod;
    float output = env * (noise + metallic1 + metallic2 + metallic3);
    static AudioUtils::Reverb reverb(0.15f, 0.6f, 0.4f, 44100.0f);
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.3f;
    return output;
}

// forgot why I asked for this
struct VocalState {
    float current_freq = 0.0f;
    float current_dur = 0.0f;
    float start_time = 0.0f;
    bool is_new_note = true;
    float prev_output = 0.0f;
    float prev_time = -1.0f;
};

// pick new voices, I think they should seperate further
static float generateVocalWave(float t, float freq, int phoneme, float dur, int depth) {
    static AudioUtils::RandomGenerator rng;
    static VocalState male_state;
    static VocalState female_state;
    VocalState& state = (depth == 1) ? male_state : female_state;
    
    // Detect new note
    bool is_new_note = (freq != state.current_freq || t < state.prev_time || 
                       t >= state.start_time + state.current_dur + 1.5f);
    if (is_new_note) {
        state.is_new_note = true;
        state.current_freq = freq;
        state.current_dur = dur;
        state.start_time = t;
    }
    state.prev_time = t;
    
    // Envelope: Attack, Decay, Sustain, Release (ADSR)
    float attack = 0.05f, decay = 0.5f, sustain = 0.8f, release = 1.5f;
    float env_current = 0.0f, env_prev = 0.0f;
    float crossfade_dur = 0.05f;
    
    // Current note envelope
    float t_current = t - state.start_time;
    if (t_current < attack) {
        env_current = t_current / attack;
    } else if (t_current < attack + decay) {
        env_current = 1.0f - (t_current - attack) / decay * (1.0f - sustain);
    } else if (t_current < dur) {
        env_current = sustain;
    } else if (t_current < dur + release) {
        float t_rel = (t_current - dur) / release;
        env_current = sustain * (1.0f - t_rel) * std::exp(-t_rel * 4.0f);
    } else {
        env_current = 0.0f;
    }
    
    // Previous note envelope (for crossfade, up to 1.5 seconds)
    if (state.is_new_note && t_current < 1.5f) {
        float t_prev = t - (state.start_time - crossfade_dur);
        if (t_prev >= 0.0f && t_prev < state.current_dur + 1.5f) {
            if (t_prev < attack) {
                env_prev = t_prev / attack;
            } else if (t_prev < attack + decay) {
                env_prev = 1.0f - (t_prev - attack) / decay * (1.0f - sustain);
            } else if (t_prev < state.current_dur) {
                env_prev = sustain;
            } else if (t_prev < state.current_dur + release) {
                float t_rel = (t_prev - state.current_dur) / release;
                env_prev = sustain * (1.0f - t_rel) * std::exp(-t_rel * 4.0f);
            }
        }
    }
    
    // Gender-specific parameters
    float formant_scale = (depth == 1) ? 1.0f : 1.25f;
    float breath_amount = (depth == 1) ? 0.25f : 0.5f;
    float vibrato_depth = (depth == 1) ? 0.015f : 0.025f;
    float distortion_drive = (depth == 1) ? 2.0f : 1.0f;
    float filter_cutoff = (depth == 1) ? 2000.0f : 4500.0f;
    
    // Adjust fundamental frequency for gender
    float base_freq = freq * ((depth == 1) ? 0.3f : 1.2f);
    if (depth == 1) {
        base_freq = std::max(20.0f, base_freq); // Clamp to prevent cutout
    }
    
    // Phoneme selection based on freq (deterministic)
    int selected_phoneme;
    if (depth == 1) { // Male: 80-150 Hz, 14 phonemes
        float freq_normalized = (base_freq - 80.0f) / (150.0f - 80.0f);
        freq_normalized = std::max(0.0f, std::min(1.0f, freq_normalized));
        selected_phoneme = static_cast<int>(freq_normalized * 14);
    } else { // Female: 160-300 Hz, 13 phonemes
        float freq_normalized = (base_freq - 160.0f) / (300.0f - 160.0f);
        freq_normalized = std::max(0.0f, std::min(1.0f, freq_normalized));
        selected_phoneme = static_cast<int>(freq_normalized * 13);
    }
    
    // Phoneme-specific formant frequencies and bandwidths
    float f1, f2, bw1, bw2;
    if (depth == 1) { // Male phonemes
        switch (selected_phoneme) {
            case 0: // "uhh" (/ʌ/)
                f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale;
                bw1 = 140.0f; bw2 = 180.0f;
                break;
            case 1: // "ow" (/aʊ/)
                f1 = 600.0f * formant_scale; f2 = 800.0f * formant_scale;
                bw1 = 150.0f; bw2 = 170.0f;
                break;
            case 2: // "huh" (/hʌ/)
                f1 = 350.0f * formant_scale; f2 = 1000.0f * formant_scale;
                bw1 = 130.0f; bw2 = 190.0f;
                break;
            case 3: // "woof" (growl)
                f1 = 300.0f * formant_scale; f2 = 700.0f * formant_scale;
                bw1 = 160.0f; bw2 = 200.0f;
                break;
            case 4: // "ah" (/ɑ/)
                f1 = 500.0f * formant_scale; f2 = 950.0f * formant_scale;
                bw1 = 140.0f; bw2 = 180.0f;
                break;
            case 5: // "oh" (/oʊ/)
                f1 = 400.0f * formant_scale; f2 = 800.0f * formant_scale;
                bw1 = 130.0f; bw2 = 170.0f;
                break;
            case 6: // "ooh" (/u/)
                f1 = 300.0f * formant_scale; f2 = 600.0f * formant_scale;
                bw1 = 120.0f; bw2 = 160.0f;
                break;
            case 7: // "eh" (/ɛ/)
                f1 = 450.0f * formant_scale; f2 = 1100.0f * formant_scale;
                bw1 = 140.0f; bw2 = 180.0f;
                break;
            case 8: // "ar" (/ɑr/)
                f1 = 500.0f * formant_scale; f2 = 1000.0f * formant_scale;
                bw1 = 150.0f; bw2 = 190.0f;
                break;
            case 9: // "mm" (nasal)
                f1 = 350.0f * formant_scale; f2 = 900.0f * formant_scale;
                bw1 = 130.0f; bw2 = 170.0f;
                break;
            case 10: // "growl" (low buzz)
                f1 = 250.0f * formant_scale; f2 = 650.0f * formant_scale;
                bw1 = 160.0f; bw2 = 200.0f;
                break;
            case 11: // "boom" (resonant low)
                f1 = 200.0f * formant_scale; f2 = 700.0f * formant_scale;
                bw1 = 150.0f; bw2 = 190.0f;
                break;
            case 12: // "ho" (/hɔ/)
                f1 = 400.0f * formant_scale; f2 = 800.0f * formant_scale;
                bw1 = 140.0f; bw2 = 180.0f;
                break;
            case 13: // "raw" (/rɔ/)
                f1 = 450.0f * formant_scale; f2 = 850.0f * formant_scale;
                bw1 = 150.0f; bw2 = 190.0f;
                break;
            default:
                f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale;
                bw1 = 140.0f; bw2 = 180.0f;
        }
    } else { // Female phonemes
        switch (selected_phoneme) {
            case 0: // "hi" (/aɪ/)
                f1 = 800.0f * formant_scale; f2 = 2000.0f * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
                break;
            case 1: // "be" (/i/)
                f1 = 600.0f * formant_scale; f2 = 2700.0f * formant_scale;
                bw1 = 70.0f; bw2 = 90.0f;
                break;
            case 2: // "meow" (stylized /aʊ/ with glide)
                f1 = 750.0f * formant_scale; f2 = (1800.0f + 200.0f * std::sin(2.0f * M_PI * 0.5f * t)) * formant_scale;
                bw1 = 90.0f; bw2 = 110.0f;
                break;
            case 3: // "ee" (/i/)
                f1 = 550.0f * formant_scale; f2 = 2800.0f * formant_scale;
                bw1 = 70.0f; bw2 = 90.0f;
                break;
            case 4: // "ay" (/eɪ/)
                f1 = 700.0f * formant_scale; f2 = 2400.0f * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
                break;
            case 5: // "oh" (/oʊ/)
                f1 = 500.0f * formant_scale; f2 = 1500.0f * formant_scale;
                bw1 = 90.0f; bw2 = 110.0f;
                break;
            case 6: // "ah" (/ɑ/)
                f1 = 800.0f * formant_scale; f2 = 1400.0f * formant_scale;
                bw1 = 90.0f; bw2 = 110.0f;
                break;
            case 7: // "oo" (/u/)
                f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
                break;
            case 8: // "eh" (/ɛ/)
                f1 = 650.0f * formant_scale; f2 = 2000.0f * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
                break;
            case 9: // "la" (/lɑ/)
                f1 = 750.0f * formant_scale; f2 = 1600.0f * formant_scale;
                bw1 = 90.0f; bw2 = 110.0f;
                break;
            case 10: // "wee" (/wi/)
                f1 = 600.0f * formant_scale; f2 = 2900.0f * formant_scale;
                bw1 = 70.0f; bw2 = 90.0f;
                break;
            case 11: // "yah" (/jɑ/)
                f1 = 800.0f * formant_scale; f2 = 1800.0f * formant_scale;
                bw1 = 90.0f; bw2 = 110.0f;
                break;
            case 12: // "mew" (stylized high glide)
                f1 = 700.0f * formant_scale; f2 = (2000.0f + 300.0f * std::sin(2.0f * M_PI * 0.7f * t)) * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
                break;
            default:
                f1 = 700.0f * formant_scale; f2 = 2000.0f * formant_scale;
                bw1 = 80.0f; bw2 = 100.0f;
        }
    }
    
    // Band-limited sawtooth wave for current note
    float saw = 0.0f;
    const int num_harmonics = 20;
    for (int i = 1; i <= num_harmonics; ++i) {
        float harmonic_freq = base_freq * i;
        if (harmonic_freq > 20000.0f) break;
        saw += std::sin(2.0f * M_PI * harmonic_freq * t) / i;
    }
    saw *= 1.2f;
    
    // Formant processing
    AudioUtils::BandPassFilter formant1_filter(f1, bw1 / f1, 44100.0f);
    AudioUtils::BandPassFilter formant2_filter(f2, bw2 / f2, 44100.0f);
    float formant1 = formant1_filter.process(saw) * 0.7f;
    float formant2 = formant2_filter.process(saw) * 0.6f;
    
    // Vocal mix (no resonance filter for male voice)
    float vocal_current = (depth == 1) ? 
        (0.4f * saw + 0.6f * (formant1 + formant2)) : // Increased saw weight for deeper male voice
        (0.3f * saw + 0.7f * (formant1 + formant2));
    
    // Breath noise
    float breath = rng.generatePinkNoise() * std::exp(-6.0f * t / dur) * breath_amount;
    
    // Vibrato
    float vibrato = 1.0f + vibrato_depth * std::sin(2.0f * M_PI * 5.0f * t);
    
    // Current note output
    float output_current = env_current * (vocal_current + breath) * vibrato;
    
    // Previous note output (for crossfade)
    float output_prev = state.prev_output;
    if (state.is_new_note && t_current < 1.5f) {
        float prev_base_freq = state.current_freq * ((depth == 1) ? 0.3f : 1.2f);
        if (depth == 1) {
            prev_base_freq = std::max(20.0f, prev_base_freq);
        }
        float prev_f1 = f1, prev_f2 = f2, prev_bw1 = bw1, prev_bw2 = bw2;
        float prev_saw = 0.0f;
        for (int i = 1; i <= num_harmonics; ++i) {
            float harmonic_freq = prev_base_freq * i;
            if (harmonic_freq > 20000.0f) break;
            prev_saw += std::sin(2.0f * M_PI * harmonic_freq * t) / i;
        }
        prev_saw *= 1.2f;
        AudioUtils::BandPassFilter prev_formant1_filter(prev_f1, prev_bw1 / prev_f1, 44100.0f);
        AudioUtils::BandPassFilter prev_formant2_filter(prev_f2, prev_bw2 / prev_f2, 44100.0f);
        float prev_formant1 = prev_formant1_filter.process(prev_saw) * 0.7f;
        float prev_formant2 = prev_formant2_filter.process(prev_saw) * 0.6f;
        float prev_vocal = (depth == 1) ? 
            (0.4f * prev_saw + 0.6f * (prev_formant1 + prev_formant2)) :
            (0.3f * prev_saw + 0.7f * (prev_formant1 + prev_formant2));
        float prev_breath = rng.generatePinkNoise() * std::exp(-6.0f * t / state.current_dur) * breath_amount;
        float prev_vibrato = 1.0f + vibrato_depth * std::sin(2.0f * M_PI * 5.0f * t);
        output_prev = env_prev * (prev_vocal + prev_breath) * prev_vibrato;
    }
    
    // Crossfade
    float output;
    if (state.is_new_note && t_current < crossfade_dur) {
        float crossfade_t = t_current / crossfade_dur;
        float smooth_t = 0.5f * (1.0f - std::cos(M_PI * crossfade_t));
        output = (1.0f - smooth_t) * output_prev + smooth_t * output_current;
    } else {
        output = output_current;
        state.is_new_note = false;
    }
    
    // Apply distortion for male voice
    if (depth == 1) {
        AudioUtils::Distortion distortion(distortion_drive, 0.7f);
        output = distortion.process(output);
    }
    
    // Post-processing
    AudioUtils::Reverb reverb(0.25f, 0.6f, 0.4f);
    AudioUtils::LowPassFilter filter(filter_cutoff, 44100.0f);
    output = reverb.process(output);
    output = filter.process(output);
    
    // Final gain stage (increased for male voice)
    output *= (depth == 1) ? 2.0f : 1.8f;
    
    // Clamp to prevent clipping
    output = std::max(std::min(output, 1.0f), -1.0f);
    
    // Store current output for next iteration
    state.prev_output = output_current;
    
	output *= 0.3f; // I add these for final volume adjustments
    return output;
}

// My Dad picked the flute and I messed with 1.5 second tail durations.
// copy and paste generateFluteWave into AI and tell it how would like it fixed to sound better.
// Then if it does not, you can tell it more like whatever you can do to make it more realistic using those three properties and an 8 channel tone generator.
// tell it can look at AudioManager and the filters and stuff later if it cares, but it is all in one file already.
//
// this should never change: 
//
// static float generateFluteWave(float t, float freq, float dur) {
// }
// copy that portion from down below.
//
// This is enough for AI to design your instruments.
//
// it needs to understand how much duration. now and freq and dur
// dur is go fix your code over in songen.h
// types.h and structs if you a hotshot, go see queued branch on github instead.
//
// I listen to real flute videos before bossing it around most times.
// make clean and make and generateFluteWave will work with the engine.
// if it does not then I use CTRL-Z to undo changes.
// if you lose the one I gave you, you can download it again. Might be a new update out.
// my songgen.h instruments.h should be updating frequently.
// you can fork the code and upload your changes to your own github page for free.
 
// save. make clean make
// copy below here.
static float generateFluteWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(1600.0f, 300.0f, 44100.0f); // Lower, narrower for natural breath

    // Input validation
    if (!std::isfinite(t) || t < 0.0f || !std::isfinite(freq) || freq <= 0.0f || !std::isfinite(dur) || dur <= 0.0f) {
        SDL_Log("Invalid t %.2f, freq %.2f, or dur %.2f, returning 0.0", t, freq, dur);
        return 0.0f;
    }
    freq = std::max(261.63f, std::min(2093.0f, freq)); // Flute range (C4 to C7)

    // ADSR envelope for smoother, natural onset
    float attack = 0.015f, decay = 0.05f, sustain = 0.9f, release = 0.12f, env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else if (t < dur + release) {
        env = sustain * std::exp(-(t - dur) / release);
    } else {
        env = 0.0f;
    }

    // No vibrato for steady pitch
    float modulatedFreq = freq;

    // Additive synthesis for warm, natural flute timbre
    float harmonic1 = 1.0f * std::sin(2.0f * M_PI * modulatedFreq * t); // Strong fundamental
    float harmonic2 = 0.25f * std::sin(2.0f * M_PI * 2.0f * modulatedFreq * t); // Softer 2nd harmonic
    float harmonic3 = 0.08f * std::sin(2.0f * M_PI * 3.0f * modulatedFreq * t); // Minimal 3rd harmonic
    float output = (harmonic1 + harmonic2 + harmonic3) * 0.3f * env; // Lower gain for warmth

    // Check harmonics
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite harmonics at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }
    output = std::max(-0.8f, std::min(0.8f, output)); // Tighter clipping for cleaner sound

    // Very subtle breath noise for organic feel
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.008f * (t < 0.04f ? 0.9f : 0.15f);
    breathNoise = std::max(-0.15f, std::min(0.15f, breathNoise));
    if (!std::isfinite(breathNoise)) {
        SDL_Log("Non-finite breath noise at t %.2f, freq %.2f: %.2f", t, freq, breathNoise);
        breathNoise = 0.0f;
    }

    // Minimal articulation noise for natural note starts
    float articulation = (t < 0.004f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.02f * env : 0.0f;
    articulation = std::max(-0.15f, std::min(0.15f, articulation));
    if (!std::isfinite(articulation)) {
        SDL_Log("Non-finite articulation at t %.2f, freq %.2f: %.2f", t, freq, articulation);
        articulation = 0.0f;
    }

    // Combine output
    output = output + breathNoise * env + articulation;
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite combined output at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }

    // Apply minimal reverb and high-pass filter for clarity
    AudioUtils::Reverb reverb(0.02f, 0.15f, 0.1f); // Extremely light reverb
    AudioUtils::HighPassFilter filter(200.0f, 0.707f, 44100.0f); // Remove low-end mud, Q=0.707
    output = reverb.process(output);
    output = filter.process(output);

    // Soft clipping for natural dynamics
    output = std::tanh(output * 0.7f); // Softer gain for less synth-like aggression
    output *= 0.45f; // Moderate amplitude for balanced output

    // Final validation
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite final output at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }
    output = std::max(-1.0f, std::min(1.0f, output)); // Final clip
	output *= 1.0f; // These are added to adjust this instrument volume
    return output;
}
// This is enough for AI to design your instruments.
// You can try other instruments but only the ones here already can be used so do them individually so you can hear them inbetween and make sure it sounds better.

// crank this one up
static float generateTrumpetWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(2500.0f, 600.0f, 44100.0f); // Breath noise filter

    // Input validation
    if (!std::isfinite(t) || t < 0.0f || !std::isfinite(freq) || freq <= 0.0f || !std::isfinite(dur) || dur <= 0.0f) {
        SDL_Log("Invalid t %.2f, freq %.2f, or dur %.2f, returning 0.0", t, freq, dur);
        return 0.0f;
    }
    freq = std::max(155.56f, std::min(1244.51f, freq)); // Trumpet range (D#3 to D#6)

    // ADSR envelope
    float attack = 0.005f, decay = 0.02f, sustain = 0.9f, release = 0.15f, env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else if (t < dur + release) {
        env = sustain * std::exp(-(t - dur) / release);
    } else {
        env = 0.0f;
    }

    // Vibrato
    float vibratoFreq = 5.0f;
    float vibratoDepth = 0.005f * (t > 0.15f ? 1.0f : t / 0.15f);
    float vibrato = std::sin(2.0f * M_PI * vibratoFreq * t) * vibratoDepth;
    float modulatedFreq = freq * (1.0f + vibrato);

    // Additive synthesis for trumpet-like timbre
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t);
    float harmonic2 = 0.9f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t);
    float harmonic3 = 0.7f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t);
    float harmonic4 = 0.5f * std::cos(2.0f * M_PI * 4.0f * modulatedFreq * t);
    float harmonic5 = 0.3f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t);
    float output = (harmonic1 + harmonic2 + harmonic3 + harmonic4 + harmonic5) * 0.2f * env;

    // Check harmonics
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite harmonics at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }
    output = std::max(-0.8f, std::min(0.8f, output)); // Clip harmonics

    // Breath noise
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.04f * (t < 0.05f ? 1.3f : 0.4f);
    breathNoise = std::max(-0.4f, std::min(0.4f, breathNoise));
    if (!std::isfinite(breathNoise)) {
        SDL_Log("Non-finite breath noise at t %.2f, freq %.2f: %.2f", t, freq, breathNoise);
        breathNoise = 0.0f;
    }

    // Articulation noise
    float articulation = (t < 0.006f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.08f * env : 0.0f;
    articulation = std::max(-0.4f, std::min(0.4f, articulation));
    if (!std::isfinite(articulation)) {
        SDL_Log("Non-finite articulation at t %.2f, freq %.2f: %.2f", t, freq, articulation);
        articulation = 0.0f;
    }

    // Combine output
    output = output + breathNoise * env + articulation;
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite combined output at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }

    // Soft clipping
    output = std::tanh(output * 0.5f); // Conservative gain
    output *= 0.3f;

    // Final validation
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite final output at t %.2f, freq %.2f: %.2f", t, freq, output);
        output = 0.0f;
    }
    output = std::max(-1.0f, std::min(1.0f, output)); // Final clip
	output *= 1.7f; // adjusts instrument volume
    return output;
}

// guitar
static float generateBassWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }

    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        std::vector<float> oldDelayLine1 = state1.delayLine;
        std::vector<float> oldDelayLine2 = state2.delayLine;
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = state2.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);

        size_t initSize = state1.delayLineSize / 4;
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float pulse = std::sin(2.0f * M_PI * x) * (1.0f - x);
            float noise = rng.generatePinkNoise() * 0.2f; // Reduced noise
            state1.delayLine[i] = pulse * 0.45f + noise;
            state2.delayLine[i] = pulse * 0.4f + noise * 0.8f;
        }

        if (!oldDelayLine1.empty()) {
            size_t crossfadeLen = std::min(oldDelayLine1.size(), state1.delayLineSize) / 2; // Longer crossfade
            for (size_t i = 0; i < crossfadeLen; ++i) {
                float t = static_cast<float>(i) / crossfadeLen;
                float smoothT = 0.5f * (1.0f - std::cos(M_PI * t)); // Cosine taper
                state1.delayLine[i] = (1.0f - smoothT) * oldDelayLine1[i % oldDelayLine1.size()] + smoothT * state1.delayLine[i];
                state2.delayLine[i] = (1.0f - smoothT) * oldDelayLine2[i % oldDelayLine2.size()] + smoothT * state2.delayLine[i];
            }
        }

        size_t harmonicSize = state1.delayLineSize / 2;
        std::vector<float> harmonicLine(harmonicSize, 0.0f);
        for (size_t i = 0; i < std::min(initSize, harmonicSize); ++i) {
            float x = static_cast<float>(i) / initSize;
            harmonicLine[i] = std::sin(2.0f * M_PI * x) * (1.0f - x) * 0.08f; // Slightly reduced
        }
        state1.delayLine.insert(state1.delayLine.end(), harmonicLine.begin(), harmonicLine.end());
        state1.delayLine.resize(state1.delayLineSize);
        state2.delayLine.insert(state2.delayLine.end(), harmonicLine.begin(), harmonicLine.end());
        state2.delayLine.resize(state2.delayLineSize);
    }

    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);

    static AudioUtils::LowPassFilter feedbackLPF1(200.0f, sampleRate);
    static AudioUtils::LowPassFilter feedbackLPF2(200.0f, sampleRate);
    float pseudoVelocity = std::min(0.8f, 1.0f - time / (dur + 0.3f)); // Velocity scaling
    float cutoff = 200.0f - 100.0f * (time / (dur + 1.0f)); // 200–100 Hz
    cutoff = std::max(100.0f, std::min(200.0f, cutoff));
    feedbackLPF1.setCutoff(cutoff);
    feedbackLPF2.setCutoff(cutoff);
    float y1 = feedbackLPF1.process(x1);
    float y2 = feedbackLPF2.process(x2);

    float pitchVariation = 1.0f - 0.02f * (time / (dur + 2.0f)); // Slightly reduced
    pitchVariation *= 1.0f + 0.002f * std::sin(2.0f * M_PI * 0.2f * time);

    float pluckNoise = rng.generatePinkNoise() * std::exp(-25.0f * time) * 0.04f * pseudoVelocity; // Reduced, faster decay
    float fingerTap = (time < 0.005f) ? rng.generatePinkNoise() * 0.03f * pseudoVelocity : 0.0f; // Pink noise, shorter

    state1.delayLine[state1.writePos] = (y1 * 0.98f + pluckNoise + fingerTap) * pitchVariation; // Lower feedback
    state2.delayLine[state2.writePos] = (y2 * 0.975f + pluckNoise * 0.8f + fingerTap * 0.8f) * pitchVariation;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;

    float attack = 0.005f, decay = 0.05f, sustain = 0.5f + 0.1f * pseudoVelocity, release = 1.5f, env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain; // Removed modulation
    } else if (time < dur + release) {
        float t = (time - dur) / release;
        env = sustain * (1.0f - t) * std::exp(-t * 6.0f); // Faster decay
    } else {
        env = 0.0f;
        output = 0.0f; // Force zero output
    }

    float resonanceFreq = 60.0f; // Lowered for bass
    float resonanceFilter = std::sin(2.0f * M_PI * resonanceFreq * time) * 0.5f + 0.5f;
    float resonanceNoise = rng.generatePinkNoise() * resonanceFilter * 0.03f * env; // Reduced

    pluckNoise *= env;
    fingerTap *= env;
    resonanceNoise *= env;
    output += resonanceNoise;

    float harmonic1 = 0.4f * std::cos(2.0f * M_PI * freq * pitchVariation * time) * std::exp(-1.0f * time); // Reduced, faster decay
    float harmonic2 = 0.15f * std::cos(2.0f * M_PI * 2.0f * freq * pitchVariation * time) * std::exp(-1.5f * time);
    float harmonic3 = 0.05f * std::cos(2.0f * M_PI * 3.0f * freq * pitchVariation * time) * std::exp(-2.0f * time);
    output += (harmonic1 + harmonic2 + harmonic3) * env * 0.6f; // Scaled down

    output = (output + pluckNoise + fingerTap) * env;

    static AudioUtils::LowPassFilter bodyResonance(200.0f, sampleRate); // Lowered base
    float resonanceCutoff = 200.0f;
    if (time > dur) {
        resonanceCutoff *= std::exp(-(time - dur) / release);
    }
    bodyResonance.setCutoff(std::max(80.0f, resonanceCutoff)); // Lower minimum
    output = bodyResonance.process(output);

    // Skip reverb since UseReverb: false
    output *= 0.6f; // Lower gain to prevent clipping
    output = std::max(-1.0f, std::min(1.0f, output));

    if (time > dur + release - 0.01f && std::abs(output) > 0.001f) {
        SDL_Log("Non-zero output at note end: %.6f", output);
    }

	output *= 5.0f; // adjusts instrument volume
    return output;
}

// room for improvement
static float generateGuitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = state1.delayLineSize / 4;
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float pulse = std::sin(2.0f * M_PI * x) * (1.0f - x);
            float noise = rng.generatePinkNoise() * 0.4f;
            state1.delayLine[i] = pulse * 0.6f + noise;
            state2.delayLine[i] = pulse * 0.55f + noise * 0.9f;
        }
        size_t harmonicSize = state1.delayLineSize / 2;
        std::vector<float> harmonicLine(harmonicSize, 0.0f);
        for (size_t i = 0; i < std::min(initSize, harmonicSize); ++i) {
            float x = static_cast<float>(i) / initSize;
            harmonicLine[i] = std::sin(2.0f * M_PI * x) * (1.0f - x) * 0.2f;
        }
        state1.delayLine.insert(state1.delayLine.end(), harmonicLine.begin(), harmonicLine.end());
        state1.delayLine.resize(state1.delayLineSize);
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);

    // Warmer, stable tone
    static AudioUtils::LowPassFilter feedbackLPF1(800.0f, sampleRate);
    static AudioUtils::LowPassFilter feedbackLPF2(800.0f, sampleRate);
    float cutoff = 800.0f; // Fixed cutoff for natural decay
    feedbackLPF1.setCutoff(cutoff);
    feedbackLPF2.setCutoff(cutoff);
    float y1 = feedbackLPF1.process(x1);
    float y2 = feedbackLPF2.process(x2);

    // Minimal pitch variation, no vibrato
    float pitchVariation = 1.0f - 0.05f * (time / (dur + 3.0f)); // Subtle detune only

    // Strong pluck, minimal noise
    float fretNoise = rng.generatePinkNoise() * std::exp(-60.0f * time) * 0.02f;
    float pickScrape = (time < 0.012f) ? rng.generateWhiteNoise() * 0.15f : 0.0f;

    // Feedback for natural decay
    state1.delayLine[state1.writePos] = (y1 * 0.995f + fretNoise + pickScrape) * pitchVariation;
    state2.delayLine[state2.writePos] = (y2 * 0.990f + fretNoise * 0.8f + pickScrape * 0.8f) * pitchVariation;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;

    // Natural decay envelope
    float attack = 0.008f, decay = 0.2f, sustain = 0.5f, release = 1.5f, env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * std::exp(-2.0f * (time - attack - decay) / dur);
    } else if (time < dur + release) {
        env = sustain * std::exp(-4.0f * (time - dur) / release);
    } else {
        env = 0.0f;
    }

    // Resonance for body
    float resonanceFreq = 300.0f;
    float resonanceFilter = std::sin(2.0f * M_PI * resonanceFreq * time) * 0.5f + 0.5f;
    float resonanceNoise = rng.generatePinkNoise() * resonanceFilter * 0.06f * (time < dur ? 1.0f : std::exp(-(time - dur) / release));
    output += resonanceNoise;

    // Tamed harmonics with faster decay
    float harmonic1 = 0.3f * std::cos(2.0f * M_PI * freq * pitchVariation * time) * std::exp(-1.5f * time);
    float harmonic2 = 0.15f * std::cos(2.0f * M_PI * 2.0f * freq * pitchVariation * time) * std::exp(-2.0f * time);
    float harmonic3 = 0.08f * std::cos(2.0f * M_PI * 3.0f * freq * pitchVariation * time) * std::exp(-2.5f * time);
    float harmonic4 = 0.02f * std::cos(2.0f * M_PI * 4.0f * freq * pitchVariation * time) * std::exp(-3.0f * time);
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * env;

    output = (output + fretNoise + pickScrape) * env;

    // Warmer body resonance
    static AudioUtils::LowPassFilter bodyResonance(800.0f, sampleRate);
    output = bodyResonance.process(output);

    // Gritty overdrive
    static AudioUtils::Distortion distortion(1.7f, 0.7f);
    output = distortion.process(output);

    // High-pass to avoid bass overlap
    static AudioUtils::HighPassFilter outputHPF(100.0f, 0.707f, sampleRate);
    output = outputHPF.process(output);

    // Subtle reverb
    static AudioUtils::Reverb reverb(0.1f, 0.3f, 0.1f, sampleRate);
    output = reverb.process(output);

    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.2f; // These are added to adjust this instrument volume
    return output;
}

// room for improvement
static float generateSaxophoneWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(2500.0f, 600.0f, sampleRate); // Breath noise filter

    // Input validation
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f || !std::isfinite(time) || time < 0.0f) {
        SDL_Log("Invalid sampleRate %.2f, freq %.2f, or time %.2f, returning 0.0", sampleRate, freq, time);
        return 0.0f;
    }
    freq = std::max(138.59f, std::min(880.0f, freq)); // Saxophone range (C#3 to A5)

    // ADSR envelope
    float attack = 0.005f, decay = 0.03f, sustain = 0.85f, release = 0.25f, env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain;
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }

    // Vibrato
    float vibratoFreq = 5.0f;
    float vibratoDepth = 0.005f * (time > 0.15f ? 1.0f : time / 0.15f);
    float vibrato = std::sin(2.0f * M_PI * vibratoFreq * time) * vibratoDepth;
    float modulatedFreq = freq * (1.0f + vibrato);

    // Additive synthesis for saxophone-like timbre
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * time);
    float harmonic2 = 0.6f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * time);
    float harmonic3 = 0.3f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * time);
    float output = (harmonic1 + harmonic2 + harmonic3) * 0.3f * env;

    // Check harmonics
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite harmonics at time %.2f, freq %.2f: %.2f", time, freq, output);
        output = 0.0f;
    }
    output = std::max(-0.8f, std::min(0.8f, output)); // Clip harmonics

    // Breath noise
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.05f * (time < 0.05f ? 1.2f : 0.5f);
    breathNoise = std::max(-0.4f, std::min(0.4f, breathNoise));
    if (!std::isfinite(breathNoise)) {
        SDL_Log("Non-finite breath noise at time %.2f, freq %.2f: %.2f", time, freq, breathNoise);
        breathNoise = 0.0f;
    }

    // Articulation noise
    float articulation = (time < 0.008f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.1f * env : 0.0f;
    articulation = std::max(-0.4f, std::min(0.4f, articulation));
    if (!std::isfinite(articulation)) {
        SDL_Log("Non-finite articulation at time %.2f, freq %.2f: %.2f", time, freq, articulation);
        articulation = 0.0f;
    }

    // Combine output
    output = output + breathNoise * env + articulation;
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite combined output at time %.2f, freq %.2f: %.2f", time, freq, output);
        output = 0.0f;
    }

    // Soft clipping
    output = std::tanh(output * 0.5f); // Conservative gain
    output *= 0.3f;

    // Final validation
    if (!std::isfinite(output)) {
        SDL_Log("Non-finite final output at time %.2f, freq %.2f: %.2f", time, freq, output);
        output = 0.0f;
    }
    output = std::max(-1.0f, std::min(1.0f, output)); // Final clip
	output *= 1.0f; // These are added to adjust this instrument volume
    return output;
}

// gl
static float generatePianoWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(1600.0f, 44100.0f); // Lowered cutoff for warmth
    static AudioUtils::Reverb reverb(0.15f, 0.65f, 0.45f, 44100.0f); // Warmer, longer reverb

    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }

    freq = std::max(27.5f, std::min(4186.0f, freq));

    // Simulate velocity (0.1 to 1.0) based on duration or randomization
    float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f); // Random variation around mezzo-forte
    if (dur < 0.1f) velocity *= 0.6f; // Short notes are softer
    velocity = std::max(0.1f, std::min(1.0f, velocity));

    // Simulate sustain pedal: active for longer notes or after some time
    bool sustainPedal = (dur > 1.0f || time > 2.0f); // Heuristic for pedal effect

    // Initialize delay lines if frequency changes or empty
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);

        // Hammer-like excitation: short, broadband pulse
        size_t initSize = std::min(state1.delayLineSize / 8, static_cast<size_t>(4));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float impulse = (1.0f - x) * (1.0f - x) * velocity;
            float noise = rng.generateWhiteNoise() * 0.06f * velocity;
            state1.delayLine[i] = impulse * 0.9f + noise;
            state2.delayLine[i] = impulse * 0.85f + noise * 0.9f;
        }
    }

    // Karplus-Strong processing
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float filteredOutput = stringFilter.process(output);
    float damping = 0.996f - std::min(freq / 6000.0f, 0.03f); // Slightly higher damping for high notes
    state1.delayLine[state1.writePos] = filteredOutput * damping;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;

    // Decay time: longer for low notes, shorter for high notes
    float decayTime = 6.0f * std::pow(440.0f / freq, 0.7f);
    decayTime = std::max(0.5f, std::min(10.0f, decayTime));
    if (sustainPedal) decayTime *= 1.5f; // Extend decay for pedal

    // Velocity-dependent envelope
    float attackTime = 0.002f * (1.0f - 0.4f * velocity);
    float env;
    if (time < attackTime) {
        env = time / attackTime;
    } else if (time < dur || sustainPedal) {
        if (freq > 1000.0f) { // High notes: linear + exponential
            float linearPhase = 0.1f * (1.0f - velocity);
            if (time < linearPhase) {
                env = 1.0f - (time / linearPhase) * 0.5f;
            } else {
                env = 0.5f * std::exp(-(time - linearPhase) / (decayTime * 0.8f));
            }
        } else { // Low notes: exponential
            env = std::exp(-time / decayTime);
        }
    } else {
        env = std::exp(-(time - dur) / (decayTime * (freq > 1000.0f ? 0.3f : 0.5f)));
    }

    // Hammer strike transient
    if (time < 0.002f) {
        output += rng.generateWhiteNoise() * 0.1f * velocity * (1.0f - time / 0.002f);
    }

    // Inharmonic partials
    float partial1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float partial2 = 0.35f * std::cos(2.0f * M_PI * 2.01f * freq * time) * env * std::exp(-time * freq / 4500.0f);
    float partial3 = 0.15f * std::cos(2.0f * M_PI * 3.03f * freq * time) * env * std::exp(-time * freq / 3500.0f);
    output += (partial1 + partial2 + partial3) * 0.25f * velocity;

    // Sympathetic resonance
    if (sustainPedal) {
        float resFreq1 = freq * 2.0f; // Octave
        float resFreq2 = freq * 1.5f; // Fifth
        if (resFreq1 <= 4186.0f) {
            output += 0.04f * std::cos(2.0f * M_PI * resFreq1 * time) * env * velocity;
        }
        if (resFreq2 <= 4186.0f) {
            output += 0.02f * std::cos(2.0f * M_PI * resFreq2 * time) * env * velocity;
        }
    }

    output *= env;

    // Frequency-dependent reverb
    float reverbMix = 0.45f * (1.0f - std::min(freq / 4000.0f, 0.6f));
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.3f * velocity;
	output *= 1.0f; // These are added to adjust this instrument volume
    return output;
}

// room for improvement. Worked from alto Sax.
static float generateViolinWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter bowFilter(2500.0f, 0.5f, 44100.0f);
    static AudioUtils::LowPassFilter stringFilter(3000.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.2f, 0.65f, 0.45f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(196.0f, std::min(3520.0f, freq));
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 3, static_cast<size_t>(12));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float bowNoise = bowFilter.process(rng.generatePinkNoise()) * (1.0f - x) * 0.7f;
            state1.delayLine[i] = bowNoise * 0.9f;
            state2.delayLine[i] = bowNoise * 0.85f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.06f;
    float filteredOutput = stringFilter.process(output);
    float damping = 0.998f - std::min(freq / 15000.0f, 0.015f);
    state1.delayLine[state1.writePos] = filteredOutput * damping + bowNoise;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f + bowNoise * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.01f, decay = 0.05f, sustain = 0.95f, release = 0.2f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.05f * std::sin(2.0f * M_PI * 5.0f * time));
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.7f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.5f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.3f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env;
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * 0.55f;
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f; // These are added to adjust this instrument volume
    return output;
}

// might have blow out somewhere but has strong notes.
static float generateOrganWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter windFilter(1200.0f, 0.6f, 44100.0f);
    static AudioUtils::LowPassFilter pipeFilter(2500.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.5f, 0.75f, 0.5f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(32.7f, std::min(2093.0f, freq));
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 2, static_cast<size_t>(15));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float windNoise = windFilter.process(rng.generatePinkNoise()) * (1.0f - x) * 0.8f;
            state1.delayLine[i] = windNoise * 0.95f;
            state2.delayLine[i] = windNoise * 0.9f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float windNoise = windFilter.process(rng.generatePinkNoise()) * 0.08f;
    float filteredOutput = pipeFilter.process(output);
    float damping = 0.999f - std::min(freq / 20000.0f, 0.005f);
    state1.delayLine[state1.writePos] = filteredOutput * damping + windNoise;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f + windNoise * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.02f, decay = 0.05f, sustain = 1.0f, release = 0.15f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain;
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.8f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.6f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.4f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env;
    float harmonic5 = 0.2f * std::cos(2.0f * M_PI * 5.0f * freq * time) * env;
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4 + harmonic5) * 0.6f;
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
	output *= 0.3f; // These are added to adjust this instrument volume	
    return output;
}

// room for improvement
static float generateCelloWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter bowFilter(1800.0f, 0.6f, 44100.0f);
    static AudioUtils::LowPassFilter stringFilter(2200.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.18f, 0.6f, 0.4f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(65.41f, std::min(783.99f, freq)); // C2 to G5
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 3, static_cast<size_t>(10));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float bowNoise = bowFilter.process(rng.generatePinkNoise()) * (1.0f - x) * 0.75f;
            state1.delayLine[i] = bowNoise * 0.9f;
            state2.delayLine[i] = bowNoise * 0.85f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.07f;
    float filteredOutput = stringFilter.process(output);
    float damping = 0.997f - std::min(freq / 10000.0f, 0.017f);
    state1.delayLine[state1.writePos] = filteredOutput * damping + bowNoise;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f + bowNoise * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.015f, decay = 0.06f, sustain = 0.92f, release = 0.25f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.04f * std::sin(2.0f * M_PI * 4.0f * time));
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.65f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.45f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.3f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env;
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * 0.6f;
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f; // These are added to adjust this instrument volume	
    return output;
}

// more this
static float generateMarimbaWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter barFilter(1500.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.08f, 0.5f, 0.3f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(261.63f, std::min(2093.0f, freq)); // C4 to C7, typical marimba range
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 4, static_cast<size_t>(8));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float strike = std::sin(2.0f * M_PI * x) * (1.0f - x); // Simulate mallet strike
            float noise = rng.generatePinkNoise() * 0.05f;
            state1.delayLine[i] = strike * 0.9f + noise;
            state2.delayLine[i] = strike * 0.85f + noise * 0.8f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float filteredOutput = barFilter.process(output); // Bright, woody tone
    float damping = 0.99f - std::min(freq / 12000.0f, 0.02f); // Quick decay for percussive sound
    state1.delayLine[state1.writePos] = filteredOutput * damping;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.002f, decay = 0.1f, sustain = 0.3f, release = 0.2f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain;
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    // Marimba has strong fundamental and weaker harmonics
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.4f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.2f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env; // 4th harmonic is prominent
    output += (harmonic1 + harmonic2 + harmonic3) * 0.5f;
    output *= env;
    output = reverb.process(output); // Subtle reverb for resonance
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.7f; // These are added to adjust this instrument volume	
    return output;
}

// I hit the volume hard
static float generateSteelGuitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(2000.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.12f, 0.55f, 0.35f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(82.41f, std::min(1318.51f, freq)); // E2 to E5, typical steel guitar range
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 4, static_cast<size_t>(10));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float pluck = std::sin(2.0f * M_PI * x) * (1.0f - x); // Simulate slide or pick
            float noise = rng.generatePinkNoise() * 0.1f;
            state1.delayLine[i] = pluck * 0.8f + noise;
            state2.delayLine[i] = pluck * 0.75f + noise * 0.9f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float filteredOutput = stringFilter.process(output); // Bright, twangy tone
    float damping = 0.995f - std::min(freq / 15000.0f, 0.015f);
    float slideNoise = rng.generatePinkNoise() * std::exp(-30.0f * time) * 0.05f; // Slide effect
    state1.delayLine[state1.writePos] = filteredOutput * damping + slideNoise;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f + slideNoise * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.01f, decay = 0.2f, sustain = 0.7f, release = 0.8f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.02f * std::sin(2.0f * M_PI * 3.0f * time));
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    // Steel guitar has bright harmonics and sustain
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.6f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.3f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.2f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env;
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * 0.55f;
    output = (output + slideNoise) * env;
    output = reverb.process(output); // Moderate reverb for ambiance
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.1f; // These are added to adjust this instrument volume	
    return output;
}

// seems accurate for a tone generator - challenge you to do better
static float generateSitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(2500.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.15f, 0.6f, 0.4f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(146.83f, std::min(880.0f, freq)); // D3 to E5, typical sitar range
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 4, static_cast<size_t>(12));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float pluck = std::sin(2.0f * M_PI * x) * (1.0f - x); // Simulate plucked string
            float noise = rng.generatePinkNoise() * 0.15f;
            state1.delayLine[i] = pluck * 0.85f + noise;
            state2.delayLine[i] = pluck * 0.8f + noise * 0.9f;
        }
        // Add sympathetic resonance simulation
        size_t sympatheticSize = state1.delayLineSize / 2;
        std::vector<float> sympatheticLine(sympatheticSize, 0.0f);
        for (size_t i = 0; i < std::min(initSize, sympatheticSize); ++i) {
            float x = static_cast<float>(i) / initSize;
            sympatheticLine[i] = std::sin(4.0f * M_PI * x) * (1.0f - x) * 0.2f;
        }
        state1.delayLine.insert(state1.delayLine.end(), sympatheticLine.begin(), sympatheticLine.end());
        state1.delayLine.resize(state1.delayLineSize);
        state2.delayLine.insert(state2.delayLine.end(), sympatheticLine.begin(), sympatheticLine.end());
        state2.delayLine.resize(state2.delayLineSize);
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float filteredOutput = stringFilter.process(output); // Rich, buzzing tone
    float damping = 0.996f - std::min(freq / 10000.0f, 0.016f);
    float buzz = rng.generatePinkNoise() * std::exp(-20.0f * time) * 0.07f; // Simulate bridge buzz
    state1.delayLine[state1.writePos] = filteredOutput * damping + buzz;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f + buzz * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.008f, decay = 0.15f, sustain = 0.8f, release = 0.6f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.03f * std::sin(2.0f * M_PI * 5.0f * time));
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    // Sitar has complex harmonics and sympathetic resonance
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.7f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.5f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.3f * std::cos(2.0f * M_PI * 5.0f * freq * time) * env; // 5th harmonic for sitar
    float sympathetic = 0.2f * std::sin(2.0f * M_PI * freq * 1.5f * time) * env; // Sympathetic strings
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4 + sympathetic) * 0.6f;
    output = (output + buzz) * env;
    output = reverb.process(output); // Reverb for depth
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f; // These are added to adjust this instrument volume	
    return output;
}

#pragma GCC diagnostic pop

// SampleManager
class SampleManager {
    std::mutex mutex;
    std::map<std::string, std::vector<InstrumentSample>> samples;
    static float generateSample(const std::string& instrument, float sampleRate, float freq, float dur, int phoneme, bool open, float t) {
        if (instrument == "kick") return generateKickWave(t, freq, dur);
        if (instrument == "hihat_closed") return generateHiHatWave(t, freq, false, dur);
        if (instrument == "hihat_open") return generateHiHatWave(t, freq, true, dur);
        if (instrument == "snare") return generateSnareWave(t, dur);
        if (instrument == "clap") return generateClapWave(t, dur);
        if (instrument == "tom") return generateTomWave(t, freq, dur);
        if (instrument == "subbass") return generateSubBassWave(t, freq, dur);
        if (instrument == "syntharp") return generateSynthArpWave(t, freq, dur);
        if (instrument == "leadsynth") return generateLeadSynthWave(t, freq, dur);
        if (instrument == "pad") return generatePadWave(t, freq, dur);
        if (instrument == "cymbal") return generateCymbalWave(t, freq, dur);
        if (instrument == "vocal_0") return generateVocalWave(t, freq, phoneme, dur, 0);
        if (instrument == "vocal_1") return generateVocalWave(t, freq, phoneme, dur, 1);
        if (instrument == "flute") return generateFluteWave(t, freq, dur);
        if (instrument == "trumpet") return generateTrumpetWave(t, freq, dur);
        thread_local static KarplusStrongState state1, state2;
        if (t == 0.0f) { state1 = KarplusStrongState(); state2 = KarplusStrongState(); }
        if (instrument == "guitar") return generateGuitarWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "organ") return generateOrganWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "bass") return generateBassWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "piano") return generatePianoWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "violin") return generateViolinWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "cello") return generateCelloWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "marimba") return generateMarimbaWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "steelguitar") return generateSteelGuitarWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "sitar") return generateSitarWave(sampleRate, freq, t, dur, state1, state2);
        if (instrument == "saxophone") return generateSaxophoneWave(sampleRate, freq, t, dur, state1, state2);
        return 0.0f;
    }
public:
    SampleManager() {}
    const std::vector<float>& getSample(const std::string& instrument, float sampleRate, float freq, float dur, int phoneme = -1, bool open = false) {
        std::lock_guard<std::mutex> lock(mutex);
        auto& instrumentSamples = samples[instrument];
        for (const auto& sample : instrumentSamples) {
            if (std::abs(sample.freq - freq) < 0.1f && std::abs(sample.dur - dur) < 0.01f &&
                sample.phoneme == phoneme && sample.open == open) {
                return sample.samples;
            }
        }
        float tail = getTailDuration(instrument);
        size_t sampleCount = static_cast<size_t>((dur + tail) * sampleRate);
        std::vector<float> newSamples(sampleCount);
        for (size_t i = 0; i < sampleCount; ++i) {
            float t = i / sampleRate;
            newSamples[i] = generateSample(instrument, sampleRate, freq, dur, phoneme, open, t);
        }
        instrumentSamples.emplace_back(freq, dur, phoneme, open, std::move(newSamples));
        return instrumentSamples.back().samples;
    }
};

static SampleManager sampleManager;

// Song structures
struct AutomationPoint {
    float time, value;
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
    int channels; // 2 for stereo, 6 for 5.1
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

// Utility functions
float interpolateAutomation(float t, const std::vector<AutomationPoint>& points, float defaultValue);
size_t countNotesInSection(const Song& song, const Section& section);
std::string getInstrumentsInSection(const Song& song, const Section& section);

} // namespace Instruments

#endif // INSTRUMENTS_H