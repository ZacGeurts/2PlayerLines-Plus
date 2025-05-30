// instruments/trumpet.h
// trumpet instrument
// creates a trumpet note with
// float t time
// float freq frequency
// float dur duration
#ifndef TRUMPET_H
#define TRUMPET_H

// Sound tuned for big, bold trumpet with bright, piercing tone and concert hall presence
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Trumpet : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::LowPassFilter smoothFilter;
    AudioUtils::Reverb reverb;
    AudioUtils::Distortion overdrive;
    float gain; // 1.2f for strong presence
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Trumpet(float gain = 1.2f)
        : protector(0.01f, 0.85f),
          rng(),
          breathFilter(1500.0f, 500.0f),
          smoothFilter(4000.0f),
          reverb(0.05f, 0.4f, 0.25f), // Concert hall ambiance
          overdrive(2.0f, 0.85f), // Punchy grit
          gain(gain),
          sampleRate(AudioUtils::DEFAULT_SAMPLE_RATE) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain to trumpet range (E3 to C6)
        freq = std::max(164.81f, std::min(1046.5f, freq));

        // Envelope: sharp attack for fanfare
        float attack = 0.002f, decay = 0.0f, sustain = 0.0f, release = 0.3f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 5.5 Hz, ±0.4 after 0.15s
        float vibrato = t > 0.15f ? 0.004f * std::sin(2.0f * M_PI * 5.5f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Harmonics: bright and bold
        float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.9f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.7f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t);
        float harmonic4 = 0.5f * std::cos(2.0f * M_PI * 4.0f * modulatedFreq * t);
        float harmonic5 = 0.3f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t);
        float harmonic6 = 0.2f * std::cos(2.0f * M_PI * 6.0f * modulatedFreq * t);

        // Combine harmonics
        float output = (harmonic1 + harmonic2 + harmonic3 + harmonic4 + harmonic5 + harmonic6) * 0.18f * env;

        // Chorus effect
        float detune = 1.005f;
        float chorus = 0.3f * std::cos(2.0f * M_PI * modulatedFreq * detune * t) * env;
        output += chorus;

        // Breath noise and articulation
        float breathEnv = (t < 0.05f ? 1.5f : 0.4f) * env;
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.05f * breathEnv;
        float articulation = t < 0.005f ? breathFilter.process(rng.generateWhiteNoise()) * 0.1f * env : 0.0f;

        // Combine
        output += breathNoise + articulation;

        // Apply effects
        output = smoothFilter.process(output);
        output = overdrive.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // TRUMPET_H