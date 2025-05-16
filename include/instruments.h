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

class HighPassFilter {
    float cutoffFreq, q, sampleRate;
    float x1, x2, y1, y2; // Store two previous inputs and outputs for second-order filter
    float b0, b1, b2, a1, a2; // Biquad coefficients
public:
    HighPassFilter(float cutoff, float qVal, float sr) : cutoffFreq(cutoff), q(qVal), sampleRate(sr), 
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
        updateCoefficients();
    }

    void updateCoefficients() {
        float w0 = 2.0f * M_PI * cutoffFreq / sampleRate;
        float cosW0 = std::cos(w0);
        float alpha = std::sin(w0) / (2.0f * q);
        float a0 = 1.0f + alpha;
        b0 = (1.0f + cosW0) / 2.0f / a0;
        b1 = -(1.0f + cosW0) / a0;
        b2 = (1.0f + cosW0) / 2.0f / a0;
        a1 = -2.0f * cosW0 / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process(float input) {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        return output;
    }

    void setCutoff(float cutoff) {
        cutoffFreq = cutoff;
        updateCoefficients();
    }

    void setQ(float qVal) {
        q = qVal;
        updateCoefficients();
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
struct WaveguideState;
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

struct FormantFilter {
    float centerFreq, bandwidth, sampleRate;
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;

    FormantFilter(float freq, float bw, float sr) : centerFreq(freq), bandwidth(bw), sampleRate(sr),
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
        updateCoefficients();
    }

    void updateCoefficients() {
        float r = expf(-M_PI * bandwidth / sampleRate);
        float theta = 2.0f * M_PI * centerFreq / sampleRate;
        b0 = 1.0f - r;
        b1 = 0.0f;
        b2 = 0.0f;
        a1 = -2.0f * r * cosf(theta);
        a2 = r * r;
    }

    float process(float input) {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = input;
        y2 = y1; y1 = output;
        return output;
    }

    void setParameters(float freq, float bw) {
        centerFreq = freq;
        bandwidth = bw;
        updateCoefficients();
    }
};

struct WaveguideState {
    std::vector<float> forwardWave, backwardWave;
    size_t delayLineSize = 0;
    size_t writePos = 0;
    float lastFreq = 0.0f;
    float pressure = 0.0f; // Breath pressure
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
    if (instrument == "hihat_closed") return generateHiHatWave(t, freq, open, dur);
	if (instrument == "hihat_open") return generateHiHatWave(t, freq, open, dur);
    if (instrument == "snare") return generateSnareWave(t, dur);
    if (instrument == "clap") return generateClapWave(t, dur);
    if (instrument == "tom") return generateTomWave(t, freq, dur);
    if (instrument == "subbass") return generateSubBassWave(t, freq, dur);
    if (instrument == "syntharp") return generateSynthArpWave(t, freq, dur);
    if (instrument == "leadsynth") return generateLeadSynthWave(t, freq, dur);
    if (instrument == "pad") return generatePadWave(t, freq, dur);
    if (instrument == "cymbal") return generateCymbalWave(t, freq, dur);
	if (instrument == "vocal_0") return generateVocalWave(t, freq, phoneme, dur);
    if (instrument == "vocal_1") return generateVocalWave(t, freq, phoneme, dur);
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

// Existing instrument implementations (unchanged)
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
    float release = open ? 0.8f : 0.1f; // Longer for open, short for closed
    if (t > dur + release) {
        return 0.0f; // Force zero output
    }

    // Envelope: tailored for short durations
    float envDecay = open ? -10.0f * t / (dur * 4.0f) : -12.0f * t / (dur * 0.5f); // ~0.6s open, ~0.08s closed
    float env = std::exp(envDecay);
    if (open) {
        env *= (1.0f + 0.1f * std::sin(2.0f * M_PI * 5.0f * t)); // Subtle shimmer for open
    }

    // Base frequency for tonal components
    float baseFreq = (freq > 0.0f ? freq : 1000.0f); // Default 1 kHz if freq invalid
    float filterFreq = baseFreq * 8.0f; // Center ~8 kHz for hi-hat
    float filterQ = open ? 1.5f : 2.0f; // Broader for open, tighter for closed

    // Time-varying filter with slight sweep
    float filterSweep = std::exp(-5.0f * t / (open ? dur * 2.0f : dur * 0.3f)); // Faster sweep for closed
    filterFreq *= (0.8f + 0.2f * filterSweep); // Sweep down slightly
    AudioUtils::BandPassFilter filter(filterFreq, filterQ, 44100.0f);

    // Noise components
    float pseudoVelocity = 0.7f; // Placeholder (adjust with actual velocity if available)
    float mainNoise = rng.generatePinkNoise() * 0.5f * pseudoVelocity; // Pink noise for warmth
    mainNoise = filter.process(mainNoise);

    // Additional high-passed noise for sizzle
    float sizzleFreq = open ? 6000.0f : 8000.0f; // Lower for open, higher for closed
    AudioUtils::HighPassFilter sizzleFilter(sizzleFreq, 1.0f, 44100.0f);
    float sizzleNoise = rng.generatePinkNoise() * (open ? 0.3f : 0.15f) * pseudoVelocity;
    sizzleNoise = sizzleFilter.process(sizzleNoise);

    // Tonal components: sine partials for metallic ring
    float tonal = 0.0f;
    tonal += 0.1f * std::sin(2.0f * M_PI * baseFreq * t) * (open ? 0.6f : 1.0f); // Fundamental
    tonal += 0.07f * std::sin(2.0f * M_PI * 2.0f * baseFreq * t) * (open ? 0.8f : 0.9f); // 2nd partial
    tonal += 0.03f * std::sin(2.0f * M_PI * 3.0f * baseFreq * t) * (open ? 1.0f : 0.7f); // 3rd partial
    tonal *= std::exp(-8.0f * t / (open ? dur * 2.0f : dur * 0.4f)); // Faster decay for closed

    // Combine components
    float output = env * (mainNoise + sizzleNoise + tonal);

    // Soft distortion for grit
    AudioUtils::Distortion dist(1.2f, 0.9f);
    output = dist.process(output);

    // Skip reverb since UseReverb: false
    output *= 0.7f; // Lower gain to prevent clipping
    output = std::max(-1.0f, std::min(1.0f, output));

    // Log non-zero output
    if (t > dur + release - 0.01f && std::abs(output) > 0.001f) {
        SDL_Log("Non-zero output at note end: %.6f", output);
    }

	output *= 5.0f;
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
	
	output *= 0.5f;
    return output;
}

static float generateClapWave(float t, float dur) {
    static AudioUtils::RandomGenerator rng;

    // Shorten duration for clap-like transient (80-150 ms typical)
    dur = std::clamp(dur, 0.08f, 0.15f);

    // Tight ADSR envelope for sharp attack and quick decay
    float attack = 0.002f, decay = 0.03f, sustain = 0.2f, release = 0.05f, env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(t - dur) / release);
    }

    // Layered noise bursts to simulate clap transients
    float burst1 = (t < 0.002f) ? rng.generateWhiteNoise() * 1.0f : 0.0f;
    float burst2 = (t >= 0.002f && t < 0.004f) ? rng.generateWhiteNoise() * 0.8f : 0.0f;
    float burst3 = (t >= 0.004f && t < 0.006f) ? rng.generateWhiteNoise() * 0.6f : 0.0f;
    float noise = rng.generatePinkNoise() * 0.4f; // Continuous pink noise for body

    // Band-passed noise for tonal character (~200-2000 Hz)
    float tonal = rng.generateWhiteNoise() * std::sin(2.0f * M_PI * 800.0f * t) * 0.3f;

    // Combine sound components
    float output = env * (burst1 + burst2 + burst3 + noise + tonal);

    // Apply band-pass filter effect implicitly via tonal shaping
    static AudioUtils::Distortion dist(1.4f, 0.6f); // Light distortion for grit
    static AudioUtils::Reverb reverb(0.03f, 0.3f, 0.2f); // Short reverb for space
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
    // Clamp frequency to 20-180 Hz
    freq = std::clamp(freq, 20.0f, 180.0f);

    // ADSR envelope parameters
    float attack = 0.02f, decay = 0.15f, sustain = 0.8f, release = 0.25f, env;

    // Calculate envelope
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(t - dur) / release);
    }

    // Generate waveform: sine for warmth, slight saw for edge
    float sine = std::sin(2.0f * M_PI * freq * t) * 0.7f;
    float saw = (std::fmod((freq * 0.99f) * t, 1.0f) - 0.5f) * 0.3f;
    float output = env * (sine + saw);

    // Apply low-pass filter at 200 Hz to preserve 20-180 Hz range
    static AudioUtils::LowPassFilter filter(200.0f, 44100.0f);
    output = filter.process(output);

    return output;
}

static float generateSynthArpWave(float t, float freq, float dur) {
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter formant1(800.0f, 1.2f, 44100.0f);
    static AudioUtils::BandPassFilter formant2(2400.0f, 1.5f, 44100.0f);
    static AudioUtils::BandPassFilter breathFilter(2000.0f, 0.7f, 44100.0f);

    // Input validation
    if (!std::isfinite(freq) || freq <= 0.0f) {
        SDL_Log("Invalid freq %.2f, returning 0.0", freq);
        return 0.0f;
    }
    freq = std::max(138.59f, std::min(880.0f, freq)); // Saxophone range (C#3 to A5)

    // ADSR envelope
    float attack = 0.003f, decay = 0.02f, sustain = 0.85f, release = 0.3f, env;
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
    float vibratoFreq = 5.5f;
    float vibratoDepth = 0.005f;
    float vibrato = (t > 0.1f) ? std::sin(2.0f * M_PI * vibratoFreq * t) * vibratoDepth : 0.0f;
    float modulatedFreq = freq * (1.0f + vibrato);

    // Dynamic harmonics
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t) * env;
    float harmonic3 = 0.8f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t) * env;
    float harmonic5 = 0.5f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t) * env;
    float harmonic7 = 0.3f * std::cos(2.0f * M_PI * 7.0f * modulatedFreq * t) * env;
    float harmonic2 = 0.2f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t) * env;
    float output = (harmonic1 + harmonic2 + harmonic3 + harmonic5 + harmonic7) * 0.5f;

    // Formant filters
    output = formant1.process(output) * 1.2f + formant2.process(output) * 0.8f;

    // Articulation noise
    float articulation = (t < 0.005f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.2f : 0.0f;

    // Combine output
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.1f * (t < 0.05f ? 1.5f : 1.0f);
    output = (output + breathNoise * env + articulation) * env;

    // Clip and scale
    output = std::max(-1.0f, std::min(1.0f, output));
    output *= 0.3f;

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

    // Clamp duration to 0.1-1.5 seconds for cymbal realism
    dur = std::clamp(dur, 0.1f, 1.5f);

    // Use freq as base for metallic components, default to 6000 Hz if invalid
    freq = (freq > 0.0f) ? std::clamp(freq, 2000.0f, 10000.0f) : 6000.0f;

    // Envelope with sharp attack and long tail
    float env = std::exp(-6.0f * t / dur) * (1.0f + 0.4f * std::sin(8.0f * M_PI * t / dur));
    env = std::max(0.0f, env);

    // High-frequency noise for cymbal shimmer
    float whiteNoise = rng.generateWhiteNoise() * 0.7f;
    float pinkNoise = rng.generatePinkNoise() * 0.3f;

    // Metallic inharmonic components (scaled by freq)
    float metallic1 = std::sin(2.0f * M_PI * freq * t) * 0.2f * std::exp(-4.0f * t / dur);
    float metallic2 = std::sin(2.0f * M_PI * (freq * 1.5f) * t) * 0.15f * std::exp(-5.0f * t / dur);
    float metallic3 = std::sin(2.0f * M_PI * (freq * 2.0f) * t) * 0.1f * std::exp(-6.0f * t / dur);

    // Modulate noise to simulate cymbal's high-passed character
    float filterMod = 0.5f + 0.5f * std::sin(2.0f * M_PI * (8000.0f + 6000.0f * std::exp(-5.0f * t / dur)) * t);
    float noise = (whiteNoise + pinkNoise) * filterMod;

    // Combine components
    float output = env * (noise + metallic1 + metallic2 + metallic3);

    // Apply reverb for long, shimmering tail
    static AudioUtils::Reverb reverb(0.15f, 0.6f, 0.4f, 44100.0f);
    output = reverb.process(output);

    // Clip output to prevent distortion
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

	output *= 5.0f;
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