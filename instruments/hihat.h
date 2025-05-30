// instruments/hihat.h
// hi-hat instrument
// creates a hi-hat note with
// float t time
// float freq frequency
// float dur duration
#ifndef HIHAT_H
#define HIHAT_H

// Sound tuned for crisp, metallic, realistic hi-hat with sharp attack and variable decay
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class HiHat {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::HighPassFilter filter; // High-pass for metallic clarity
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    HiHat(float gain = 1.0f)
        : protector(0.002f, 0.9f), // Fast fade, high gain limit
          reverb(0.03f, 0.2f, 0.1f), // Minimal room ambiance
          filter(2000.0f, 0.707f), // Emphasize high frequencies
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // Envelope: short for closed, longer for open hi-hat
        float decay = dur < 0.1f ? 0.05f : 0.3f; // Closed: 50ms, Open: 300ms
        float attack = 0.002f, sustain = 0.0f, release = 0.01f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Base waveform: white noise for metallic texture
        float noise = 0.8f * rng.generateWhiteNoise();

        // Tonal resonance: subtle sine wave at 300 Hz
        float sine = 0.1f * std::sin(2.0f * M_PI * 300.0f * t);

        // Attack emphasis: extra noise burst for first 2ms
        float attackNoise = t < 0.002f ? 0.3f * rng.generateWhiteNoise() * (1.0f - t / 0.002f) : 0.0f;

        // Combine and apply envelope
        float output = env * (noise + sine + attackNoise);

        // Apply effects
        output = reverb.process(output);
        output = filter.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // HIHAT_H