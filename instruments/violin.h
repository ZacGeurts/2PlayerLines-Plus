// instruments/violin.h
// violin instrument
// creates a violin note with
// float t time
// float freq frequency
// float dur duration
// untested
#ifndef VIOLIN_H
#define VIOLIN_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Violin {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter bowFilter;
    AudioUtils::BandPassFilter shimmerFilter;
    float gain; // 0.4f is 40% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Violin(float gain = 0.4f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.02f, 0.8f),
          stringFilter(2500.0f),
          highPass(80.0f, 0.707f),
          reverb(0.3f, 0.85f, 0.45f),
          bowFilter(2500.0f, 0.5f),
          shimmerFilter(5000.0f, 0.8f),
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(196.0f, std::min(3520.0f, freq)); // G3 to G7, violin range
        float velocity = 0.9f + rng.generateUniform(-0.1f, 0.1f);
        if (dur < 0.1f) velocity *= 0.6f;
        velocity = std::max(0.3f, std::min(1.0f, velocity));
        float attack = 0.02f * (1.0f - 0.2f * velocity);
        float decay = 0.05f, sustain = 0.95f * velocity, release = 0.6f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur) {
            env = sustain * (1.0f + 0.02f * std::sin(2.0f * M_PI * 4.0f * t));
        } else {
            env = sustain * std::exp(-(t - dur) / release);
        }
        float decayTime = 5.0f * std::pow(440.0f / freq, 0.6f);
        decayTime = std::max(0.8f, std::min(6.0f, decayTime));
        float bowTransient = 0.0f;
        if (t < 0.015f) {
            bowTransient = bowFilter.process(rng.generatePinkNoise()) * 0.1f * velocity * (1.0f - t / 0.015f);
            bowTransient = std::max(-0.15f, std::min(0.15f, bowTransient));
        }
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f};
        const float amps[] = {1.0f, 0.7f, 0.5f, 0.3f};
        float glide = (t < 0.05f) ? 1.0f + 0.01f * (1.0f - t / 0.05f) : 1.0f;
        for (int i = 0; i < 4; ++i) {
            float harmonicFreq = freq * harmonics[i] * glide;
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / (decayTime * (1.0f - 0.2f * i)));
            output += harmonic * velocity;
        }
        output *= env * 0.35f;
        float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.06f * velocity * env;
        output += bowNoise;
        float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.04f * env * velocity * std::exp(-t / (decayTime * 0.5f));
        output += shimmer;
        output += bowTransient;
        output = stringFilter.process(output);
        output = highPass.process(output);
        float reverbMix = 0.55f * (1.0f - std::min(freq / 3000.0f, 0.3f));
        output = reverb.process(output) * reverbMix + output * (1.0f - reverbMix);
        if (std::abs(output) > 0.75f) output *= 0.75f / output;
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // VIOLIN_H
