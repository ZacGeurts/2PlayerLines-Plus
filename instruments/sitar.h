// instruments/sitar.h
// sitar instrument
// creates a sitar note with
// float t time
// float freq frequency
// float dur duration
// seems accurate for a sitar tone generator
#ifndef SITAR_H
#define SITAR_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Sitar {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.1f is 10% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Sitar(float gain = 0.1f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          stringFilter(2500.0f),
          reverb(0.15f, 0.6f, 0.4f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(146.83f, std::min(880.0f, freq)); // D3 to E5, typical sitar range
        float output = 0.5f;
        float buzz = rng.generatePinkNoise() * std::exp(-20.0f * t) * 0.07f;
        float attack = 0.008f, decay = 0.15f, sustain = 0.8f, release = 0.6f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur) {
            env = sustain * (1.0f + 0.03f * std::sin(2.0f * M_PI * 5.0f * t));
        } else if (t < dur + release) {
            env = sustain * std::exp(-(t - dur) / release);
        } else {
            env = 0.0f;
        }
        float harmonic1 = 1.0f * std::cos(2.0f * M_PI * freq * t) * env;
        float harmonic2 = 0.7f * std::cos(2.0f * M_PI * 2.0f * freq * t) * env;
        float harmonic3 = 0.5f * std::cos(2.0f * M_PI * 3.0f * freq * t) * env;
        float harmonic4 = 0.3f * std::cos(2.0f * M_PI * 5.0f * freq * t) * env;
        float sympathetic = 0.2f * std::sin(2.0f * M_PI * freq * 1.5f * t) * env;
        output += (harmonic1 + harmonic2 + harmonic3 + harmonic4 + sympathetic) * 0.6f;
        output = (output + buzz) * env;
        output = reverb.process(output);
        output = std::max(-1.0f, std::min(1.0f, output));
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SITAR_H
