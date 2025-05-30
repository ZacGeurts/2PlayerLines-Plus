// instruments/guitar.h
// guitar instrument
// creates a guitar note with
// float t time
// float freq frequency
// float dur duration
#ifndef GUITAR_H
#define GUITAR_H

// Sound tuned for bright, expressive electric guitar with pick attack and rock edge
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback

#include "instruments.h"

namespace Instruments {

class Guitar : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::LowPassFilter bodyResonance;
    AudioUtils::HighPassFilter highPass;
    AudioUtils::Reverb reverb;
    AudioUtils::Distortion distortion;
    AudioUtils::BandPassFilter resonanceFilter;
    float gain; // 0.7f for balanced volume

public:
    Guitar(float gain = 0.7f)
        : protector(0.01f, 0.9f), // Smooth fade
          rng(),
          bodyResonance(1200.0f), // Midrange warmth
          highPass(100.0f, 0.707f), // Remove low-end mud
          reverb(0.1f, 0.5f, 0.3f), // Live room ambiance
          distortion(2.0f, 0.75f), // Rock grit
          resonanceFilter(300.0f, 1.2f), // String resonance
          gain(gain) {}

    float generateWave(float t, float freq, float dur) override {
        // Constrain frequency to guitar range (E2 to E5)
        freq = std::clamp(freq, 82.41f, 659.25f);

        // Dynamic velocity
        float velocity = 0.9f + rng.generateUniform(-0.15f, 0.15f);
        velocity = std::max(0.4f, std::min(1.0f, velocity));
        if (dur < 0.2f) velocity *= 0.8f;

        // ADSR envelope for plucked string
        float attack = 0.003f * (1.0f - 0.2f * velocity);
        float decay = 0.08f, sustain = 0.4f * velocity, release = 0.2f, env;
        if (t < attack) env = t / attack;
        else if (t < attack + decay) env = 1.0f - (t - attack) / decay * (1.0f - sustain);
        else if (t < dur) env = sustain * std::exp(-1.5f * (t - attack - decay) / dur);
        else env = sustain * std::exp(-(t - dur) / release);

        // Vibrato: 6 Hz, ±0.4% after 0.05s
        float vibrato = t > 0.05f ? 0.004f * std::sin(2.0f * M_PI * 6.0f * t) : 0.0f;
        float modulatedFreq = freq * (1.0f + vibrato);

        // Waveforms: sawtooth and square for bright tone
        float saw = 0.6f * (std::fmod(modulatedFreq * t, 1.0f) - 0.5f);
        float square = 0.4f * (std::sin(2.0f * M_PI * modulatedFreq * t) > 0.0f ? 1.0f : -1.0f);
        float harmonics = 0.2f * std::sin(2.0f * M_PI * 2.0f * modulatedFreq * t) * std::exp(-t / 0.5f);

        // Pluck: strong attack transient
        float pluck = t < 0.002f ? rng.generateWhiteNoise() * 0.3f * velocity * (1.0f - t / 0.002f) : 0.0f;

        // Fret noise and resonance
        float fretNoise = rng.generatePinkNoise() * std::exp(-40.0f * t) * 0.02f * velocity;
        float resonance = resonanceFilter.process(rng.generatePinkNoise()) * 0.06f * env * velocity;

        // Combine
        float output = env * velocity * (0.5f * saw + 0.3f * square + 0.2f * harmonics + pluck + fretNoise + resonance);

        // Apply effects
        output = bodyResonance.process(output);
        output = highPass.process(output);
        output = distortion.process(output);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // GUITAR_H