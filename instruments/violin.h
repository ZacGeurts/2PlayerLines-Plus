// instruments/violin.h
// violin instrument
// creates a violin note with
// float t time
// float freq frequency
// float dur duration
#ifndef VIOLIN_H
#define VIOLIN_H

// Sound tuned for highly realistic violin with expressive bowing and rich tone
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Violin : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter bowFilter;
    AudioUtils::BandPassFilter shimmerFilter;
    float gain; // 0.6f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Violin(float gain = 0.6f)
        : protector(0.02f, 0.8f),
          rng(),
          stringFilter(2500.0f),
          highPass(80.0f, 0.707f),
          reverb(0.3f, 0.85f, 0.45f), // Chamber music ambiance
          bowFilter(2500.0f, 0.5f),
          shimmerFilter(5000.0f, 0.8f),
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain to violin range (G3 to G7)
        freq = std::max(196.0f, std::min(3520.0f, freq));

        // Dynamic velocity
        float velocity = 0.9f + rng.generateUniform(-0.1f, 0.1f);
        if (dur < 0.1f) velocity *= 0.6f;
        velocity = std::max(0.3f, std::min(1.0f, velocity));

        // Envelope: expressive bowing
        float attack = 0.02f * (1.0f - 0.2f * velocity);
        float decay = 0.05f, sustain = 0.95f * velocity, release = 0.6f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain * (1.0f + 0.02f * std::sin(2.0f * M_PI * 4.0f * t));
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 5 Hz, ±0.5% after 0.2s
        float vibrato = t > 0.2f ? 0.005f * std::sin(2.0f * M_PI * 5.0f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Decay time
        float decayTime = 5.0f * std::pow(440.0f / freq, 0.6f);
        decayTime = std::max(0.8f, std::min(6.0f, decayTime));

        // Bow transient
        float bowTransient = t < 0.015f ? bowFilter.process(rng.generatePinkNoise()) * 0.15f * velocity * (1.0f - t / 0.015f) : 0.0f;

        // Harmonics: rich violin tone
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f, 5.05f};
        const float amps[] = {1.0f, 0.7f, 0.5f, 0.3f, 0.2f};
        float glide = t < 0.05f ? 1.0f + 0.01f * (1.0f - t / 0.05f) : 1.0f;
        for (int i = 0; i < 5; ++i) {
            float harmonicFreq = modulatedFreq * harmonics[i] * glide;
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / (decayTime * (1.0f - 0.2f * i)));
            output += harmonic * velocity;
        }
        output *= env * 0.3f;

        // Bow noise and shimmer
        float bowNoise = bowFilter.process(rng.generatePinkNoise()) * 0.08f * velocity * env;
        float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.06f * env * velocity * std::exp(-t / (decayTime * 0.5f));
        output += bowNoise + shimmer + bowTransient;

        // Apply effects
        output = stringFilter.process(output);
        output = highPass.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // VIOLIN_H