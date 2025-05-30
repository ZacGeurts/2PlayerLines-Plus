// instruments/cymbal.h
// cymbal instrument
// creates a cymbal note with
// float t time
// float freq frequency
// float dur duration
#ifndef CYMBAL_H
#define CYMBAL_H

// Sound tuned for shimmering, resonant crash cymbal with rich harmonics and natural decay
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Cymbal : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::HighPassFilter hpFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.6f for balanced volume

public:
    Cymbal(float gain = 0.6f)
        : protector(0.006f, 0.9f), // Smooth fade, high gain limit
          rng(),
          hpFilter(600.0f, 0.707f), // Clear high-end
          reverb(0.15f, 0.6f, 0.4f), // Spacious ambiance
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain duration and frequency
        dur = std::clamp(dur, 0.2f, 2.0f);
        freq = (freq > 0.0f) ? std::clamp(freq, 3000.0f, 12000.0f) : 8000.0f;

        // Envelope: smooth decay with slight swell
        float env = std::exp(-4.0f * t / dur) * (1.0f + 0.4f * std::sin(8.0f * M_PI * t / dur));
        env = std::max(0.0f, env);

        // Noise: balanced white and pink for shimmer and body
        float whiteNoise = rng.generateWhiteNoise() * 0.5f;
        float pinkNoise = rng.generatePinkNoise() * 0.5f;
        float noise = (whiteNoise + pinkNoise) * (0.7f + 0.3f * std::exp(-3.0f * t / dur));

        // Metallic harmonics: inharmonic ratios for cymbal character
        float pitchBend = 1.0f + 0.003f * std::sin(2.0f * M_PI * 0.3f * t);
        float metallic1 = std::sin(2.0f * M_PI * freq * pitchBend * t) * 0.2f * std::exp(-2.5f * t / dur);
        float metallic2 = std::sin(2.0f * M_PI * (freq * 1.618f) * pitchBend * t) * 0.15f * std::exp(-3.0f * t / dur);
        float metallic3 = std::sin(2.0f * M_PI * (freq * 2.414f) * pitchBend * t) * 0.1f * std::exp(-3.5f * t / dur);

        // Combine
        float output = env * (0.6f * noise + 0.4f * (metallic1 + metallic2 + metallic3));

        // Apply effects
        output = hpFilter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // CYMBAL_H