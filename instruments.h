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

namespace AudioUtils {
const float SAMPLE_RATE = 44100.0f; // SDL2 can handle up to 44100.0f - boss says so.
const int CHANNELS = 8; // SDL2 can handle up to 8. - boss says so
const int BUFFER_SIZE = 128; // 2.9ms latency
const int RING_BUFFER_COUNT = 4;

// does this change ./songgen audio volume here? - 1.0f = 100% - try 0.0f for mute
float output = 1.0f; // I add these for volume adjustments - probably overwritten and may not adjust volume here.

// RandomGenerator - it told me mt19937 rng is the step up from other rng Random Generator.
// std::mt19937(since C++11) class is a very efficient pseudo-random number generator and is defined in a random header file. It produces 32-bit pseudo-random numbers using the well-known and popular algorithm named Mersenne twister algorithm. std::mt19937 class is basically a type of std::mersenne_twister_engine class. 
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
    float generateUniform(float min, float max) {
        std::uniform_real_distribution<float> uniform_dist(min, max);
        return uniform_dist(rng);
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
    float cutoffFreq, x1, y1;
public:
    LowPassFilter(float cutoff) : cutoffFreq(cutoff), x1(0.0f), y1(0.0f) {}
    float process(float input) {
        float alpha = 1.0f / (1.0f + 2.0f * M_PI * cutoffFreq / AudioUtils::SAMPLE_RATE);
        float output = alpha * input + (1.0f - alpha) * y1;
        x1 = input; y1 = output;
        return output;
    }
    void setCutoff(float cutoff) {
        cutoffFreq = cutoff;
    }
};

// BandPassFilter - do not change - pass 
class BandPassFilter {
    float centerFreq, bandwidth, x1, x2, y1, y2;
public:
    BandPassFilter(float center, float bw) : centerFreq(center), bandwidth(bw), 
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {}
    float process(float input) {
        float w0 = 2.0f * M_PI * centerFreq / AudioUtils::SAMPLE_RATE;
        float alpha = std::sin(w0) * std::sinh(std::log(2.0f) / 2.0f * bandwidth * w0 / std::sin(w0));
        float b0 = alpha, b1 = 0.0f, b2 = -alpha;
        float a0 = 1.0f + alpha, a1 = -2.0f * std::cos(w0), a2 = 1.0f - alpha;
        output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2;
        x2 = x1; x1 = input; y2 = y1; y1 = output;
        return output;
    }
};

// HighPassFilter
class HighPassFilter {
    float cutoffFreq, x1, y1;
public:
    HighPassFilter(float cutoff, float q) : cutoffFreq(cutoff), x1(0.0f), y1(0.0f) {}
    float process(float input) {
        float omega = 2.0f * M_PI * cutoffFreq / AudioUtils::SAMPLE_RATE; // SDL2 supports 44100hz so we set it to that.
        float alpha = std::sin(omega) / (2.0f * 0.707f); // Q = 0.707f for Butterworth response, damping ratio (ζ)
        float cosOmega = std::cos(omega);
        float a0 = 1.0f + alpha;
        float b0 = (1.0f + cosOmega) / 2.0f;
        float b1 = -(1.0f + cosOmega);
        float b2 = (1.0f + cosOmega) / 2.0f;
        float a1 = -2.0f * cosOmega;
        float a2 = 1.0f - alpha;
        float output = (b0 / a0) * input + (b1 / a0) * x1 + (b2 / a0) * y1 - (a1 / a0) * x1 - (a2 / a0) * y1;
        y1 = x1; x1 = input;
        return output;
    }
};

// Reverb
class Reverb {
    std::vector<float> delayBuffer;
    size_t bufferSize;
    size_t writePos;
    float decay;
    float mix;
public:
    Reverb(float delayTime = 0.1f, float decayFactor = 0.5f, float mixFactor = 0.3f)
        : bufferSize(static_cast<size_t>(delayTime * AudioUtils::SAMPLE_RATE)), writePos(0),
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

} // namespace AudioUtils

namespace Instruments {
// instruments should not blow aside from prescribed frequencies
// a trumpet should not have access to all of the same ranges as a flute
// a snare drum has a velocity and cool off period less than 1 second.
struct AudioProtector {
    AudioUtils::HighPassFilter dcBlocker;
    float fadeOutTime;
    float maxGain;

    AudioProtector(float fadeTime = 0.005f, float gain = 0.9f)
        : dcBlocker(20.0f, 0.707f), // Block DC below 20Hz
          fadeOutTime(fadeTime),
          maxGain(gain) {}

    float process(float input, float t, float dur) {
        // Apply DC blocker
        float output = dcBlocker.process(input);

        // Apply fade-out near note end to prevent clicks
        if (t > dur - fadeOutTime) {
            float fade = 1.0f - (t - (dur - fadeOutTime)) / fadeOutTime;
            output *= std::max(0.0f, std::min(1.0f, fade));
        }

        // Soft clipping with tanh
        output = std::tanh(output * 1.2f) / 1.2f;

        // Simple peak limiter - Audioprotector
        float absOutput = std::abs(output);
        if (absOutput > maxGain) {
            output *= maxGain / absOutput;

        return output;
    }
};

struct FormantFilter {
    float centerFreq, bandwidth;
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
    FormantFilter(float freq, float bw) : centerFreq(freq), bandwidth(bw),
        x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) { updateCoefficients(); }
    void updateCoefficients() {
        float r = std::exp(-M_PI * bandwidth / AudioUtils::SAMPLE_RATE);
        float theta = 2.0f * M_PI * centerFreq / AudioUtils::SAMPLE_RATE;
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

// Wave generation function implementations
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

// Tom drum Drum
static float generateTomWave(float t, float freq, float dur) {
	static AudioProtector protector(0.008f, 0.85f); // fadeOutTime=0.008f, maxGain=0.85f
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
    AudioUtils::LowPassFilter filter(300.0f);
    output = reverb.process(output);
    output = filter.process(output);
    
	// Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 1.0f; // I add these for volume adjustments
    return output;
}
// end Tom Wave drum Drum

// kick Kick drum Drum
static float generateKickWave(float t, float freq, float dur) {
    static AudioProtector protector(0.012f, 0.75f); // Optimized for clean kick
	static AudioUtils::RandomGenerator rng;    
    static AudioUtils::LowPassFilter filter(100.0f); // Tighter low-pass

    freq = std::max(50.0f, std::min(150.0f, freq)); // Typical kick range

    // Envelope parameters for punchy, dynamic kick
    float attack = 0.003f; // Sharper attack
    float decay = 0.12f; // Slightly faster decay
    float sustain = 0.35f; // Lower sustain for punch
    float release = 0.15f; // Longer release for smooth tail
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

    // Pitch-modulated body
    float baseFreq = freq * 0.8f;
    float pitchDecay = std::exp(-20.0f * t / dur); // Faster pitch decay
    float pitchMod = baseFreq * (1.5f * pitchDecay + 0.5f); // Reduced pitch sweep
    float sine = std::sin(2.0f * M_PI * pitchMod * t);
    float subSine = 0.25f * std::sin(2.0f * M_PI * (baseFreq * 0.5f) * t); // Softer sub

    // Noise component for click (reduced)
    float clickEnv = std::exp(-100.0f * t); // Faster click decay
    float whiteNoise = rng.generateWhiteNoise();
    float pinkNoise = rng.generatePinkNoise();
    AudioUtils::BandPassFilter clickFilter(1500.0f, 0.8f); // Lower, narrower
    float click = clickFilter.process(0.5f * whiteNoise + 0.5f * pinkNoise) * clickEnv * 0.15f; // Lower amplitude

    // Combine components
    float output = env * (0.65f * sine + 0.2f * subSine + 0.15f * click); // Adjusted weights

    // Apply low-pass filter (static, tight)
    output = filter.process(output);

    // Soft compression
    float absOutput = std::abs(output);
    if (absOutput > 0.7f) {
        output *= 0.7f / absOutput; // Gentle peak control
    }

    // Apply protective processing
    output = protector.process(output, t, dur);

    // Final gain - volume
    output *= 0.9f; // Strong but safe

    return output;
}
// end kick Kick drum Drum

// sounds decent. has open and closed versions of Hi Hat Cymbals.
static float generateHiHatWave(float t, float freq, bool open, float dur) {
	// Hi-Hat: Shorter 3ms fade for crisp transients; higher cutoff in DC blocker (already high-passed).
    static AudioProtector protector(0.003f, 0.9f); // Shorter fade for hi-hat
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::HighPassFilter openFilter(6000.0f, 0.707f);
    static AudioUtils::HighPassFilter closedFilter(10000.0f, 0.707f);
    static AudioUtils::Reverb reverb(0.02f, 0.2f, 0.15f);
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
	
    // Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 0.3f; // I add these for volume adjustments
    return output;
}
// end hi hat drum Drum more below

// decent
static float generateSnareWave(float t, float dur) {
    static AudioProtector protector(0.008f, 0.85f); // Longer fade, stricter gain
	static AudioUtils::RandomGenerator rng;    
    static AudioUtils::BandPassFilter crackFilter(1800.0f, 1.2f); // Softer crack
    static AudioUtils::BandPassFilter rattleFilter(3500.0f, 0.8f); // Smoother rattle
    static AudioUtils::Reverb reverb(0.08f, 0.4f, 0.2f); // Subtler reverb

    dur = std::clamp(dur, 0.05f, 0.5f); // Constrain duration

    // Velocity
    float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f);
    velocity = std::max(0.4f, std::min(1.0f, velocity));
    if (dur < 0.05f) velocity *= 0.7f;

    // ADSR envelope
    float attack = 0.0015f * (1.0f - 0.4f * velocity); // Sharper attack
    float decay = 0.04f;
    float sustain = 0.15f; // Lower sustain
    float release = 0.1f; // Smoother release
    float env;
    if (t < attack) {
        env = t / attack;
    } else if (t < attack + decay) {
        env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    } else if (t < dur) {
        env = sustain * std::exp(-12.0f * (t - attack - decay));
    } else {
        env = sustain * std::exp(-(t - dur) / release);
    }

    // Components
    float noise = rng.generatePinkNoise() * 0.3f; // Softer noise
    float crack = rng.generateWhiteNoise() * std::exp(-60.0f * t) * 0.25f * velocity; // Reduced crack
    crack = crackFilter.process(crack);
    float toneFreq = 200.0f + rng.generateUniform(-15.0f, 15.0f); // Tighter tone
    float phase = 2.0f * M_PI * toneFreq * t;
    float tone = (1.0f - 2.0f * std::fmod(phase / M_PI, 1.0f)) * std::exp(-25.0f * t) * 0.25f * velocity; // Stronger tone
    float rattleDecay = 0.08f + rng.generateUniform(-0.015f, 0.015f); // Shorter rattle
    float rattle = rng.generateWhiteNoise() * std::exp(-t / rattleDecay) * 0.3f * velocity;
    rattle = rattleFilter.process(rattle);

    // Combine components
    float output = env * (0.3f * noise + 0.25f * crack + 0.25f * tone + 0.2f * rattle); // Balanced weights

    // Apply reverb
    output = reverb.process(output);

    // Soft compression
    float absOutput = std::abs(output);
    if (absOutput > 0.8f) {
        output *= 0.8f / absOutput;
    }

    // Apply protective processing
    output = protector.process(output, t, dur);

    // Final volume 70%
    output *= 0.7f;

    return output;
}
// end snare Drum drum

// hand clap sound
static float generateClapWave(float t, float dur) {
	// 3ms fade for sharp bursts
	static AudioProtector protector(0.003f, 0.9f); // Shorter fade 0.003f for clap (sharper)
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::Distortion dist(1.4f, 0.6f);
    static AudioUtils::Reverb reverb(0.03f, 0.3f, 0.2f);    

    dur = std::clamp(dur, 0.08f, 0.15f); // clamp your hands
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
    output = dist.process(output);
    output = reverb.process(output);

    // Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 1.0f; // I add these for volume adjustments
    return output;
}
// end hand clap

// sub woofer needs work - sub bass Sub Bass Wave subs
static float generateSubBassWave(float t, float freq, float dur) {
	static AudioProtector protector(0.008f, 0.85f); // fadeOutTime=0.008f, maxGain=0.85f
    freq = std::clamp(freq, 20.0f, 80.0f);
    float attack = 0.005f, decay = 0.1f, sustain = 0.6f, release = 0.25f, env;
    if (t < attack) env = t / attack;
    else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
    else if (t < dur) env = sustain;
    else env = sustain * std::exp(-(t - dur) / release);
    float sine = std::sin(2.0f * M_PI * freq * t) * 0.85f;
    float triangle = (2.0f / M_PI) * std::asin(std::sin(2.0f * M_PI * freq * 0.99f * t)) * 0.15f;
    float output = env * (sine + triangle);
    static AudioUtils::LowPassFilter filter(80.0f);
    output = filter.process(output);
    output = std::tanh(output * 1.2f);
	
    // Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 1.0f; // I add these for volume adjustments 100%
    return output;
}
// end sub woofer

// what year is it? I am minimizing logic for it over in songgen.h
static float generateSynthArpWave(float t, float freq, float dur) {
	static AudioProtector protector(0.1f, 0.9f); // Adjust fade/gain
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
    static AudioUtils::LowPassFilter filter(4000.0f);
    static AudioUtils::Reverb reverb(0.1f, 0.5f, 0.3f);
    output = filter.process(output);
    output = reverb.process(output);
    output = std::max(-1.0f, std::min(1.0f, output));
	
    // Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 0.8f; // I add these for volume adjustments
    return output;
}
// end syntharp

// oof and not a good way. Fix this.
static float generateLeadSynthWave(float t, float freq, float dur) {
	static AudioProtector protector(0.1f, 0.9f); // Adjust fade/gain
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
    AudioUtils::LowPassFilter filter(5000.0f);
    output = dist.process(output);
    output = reverb.process(output);
    output = filter.process(output);
    
	// Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 1.0f; // I add these for volume adjustments
    return output;
}
// end leadsynth

// I modified from AI recommendation here
static float generatePadWave(float t, float freq, float dur) {
	static AudioProtector protector(0.1f, 0.9f); // Adjust fade/gain : default 0.001f not 0.1f
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter filter(800.0f);
    static AudioUtils::Reverb reverb(0.8f, 0.8f, 0.6f);
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
    
	// Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 0.25f; // I add these for volume adjustments
    return output;
}
// end pad Pad

// might need more work on the songgen.h side.
static float generateCymbalWave(float t, float freq, float dur) {
    static AudioProtector protector(0.008f, 0.85f); // Longer fade, stricter limiting
	static AudioUtils::RandomGenerator rng;    
    static AudioUtils::HighPassFilter hpFilter(500.0f, 0.707f); // Brighten sound
    static AudioUtils::Reverb reverb(0.1f, 0.5f, 0.35f); // Subtler reverb

    dur = std::clamp(dur, 0.1f, 1.5f);
    freq = (freq > 0.0f) ? std::clamp(freq, 2000.0f, 10000.0f) : 6000.0f;

    // Envelope with smoother modulation
    float env = std::exp(-5.0f * t / dur) * (1.0f + 0.3f * std::sin(6.0f * M_PI * t / dur));
    env = std::max(0.0f, env);

    // Noise mix
    float whiteNoise = rng.generateWhiteNoise() * 0.6f;
    float pinkNoise = rng.generatePinkNoise() * 0.4f;

    // Metallic tones with pitch bend
    float pitchBend = 1.0f + 0.005f * std::sin(2.0f * M_PI * 0.5f * t);
    float metallic1 = std::sin(2.0f * M_PI * freq * pitchBend * t) * 0.25f * std::exp(-3.5f * t / dur);
    float metallic2 = std::sin(2.0f * M_PI * (freq * 1.5f) * pitchBend * t) * 0.2f * std::exp(-4.5f * t / dur);
    float metallic3 = std::sin(2.0f * M_PI * (freq * 2.0f) * pitchBend * t) * 0.15f * std::exp(-5.5f * t / dur);

    // Simplified filter modulation
    float filterMod = 0.6f + 0.4f * std::exp(-4.0f * t / dur);
    float noise = (whiteNoise + pinkNoise) * filterMod;

    // Combine components
    float output = env * (0.7f * noise + 0.3f * (metallic1 + metallic2 + metallic3));

    // Apply high-pass filter
    output = hpFilter.process(output);

    // Apply reverb
    output = reverb.process(output);

    // Soft compression
    float absOutput = std::abs(output);
    if (absOutput > 0.8f) {
        output *= 0.8f / absOutput;
    }

    // Apply protective processing
    output = protector.process(output, t, dur);

    // Final volume gain adjustment
    output *= 0.4f;

    return output;
}

// forgot why I asked for this.
// oh yeah, 2 singers vocal_0 and vocal_1
// generateSingerWave generateVocalWave
struct VocalState {
    float current_freq = 0.0f;
    float current_dur = 0.0f;
    float start_time = 0.0f;
    bool is_new_note = true;
    float prev_output = 0.0f;
    float prev_time = -1.0f;
};

// pick new voices but this is a tone generator
static float generateVocalWave(float t, float freq, int phoneme, float dur, int depth) {
	static AudioProtector protector(0.005f, 0.9f); // Adjust fade/gain
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
    
    // Phoneme selection based on freq, went with deep 20hz male
    int selected_phoneme;
    if (depth == 1) { // Male: 20-90 Hz, 14 phonemes
        float freq_normalized = (base_freq - 20.0f) / (90.0f - 20.0f);
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
    AudioUtils::BandPassFilter formant1_filter(f1, bw1);
    AudioUtils::BandPassFilter formant2_filter(f2, bw2);
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
        AudioUtils::BandPassFilter prev_formant1_filter(prev_f1, prev_bw1 / prev_f1);
        AudioUtils::BandPassFilter prev_formant2_filter(prev_f2, prev_bw2 / prev_f2);
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
    AudioUtils::LowPassFilter filter(filter_cutoff);
    output = reverb.process(output);
    output = filter.process(output);
    
    // Final gain stage (increased for male voice)
    output *= (depth == 1) ? 2.0f : 1.8f;
    
    // Clamp to prevent clipping
    output = std::max(std::min(output, 1.0f), -1.0f);
    
    // Store current output for next iteration
    state.prev_output = output_current;
    
	// Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 0.2f; // I add these for volume adjustments
    return output;
}

// My Dad picked the flute.
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
	static AudioProtector protector(0.005f, 0.9f); // Adjust fade/gain
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(1600.0f, 300.0f); // Lower, narrower for natural breath
    
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

    output = std::max(-0.8f, std::min(0.8f, output)); // Tighter clipping for cleaner sound

    // Very subtle breath noise for organic feel
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.008f * (t < 0.04f ? 0.9f : 0.15f);
    breathNoise = std::max(-0.15f, std::min(0.15f, breathNoise));

    // Minimal articulation noise for natural note starts - some can probably be expanded
    float articulation = (t < 0.004f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.02f * env : 0.0f;
    articulation = std::max(-0.15f, std::min(0.15f, articulation));

    // Combine output
    output = output + breathNoise * env + articulation;

    // Apply minimal reverb and high-pass filter for clarity
    AudioUtils::Reverb reverb(0.02f, 0.15f, 0.1f); // Extremely light reverb
    AudioUtils::HighPassFilter filter(200.0f, 0.707f); // Remove low-end mud
    output = reverb.process(output);
    output = filter.process(output);

    // Soft clipping for natural dynamics
    output = std::tanh(output * 0.7f); // Softer gain for less synth-like aggression
    output *= 0.45f; // Moderate amplitude for balanced output
    output = std::max(-1.0f, std::min(1.0f, output)); // Final clip
	
	// Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 2.0f; // I add these for volume adjustments
    return output;
}
// This is enough for AI to design your flute instrument.
// You can try other instruments but only the ones here already can be used so do them individually so you can hear them inbetween and make sure it sounds better.

static float generateTrumpetWave(float t, float freq, float dur) {
    static AudioProtector protector(0.01f, 0.85f); // Longer fade, lower gain for safety
	static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(1500.0f, 500.0f); // Warmer breath noise
    static AudioUtils::LowPassFilter smoothFilter(4000.0f); // Smooth high frequencies
    static AudioUtils::Reverb reverb(0.03f, 0.3f, 0.15f); // Lighter reverb
    static AudioUtils::Distortion overdrive(1.8f, 0.8f); // Softer overdrive
	
    freq = std::max(155.56f, std::min(1244.51f, freq)); // Trumpet range (D#3 to D#6)

    // ADSR envelope (punchy attack, sustained presence)
    float attack = 0.002f; // Very sharp attack
    float decay = 0.01f; // Quick decay to sustain
    float sustain = 0.9f; // Strong sustain
    float release = 0.25f; // Smooth release for richness
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

    // Vibrato (subtle and stable)
    float vibratoFreq = 5.5f;
    float vibratoDepth = 0.004f * (t > 0.15f ? 1.0f : t / 0.15f); // Reduced depth
    float vibrato = std::sin(2.0f * M_PI * vibratoFreq * t) * vibratoDepth;
    float modulatedFreq = freq * (1.0f + vibrato);

    // Additive synthesis with warmer, richer harmonics
    float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t); // Fundamental
    float harmonic2 = 0.9f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t); // Strong 2nd
    float harmonic3 = 0.7f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t); // Prominent 3rd
    float harmonic4 = 0.5f * std::cos(2.0f * M_PI * 4.0f * modulatedFreq * t); // Softer 4th
    float harmonic5 = 0.3f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t); // Minimal 5th
    float output = (harmonic1 + harmonic2 + harmonic3 + harmonic4 + harmonic5) * 0.2f * env;

    // Chorus effect for richness (subtle detuned oscillator)
    float detune = 1.005f; // 0.5% detune
    float chorus = 0.3f * std::cos(2.0f * M_PI * modulatedFreq * detune * t) * env;
    output += chorus;
    output = std::max(-0.8f, std::min(0.8f, output)); // Tighter clipping

    // Breath noise (reduced and warmer)
    float breathEnv = (t < 0.05f ? 1.2f : 0.3f) * env;
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.03f * breathEnv;
    breathNoise = std::max(-0.3f, std::min(0.3f, breathNoise));

    // Articulation noise (subtle transient)
    float articulation = (t < 0.005f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.06f * env : 0.0f;
    articulation = std::max(-0.3f, std::min(0.3f, articulation));

    // Combine components
    output = output + breathNoise + articulation;

    // Smooth high frequencies
    output = smoothFilter.process(output);

    // Apply overdrive for brassy warmth
    output = overdrive.process(output);

    // Apply reverb for depth
    output = reverb.process(output);

    // Soft clipping and gain
    output = std::tanh(output * 1.2f); // Moderate clipping
    output *= 0.6f; // Balanced gain for boldness

    // Apply protective processing
    output = protector.process(output, t, dur);
	
	output *= 1.0f; // I add these for volume adjustments

    return output;
}

// guitar
static float generateBassWave(float freq, float time, float dur) {
    static AudioProtector protector(0.015f, 0.8f); // Optimized for bass
	static AudioUtils::RandomGenerator rng;    
    static AudioUtils::LowPassFilter filter(150.0f); // Tight low-pass for bass

    freq = std::max(40.0f, std::min(200.0f, freq)); // Bass guitar range

    // Velocity
    float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f);
    velocity = std::max(0.2f, std::min(1.0f, velocity));

    // ADSR envelope
    float attack = 0.005f;
    float decay = 0.1f;
    float sustain = 0.6f;
    float release = 0.2f;
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Waveform (simple sine for bass)
    output = std::sin(2.0f * M_PI * freq * time) * env * velocity;

    // Add subtle harmonic
    output += 0.3f * std::sin(2.0f * M_PI * 2.0f * freq * time) * env * velocity * std::exp(-time / 0.5f);

    // Apply low-pass filter
    output = filter.process(output);

    // Apply protective processing
    output = protector.process(output, time, dur);

    output *= 0.6f; // I add these for volume adjustments

    return output;
}

static float generateGuitar(float freq, float time, float dur) {
    static AudioProtector protector(0.015f, 0.85f); // Longer fade, stricter gain
	static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter bodyResonance(1000.0f); // Warmer resonance
    static AudioUtils::HighPassFilter highPass(80.0f, 0.707f); // Reduce low-end mud
    static AudioUtils::Reverb reverb(0.12f, 0.4f, 0.25f); // Subtle ambiance
    static AudioUtils::Distortion distortion(1.5f, 0.7f); // Light overdrive
    freq = std::clamp(freq, 80.0f, 1000.0f); // Guitar range (E2 to D5)

    // Velocity for dynamic pluck
    float velocity = 0.8f + rng.generateUniform(-0.2f, 0.2f); // Centered on forte
    velocity = std::max(0.3f, std::min(1.0f, velocity));
    if (dur < 0.2f) velocity *= 0.7f; // Softer for short notes

    // ADSR envelope for sharp attack and natural decay
    float attack = 0.005f * (1.0f - 0.3f * velocity); // Fast attack
    float decay = 0.1f;
    float sustain = 0.3f * velocity;
    float release = 0.3f; // Smooth release
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * std::exp(-2.0f * (time - attack - decay) / dur);
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Frequency-dependent decay
    float decayTime = 3.0f * std::pow(440.0f / freq, 0.5f);
    decayTime = std::max(0.5f, std::min(3.0f, decayTime));

    // Pluck transient
    float pluck = (time < 0.003f) ? rng.generateWhiteNoise() * 0.2f * velocity * (1.0f - time / 0.003f) : 0.0f;
    pluck = std::max(-0.25f, std::min(0.25f, pluck));

    // Additive synthesis for guitar tone
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.002f, 3.005f, 4.008f, 5.012f}; // Inharmonic ratios
    const float amps[] = {1.0f, 0.8f, 0.5f, 0.3f, 0.15f}; // Harmonic amplitudes
    for (int i = 0; i < 5; ++i) {
        float harmonicFreq = freq * harmonics[i];
        float harmonic = amps[i] * std::sin(2.0f * M_PI * harmonicFreq * time) * std::exp(-time / (decayTime * (1.0f - 0.15f * i)));
        output += harmonic * velocity;
    }
    output *= env * 0.3f; // Scale harmonics

    // Fret noise and body resonance
    float fretNoise = rng.generatePinkNoise() * std::exp(-50.0f * time) * 0.015f * velocity;
    static AudioUtils::BandPassFilter resonanceFilter(250.0f, 1.0f);
    float resonance = resonanceFilter.process(rng.generatePinkNoise()) * 0.05f * env * velocity;

    // Combine components
    output += pluck + fretNoise + resonance;

    // Apply filters
    output = bodyResonance.process(output);
    output = highPass.process(output);

    // Apply distortion
    output = distortion.process(output);

    // Apply reverb
    output = reverb.process(output);

    // Soft compression
    float absOutput = std::abs(output);
    if (absOutput > 0.8f) {
        output *= 0.8f / absOutput;
    }

    // Apply protective processing
    output = protector.process(output, time, dur);

    output *= 0.5f; // I add these for volume adjustments

    return output;
}
// end guitar

// room for improvement
static float generateSaxophoneWave(float freq, float time, float dur) {
	static AudioProtector protector(0.005f, 0.9f); // Adjust fade/gain
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::BandPassFilter breathFilter(2500.0f, 600.0f); // Breath noise filter

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

    output = std::max(-0.8f, std::min(0.8f, output)); // Clip harmonics

    // Breath noise
    float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.05f * (time < 0.05f ? 1.2f : 0.5f);
    breathNoise = std::max(-0.4f, std::min(0.4f, breathNoise));

    // Articulation noise
    float articulation = (time < 0.008f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.1f * env : 0.0f;
    articulation = std::max(-0.4f, std::min(0.4f, articulation));

    // Combine output
    output = output + breathNoise * env + articulation;

    // Soft clipping
    output = std::tanh(output * 0.5f); // Conservative gain
    output *= 0.3f;
    output = std::max(-1.0f, std::min(1.0f, output)); // Final clip
	
	// Apply protective processing
    output = protector.process(output, time, dur);
	
	output *= 1.0f; // I add these for volume adjustments
    return output;
}

// piano Piano
static float generatePianoWave(float freq, float time, float dur) {
    static AudioProtector protector(0.01f, 0.85f); // Smooth fade, strict limiting
	static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(4500.0f); // Warm, bright cutoff
    static AudioUtils::Reverb reverb(0.12f, 0.65f, 0.35f); // Concert-hall reverb

    freq = std::max(27.5f, std::min(4186.0f, freq)); // Piano range (A0 to C8)

    // Simulate velocity (0.2 to 1.0)
    float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f); // Mezzo-forte center
    if (dur < 0.1f) velocity *= 0.6f; // Short notes softer
    velocity = std::max(0.2f, std::min(1.0f, velocity));

    // Simulate sustain pedal
    bool sustainPedal = (dur > 1.5f || time > 3.0f); // Conservative pedal

    // ADSR envelope (sharp attack, natural decay)
    float attack = 0.001f * (1.0f - 0.4f * velocity); // Fast attack
    float decay = 0.05f; // Quick decay to sustain
    float sustain = 0.7f * velocity; // Dynamic sustain
    float release = 0.3f; // Smooth release
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur || sustainPedal) {
        env = sustain * std::exp(-(time - attack - decay) / (2.0f * (440.0f / freq))); // Frequency-dependent decay
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Frequency-dependent decay time (longer for low notes)
    float decayTime = 6.0f * std::pow(440.0f / freq, 0.7f);
    decayTime = std::max(0.5f, std::min(8.0f, decayTime));
    if (sustainPedal) decayTime *= 1.5f;

    // Hammer strike transient
    float transient = 0.0f;
    if (time < 0.002f) {
        transient = rng.generateWhiteNoise() * 0.25f * velocity * (1.0f - time / 0.002f);
        transient = std::max(-0.3f, std::min(0.3f, transient));
    }

    // Additive synthesis with piano harmonics
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.01f, 3.03f, 4.05f, 5.08f}; // Inharmonic ratios
    const float amps[] = {1.0f, 0.6f, 0.3f, 0.15f, 0.08f}; // Harmonic amplitudes
    const float decays[] = {1.0f, 0.8f, 0.6f, 0.4f, 0.3f}; // Relative decay rates
    for (int i = 0; i < 5; ++i) {
        float harmonicFreq = freq * harmonics[i];
        float harmonicDecay = decayTime * decays[i] * (440.0f / freq);
        float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * time) * std::exp(-time / harmonicDecay);
        output += harmonic * velocity;
    }
    output *= env * 0.3f; // Scale harmonics

    // Sympathetic resonance (subtle)
    if (sustainPedal) {
        float resFreq1 = freq * 2.0f; // Octave
        float resFreq2 = freq * 1.5f; // Fifth
        if (resFreq1 <= 4186.0f) {
            output += 0.05f * std::cos(2.0f * M_PI * resFreq1 * time) * env * velocity * std::exp(-time / (decayTime * 0.8f));
        }
        if (resFreq2 <= 4186.0f) {
            output += 0.03f * std::cos(2.0f * M_PI * resFreq2 * time) * env * velocity * std::exp(-time / (decayTime * 0.8f));
        }
    }

    // Add transient
    output += transient;

    // Apply low-pass filter
    output = stringFilter.process(output);

    // Apply reverb
    float reverbMix = 0.35f * (1.0f - std::min(freq / 4000.0f, 0.5f));
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    // Dynamic compression
    float absOutput = std::abs(output);
    if (absOutput > 0.8f) {
        output *= 0.8f / absOutput; // Soft limit
    }

    // Apply protective processing
    output = protector.process(output, time, dur);

    output *= 0.5f; // I add these for volume adjustments 50%

    return output;
}
// end piano

// untested
static float generateViolinWave(float freq, float time, float dur) {
    static AudioProtector protector(0.02f, 0.8f); // Longer fade, strict limiting
	static AudioUtils::RandomGenerator rng; // we need like random 1-5 and it does a bazillion of them (source)
    static AudioUtils::LowPassFilter stringFilter(2500.0f); // Warm, rich tone
    static AudioUtils::HighPassFilter highPass(80.0f, 0.707f); // Reduce low-end mud
    static AudioUtils::Reverb reverb(0.3f, 0.85f, 0.45f); // Lush, cloud-like reverb
    static AudioUtils::BandPassFilter bowFilter(2500.0f, 0.5f); // Bowed texture
    static AudioUtils::BandPassFilter shimmerFilter(5000.0f, 0.8f); // Ethereal shimmer

    freq = std::max(196.0f, std::min(3520.0f, freq)); // G3 to G7, violin range

    // Velocity for bold expression
    float velocity = 0.9f + rng.generateUniform(-0.1f, 0.1f); // Forte center for bold notes
    if (dur < 0.1f) velocity *= 0.6f; // Short notes softer
    velocity = std::max(0.3f, std::min(1.0f, velocity));

    // ADSR envelope for smooth attack and sustained, airy decay
    float attack = 0.02f * (1.0f - 0.2f * velocity); // Softer attack for blending
    float decay = 0.05f;
    float sustain = 0.95f * velocity; // High sustain for bold, continuous tone
    float release = 0.6f; // Long release for note overlap
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.02f * std::sin(2.0f * M_PI * 4.0f * time)); // Gentle vibrato
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Frequency-dependent decay time
    float decayTime = 5.0f * std::pow(440.0f / freq, 0.6f);
    decayTime = std::max(0.8f, std::min(6.0f, decayTime));

    // Bow transient for smooth onset
    float bowTransient = 0.0f;
    if (time < 0.015f) {
        bowTransient = bowFilter.process(rng.generatePinkNoise()) * 0.1f * velocity * (1.0f - time / 0.015f);
        bowTransient = std::max(-0.15f, std::min(0.15f, bowTransient));
    }

    // Additive synthesis with slight pitch glide for blending
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f}; // Inharmonic ratios
    const float amps[] = {1.0f, 0.7f, 0.5f, 0.3f}; // Rich, expressive partials
    float glide = (time < 0.05f) ? 1.0f + 0.01f * (1.0f - time / 0.05f) : 1.0f; // Subtle pitch glide
    for (int i = 0; i < 4; ++i) {
        float harmonicFreq = freq * harmonics[i] * glide;
        float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * time) * std::exp(-time / (decayTime * (1.0f - 0.2f * i)));
        output += harmonic * velocity;
    }
    output *= env * 0.35f; // Scale for bold, airy tone

    // Bow noise for realism
    float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.06f * velocity * env;
    output += bowNoise;

    // Shimmer for cloud-like sparkle
    float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.04f * env * velocity * std::exp(-time / (decayTime * 0.5f));
    output += shimmer;

    // Add transient
    output += bowTransient;

    // Apply filters
    output = stringFilter.process(output);
    output = highPass.process(output);

    // Apply lush reverb
    float reverbMix = 0.55f * (1.0f - std::min(freq / 3000.0f, 0.3f)); // Strong reverb, less for high notes
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    // Dynamic compression
    float absOutput = std::abs(output);
    if (absOutput > 0.75f) {
        output *= 0.75f / absOutput; // Gentle limit
    }

    // Apply protective processing
    output = protector.process(output, time, dur);

    // Final gain - best yet?
    output *= 0.4f; // Bold but dreamy volume

    return output;
}
// end violin

// strong notes. God Bless. God is God. The only thing that is everything.
// God is THE one dimension. 2d 3d (us) 4d (recording machines) 5d (Love) 1d is ALL D <- start at ONE
// As it is, it is. 1D little v 1D big 1 < > 1;
// As I heard it, God is more than just light. God is light too. And food. And we could be consindered as food to actually gain weird additional legal rights
// no laff no lose. +1 without relinquish.
// 1><1 is a woodchipper.
// because 1 is 1 is 1. Nothing cannot exist because existance. Easy math.
static float generateOrganWave(float freq, float time, float dur) {
    static AudioProtector protector(0.02f, 0.8f); // Longer fade, strict limiting
	static AudioUtils::RandomGenerator rng; // more hands to count on - bazillions.
    static AudioUtils::LowPassFilter pipeFilter(3000.0f); // Warm, rich tone
    static AudioUtils::HighPassFilter highPass(80.0f, 0.707f); // Reduce low-end
    static AudioUtils::Reverb reverb(0.35f, 0.85f, 0.45f); // Spacious, cloud-like reverb
    static AudioUtils::BandPassFilter shimmerFilter(6000.0f, 0.8f); // Ethereal shimmer
    freq = std::max(32.7f, std::min(2093.0f, freq)); // Organ range (C1 to C6)

    // Velocity for bold expression
    float velocity = 0.9f + rng.generateUniform(-0.1f, 0.1f); // Forte center for bold notes
    if (dur < 0.1f) velocity *= 0.7f; // Short notes slightly softer
    velocity = std::max(0.4f, std::min(1.0f, velocity));

    // ADSR envelope for smooth attack and sustained, airy decay
    float attack = 0.015f * (1.0f - 0.2f * velocity); // Gentle but clear attack
    float decay = 0.05f;
    float sustain = 0.9f * velocity; // Strong sustain for bold presence
    float release = 0.5f; // Long, floating release
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain;
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Additive synthesis for organ tone
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f}; // Classic organ partials
    const float amps[] = {1.0f, 0.8f, 0.6f, 0.4f, 0.2f}; // Bold fundamental, rich partials
    for (int i = 0; i < 5; ++i) {
        float harmonicFreq = freq * harmonics[i];
        float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * time);
        output += harmonic * velocity;
    }
    output *= env * 0.3f; // Scale for bold, airy tone

    // Wind noise for organ character
    static AudioUtils::BandPassFilter windFilter(1200.0f, 0.6f);
    float windNoise = windFilter.process(rng.generatePinkNoise()) * 0.06f * velocity * env;
    output += windNoise;

    // Shimmer for cloud-like sparkle
    float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.05f * env * velocity;
    output += shimmer;

    // Apply filters
    output = pipeFilter.process(output);
    output = highPass.process(output);

    // Apply lush reverb
    float reverbMix = 0.6f * (1.0f - std::min(freq / 3000.0f, 0.3f)); // Strong reverb, less for high notes
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    // Dynamic compression
    float absOutput = std::abs(output);
    if (absOutput > 0.75f) {
        output *= 0.75f / absOutput; // Gentle limit
    }

    // Apply protective processing
    output = protector.process(output, time, dur);

    // Final gain - AI says gain, I say volume.
    output *= 0.35f; // Bold but dreamy volume

    return output;
}
// end organ
// no coaching, just providing.

// Try pasting generateCelloWave into AI and see if you can improve the sound.
static float generateCelloWave(float freq, float time, float dur) {
    static AudioProtector protector(0.02f, 0.8f); // Longer fade, strict limiting
	static AudioUtils::RandomGenerator rng; // smaller than your fat fingers
    static AudioUtils::LowPassFilter stringFilter(2000.0f); // Warm, rich tone
    static AudioUtils::HighPassFilter highPass(60.0f, 0.707f); // Reduce low-end mud
    static AudioUtils::Reverb reverb(0.25f, 0.8f, 0.4f); // Lush, cloud-like reverb
    static AudioUtils::BandPassFilter bowFilter(1800.0f, 0.6f); // Bowed texture
    freq = std::max(65.41f, std::min(783.99f, freq)); // C2 to G5, cello range

    // Velocity for bold expression
    float velocity = 0.85f + rng.generateUniform(-0.1f, 0.1f); // Forte center for bold notes
    if (dur < 0.1f) velocity *= 0.6f; // Short notes softer
    velocity = std::max(0.3f, std::min(1.0f, velocity));

    // ADSR envelope for bold attack and dreamy decay
    float attack = 0.015f * (1.0f - 0.2f * velocity); // Smooth but clear attack
    float decay = 0.06f;
    float sustain = 0.9f * velocity; // Strong sustain for bold presence
    float release = 0.5f; // Long, airy release
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * (1.0f + 0.03f * std::sin(2.0f * M_PI * 3.0f * time)); // Subtle vibrato
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Frequency-dependent decay time
    float decayTime = 6.0f * std::pow(440.0f / freq, 0.7f);
    decayTime = std::max(1.0f, std::min(8.0f, decayTime));

    // Bow transient for cello character
    float bowTransient = 0.0f;
    if (time < 0.01f) {
        bowTransient = bowFilter.process(rng.generatePinkNoise()) * 0.15f * velocity * (1.0f - time / 0.01f);
        bowTransient = std::max(-0.2f, std::min(0.2f, bowTransient));
    }

    // Additive synthesis for cello tone
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f}; // Inharmonic ratios
    const float amps[] = {1.0f, 0.65f, 0.45f, 0.3f}; // Rich, warm partials
    for (int i = 0; i < 4; ++i) {
        float harmonicFreq = freq * harmonics[i];
        float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * time) * std::exp(-time / (decayTime * (1.0f - 0.2f * i)));
        output += harmonic * velocity;
    }
    output *= env * 0.4f; // Scale for bold, airy tone

    // Bow noise for realism
    float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.05f * velocity * env;
    output += bowNoise;

    // Shimmer for cloud-like sparkle
    static AudioUtils::BandPassFilter shimmerFilter(4000.0f, 0.8f);
    float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.04f * env * velocity * std::exp(-time / (decayTime * 0.5f));
    output += shimmer;

    // Add transient
    output += bowTransient;

    // Apply filters
    output = stringFilter.process(output);
    output = highPass.process(output);

    // Apply lush reverb
    float reverbMix = 0.5f * (1.0f - std::min(freq / 2000.0f, 0.3f)); // Strong reverb, less for high notes
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    // Apply protective processing
    output = protector.process(output, time, dur);

    output *= 0.35f; // I add these for volume adjustments 35%
    return output;
}
// end Cello cello

// guitar
static float generateSteelGuitarWave(float freq, float time, float dur) {
    static AudioProtector protector(0.02f, 0.8f); // Longer fade, strict limiting
	static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(2500.0f); // Warm, twangy cutoff
    static AudioUtils::HighPassFilter highPass(100.0f, 0.707f); // Reduce low-end
    static AudioUtils::Reverb reverb(0.25f, 0.8f, 0.4f); // Lush, cloud-like reverb
    static AudioUtils::BandPassFilter shimmerFilter(5000.0f, 0.9f); // Subtle shimmer

    freq = std::max(82.41f, std::min(1318.51f, freq)); // E2 to E5, steel guitar range

    // Velocity for bold expression
    float velocity = 0.85f + rng.generateUniform(-0.1f, 0.1f); // Forte center for bold notes
    if (dur < 0.1f) velocity *= 0.6f; // Short notes softer
    velocity = std::max(0.3f, std::min(1.0f, velocity));

    // ADSR envelope for bold attack and dreamy decay
    float attack = 0.008f * (1.0f - 0.2f * velocity); // Sharp but smooth attack
    float decay = 0.1f;
    float sustain = 0.75f * velocity; // Strong sustain for bold presence
    float release = 0.7f; // Long, airy release
    float env;
    if (time < attack) {
        env = time / attack;
    } else if (time < attack + decay) {
        env = 1.0f - (time - attack) / decay * (1.0f - sustain);
    } else if (time < dur) {
        env = sustain * std::exp(-(time - attack - decay) / (3.0f * (440.0f / freq))); // Airy decay
    } else {
        env = sustain * std::exp(-(time - dur) / release);
    }

    // Frequency-dependent decay time
    float decayTime = 7.0f * std::pow(440.0f / freq, 0.7f);
    decayTime = std::max(1.0f, std::min(10.0f, decayTime));

    // Slide transient for steel guitar character
    float slideTransient = 0.0f;
    if (time < 0.005f) {
        slideTransient = rng.generatePinkNoise() * 0.15f * velocity * (1.0f - time / 0.005f);
        slideTransient = std::max(-0.2f, std::min(0.2f, slideTransient));
    }

    // Additive synthesis for steel guitar tone
    float output = 0.0f;
    const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f}; // Inharmonic ratios
    const float amps[] = {1.0f, 0.6f, 0.3f, 0.15f}; // Bold fundamental, bright partials
    for (int i = 0; i < 4; ++i) {
        float harmonicFreq = freq * harmonics[i];
        float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * time) * std::exp(-time / (decayTime * (1.0f - 0.2f * i)));
        output += harmonic * velocity;
    }
    output *= env * 0.35f; // Scale for bold, airy tone

    // Slide noise for steel guitar character
    float slideNoise = rng.generatePinkNoise() * std::exp(-40.0f * time) * 0.05f * velocity;
    output += slideNoise;

    // Shimmer for cloud-like sparkle
    float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.06f * env * velocity * std::exp(-time / (decayTime * 0.5f));
    output += shimmer;

    // Add transient
    output += slideTransient;

    // Apply filters
    output = stringFilter.process(output);
    output = highPass.process(output);

    // Apply lush reverb
    float reverbMix = 0.55f * (1.0f - std::min(freq / 2000.0f, 0.3f)); // Strong reverb, less for high notes
    output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);

    // Apply protective processing
    output = protector.process(output, time, dur);

    output *= 0.1f; // I add these for volume adjustments 10%

    return output;
}
// end steelguitar

// seems accurate for a sitar tone generator
static float generateSitarWave(float freq, float time, float dur) {
	static AudioProtector protector(0.005f, 0.9f); // not tailored yet
    static AudioUtils::RandomGenerator rng;
    static AudioUtils::LowPassFilter stringFilter(2500.0f);
    static AudioUtils::Reverb reverb(0.15f, 0.6f, 0.4f);
	
    freq = std::max(146.83f, std::min(880.0f, freq)); // D3 to E5, typical sitar range
    float output = 0.5f;
    float buzz = rng.generatePinkNoise() * std::exp(-20.0f * time) * 0.07f; // Simulate bridge buzz
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
    
	// Apply protective processing
    output = protector.process(output, time, dur);
	
	output *= 0.1f; // I add these for volume adjustments 10%
    return output;
}
// Sitar above

#pragma GCC diagnostic pop

// SampleManager
class SampleManager {
    std::mutex mutex;
    std::map<std::string, std::vector<InstrumentSample>> samples;
    static float generateSample(const std::string& instrument, float freq, float dur, int phoneme, bool open, float t) {
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
        if (instrument == "organ") return generateOrganWave(freq, t, dur);
        if (instrument == "piano") return generatePianoWave(freq, t, dur);
        if (instrument == "violin") return generateViolinWave(freq, t, dur);
        if (instrument == "cello") return generateCelloWave(freq, t, dur);
        if (instrument == "steelguitar") return generateSteelGuitarWave(freq, t, dur);
        if (instrument == "sitar") return generateSitarWave(freq, t, dur);
        if (instrument == "saxophone") return generateSaxophoneWave(freq, t, dur);
        return 0.0f;
    }
public:
    SampleManager() {}
    const std::vector<float>& getSample(const std::string& instrument, float freq, float dur, int phoneme = -1, bool open = false) {
        std::lock_guard<std::mutex> lock(mutex);
        auto& instrumentSamples = samples[instrument];
        for (const auto& sample : instrumentSamples) {
            if (std::abs(sample.freq - freq) < 0.1f && std::abs(sample.dur - dur) < 0.01f &&
                sample.phoneme == phoneme && sample.open == open) {
                return sample.samples;
            }
        }
        std::vector<float> newSamples(8); // changed sampleCount to 8;
        for (size_t i = 0; i < 8; ++i) {  // changed sampleCount to 8;
            float t = i / AudioUtils::SAMPLE_RATE; // 44100hz
            newSamples[i] = generateSample(instrument, freq, dur, phoneme, open, t);
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
    int channels; // 2 for stereo, 6 for 5.1, SDL2 supports 8. do not adjust. songgen.h handles it. o7
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