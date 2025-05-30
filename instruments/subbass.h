// instruments/subbass.h
// sub-bass instrument
// creates a sub-bass note with
// float t time
// float freq frequency
// float dur duration
#ifndef SUBBASS_H
#define SUBBASS_H

// Sound tuned for deep, clean, realistic sub-bass optimized for subwoofer down to 20 Hz
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class SubBass {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter; // Low-pass to focus on sub-frequencies
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    SubBass(float gain = 1.0f)
        : protector(0.01f, 0.95f), // Smooth fade, high gain limit
          reverb(0.02f, 0.2f, 0.05f), // Minimal ambiance for clarity
          filter(100.0f), // Focus on sub-100 Hz
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // Envelope: smooth for sustained basslines, shorter for percussive hits
        float attack = 0.02f, decay = 0.2f, sustain = 0.8f, release = 0.3f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Subtle pitch modulation: ±0.2% for analog feel
        float mod = 0.002f * std::sin(2.0f * M_PI * 0.5f * t); // 0.5 Hz LFO
        float pitchMod = freq * (1.0f + mod);

        // Fundamental: sine wave for clean sub-bass
        float sine = 0.7f * std::sin(2.0f * M_PI * pitchMod * t);

        // Harmonics: triangle wave at 2x freq for warmth
        float triangle = 0.2f * (2.0f / M_PI * std::asin(std::sin(2.0f * M_PI * pitchMod * 2.0f * t)));

        // Combine and apply envelope
        float output = env * (sine + triangle);

        // Apply effects
        output = filter.process(output); // Keep sub-frequencies clean
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SUBBASS_H