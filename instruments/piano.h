// instruments/piano.h
// piano instrument
// creates a piano note with
// float t time
// float freq frequency
// float dur duration
#ifndef PIANO_H
#define PIANO_H

// Sound tuned for rich, resonant grand piano with dynamic hammer strikes and warm ambiance

#include "instruments.h"

namespace Instruments {

class Piano : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter stringFilter;
    AudioUtils::Reverb reverb;
    float gain; // 0.6f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Piano(float gain = 0.6f)
        : protector(0.01f, 0.85f),
          rng(),
          stringFilter(4000.0f),
          reverb(0.15f, 0.7f, 0.4f),
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        freq = std::max(27.5f, std::min(4186.0f, freq)); // Piano range (A0 to C8)
        float velocity = 0.8f + rng.generateUniform(-0.2f, 0.2f);
        if (dur < 0.1f) velocity *= 0.6f;
        velocity = std::max(0.2f, std::min(1.0f, velocity));

        // Sustain pedal simulation
        bool sustainPedal = (dur > 1.5f || t > 3.0f);

        // Envelope: dynamic for grand piano
        float attack = 0.001f * (1.0f - 0.4f * velocity);
        float decay = 0.05f, sustain = 0.7f * velocity, release = 0.3f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur || sustainPedal) env = sustain * std::exp(-(t - attack - decay) / (2.0f * (440.0f / freq)));
        else env = sustain * std::exp(-(t - dur) / release);

        // Decay time varies with pitch
        float decayTime = 6.0f * std::pow(440.0f / freq, 0.7f);
        decayTime = std::max(0.5f, std::min(8.0f, decayTime));
        if (sustainPedal) decayTime *= 1.5f;

        // Transient: hammer strike
        float transient = t < 0.003f ? rng.generateWhiteNoise() * 0.3f * velocity * (1.0f - t / 0.003f) : 0.0f;

        // Harmonics: rich grand piano tone
        float output = 0.0f;
        const float harmonics[] = {1.0f, 2.01f, 3.03f, 4.05f, 5.08f, 6.12f};
        const float amps[] = {1.0f, 0.7f, 0.4f, 0.2f, 0.1f, 0.05f};
        const float decays[] = {1.0f, 0.85f, 0.7f, 0.55f, 0.4f, 0.3f};
        for (int i = 0; i < 6; ++i) {
            float harmonicFreq = freq * harmonics[i];
            float harmonicDecay = decayTime * decays[i] * (440.0f / freq);
            float harmonic = amps[i] * std::cos(2.0f * M_PI * harmonicFreq * t) * std::exp(-t / harmonicDecay);
            output += harmonic * velocity;
        }
        output *= env * 0.25f;

        // Sustain pedal resonances
        if (sustainPedal) {
            float resFreq1 = freq * 2.0f;
            float resFreq2 = freq * 1.5f;
            if (resFreq1 <= 4186.0f) {
                output += 0.06f * std::cos(2.0f * M_PI * resFreq1 * t) * env * velocity * std::exp(-t / (decayTime * 0.8f));
            }
            if (resFreq2 <= 4186.0f) {
                output += 0.04f * std::cos(2.0f * M_PI * resFreq2 * t) * env * velocity * std::exp(-t / (decayTime * 0.8f));
            }
        }

        // Combine
        output += transient;
        output = stringFilter.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // PIANO_H