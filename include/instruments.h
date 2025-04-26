#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <cmath>
#include <cstdlib>

// Instrument synthesis functions for reusable audio generation
namespace Instruments {

inline float generateKick(float t, float freq) {
    // Frequency sweep for dynamic pitch
    float freq_high = 2.0f * freq;
    float freq_low = freq;
    float tau = 0.05f; // Sweep duration
    float a = freq_low;
    float b = (freq_high - freq_low) / (2.0f * M_PI * tau);
    float phase = 2.0f * M_PI * (a * t - b * tau * (std::exp(-t / tau) - 1.0f));
    float body = std::sin(phase);
    // Noise burst for attack
    float attackEnv = std::exp(-20.0f * t);
    float noise = (std::rand() % 1000 / 1000.0f - 0.5f) * 0.2f * attackEnv;
    float env = std::exp(-12.0f * t); // Body decay
    return 0.5f * (body + 0.2f * std::sin(phase * 2.0f) + noise) * env;
}

inline float generateHiHat(float t, float freq, bool open) {
    float noise = (std::rand() % 1000 / 1000.0f - 0.5f) * 0.3f;
    float tonal = std::sin(t * 2.0f * M_PI * freq);
    float env = std::exp(open ? -10.0f * t : -15.0f * t); // Faster decay for closed
    return (0.1f * tonal + noise) * env * (open ? 0.6f : 0.3f);
}

inline float generateSnare(float t) {
    float noise = (std::rand() % 1000 / 500.0f - 1.0f);
    float tonal = std::sin(t * 2.0f * M_PI * 200.0f); // Drumhead resonance
    float env = std::exp(-15.0f * t);
    return (0.3f * noise + 0.1f * tonal) * env;
}

inline float generateClap(float t) {
    float noise = (std::rand() % 1000 / 500.0f - 1.0f);
    float env = std::exp(-25.0f * t);
    return 0.25f * noise * env;
}

inline float generateTom(float t, float freq) {
    float env1 = std::exp(-8.0f * t);
    float env2 = std::exp(-12.0f * t); // Faster decay for harmonic
    return (0.3f * std::sin(t * 2.0f * M_PI * freq) * env1 +
            0.15f * std::sin(t * 2.0f * M_PI * freq * 2.0f) * env2);
}

inline float generateBass(float t, float freq) {
    float env = std::exp(-5.0f * t);
    return 0.35f * (std::sin(t * 2.0f * M_PI * freq) +
                    0.3f * std::sin(t * 2.0f * M_PI * freq * 2) +
                    0.1f * std::sin(t * 2.0f * M_PI * freq * 3)) * env;
}

inline float generateSubBass(float t, float freq) {
    return 0.3f * std::sin(t * 2.0f * M_PI * freq) * std::exp(-3.0f * t);
}

inline float generateSynthArp(float t, float freq) {
    float env = std::exp(-10.0f * t);
    return 0.15f * (std::sin(t * 2.0f * M_PI * freq) +
                    0.4f * std::sin(t * 2.0f * M_PI * freq * 2) +
                    0.2f * std::sin(t * 2.0f * M_PI * freq * 3)) * env;
}

inline float generateLeadSynth(float t, float freq) {
    float env = std::exp(-8.0f * t);
    return 0.2f * (std::sin(t * 2.0f * M_PI * freq) +
                   0.3f * std::sin(t * 2.0f * M_PI * freq * 1.5f) +
                   0.1f * std::sin(t * 2.0f * M_PI * freq * 3.0f)) * env;
}

inline float generatePad(float t, float freq) {
    // Triangle wave for smoother sound
    float tri = std::fabs(std::fmod(t * freq, 1.0f) * 4.0f - 2.0f) - 1.0f;
    float env = std::exp(-2.0f * t);
    return 0.1f * tri * env;
}

inline float generateGuitar(float t, float freq) {
    float env1 = std::exp(-6.0f * t);
    float env2 = std::exp(-10.0f * t);
    float env3 = std::exp(-15.0f * t);
    float wave = (std::sin(t * 2.0f * M_PI * freq) * env1 +
                  0.5f * std::sin(t * 2.0f * M_PI * freq * 2) * env2 +
                  0.3f * std::sin(t * 2.0f * M_PI * freq * 3) * env3 +
                  0.1f * (std::rand() % 1000 / 1000.0f - 0.5f));
    return 0.25f * wave;
}

inline float generatePiano(float t, float freq) {
    // Inharmonicity for realistic piano strings
    float inharm = 0.001f * freq * freq;
    float env = std::exp(-5.0f * t);
    float wave = (std::sin(t * 2.0f * M_PI * freq) +
                  0.5f * std::sin(t * 2.0f * M_PI * (freq * 2 + inharm)) +
                  0.3f * std::sin(t * 2.0f * M_PI * (freq * 3 + inharm * 2)) +
                  0.1f * std::sin(t * 2.0f * M_PI * (freq * 4 + inharm * 3)));
    return 0.15f * wave * env;
}

inline float generateVocal(float t, float freq, int phoneme) {
    float env = std::exp(-3.0f * t);
    float carrier = std::sin(t * 2.0f * M_PI * freq);
    float formant1, formant2;
    switch (phoneme) {
        case 0: // 'k' (plosive, broadband noise)
            formant1 = (std::rand() % 1000 / 1000.0f - 0.5f) * 0.4f;
            formant2 = std::sin(t * 2.0f * M_PI * 2000.0f) * 0.2f;
            break;
        case 1: // 'ee' (high, close vowel)
            formant1 = std::sin(t * 2.0f * M_PI * 400.0f) * 0.4f;
            formant2 = std::sin(t * 2.0f * M_PI * 2500.0f) * 0.3f;
            break;
        case 2: // 'p' (plosive, softer)
            formant1 = (std::rand() % 1000 / 1000.0f - 0.5f) * 0.3f;
            formant2 = std::sin(t * 2.0f * M_PI * 1800.0f) * 0.2f;
            break;
        case 3: // 'o' (mid, open vowel)
            formant1 = std::sin(t * 2.0f * M_PI * 600.0f) * 0.4f;
            formant2 = std::sin(t * 2.0f * M_PI * 900.0f) * 0.2f;
            break;
        case 4: // 'n' (nasal)
            formant1 = std::sin(t * 2.0f * M_PI * 300.0f) * 0.3f;
            formant2 = std::sin(t * 2.0f * M_PI * 1500.0f) * 0.3f;
            break;
        case 5: // 'm' (nasal, lower)
            formant1 = std::sin(t * 2.0f * M_PI * 250.0f) * 0.3f;
            formant2 = std::sin(t * 2.0f * M_PI * 1200.0f) * 0.3f;
            break;
        default: // Fallback ('ah')
            formant1 = std::sin(t * 2.0f * M_PI * 700.0f) * 0.4f;
            formant2 = std::sin(t * 2.0f * M_PI * 1200.0f) * 0.2f;
    }
    return 0.3f * (carrier + formant1 + formant2) * env;
}

inline float generateFlute(float t, float freq) {
    float env = std::exp(-2.0f * t); // Slow decay for sustained sound
    float breath = 0.05f * (std::rand() % 1000 / 1000.0f - 0.5f); // Slight noise for breathiness
    float wave = std::sin(t * 2.0f * M_PI * freq) + breath;
    return 0.2f * wave * env;
}

inline float generateTrumpet(float t, float freq) {
    // Sawtooth wave approximation for brass
    float saw = 0.0f;
    for (int i = 1; i <= 4; ++i) {
        saw += std::sin(t * 2.0f * M_PI * freq * i) / i;
    }
    // Slight vibrato
    float vibrato = 0.05f * std::sin(t * 2.0f * M_PI * 5.0f);
    float env = std::exp(-3.0f * t);
    return 0.2f * (saw + vibrato) * env;
}

inline float generateViolin(float t, float freq) {
    // Square wave approximation for bowed strings
    float square = 0.0f;
    for (int i = 1; i <= 3; i += 2) {
        square += std::sin(t * 2.0f * M_PI * freq * i) / i;
    }
    // Slow attack and vibrato
    float attack = 1.0f - std::exp(-10.0f * t); // Slow attack
    float vibrato = 0.05f * std::sin(t * 2.0f * M_PI * 4.0f);
    float env = std::exp(-2.0f * t);
    return 0.2f * (square + vibrato) * env * attack;
}

} // namespace Instruments

#endif // INSTRUMENTS_H