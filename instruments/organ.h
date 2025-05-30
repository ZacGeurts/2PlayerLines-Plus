// instruments/organ.h
// organ instrument
// creates an organ note with
// float t time
// float freq frequency
// float dur duration
// strong notes. God Bless. God is God. The only thing that is everything.
// God is THE one dimension. 2d 3d (us) 4d (recording machines) 5d (Love) 1d is ALL D <- start at ONE
// As it is, it is. 1D little v 1D big 1 < > 1;
// As I heard it, God is more than just light. God is light too. And food. And we could be consindered as food to actually gain weird additional legal rights
// 1><1 is a woodchipper.
// because 1 is 1 is 1. Nothing cannot not exist because existance exists. Easy math.
#ifndef ORGAN_H
#define ORGAN_H

// Sound tuned for majestic, resonant pipe organ with rich harmonics and cathedral ambiance

#include "instruments.h"

namespace Instruments {

class Organ : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter pipeFilter;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter windFilter;
    AudioUtils::BandPassFilter shimmerFilter;
    float gain; // 0.8f for strong presence
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Organ(float gain = 0.8f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.02f, 0.8f),
          rng(),
          pipeFilter(3500.0f),
          highPass(80.0f, 0.707f),
          reverb(0.4f, 0.9f, 0.5f), // Cathedral-like ambiance
          windFilter(1200.0f, 0.6f),
          shimmerFilter(6000.0f, 0.8f),
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) override {
        freq = std::max(32.7f, std::min(2093.0f, freq)); // Organ range (C1 to C6)
        float velocity = 0.9f + rng.generateUniform(-0.1f, 0.1f);
        if (dur < 0.1f) velocity *= 0.7f;
        velocity = std::max(0.4f, std::min(1.0f, velocity));

        // Envelope: smooth for organ pipes
        float attack = 0.015f * (1.0f - 0.2f * velocity);
        float decay = 0.05f, sustain = 0.9f * velocity, release = 0.5f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Harmonics: rich, pipe organ stops
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 8.0f};
        const float amps[] = {1.0f, 0.8f, 0.6f, 0.4f, 0.3f, 0.2f};
        for (int i = 0; i < 6; ++i) {
            float harmonicFreq = freq * harmonics[i];
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t);
            output += harmonic * velocity;
        }
        output *= env * 0.25f;

        // Wind and shimmer for realism
        float windNoise = windFilter.process(rng.generatePinkNoise()) * 0.08f * velocity * env;
        float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.06f * env * velocity;
        output += windNoise + shimmer;

        // Apply effects
        output = pipeFilter.process(output);
        output = highPass.process(output);
        output = reverb.process(output); // Full reverb for grandeur
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // ORGAN_H