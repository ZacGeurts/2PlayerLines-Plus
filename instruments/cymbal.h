// instruments/cymbal.h
// cymbal instrument
// creates a cymbal note with
// float t time
// float freq frequency
// float dur duration
#ifndef CYMBAL_H
#define CYMBAL_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Cymbal {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::HighPassFilter hpFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.4f is 40% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Cymbal(float gain = 0.4f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.008f, 0.85f),
          hpFilter(500.0f, 0.707f),
          reverb(0.1f, 0.5f, 0.35f),
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) {
        dur = std::clamp(dur, 0.1f, 1.5f);
        freq = (freq > 0.0f) ? std::clamp(freq, 2000.0f, 10000.0f) : 6000.0f;
        float env = std::exp(-5.0f * t / dur) * (1.0f + 0.3f * std::sin(6.0f * M_PI * t / dur));
        env = std::max(0.0f, env);
        float whiteNoise = rng.generateWhiteNoise() * 0.6f;
        float pinkNoise = rng.generatePinkNoise() * 0.4f;
        float pitchBend = 1.0f + 0.005f * std::sin(2.0f * M_PI * 0.5f * t);
        float metallic1 = std::sin(2.0f * M_PI * freq * pitchBend * t) * 0.25f * std::exp(-3.5f * t / dur);
        float metallic2 = std::sin(2.0f * M_PI * (freq * 1.5f) * pitchBend * t) * 0.2f * std::exp(-4.5f * t / dur);
        float metallic3 = std::sin(2.0f * M_PI * (freq * 2.0f) * pitchBend * t) * 0.15f * std::exp(-5.5f * t / dur);
        float filterMod = 0.6f + 0.4f * std::exp(-4.0f * t / dur);
        float noise = (whiteNoise + pinkNoise) * filterMod;
        float output = env * (0.7f * noise + 0.3f * (metallic1 + metallic2 + metallic3));
        output = hpFilter.process(output);
        output = reverb.process(output);
        if (std::abs(output) > 0.8f) output *= 0.8f / std::abs(output);
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // CYMBAL_H
