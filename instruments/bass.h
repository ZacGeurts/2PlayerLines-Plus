// instruments/bass.h
// bass instrument
// creates a bass note with
// float t time
// float freq frequency
// float dur duration
#ifndef BASS_H
#define BASS_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Bass {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter filter;
    float gain; // 0.6f is 60% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Bass(float gain = 0.6f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.015f, 0.8f),
          filter(150.0f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(40.0f, std::min(200.0f, freq));
        float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f);
        velocity = std::max(0.2f, std::min(1.0f, velocity));
        float attack = 0.005f, decay = 0.1f, sustain = 0.6f, release = 0.2f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur) {
            env = sustain;
        } else {
            env = sustain * std::exp(-(t - dur) / release);
        }
        float output = std::sin(2.0f * M_PI * freq * t) * env * velocity;
        output += 0.3f * std::sin(2.0f * M_PI * 2.0f * freq * t) * env * velocity * std::exp(-t / 0.5f);
        output = filter.process(output);
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // BASS_H
