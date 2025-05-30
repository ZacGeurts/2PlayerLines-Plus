// instruments/saxophone.h
// saxophone instrument
// creates a saxophone note with
// float t time
// float freq frequency
// float dur duration
#ifndef SAXOPHONE_H
#define SAXOPHONE_H

// Sound tuned for a big, bold alto saxophone with reedy tone and live jazz vibe
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Saxophone : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::BandPassFilter breathFilter;
    AudioUtils::LowPassFilter lowPassFilter;
    AudioUtils::Reverb reverb;
    AudioUtils::Distortion distortion;
    float gain; // 0.8f for balanced volume
    float sampleRate; // DEFAULT_SAMPLE_RATE

public:
    Saxophone(float gain = 0.8f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          rng(),
          breathFilter(2000.0f, 500.0f), // Reedy breathiness
          lowPassFilter(3000.0f),
          reverb(0.1f, 0.5f, 0.3f), // Jazz club ambiance
          distortion(1.5f, 0.8f), // Subtle grit
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain to alto sax range (C#3 to C#6)
        freq = std::max(138.59f, std::min(1046.5f, freq));

        // Envelope: dynamic for expressive phrasing
        float attack = 0.008f, decay = 0.1f, sustain = 0.8f, release = 0.2f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain;
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 6 Hz, ±0.5% after 0.2s
        float vibrato = t > 0.2f ? 0.005f * std::sin(2.0f * M_PI * 6.0f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Waveforms: square for reed, sawtooth for body
        float square = 0.6f * (std::sin(2.0f * M_PI * modulatedFreq * t) > 0.0f ? 1.0f : -1.0f);
        float saw = 0.4f * (std::fmod(modulatedFreq * t, 1.0f) - 0.5f);
        float harmonic3 = 0.2f * std::sin(2.0f * M_PI * 3.0f * modulatedFreq * t);

        // Breath noise for realism
        float breathNoise = breathFilter.process(rng.generateWhiteNoise()) * 0.15f * (t < 0.06f ? 1.5f : 0.6f);

        // Articulation: tongue attack
        float articulation = t < 0.01f ? breathFilter.process(rng.generateWhiteNoise()) * 0.3f * env : 0.0f;

        // Combine
        float output = env * (0.5f * square + 0.3f * saw + 0.2f * harmonic3 + breathNoise + articulation);

        // Apply effects
        output = distortion.process(output); // Add grit
        output = lowPassFilter.process(output); // Smooth high-end
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // SAXOPHONE_H