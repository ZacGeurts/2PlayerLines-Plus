// instruments/leadsynth.h
// lead synthesizer instrument
// creates a lead synth note with
// float t time
// float freq frequency
// float dur duration
#ifndef LEADSYNTH_H
#define LEADSYNTH_H

// Sound tuned for expressive lead synth with clear attack, rich harmonics, and balanced presence
// Sample rate removed; assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class LeadSynth : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter; // Low-pass with resonance for tonal shaping
    AudioUtils::RandomGenerator rng;
    float gain; // 0.6f for balanced volume

public:
    LeadSynth(float gain = 0.6f)
        : protector(0.005f, 0.9f), // Smooth fade, high gain limit
          reverb(0.12f, 0.6f, 0.25f), // Moderate ambiance
          filter(2000.0f), // Warm, clear tone
          rng(),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Envelope: sharp for lead clarity, sustained for melodies
        float attack = 0.003f, decay = 0.2f, sustain = 0.6f, release = 0.2f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 7 Hz, ±0.7% pitch modulation after 0.1s
        float vibrato = t > 0.1f ? 0.007f * std::sin(2.0f * M_PI * 7.0f * t) : 0.0f;
        float pitchMod = freq * (1.0f + vibrato);

        // Pulse-width modulation: 0.5 Hz LFO for dynamic timbre
        float pwm = 0.3f + 0.2f * std::sin(2.0f * M_PI * 0.5f * t);

        // Base waveform: sawtooth for harmonic richness
        float saw = 0.5f * (std::fmod(pitchMod * t, 1.0f) - 0.5f);

        // Secondary waveform: pulse wave with PWM
        float pulse = 0.4f * (std::fmod(pitchMod * t, 1.0f) < pwm ? 1.0f : -1.0f);

        // Subtle detuning: second sawtooth slightly offset
        float detune = 0.2f * (std::fmod((pitchMod * 1.015f) * t, 1.0f) - 0.5f);

        // Combine and apply envelope
        float output = env * (saw + pulse + detune);

        // Apply effects
        output = filter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // LEADSYNTH_H