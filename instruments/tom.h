// instruments/tom.h
// tom drum instrument
// creates a tom drum note with
// float t time
// float freq frequency
// float dur duration
#ifndef TOM_H
#define TOM_H

// Sound tuned for resonant, punchy, realistic tom drum with clear attack and warm body
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Tom {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter; // Low-pass for warm, resonant tone
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    Tom(float gain = 1.0f)
        : protector(0.002f, 0.9f), // Fast fade, high gain limit
          reverb(0.06f, 0.4f, 0.15f), // Subtle room ambiance
          filter(500.0f), // Warm, focused tone
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // Envelope: tight for short hits, longer for resonant toms
        float decay = dur < 0.3f ? 0.2f : 0.4f; // Tight: 200ms, Resonant: 400ms
        float attack = 0.002f, sustain = 0.0f, release = 0.03f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Pitch glide: slight downward modulation for natural resonance
        float pitchMod = freq * (1.0f - 0.05f * std::min(t / 0.2f, 1.0f));

        // Body: sine wave for main drumhead pitch
        float sine = 0.6f * std::sin(2.0f * M_PI * pitchMod * t);

        // Harmonics: detuned sine wave for warmth
        float harmonic = 0.2f * std::sin(2.0f * M_PI * (pitchMod * 1.5f) * t);

        // Attack: high-pass filtered white noise for stick impact
        float noise = t < 0.005f ? 0.3f * rng.generateWhiteNoise() * (1.0f - t / 0.005f) : 0.0f;

        // Combine and apply envelope
        float output = env * (sine + harmonic + noise);

        // Apply effects
        output = reverb.process(output);
        output = filter.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // TOM_H