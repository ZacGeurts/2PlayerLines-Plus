// instruments/syntharp.h
// synth arpeggio instrument
// creates a synth arpeggio note with
// float t time
// float freq frequency
// float dur duration
#ifndef SYNTHARP_H
#define SYNTHARP_H

// Sound tuned for bright, melodic synth arpeggio with crisp attack and balanced presence
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class SynthArp : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter; // Low-pass for tonal shaping
    AudioUtils::RandomGenerator rng;
    float gain; // 0.6f for balanced volume

public:
    SynthArp(float gain = 0.6f)
        : protector(0.005f, 0.9f), // Smooth fade, high gain limit
          reverb(0.1f, 0.5f, 0.2f), // Subtle ambiance
          filter(1500.0f), // Warm, clear tone
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Envelope: crisp for arpeggios
        float attack = 0.005f, decay = 0.1f, sustain = 0.5f, release = 0.15f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 6 Hz, ±0.5% after 0.05s
        float vibrato = t > 0.05f ? 0.005f * std::sin(2.0f * M_PI * 6.0f * t) : 0.0f;
        float pitchMod = freq * (1.0f + vibrato);

        // Waveforms: sawtooth and square for brightness
        float saw = 0.5f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);
        float square = 0.3f * (std::sin(2.0f * M_PI * pitchMod * t) > 0.0f ? 1.0f : -1.0f);
        float detune = 0.2f * (std::fmod((pitchMod * 1.02f) * t, 1.0f) - 0.5f);

        // Combine and apply envelope
        float output = env * (saw + square + detune);

        // Apply effects
        output = filter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SYNTHARP_H