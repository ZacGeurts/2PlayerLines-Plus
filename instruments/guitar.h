// instruments/guitar.h
// guitar instrument
// creates a guitar note with
// float t time
// float freq frequency
// float dur duration
#ifndef GUITAR_H
#define GUITAR_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Guitar {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter bodyResonance;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::Distortion distortion;
    AudioUtils::BandPassFilter resonanceFilter;
    float gain; // 0.5f is 50% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Guitar(float gain = 0.5f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.015f, 0.85f),
          bodyResonance(1000.0f),
          highPass(80.0f, 0.707f),
          reverb(0.12f, 0.4f, 0.25f),
          distortion(1.5f, 0.7f),
          resonanceFilter(250.0f, 1.0f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::clamp(freq, 80.0f, 1000.0f);
        float velocity = 0.8f + rng.generateUniform(-0.2f, 0.2f);
        velocity = std::max(0.3f, std::min(1.0f, velocity));
        if (dur < 0.2f) velocity *= 0.7f;
        float attack = 0.005f * (1.0f - 0.3f * velocity);
        float decay = 0.1f, sustain = 0.3f * velocity, release = 0.3f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur) {
            env = sustain * std::exp(-2.0f * (t - attack - decay) / dur);
        } else {
            env = sustain * std::exp(-(t - dur) / release);
        }
        float decayTime = 3.0f * std::pow(440.0f / freq, 0.5f);
        decayTime = std::max(0.5f, std::min(3.0f, decayTime));
        float pluck = (t < 0.003f) ? rng.generateWhiteNoise() * 0.2f * velocity * (1.0f - t / 0.003f) : 0.0f;
        pluck = std::max(-0.25f, std::min(0.25f, pluck));
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.002f, 3.005f, 4.008f, 5.012f};
        const float amps[] = {1.0f, 0.8f, 0.5f, 0.3f, 0.15f};
        for (int i = 0; i < 5; ++i) {
            float harmonicFreq = freq * harmonics[i];
            float harmonic = amps[i] * std::sin(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / (decayTime * (1.0f - 0.15f * i)));
            output += harmonic * velocity;
        }
        output *= env * 0.3f;
        float fretNoise = rng.generatePinkNoise() * std::exp(-50.0f * t) * 0.015f * velocity;
        float resonance = resonanceFilter.process(rng.generatePinkNoise()) * 0.05f * env * velocity;
        output += pluck + fretNoise + resonance;
        output = bodyResonance.process(output);
        output = highPass.process(output);
        output = distortion.process(output);
        output = reverb.process(output);
        if (std::abs(output) > 0.8f) output *= 0.8f / std::abs(output);
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // GUITAR_H
