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
#include <SDL2/SDL.h>

// Debug logging control: set to 1 to enable instrument loading logs, 0 to disable
#define DEBUG_LOG 0

namespace AudioUtils {
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

thread_local std::mt19937 AudioUtils::RandomGenerator::rng(std::random_device{}());

class Reverb {
    std::vector<float> delayBuffer;
    size_t bufferSize;
    size_t writePos;
    float decay;
    float mix;
public:
    Reverb(float delayTime = 0.1f, float decayFactor = 0.5f, float mixFactor = 0.3f, float sampleRate = 44100.0f)
        : bufferSize(static_cast<size_t>(delayTime * sampleRate)), writePos(0),
          decay(decayFactor), mix(mixFactor) {
        delayBuffer.resize(bufferSize, 0.0f);
    }
    float process(float input) {
        size_t readPos = (writePos + bufferSize - bufferSize / 2) % bufferSize;
        float output = input + decay * delayBuffer[readPos];
        delayBuffer[writePos] = input + decay * delayBuffer[readPos];
        writePos = (writePos + 1) % bufferSize;
        return input * (1.0f - mix) + output * mix;
    }
};

class Distortion {
    float drive;
    float threshold;
public:
    Distortion(float driveFactor = 2.0f, float clipThreshold = 0.7f)
        : drive(driveFactor), threshold(clipThreshold) {}
    float process(float input) {
        float x = input * drive;
        return std::max(std::min(x, threshold), -threshold) / threshold;
    }
};

class LowPassFilter {
    float cutoffFreq, sampleRate, x1, y1;
public:
    LowPassFilter(float cutoff, float sr) : cutoffFreq(cutoff), sampleRate(sr), x1(0.0f), y1(0.0f) {}
    float process(float input) {
        float alpha = 1.0f / (1.0f + 2.0f * M_PI * cutoffFreq / sampleRate);
        float output = alpha * input + (1.0f - alpha) * y1;
        x1 = input;
        y1 = output;
        return output;
    }
    void setCutoff(float cutoff) {
        cutoffFreq = cutoff;
    }
};

class BandPassFilter {
    float centerFreq, bandwidth, sampleRate, x1, x2, y1, y2;
public:
    BandPassFilter(float center, float bw, float sr)
        : centerFreq(center), bandwidth(bw), sampleRate(sr), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
    float process(float input) {
        float w0 = 2.0f * M_PI * centerFreq / sampleRate;
        float alpha = std::sin(w0) * std::sinh(std::log(2.0f) / 2.0f * bandwidth * w0 / std::sin(w0));
        float b0 = alpha, b1 = 0.0f, b2 = -alpha;
        float a0 = 1.0f + alpha, a1 = -2.0f * std::cos(w0), a2 = 1.0f - alpha;
        float output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * x2
                     - (a1 / a0) * y1 - (a2 / a0) * y2;
        x2 = x1; x1 = input;
        y2 = y1; y1 = output;
        return output;
    }
    void setCenterFrequency(float center) {
        centerFreq = center;
    }
};
}

namespace Instruments {
// Forward declarations for all instrument wave generation functions
struct KarplusStrongState;
struct InstrumentSample;
class SampleManager;

static float generateKickWave(float t, float freq, float dur);
static float generateHiHatWave(float t, float freq, bool open, float dur);
static float generateSnareWave(float t, float dur);
static float generateClapWave(float t, float dur);
static float generateTomWave(float t, float freq, float dur);
static float generateBassWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateSubBassWave(float t, float freq, float dur);
static float generateSynthArpWave(float t, float freq, float dur);
static float generateLeadSynthWave(float t, float freq, float dur);
static float generatePadWave(float t, float freq, float dur);
static float generateCymbalWave(float t, float freq, float dur);
static float generateVocalWave(float t, float freq, int phoneme, float dur);
static float generateFluteWave(float t, float freq, float dur);
static float generateTrumpetWave(float t, float freq, float dur);
static float generateGuitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateOrganWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generatePianoWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateViolinWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateCelloWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateMarimbaWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateSteelGuitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateSitarWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);
static float generateSaxophoneWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2);

// Per-note state for Karplus-Strong instruments to support polyphony
struct KarplusStrongState {
    float lastFreq;
    size_t delayLineSize;
    size_t writePos;
    std::vector<float> delayLine;
    KarplusStrongState() : lastFreq(0.0f), delayLineSize(0), writePos(0), delayLine() {}
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

// Tail durations matching songgen.cpp's getTailDuration
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
    return 1.5f;
}

// SampleManager class definition
class SampleManager {
    std::mutex mutex;
    std::map<std::string, std::vector<InstrumentSample>> samples;

static float generateSample(const std::string& instrument, float sampleRate, float freq, float dur, int phoneme, bool open, float t) {
    if (instrument == "kick") return generateKickWave(t, freq, dur);
    if (instrument == "hihat") return generateHiHatWave(t, freq, open, dur);
    if (instrument == "snare") return generateSnareWave(t, dur);
    if (instrument == "clap") return generateClapWave(t, dur);
    if (instrument == "tom") return generateTomWave(t, freq, dur);
    if (instrument == "subbass") return generateSubBassWave(t, freq, dur);
    if (instrument == "syntharp") return generateSynthArpWave(t, freq, dur);
    if (instrument == "leadsynth") return generateLeadSynthWave(t, freq, dur);
    if (instrument == "pad") return generatePadWave(t, freq, dur);
    if (instrument == "cymbal") return generateCymbalWave(t, freq, dur);
    if (instrument == "vocal") return generateVocalWave(t, freq, phoneme, dur);
    if (instrument == "flute") return generateFluteWave(t, freq, dur);
    if (instrument == "trumpet") return generateTrumpetWave(t, freq, dur);

    thread_local static KarplusStrongState state1, state2;
    if (t == 0.0f) {
        state1 = KarplusStrongState();
        state2 = KarplusStrongState();
    }
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
        if (DEBUG_LOG) SDL_Log("Generated new sample for %s: freq=%.2f, dur=%.2f, phoneme=%d, open=%d", instrument.c_str(), freq, dur, phoneme, open);
        return instrumentSamples.back().samples;
    }
};

// Static SampleManager instance
static SampleManager sampleManager;

// Suppress unused function warnings pending confirmation of usage
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static float generateKickWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.01f, decay = 0.2f, sustain = 0.6f, release = 0.15f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float pitchMod = freq * (1.8f * std::exp(-20.0f * t / dur));
    float sine = std::sin(2.0f * M_PI * pitchMod * t);
    float saw = 0.3f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);
    float click = rng.generateWhiteNoise() * std::exp(-50.0f * t / dur) * 0.2f;
    float output = env * (0.7f * sine + 0.2f * saw + 0.1f * click);
    AudioUtils::Distortion dist(2.0f, 0.75f);
    AudioUtils::LowPassFilter filter(200.0f, 44100.0f);
    output = dist.process(output);
    output = filter.process(output);
    return output;
}

static float generateHiHatWave(float t, float freq, bool open, float dur) {
    static AudioUtils::RandomGenerator rng;
    float env = std::exp(-15.0f * t / (open ? dur * 1.5f : dur * 0.2f));
    float filterFreq = (freq > 0.0f ? freq * 10.0f : 10000.0f);
    AudioUtils::BandPassFilter filter(filterFreq, 1.0f, 44100.0f);
    float noise = rng.generateWhiteNoise() * 0.7f;
    noise = filter.process(noise);
    float saw = 0.3f * (std::fmod(filterFreq * t, 1.0f) - 0.5f) * (open ? 0.5f : 0.8f);
    float output = env * (noise + saw);
    AudioUtils::Distortion dist(1.4f, 0.85f);
    AudioUtils::Reverb reverb(0.05f, 0.4f, 0.2f);
    output = dist.process(output);
    output = reverb.process(output);
    return output;
}

static float generateSnareWave(float t, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.005f, decay = 0.1f, sustain = 0.5f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float noise = rng.generatePinkNoise() * 0.5f;
    float tone = std::sin(2.0f * M_PI * 200.0f * t) * 0.25f;
    float rattle = rng.generateWhiteNoise() * std::exp(-40.0f * t / dur) * 0.25f;
    float output = env * (noise + tone + rattle);
    AudioUtils::Distortion dist(2.0f, 0.7f);
    AudioUtils::Reverb reverb(0.05f, 0.4f, 0.2f);
    output = dist.process(output);
    output = reverb.process(output);
    return output;
}

static float generateClapWave(float t, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.005f, decay = 0.05f, sustain = 0.4f, release = 0.1f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float burst = 1.0f + 0.5f * (std::sin(50.0f * M_PI * t) + std::sin(70.0f * M_PI * t));
    env *= (t < dur * 0.1f ? burst : 0.5f);
    float noise = rng.generateWhiteNoise() * 0.6f + rng.generatePinkNoise() * 0.2f;
    float saw = 0.3f * (std::fmod(1000.0f * t, 1.0f) - 0.5f);
    float output = env * (noise + saw);
    AudioUtils::Distortion dist(1.6f, 0.8f);
    AudioUtils::Reverb reverb(0.05f, 0.4f, 0.2f);
    output = dist.process(output);
    output = reverb.process(output);
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
    return output;
}

static float generateSubBassWave(float t, float freq, float dur) {
    float attack = 0.02f, decay = 0.15f, sustain = 0.8f, release = 0.25f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float sine = std::sin(2.0f * M_PI * freq * t) * 0.7f;
    float saw = (std::fmod((freq * 0.99f) * t, 1.0f) - 0.5f) * 0.3f;
    float output = env * (sine + saw);
    AudioUtils::LowPassFilter filter(100.0f, 44100.0f);
    output = filter.process(output);
    return output;
}

static float generateSynthArpWave(float t, float freq, float dur) {
    float attack = 0.01f, decay = 0.05f, sustain = 0.6f, release = 0.1f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float modFreq = freq * 3.0f;
    float modIndex = 1.0f + 0.5f * std::sin(2.0f * M_PI * t / dur);
    float carrier = std::sin(2.0f * M_PI * freq * t + modIndex * std::sin(2.0f * M_PI * modFreq * t));
    float saw = (std::fmod(freq * t, 1.0f) - 0.5f) * 0.3f;
    float output = env * (carrier * 0.7f + saw);
    AudioUtils::Distortion dist(1.7f, 0.75f);
    AudioUtils::Reverb reverb(0.06f, 0.4f, 0.2f);
    AudioUtils::LowPassFilter filter(4000.0f, 44100.0f);
    output = dist.process(output);
    output = reverb.process(output);
    output = filter.process(output);
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
    return output;
}

static float generatePadWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter filter(800.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.8f, 0.8f, 0.6f, 44100.0f);
    if (!std::isfinite(t) || t < 0.0f || !std::isfinite(freq) || freq <= 0.0f || !std::isfinite(dur)) {
        return 0.0f;
    }
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
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
    return output;
}

static float generateCymbalWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float env = std::exp(-8.0f * t / dur) * (1.0f + 0.3f * std::sin(4.0f * M_PI * t / dur));
    float baseFilterFreq = (freq > 0.0f ? std::min(freq * 12.0f, 14000.0f) : 12000.0f);
    float filterCutoff = 8000.0f + 4000.0f * std::exp(-6.0f * t / dur);
    float filter = std::sin(2.0f * M_PI * baseFilterFreq * t) * 0.5f + 0.5f;
    float noise = rng.generateWhiteNoise() * filter * 0.7f;
    float saw = 0.3f * (std::fmod(baseFilterFreq * t, 1.0f) - 0.5f);
    float tonal = 0.1f * std::sin(2.0f * M_PI * freq * t) * std::exp(-4.0f * t / dur);
    float output = env * (noise + saw + tonal);
    output *= (filterCutoff - 8000.0f) / 4000.0f;
    static AudioUtils::Reverb reverb(0.1f, 0.5f, 0.3f);
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    return output;
}

static float generateVocalWave(float t, float freq, int phoneme, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.05f, decay = 0.1f, sustain = 0.8f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float formant1 = 400.0f + (phoneme % 7) * 150.0f;
    float formant2 = formant1 * 1.8f + 200.0f * std::sin(2.0f * M_PI * 0.5f * t);
    float saw = (std::fmod(freq * t, 1.0f) - 0.5f) * 0.5f;
    float formant = 0.3f * std::sin(2.0f * M_PI * formant1 * t) + 0.2f * std::sin(2.0f * M_PI * formant2 * t);
    float breath = rng.generatePinkNoise() * std::exp(-10.0f * t / dur) * 0.25f;
    float vibrato = 1.0f + 0.015f * std::sin(2.0f * M_PI * 5.0f * t);
    float output = env * (saw + formant + breath) * vibrato;
    AudioUtils::Reverb reverb(0.15f, 0.5f, 0.35f);
    AudioUtils::LowPassFilter filter(3500.0f, 44100.0f);
    output = reverb.process(output);
    output = filter.process(output);
    return output;
}

static float generateFluteWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.05f, decay = 0.1f, sustain = 0.8f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float breath = rng.generatePinkNoise() * std::exp(-10.0f * t / dur) * 0.35f;
    AudioUtils::BandPassFilter breathFilter(2000.0f, 1.0f, 44100.0f);
    breath = breathFilter.process(breath);
    float vibrato = 1.0f + 0.01f * std::sin(2.0f * M_PI * 6.0f * t);
    float sine = std::sin(2.0f * M_PI * freq * t * vibrato) * 0.65f;
    float saw = (std::fmod(freq * t, 1.0f) - 0.5f) * 0.15f;
    float output = env * (sine + saw + breath);
    AudioUtils::Reverb reverb(0.1f, 0.45f, 0.25f);
    AudioUtils::LowPassFilter filter(3000.0f, 44100.0f);
    output = reverb.process(output);
    output = filter.process(output);
    return output;
}

static float generateTrumpetWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    float attack = 0.03f, decay = 0.1f, sustain = 0.8f, release = 0.2f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float modFreq = freq * 2.0f;
    float modIndex = 2.0f + 0.5f * std::sin(2.0f * M_PI * t / dur);
    float carrier = std::sin(2.0f * M_PI * freq * t + modIndex * std::sin(2.0f * M_PI * modFreq * t));
    float breath = rng.generatePinkNoise() * std::exp(-15.0f * t / dur) * 0.2f;
    float vibrato = 1.0f + 0.02f * std::sin(2.0f * M_PI * 5.5f * t);
    float output = env * (carrier * 0.75f + breath) * vibrato;
    AudioUtils::Distortion dist(1.5f, 0.8f);
    AudioUtils::Reverb reverb(0.12f, 0.5f, 0.3f);
    AudioUtils::BandPassFilter filter(2000.0f, 1.0f, 44100.0f);
    output = dist.process(output);
    output = reverb.process(output);
    output = filter.process(output);
    return output;
}

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
            float noise = rng.generatePinkNoise() * 0.4f;
            state1.delayLine[i] = pulse * 0.5f + noise;
            state2.delayLine[i] = pulse * 0.45f + noise * 0.9f;
        }
        if (!oldDelayLine1.empty()) {
            size_t crossfadeLen = std::min(oldDelayLine1.size(), state1.delayLineSize) / 4;
            for (size_t i = 0; i < crossfadeLen; ++i) {
                float t = static_cast<float>(i) / crossfadeLen;
                state1.delayLine[i] = (1.0f - t) * oldDelayLine1[i % oldDelayLine1.size()] + t * state1.delayLine[i];
                state2.delayLine[i] = (1.0f - t) * oldDelayLine2[i % oldDelayLine2.size()] + t * state2.delayLine[i];
            }
        }
        size_t harmonicSize = state1.delayLineSize / 2;
        std::vector<float> harmonicLine(harmonicSize, 0.0f);
        for (size_t i = 0; i < std::min(initSize, harmonicSize); ++i) {
            float x = static_cast<float>(i) / initSize;
            harmonicLine[i] = std::sin(2.0f * M_PI * x) * (1.0f - x) * 0.1f;
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
    static AudioUtils::LowPassFilter feedbackLPF1(400.0f, sampleRate);
    static AudioUtils::LowPassFilter feedbackLPF2(400.0f, sampleRate);
    float cutoff = 400.0f - 200.0f * (time / (dur + 2.0f));
    cutoff = std::max(200.0f, cutoff);
    feedbackLPF1.setCutoff(cutoff);
    feedbackLPF2.setCutoff(cutoff);
    float y1 = feedbackLPF1.process(x1);
    float y2 = feedbackLPF2.process(x2);
    float pitchVariation = 1.0f - 0.03f * (time / (dur + 2.0f));
    pitchVariation *= 1.0f + 0.002f * std::sin(2.0f * M_PI * 0.2f * time);
    float pluckNoise = rng.generatePinkNoise() * std::exp(-15.0f * time) * 0.08f;
    float fingerTap = (time < 0.01f) ? rng.generateWhiteNoise() * 0.06f : 0.0f;
    state1.delayLine[state1.writePos] = (y1 * 0.995f + pluckNoise + fingerTap) * pitchVariation;
    state2.delayLine[state2.writePos] = (y2 * 0.99f + pluckNoise * 0.8f + fingerTap * 0.8f) * pitchVariation;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.01f, decay = 0.15f, sustain = 0.5f, release = 1.5f, env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain + 0.03f * std::sin(2.0f * M_PI * 1.0f * time);
    } else if (time < dur + release) {
        float t = (time - dur) / release;
        env = sustain * (1.0f - t) * std::exp(-t * 4.0f);
    } else {
        env = 0.0f;
    }
    float resonanceFreq = 80.0f;
    float resonanceFilter = std::sin(2.0f * M_PI * resonanceFreq * time) * 0.5f + 0.5f;
    float resonanceNoise = rng.generatePinkNoise() * resonanceFilter * 0.06f * (time < dur ? 1.0f : std::exp(-(time - dur) / release));
    pluckNoise *= env;
    fingerTap *= env;
    resonanceNoise *= env;
    output += resonanceNoise;
    float harmonic1 = 0.6f * std::cos(2.0f * M_PI * freq * pitchVariation * time) * std::exp(-0.7f * time);
    float harmonic2 = 0.25f * std::cos(2.0f * M_PI * 2.0f * freq * pitchVariation * time) * std::exp(-1.0f * time);
    float harmonic3 = 0.1f * std::cos(2.0f * M_PI * 3.0f * freq * pitchVariation * time) * std::exp(-1.4f * time);
    output += (harmonic1 + harmonic2 + harmonic3) * env;
    output = (output + pluckNoise + fingerTap) * env;
    static AudioUtils::LowPassFilter bodyResonance(400.0f, sampleRate);
    float resonanceCutoff = 400.0f;
    if (time > dur) {
        resonanceCutoff *= std::exp(-(time - dur) / release);
    }
    bodyResonance.setCutoff(std::max(100.0f, resonanceCutoff));
    output = bodyResonance.process(output);
    static AudioUtils::Reverb reverb(0.05f, 0.3f, 0.1f, sampleRate);
    output = reverb.process(output);
    output *= 0.8f;
    output = std::max(-1.0f, std::min(1.0f, output));
    if (time > dur + release - 0.01f && std::abs(output) > 0.001f) {
        SDL_Log("Non-zero output at note end: %.6f", output);
    }
    return output;
}

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
    static AudioUtils::LowPassFilter feedbackLPF1(1200.0f, sampleRate);
    static AudioUtils::LowPassFilter feedbackLPF2(1200.0f, sampleRate);
    float cutoff = 1200.0f - 800.0f * (time / (dur + 3.0f));
    cutoff = std::max(400.0f, cutoff);
    feedbackLPF1.setCutoff(cutoff);
    feedbackLPF2.setCutoff(cutoff);
    float y1 = feedbackLPF1.process(x1);
    float y2 = feedbackLPF2.process(x2);
    float pitchVariation = 1.0f - 0.07f * (time / (dur + 3.0f));
    pitchVariation *= 1.0f + 0.005f * std::sin(2.0f * M_PI * 0.6f * time);
    float fretNoise = rng.generatePinkNoise() * std::exp(-50.0f * time) * 0.05f;
    float pickScrape = (time < 0.008f) ? rng.generateWhiteNoise() * 0.1f : 0.0f;
    state1.delayLine[state1.writePos] = (y1 * 0.995f + fretNoise + pickScrape) * pitchVariation;
    state2.delayLine[state2.writePos] = (y2 * 0.99f + fretNoise * 0.8f + pickScrape * 0.8f) * pitchVariation;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.008f, decay = 0.3f, sustain = 0.3f, release = 3.0f, env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain + 0.1f * std::sin(2.0f * M_PI * 2.0f * time);
    } else if (time < dur + release) {
        env = sustain * std::exp(-(time - dur) / release);
    } else {
        env = 0.0f;
    }
    float resonanceFreq = 300.0f;
    float resonanceFilter = std::sin(2.0f * M_PI * resonanceFreq * time) * 0.5f + 0.5f;
    float resonanceNoise = rng.generatePinkNoise() * resonanceFilter * 0.1f * (time < dur ? 1.0f : std::exp(-(time - dur) / release));
    output += resonanceNoise;
    float harmonic1 = 0.7f * std::cos(2.0f * M_PI * freq * pitchVariation * time) * std::exp(-1.0f * time);
    float harmonic2 = 0.4f * std::cos(2.0f * M_PI * 2.0f * freq * pitchVariation * time) * std::exp(-1.5f * time);
    float harmonic3 = 0.2f * std::cos(2.0f * M_PI * 3.0f * freq * pitchVariation * time) * std::exp(-2.0f * time);
    float harmonic4 = 0.1f * std::cos(2.0f * M_PI * 4.0f * freq * pitchVariation * time) * std::exp(-2.5f * time);
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * env;
    output = (output + fretNoise + pickScrape) * env;
    static AudioUtils::LowPassFilter bodyResonance(1200.0f, sampleRate);
    output = bodyResonance.process(output);
    static AudioUtils::Reverb reverb(0.1f, 0.5f, 0.15f, sampleRate);
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
    return output;
}

static float generateSaxophoneWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter noiseFilter(1650.0f, 0.8f, sampleRate);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(138.59f, std::min(880.0f, freq));
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = state1.delayLineSize / 3;
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float sawtooth = (2.0f * (x - std::floor(x + 0.5f))) * (1.0f - x);
            float noise = noiseFilter.process(rng.generateWhiteNoise()) * 0.2f;
            state1.delayLine[i] = sawtooth * 0.85f + noise;
            state2.delayLine[i] = sawtooth * 0.8f + noise * 0.8f;
        }
        size_t harmonicSize = state1.delayLineSize / 2;
        std::vector<float> harmonicLine(harmonicSize, 0.0f);
        for (size_t i = 0; i < std::min(initSize, harmonicSize); ++i) {
            float x = static_cast<float>(i) / initSize;
            harmonicLine[i] = (std::sin(2.0f * M_PI * x) + 0.7f * std::sin(6.0f * M_PI * x) + 0.4f * std::sin(10.0f * M_PI * x)) * (1.0f - x) * 0.45f;
        }
        state1.delayLine.insert(state1.delayLine.end(), harmonicLine.begin(), harmonicLine.end());
        state1.delayLine.resize(state1.delayLineSize);
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float breathNoise = noiseFilter.process(rng.generateWhiteNoise()) * 0.06f * (time < dur ? 1.0f : std::exp(-(time - dur) / 0.5f));
    state1.delayLine[state1.writePos] = x1 * 0.999f + breathNoise;
    state2.delayLine[state2.writePos] = x2 * 0.998f + breathNoise * 0.8f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float attack = 0.005f, decay = 0.03f, sustain = 0.9f, release = 0.5f, env;
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
    float overblow = (time < 0.008f) ? 0.12f * noiseFilter.process(rng.generateWhiteNoise()) : 0.0f;
    float breath = noiseFilter.process(rng.generateWhiteNoise()) * 0.08f * env;
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic3 = 0.7f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic5 = 0.4f * std::cos(2.0f * M_PI * 5.0f * freq * time) * env;
    float harmonic7 = 0.2f * std::cos(2.0f * M_PI * 7.0f * freq * time) * env;
    output += (harmonic1 + harmonic3 + harmonic5 + harmonic7) * 0.65f;
    output = (output + breath + overblow) * env;
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
    return output;
}

static float generatePianoWave(float sampleRate, float freq, float time, float dur, KarplusStrongState& state1, KarplusStrongState& state2) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(1800.0f, 44100.0f);
    static AudioUtils::Reverb reverb(0.12f, 0.55f, 0.35f, 44100.0f);
    if (!std::isfinite(sampleRate) || sampleRate <= 0.0f || !std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid sampleRate %.2f or freq %.2f, returning 0.0", sampleRate, freq);
        return 0.0f;
    }
    freq = std::max(27.5f, std::min(4186.0f, freq));
    if (std::abs(state1.lastFreq - freq) > 0.1f || state1.delayLine.empty()) {
        state1.lastFreq = state2.lastFreq = freq;
        state1.delayLineSize = state2.delayLineSize = static_cast<size_t>(sampleRate / freq);
        if (state1.delayLineSize < 2) state1.delayLineSize = 2;
        state1.delayLine.assign(state1.delayLineSize, 0.0f);
        state2.delayLine.assign(state2.delayLineSize, 0.0f);
        size_t initSize = std::min(state1.delayLineSize / 4, static_cast<size_t>(8));
        for (size_t i = 0; i < initSize; ++i) {
            float x = static_cast<float>(i) / initSize;
            float impulse = (1.0f - x) * (1.0f - x);
            float noise = rng.generatePinkNoise() * 0.04f;
            state1.delayLine[i] = impulse * 0.85f + noise;
            state2.delayLine[i] = impulse * 0.8f + noise * 0.8f;
        }
    }
    size_t readPos = (state1.writePos + state1.delayLineSize - 1) % state1.delayLineSize;
    float x1 = state1.delayLine[readPos];
    float x2 = state2.delayLine[readPos];
    float output = 0.5f * (x1 + x2);
    float filteredOutput = stringFilter.process(output);
    float damping = 0.994f - std::min(freq / 10000.0f, 0.02f);
    state1.delayLine[state1.writePos] = filteredOutput * damping;
    state2.delayLine[state2.writePos] = filteredOutput * damping * 0.98f;
    state1.writePos = (state1.writePos + 1) % state1.delayLineSize;
    state2.writePos = (state2.writePos + 1) % state2.delayLineSize;
    float decayTime = 2.0f - std::min(freq / 2000.0f, 1.5f);
    float env;
    if (time < 0.002f) {
        env = time / 0.002f;
    } else if (time < dur) {
        env = std::exp(-time / decayTime);
    } else {
        env = std::exp(-(time - dur) / (decayTime * 0.5f));
    }
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * time) * env;
    float harmonic2 = 0.55f * std::cos(2.0f * M_PI * 2.0f * freq * time) * env;
    float harmonic3 = 0.35f * std::cos(2.0f * M_PI * 3.0f * freq * time) * env;
    float harmonic4 = 0.2f * std::cos(2.0f * M_PI * 4.0f * freq * time) * env;
    output += (harmonic1 + harmonic2 + harmonic3 + harmonic4) * 0.5f;
    output *= env;
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.25f;
    return output;
}

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
    output *= 0.25f;
    return output;
}

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
    return output;
}

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
    output *= 0.25f;
    return output;
}

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
    output *= 0.25f;
    return output;
}

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
    output *= 0.25f;
    return output;
}

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
    output *= 0.25f;
    return output;
}

} // namespace Instruments

#endif // INSTRUMENTS_H