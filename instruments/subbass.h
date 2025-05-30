// instruments/subbass.h
// sub-bass instrument
// creates a sub-bass note with
// float t time
// float freq frequency
// float dur duration
#ifndef SUBBASS_H
#define SUBBASS_H

// Sound tuned for deep, booming sub-bass optimized for subwoofer down to 20 Hz, like rap songs
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Subbass : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter; // Low-pass to focus on sub-frequencies
    AudioUtils::RandomGenerator rng;
    float gain; // 1.2f for strong presence

public:
    Subbass(float gain = 1.2f)
        : protector(0.01f, 0.95f), // Smooth fade, high gain limit
          reverb(0.02f, 0.2f, 0.05f), // Minimal ambiance for clarity
          filter(80.0f), // Deep sub focus
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain frequency to sub-bass range
        freq = std::max(20.0f, std::min(100.0f, freq));

        // Envelope: smooth for sustained basslines
        float attack = 0.03f, decay = 0.2f, sustain = 0.9f, release = 0.4f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Subtle pitch modulation: ±0.3% for analog feel
        float mod = 0.003f * std::sin(2.0f * M_PI * 0.5f * t); // 0.5 Hz LFO
        float pitchMod = freq * (1.0f + mod);

        // Waveforms: sine for clean sub, second sine for punch
        float sine1 = 0.8f * std::sin(2.0f * M_PI * pitchMod * t);
        float sine2 = 0.3f * std::sin(2.0f * M_PI * pitchMod * 1.5f * t) * std::exp(-t / 0.2f);

        // Combine and apply envelope
        float output = env * (sine1 + sine2);

        // Apply effects
        output = filter.process(output); // Deep sub focus
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SUBBASS_H