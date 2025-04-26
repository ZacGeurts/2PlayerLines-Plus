#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <cmath>
#include <cstdlib>

// Instrument synthesis functions for reusable audio generation
namespace Instruments {

inline float generateKick(float t, float freq) {
    float env = std::exp(-12.0f * t); // Fast decay for punch
    return 0.5f * (sin(t * 2.0f * M_PI * freq) + 0.2f * sin(t * 2.0f * M_PI * freq * 2)) * env;
}

inline float generateHiHat(float t, float freq, bool open) {
    float noise = (rand() % 1000 / 1000.0f - 0.5f) * 0.2f;
    float env = std::exp(open ? -10.0f * t : -15.0f * t);
    return (0.1f * sin(t * 2.0f * M_PI * freq) + noise) * env * (open ? 0.6f : 0.3f);
}

inline float generateSnare(float t) {
    float noise = (rand() % 1000 / 500.0f - 1.0f);
    return 0.3f * noise * std::exp(-15.0f * t);
}

inline float generateClap(float t) {
    float noise = (rand() % 1000 / 500.0f - 1.0f);
    return 0.25f * noise * std::exp(-25.0f * t);
}

inline float generateTom(float t, float freq) {
    return 0.3f * sin(t * 2.0f * M_PI * freq) * std::exp(-8.0f * t);
}

inline float generateBass(float t, float freq) {
    float env = std::exp(-5.0f * t);
    return 0.35f * (sin(t * 2.0f * M_PI * freq) + 0.3f * sin(t * 2.0f * M_PI * freq * 2)) * env;
}

inline float generateSubBass(float t, float freq) {
    return 0.3f * sin(t * 2.0f * M_PI * freq) * std::exp(-3.0f * t);
}

inline float generateSynthArp(float t, float freq) {
    float env = std::exp(-10.0f * t);
    return 0.15f * (sin(t * 2.0f * M_PI * freq) + 0.4f * sin(t * 2.0f * M_PI * freq * 2)) * env;
}

inline float generateLeadSynth(float t, float freq) {
    float env = std::exp(-8.0f * t);
    return 0.2f * (sin(t * 2.0f * M_PI * freq) + 0.3f * sin(t * 2.0f * M_PI * freq * 1.5f)) * env;
}

inline float generatePad(float t, float freq) {
    float env = std::exp(-2.0f * t);
    return 0.1f * sin(t * 2.0f * M_PI * freq) * env;
}

inline float generateGuitar(float t, float freq) {
    float env = std::exp(-6.0f * t);
    float wave = sin(t * 2.0f * M_PI * freq) + 0.5f * sin(t * 2.0f * M_PI * freq * 2) + 0.2f * (rand() % 1000 / 1000.0f - 0.5f);
    return 0.25f * wave * env;
}

inline float generatePiano(float t, float freq) {
    float env = std::exp(-5.0f * t);
    return 0.15f * sin(t * 2.0f * M_PI * freq) * env;
}

inline float generateVocal(float t, float freq, int phoneme) {
    float env = std::exp(-3.0f * t);
    float carrier = sin(t * 2.0f * M_PI * freq);
    float formant1, formant2;
    switch (phoneme) {
        case 0: // 'k' (plosive, broadband noise)
            formant1 = (rand() % 1000 / 1000.0f - 0.5f) * 0.4f;
            formant2 = sin(t * 2.0f * M_PI * 2000.0f) * 0.2f;
            break;
        case 1: // 'ee' (high, close vowel)
            formant1 = sin(t * 2.0f * M_PI * 400.0f) * 0.4f;
            formant2 = sin(t * 2.0f * M_PI * 2500.0f) * 0.3f;
            break;
        case 2: // 'p' (plosive, softer)
            formant1 = (rand() % 1000 / 1000.0f - 0.5f) * 0.3f;
            formant2 = sin(t * 2.0f * M_PI * 1800.0f) * 0.2f;
            break;
        case 3: // 'o' (mid, open vowel)
            formant1 = sin(t * 2.0f * M_PI * 600.0f) * 0.4f;
            formant2 = sin(t * 2.0f * M_PI * 900.0f) * 0.2f;
            break;
        case 4: // 'n' (nasal)
            formant1 = sin(t * 2.0f * M_PI * 300.0f) * 0.3f;
            formant2 = sin(t * 2.0f * M_PI * 1500.0f) * 0.3f;
            break;
        case 5: // 'm' (nasal, lower)
            formant1 = sin(t * 2.0f * M_PI * 250.0f) * 0.3f;
            formant2 = sin(t * 2.0f * M_PI * 1200.0f) * 0.3f;
            break;
        default: // Fallback ('ah')
            formant1 = sin(t * 2.0f * M_PI * 700.0f) * 0.4f;
            formant2 = sin(t * 2.0f * M_PI * 1200.0f) * 0.2f;
    }
    return 0.3f * (carrier + formant1 + formant2) * env; // Increased amplitude
}

} // namespace Instruments

#endif // INSTRUMENTS_H