// instruments/piano.h
// piano instrument
// creates a piano note with
// float t time
// float freq frequency
// float dur duration
#ifndef PIANO_H
#define PIANO_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Piano {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.5f is 50% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Piano(float gain = 0.5f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.01f, 0.85f),
          stringFilter(4500.0f),
          reverb(0.12f, 0.65f, 0.35f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(27.5f, std::min(4186.0f, freq)); // Piano range (A0 to C8)
        float velocity = 0.7f + rng.generateUniform(-0.2f, 0.2f);
        if (dur < 0.1f) velocity *= 0.6f;
        velocity = std::max(0.2f, std::min(1.0f, velocity));
        bool sustainPedal = (dur > 1.5f || t > 3.0f);
        float attack = 0.001f * (1.0f - 0.4f * velocity);
        float decay = 0.05f, sustain = 0.7f * velocity, release = 0.3f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur || sustainPedal) {
            env = sustain * std::exp(-(t - attack - decay) / (2.0f * (440.0f / freq)));
        } else {
            env = sustain * std::exp(-(t - dur) / release);
        }
        float decayTime = 6.0f * std::pow(440.0f / freq, 0.7f);
        decayTime = std::max(0.5f, std::min(8.0f, decayTime));
        if (sustainPedal) decayTime *= 1.5f;
        float transient = 0.0f;
        if (t < 0.002f) {
            transient = rng.generateWhiteNoise() * 0.25f * velocity * (1.0f - t / 0.002f);
            transient = std::max(-0.3f, std::min(0.3f, transient));
        }
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.01f, 3.03f, 4.05f, 5.08f};
        const float amps[] = {1.0f, 0.6f, 0.3f, 0.15f, 0.08f};
        const float decays[] = {1.0f, 0.8f, 0.6f, 0.4f, 0.3f};
        for (int i = 0; i < 5; ++i) {
            float harmonicFreq = freq * harmonics[i];
            float harmonicDecay = decayTime * decays[i] * (440.0f / freq);
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / harmonicDecay);
            output += harmonic * velocity;
        }
        output *= env * 0.3f;
        if (sustainPedal) {
            float resFreq1 = freq * 2.0f;
            float resFreq2 = freq * 1.5f;
            if (resFreq1 <= 4186.0f) {
                output += 0.05f * std::cos(2.0f * M_PI * resFreq1 * t) * env * velocity * std::exp(-t / (decayTime * 0.8f));
            }
            if (resFreq2 <= 4186.0f) {
                output += 0.03f * std::cos(2.0f * M_PI * resFreq2 * t) * env * velocity * std::exp(-t / (decayTime * 0.8f));
            }
        }
        output += transient;
        output = stringFilter.process(output);
        float reverbMix = 0.35f * (1.0f - std::min(freq / 4000.0f, 0.5f));
        output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);
        if (std::abs(output) > 0.8f) output *= 0.8f / std::abs(output);
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // PIANO_H
