// instruments/flute.h
// flute instrument
// creates a flute note with
// float t time
// float freq frequency
// float dur duration
#ifndef FLUTE_H
#define FLUTE_H

// Sound tuned for lyrical, breathy concert flute with pure tone and subtle vibrato
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Flute : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::Reverb reverb;
    AudioUtils::HighPassFilter filter;
    float gain; // 1.5f for balanced volume

public:
    Flute(float gain = 1.5f)
        : protector(0.004f, 0.95f), // Smooth fade
          rng(),
          breathFilter(1800.0f, 400.0f), // Breathy tone
          reverb(0.03f, 0.2f, 0.15f), // Light ambiance
          filter(250.0f, 0.707f), // Remove low-end rumble
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain frequency to flute range (C4 to C7)
        freq = std::max(261.63f, std::min(2093.0f, freq));

        // ADSR envelope for lyrical tone
        float attack = 0.02f, decay = 0.06f, sustain = 0.85f, release = 0.1f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 5 Hz, ±0.3% after 0.05s
        float vibrato = t > 0.05f ? 0.003f * std::sin(2.0f * M_PI * 5.0f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Waveforms: pure sine with soft harmonics
        float harmonic1 = 1.0f * std::sin(2.0f * M_PI * modulatedFreq * t);
        float harmonic2 = 0.15f * std::sin(2.0f * M_PI * 2.0f * modulatedFreq * t);
        float harmonic3 = 0.05f * std::sin(2.0f * M_PI * 3.0f * modulatedFreq * t);

        // Breath noise: more prominent for realism
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.02f * (t < 0.05f ? 1.0f : 0.2f);

        // Articulation: soft attack transient
        float articulation = t < 0.005f ? breathFilter.process(rng.generateWhiteNoise()) * 0.03f * env : 0.0f;

        // Combine
        float output = env * (0.8f * (harmonic1 + harmonic2 + harmonic3) + breathNoise + articulation);

        // Apply effects
        output = reverb.process(output);
        output = filter.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // FLUTE_H