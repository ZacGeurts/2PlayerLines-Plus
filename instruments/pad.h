// instruments/pad.h
// pad instrument
// creates a pad note with
// float t time
// float freq frequency
// float dur duration
#ifndef PAD_H
#define PAD_H

// Sound tuned for lush, ambient synth pad with warm, evolving texture
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Pad : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter filter;
    AudioUtils::Reverb reverb;
    float gain; // 0.4f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Pad(float gain = 0.4f)
        : protector(0.1f, 0.9f),
          rng(),
          filter(800.0f),
          reverb(0.8f, 0.8f, 0.6f),
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        freq = std::max(32.7f, std::min(2093.0f, freq));

        // Envelope: slow attack for ambient swells
        float attack = 0.8f, decay = 0.3f, sustain = 0.7f, release = 1.2f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Waveforms: detuned sawtooths for warmth
        float detune1 = 1.005f, detune2 = 0.995f;
        float saw1 = (std::fmod(freq * t, 1.0f) - 0.5f);
        float saw2 = (std::fmod(freq * detune1 * t, 1.0f) - 0.5f) * 0.7f;
        float saw3 = (std::fmod(freq * detune2 * t, 1.0f) - 0.5f) * 0.7f;

        // Filter modulation: slow LFO
        float filterMod = 800.0f + 200.0f * std::sin(2.0f * M_PI * 0.2f * t);
        filter.setCutoff(filterMod);

        // Combine with subtle noise
        float output = (saw1 + saw2 + saw3) * 0.4f;
        float noise = rng.generatePinkNoise() * 0.03f;
        output += noise;

        // Apply effects
        output = filter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= env * gain;
        return output;
    }
};

} // namespace Instruments

#endif // PAD_H