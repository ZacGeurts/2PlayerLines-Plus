// instruments/kick.h
// kick drum instrument
// creates a kick drum note with
// float t time
// float freq frequency
// float dur duration
#ifndef KICK_H
#define KICK_H

// Sound tuned for deep, punchy, realistic kick drum with sharp attack and tight decay
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Kick {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter;
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    Kick(float gain = 1.0f)
        : protector(0.002f, 0.95f), // Fast fade, high gain limit
          reverb(0.05f, 0.3f, 0.1f), // Tight room ambiance
          filter(250.0f), // Warm, focused low-end
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // ADSR envelope for punchy dynamics
        float attack = 0.005f, decay = 0.15f, sustain = 0.0f, release = 0.05f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Pitch glide: slight downward modulation for natural kick
        float pitchMod = freq * (1.0f - 0.1f * std::min(t / 0.1f, 1.0f));

        // Base waveform: sine for deep thump
        float sine = 0.7f * std::sin(2.0f * M_PI * pitchMod * t);

        // Body: distorted sine wave at 128 Hz for harmonic richness
        float body = 0.2f * std::sin(2.0f * M_PI * 128.0f * t);
        body = std::tanh(3.0f * body) / 2.0f; // Add distortion for grit

        // Attack: high-pass filtered pink noise for beater click
        float noise = t < 0.02f ? 0.5f * rng.generatePinkNoise() * (1.0f - t / 0.02f) : 0.0f;

        // Combine and apply envelope
        float output = env * (sine + body + noise);

        // Apply effects
        output = reverb.process(output);
        output = filter.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // KICK_H