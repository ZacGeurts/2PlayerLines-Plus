// instruments/steelguitar.h
// steel guitar instrument
// creates a steel guitar note with
// float t time
// float freq frequency
// float dur duration
#ifndef STEELGUITAR_H
#define STEELGUITAR_H

// Sound tuned for smooth, expressive pedal steel guitar with subtle slides
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class SteelGuitar : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::BandPassFilter shimmerFilter;
    float gain; // 0.3f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    SteelGuitar(float gain = 0.3f)
        : protector(0.02f, 0.8f),
          rng(),
          stringFilter(2500.0f),
          highPass(100.0f, 0.707f),
          reverb(0.25f, 0.8f, 0.3f),
          shimmerFilter(5000.0f, 0.9f),
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain to steel guitar range (E2 to E5)
        freq = std::max(82.41f, std::min(1318.51f, freq));

        // Dynamic velocity
        float velocity = 0.85f + rng.generateUniform(-0.1f, 0.1f);
        if (dur < 0.1f) velocity *= 0.6f;
        velocity = std::max(0.3f, std::min(1.0f, velocity));

        // Envelope: smooth for slides
        float attack = 0.008f * (1.0f - 0.2f * velocity);
        float decay = 0.1f, sustain = 0.6f * velocity, release = 0.7f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain * std::exp(-(t - attack - decay) / (3.0f * (440.0f / freq)));
        else env = sustain * std::exp(-(t - dur) / release);

        // Decay time
        float decayTime = 7.0f * std::pow(440.0f / freq, 0.7f);
        decayTime = std::max(1.0f, std::min(10.0f, decayTime));

        // Slide transient
        float slideTransient = t < 0.005f ? rng.generatePinkNoise() * 0.1f * velocity * (1.0f - t / 0.005f) : 0.0f;

        // Harmonics
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.01f, 3.02f, 4.03f};
        const float amps[] = {1.0f, 0.6f, 0.3f, 0.15f};
        for (int i = 0; i < 4; ++i) {
            float harmonicFreq = freq * harmonics[i];
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / (decayTime * (1.0f - 0.2f * i)));
            output += harmonic * velocity;
        }
        output *= env * 0.3f;

        // Slide noise and shimmer
        float slideNoise = rng.generatePinkNoise() * std::exp(-40.0f * t) * 0.03f * velocity;
        float shimmer = shimmerFilter.process(rng.generatePinkNoise()) * 0.04f * env * velocity * std::exp(-t / (decayTime * 0.5f));
        output += slideNoise + shimmer + slideTransient;

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

#endif // STEELGUITAR_H