// instruments/bass.h
// bass instrument
// creates a bass note with
// float t time
// float freq frequency
// float dur duration
#ifndef BASS_H
#define BASS_H

// Sound tuned for deep, punchy, rich electric bass with harmonic warmth and subtle vibrato
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Bass : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter filter;
    AudioUtils::Distortion distortion; // Added for subtle grit
    float gain; // 0.8f for balanced volume

public:
    Bass(float gain = 0.8f)
        : protector(0.01f, 0.85f), // Slightly longer fade, moderate gain limit
          rng(),
          filter(200.0f), // Warm low-end focus
          distortion(1.5f, 0.8f), // Light distortion for character
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain frequency to typical bass range
        freq = std::max(40.0f, std::min(200.0f, freq));

        // Dynamic velocity with random variation
        float velocity = 0.8f + rng.generateUniform(-0.15f, 0.15f);
        velocity = std::max(0.3f, std::min(1.0f, velocity));

        // ADSR envelope for punchy attack and smooth sustain
        float attack = 0.003f, decay = 0.08f, sustain = 0.7f, release = 0.15f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: subtle 4 Hz modulation after 0.05s
        float vibrato = t > 0.05f ? 0.003f * std::sin(2.0f * M_PI * 4.0f * t) : 0.0f;
        float pitchMod = freq * (1.0f + vibrato);

        // Waveforms: sine for warmth, sawtooth for harmonics
        float sine = 0.6f * std::sin(2.0f * M_PI * pitchMod * t);
        float saw = 0.3f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);
        float harmonic = 0.2f * std::sin(2.0f * M_PI * 2.0f * pitchMod * t) * std::exp(-t / 0.3f);

        // Combine waveforms
        float output = env * velocity * (sine + saw + harmonic);

        // Apply effects
        output = distortion.process(output); // Add grit
        output = filter.process(output); // Smooth high-end
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // BASS_H