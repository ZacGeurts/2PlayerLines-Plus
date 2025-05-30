// instruments/saxophone.h
// saxophone instrument
// creates a saxophone note with
// float t time
// float freq frequency
// float dur duration
// room for improvement
#ifndef SAXOPHONE_H
#define SAXOPHONE_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Saxophone {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::LowPassFilter smoothFilter;
    AudioUtils::Reverb reverb;
    float gain; // 1.0f is 100% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Saxophone(float gain = 1.0f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          breathFilter(2500.0f, 600.0f),
          smoothFilter(3500.0f),
          reverb(0.05f, 0.4f, 0.2f),
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(138.59f, std::min(880.0f, freq)); // Saxophone range (C#3 to A5)
        float attack = 0.005f, decay = 0.03f, sustain = 0.85f, release = 0.25f, env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        } else if (t < dur) {
            env = sustain;
        } else if (t < dur + release) {
            env = sustain * std::exp(-(t - dur) / release);
        } else {
            env = 0.0f;
        }
        float vibratoFreq = 5.0f;
        float vibratoDepth = 0.005f * (t > 0.15f ? 1.0f : t / 0.15f);
        float vibrato = std::sin(2.0f * M_PI * vibratoFreq * t) * vibratoDepth;
        float modulatedFreq = freq * (1.0f + vibrato);
        float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.6f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.3f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t);
        float output = (harmonic1 + harmonic2 + harmonic3) * 0.3f * env;
        output = std::max(-0.8f, std::min(0.8f, output));
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.05f * (t < 0.05f ? 1.2f : 0.5f);
        breathNoise = std::max(-0.4f, std::min(0.4f, breathNoise));
        float articulation = (t < 0.008f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.1f * env : 0.0f;
        articulation = std::max(-0.4f, std::min(0.4f, articulation));
        output = output + breathNoise * env + articulation;
        output = smoothFilter.process(output);
        output = reverb.process(output);
        output = std::tanh(output * 0.5f);
        output *= 0.3f;
        output = std::max(-1.0f, std::min(1.0f, output));
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SAXOPHONE_H
