// instruments/pad.h
// pad instrument
// creates a pad note with
// float t time
// float freq frequency
// float dur duration
#ifndef PAD_H
#define PAD_H

// ai can modify this file as long as float generateWave(float t, float freq, float dur) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

class Pad {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter filter;
    AudioUtils::Reverb reverb;
    float gain; // 0.25f is 25% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Pad(float gain = 0.25f)
        : protector(0.1f, 0.9f),
          filter(800.0f),
          reverb(0.8f, 0.8f, 0.6f),
          gain(gain) {}

    float generateWave(float t, float freq, float dur) {
        freq = std::max(32.7f, std::min(2093.0f, freq));
        float phase = 2.0f * M_PI * freq * t;
        float detune1 = 1.005f;
        float detune2 = 0.995f;
        float osc1 = std::sin(phase);
        float osc2 = std::sin(phase * detune1);
        float osc3 = std::sin(phase * detune2);
        float output = (osc1 + osc2 * 0.7f + osc3 * 0.7f) / 2.4f;
        float harmonic2 = 0.5f * std::sin(2.0f * phase);
        float harmonic3 = 0.3f * std::sin(3.0f * phase);
        float harmonic4 = 0.2f * std::sin(4.0f * phase);
        output += (harmonic2 + harmonic3 + harmonic4) * 0.4f;
        float noise = rng.generatePinkNoise() * 0.05f;
        output += noise;
        output = filter.process(output);
        float attack = 0.5f, decay = 0.2f, sustain = 0.8f, release = 1.0f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else if (t < dur + release) env = sustain * std::exp(-(t - dur) / release);
        else env = 0.0f;
        output *= env;
        output = reverb.process(output);
        output = std::max(-1.0f, std::min(1.0f, output));
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // PAD_H
