// instruments/snare.h
// snare drum instrument
// creates a snare drum note with
// float t time
// float freq frequency
// float dur duration
#ifndef SNARE_H
#define SNARE_H

// Sound tuned for crisp, punchy, realistic snare drum with sharp attack and rattling snares
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Snare : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter filter; // Band-pass for snare rattle
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    Snare(float gain = 1.0f)
        : protector(0.002f, 0.9f), // Fast fade, high gain limit
          reverb(0.04f, 0.3f, 0.15f), // Subtle room ambiance
          filter(2000.0f, 1.0f), // Center at 2 kHz for snare rattle
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Envelope: tight for standard hit, slightly longer for looser sound
        float decay = dur < 0.15f ? 0.1f : 0.2f; // Tight: 100ms, Loose: 200ms
        float attack = 0.001f, sustain = 0.0f, release = 0.02f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Body: sine wave for drumhead resonance
        float sine = 0.4f * std::sin(2.0f * M_PI * 180.0f * t); // Fixed 180 Hz for body

        // Attack: white noise burst for stick impact
        float attackNoise = t < 0.005f ? 0.5f * rng.generateWhiteNoise() * (1.0f - t / 0.005f) : 0.0f;

        // Snares: band-pass filtered white noise for rattle
        float snareNoise = 0.6f * rng.generateWhiteNoise();
        snareNoise = filter.process(snareNoise) * std::exp(-5.0f * t); // Decay for rattle

        // Combine and apply envelope
        float output = env * (sine + attackNoise + snareNoise);

        // Apply effects
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SNARE_H