// instruments/sitar.h
// sitar instrument
// creates a sitar note with
// float t time
// float freq frequency
// float dur duration
// accurate for a sitar tone generator with enhanced resonance
#ifndef SITAR_H
#define SITAR_H

// Sound tuned for authentic sitar with buzzing strings and sympathetic resonance
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Sitar : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.2f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Sitar(float gain = 0.2f)
        : protector(0.005f, 0.9f),
          rng(),
          stringFilter(2500.0f),
          reverb(0.15f, 0.6f, 0.4f),
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain to sitar range (D3 to E5)
        freq = std::max(146.83f, std::min(880.0f, freq));

        // Envelope: plucked string with sustain
        float attack = 0.008f, decay = 0.15f, sustain = 0.8f, release = 0.6f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain * (1.0f + 0.03f * std::sin(2.0f * M_PI * 5.0f * t));
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: subtle 4 Hz, ±0.3% after 0.1s
        float vibrato = t > 0.1f ? 0.003f * std::sin(2.0f * M_PI * 4.0f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Harmonics: rich, buzzing tone
        float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.7f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.5f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t);
        float harmonic4 = 0.3f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t);
        float sympathetic = 0.3f * std::sin(2.0f * M_PI * modulatedFreq * 1.5f * t);

        // Buzz: string vibration
        float buzz = rng.generatePinkNoise() * std::exp(-20.0f * t) * 0.08f;

        // Combine
        float output = (harmonic1 + harmonic2 + harmonic3 + harmonic4 + sympathetic) * 0.6f + buzz;
        output *= env;

        // Apply effects
        output = stringFilter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SITAR_H