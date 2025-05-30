// instruments/flute.h
// flute instrument
// creates a flute note with
// float t time
// float freq frequency
// float dur duration
#ifndef FLUTE_H
#define FLUTE_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Flute {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::Reverb reverb;
    AudioUtils::HighPassFilter filter;
    float gain; // 2.0f is 200% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Flute(float gain = 2.0f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          breathFilter(1600.0f, 300.0f),
          reverb(0.02f, 0.15f, 0.1f),
          filter(200.0f, 0.707f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(261.63f, std::min(2093.0f, freq));
        float attack = 0.015f, decay = 0.05f, sustain = 0.9f, release = 0.12f, env;
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
        float modulatedFreq = freq;
        float harmonic1 = 1.0f * std::sin(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.25f * std::sin(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.08f * std::sin(2.0f * M_PI * 3.0f * modulatedFreq * t);
        float output = (harmonic1 + harmonic2 + harmonic3) * 0.3f * env;
        output = std::max(-0.8f, std::min(0.8f, output));
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.008f * (t < 0.04f ? 0.9f : 0.15f);
        breathNoise = std::max(-0.15f, std::min(0.15f, breathNoise));
        float articulation = (t < 0.004f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.02f * env : 0.0f;
        articulation = std::max(-0.15f, std::min(0.15f, articulation));
        output = output + breathNoise * env + articulation;
        output = reverb.process(output);
        output = filter.process(output);
        output = std::tanh(output * 0.7f);
        output *= 0.45f;
        output = std::max(-1.0f, std::min(1.0f, output));
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // FLUTE_H
