// instruments/cello.h
// cello instrument
// creates a cello note with
// float t time
// float freq frequency
// float dur duration
#ifndef CELLO_H
#define CELLO_H

// Sound tuned for rich, warm, expressive cello with realistic bowing and vibrato
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Cello {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter;
    AudioUtils::RandomGenerator rng;
    float gain; // 1.0f is 100% volume

public:
    Cello(float gain = 1.0f)
        : protector(0.005f, 0.9f), // Gentle fade, high gain limit
          reverb(0.1f, 0.5f, 0.25f), // Concert hall ambiance
          filter(2000.0f), // Warm, open tone
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        // ADSR envelope for smooth bowing dynamics
        float attack = 0.05f, decay = 0.1f, sustain = 0.8f, release = 0.2f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 5 Hz, ±0.5% pitch modulation after 0.1s
        float vibrato = t > 0.1f ? 0.005f * std::sin(2.0f * M_PI * 5.0f * t) : 0.0f;
        float pitchMod = freq * (1.0f + vibrato);

        // Base waveform: sawtooth for harmonics, sine for warmth
        float saw = 0.6f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);
        float sine = 0.3f * std::sin(2.0f * M_PI * pitchMod * t);

        // Bow friction: subtle pink noise
        float noise = 0.05f * rng.generatePinkNoise() * std::exp(-5.0f * t / dur);

        // Combine and apply envelope
        float output = env * (saw + sine + noise);

        // Apply effects
        output = reverb.process(output);
        output = filter.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // CELLO_H