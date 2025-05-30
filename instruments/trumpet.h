// instruments/trumpet.h
// trumpet instrument
// creates a trumpet note with
// float t time
// float freq frequency
// float dur duration
#ifndef TRUMPET_H
#define TRUMPET_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Trumpet {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::LowPassFilter smoothFilter;
    AudioUtils::Reverb reverb;
    AudioUtils::Distortion overdrive;
    float gain; // 1.0f is 100% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Trumpet(float gain = 1.0f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.01f, 0.85f),
          breathFilter(1500.0f, 500.0f),
          smoothFilter(4000.0f),
          reverb(0.03f, 0.3f, 0.15f),
          overdrive(1.8f, 0.8f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(155.56f, std::min(1244.51f, freq));
        float attack = 0.002f, decay = 0.01f, sustain = 0.9f, release = 0.25f, env;
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
        float vibratoFreq = 5.5f;
        float vibratoDepth = 0.004f * (t > 0.15f ? 1.0f : t / 0.15f);
        float vibrato = std::sin(2.0f * M_PI * vibratoFreq * t) * vibratoDepth;
        float modulatedFreq = freq * (1.0f + vibrato);
        float harmonic1 = 1.0f * std::cos(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.9f * std::cos(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.7f * std::cos(2.0f * M_PI * 3.0f * modulatedFreq * t);
        float harmonic4 = 0.5f * std::cos(2.0f * M_PI * 4.0f * modulatedFreq * t);
        float harmonic5 = 0.3f * std::cos(2.0f * M_PI * 5.0f * modulatedFreq * t);
        float output = (harmonic1 + harmonic2 + harmonic3 + harmonic4 + harmonic5) * 0.2f * env;
        float detune = 1.005f;
        float chorus = 0.3f * std::cos(2.0f * M_PI * modulatedFreq * detune * t) * env;
        output += chorus;
        output = std::max(-0.8f, std::min(0.8f, output));
        float breathEnv = (t < 0.05f ? 1.2f : 0.3f) * env;
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.03f * breathEnv;
        breathNoise = std::max(-0.3f, std::min(0.3f, breathNoise));
        float articulation = (t < 0.005f) ? breathFilter.process(rng.generateWhiteNoise()) * 0.06f * env : 0.0f;
        articulation = std::max(-0.3f, std::min(0.3f, articulation));
        output = output + breathNoise + articulation;
        output = smoothFilter.process(output);
        output = overdrive.process(output);
        output = reverb.process(output);
        output = std::tanh(output * 1.2f);
        output *= 0.6f;
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // TRUMPET_H