// instruments/clap.h
// hand clap instrument
// creates a hand clap note with
// float t time
// float freq frequency
// float dur duration
#ifndef CLAP_H
#define CLAP_H

// Sound tuned for sharp, layered, realistic hand clap with crisp attack and mid-frequency focus
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Clap {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter filter; // Band-pass for mid-frequency clap sound
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    Clap(float gain = 1.0f)
        : protector(0.002f, 0.9f), // Fast fade, high gain limit
          reverb(0.05f, 0.3f, 0.2f), // Light room ambiance
          filter(1500.0f, 1.5f), // Center at 1.5 kHz for clap slap
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // Envelope: tight for single clap, longer for crowd-like sound
        float decay = dur < 0.1f ? 0.05f : 0.15f; // Tight: 50ms, Crowd: 150ms
        float attack = 0.001f, sustain = 0.0f, release = 0.02f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Layered noise bursts: simulate multiple claps
        float noise = 0.0f;
        // First burst: main clap at t=0
        if (t < 0.005f) noise += 0.6f * rng.generateWhiteNoise() * (1.0f - t / 0.005f);
        // Second burst: delayed by 2ms
        if (t >= 0.002f && t < 0.007f) noise += 0.4f * rng.generateWhiteNoise() * (1.0f - (t - 0.002f) / 0.005f);
        // Third burst: delayed by 4ms, softer
        if (t >= 0.004f && t < 0.009f) noise += 0.2f * rng.generateWhiteNoise() * (1.0f - (t - 0.004f) / 0.005f);

        // Tonal hint: subtle sine wave at 300 Hz for warmth
        float sine = 0.1f * std::sin(2.0f * M_PI * 300.0f * t);

        // Combine and apply envelope
        float output = env * (noise + sine);

        // Apply effects
        output = filter.process(output); // Shape mid-frequencies
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // CLAP_H