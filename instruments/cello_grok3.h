// Copyright (c) 2025, Zachary Geurts

#ifndef CELLO_GROK3_H
#define CELLO_GROK3_H

#include "instruments.h"
#include <cmath>

namespace Instruments {

class CelloGrok3 : public Instrument {
private:
    AudioUtils::AudioProtector protector;
    AudioUtils::WhiteNoise whiteNoise;      // Bow micro-variations
    AudioUtils::PinkNoise pinkNoise;        // Rosin friction
    AudioUtils::BrownNoise brownNoise;      // Body resonance
    AudioUtils::LowPassFilter lowPass;      // Warmth control
    AudioUtils::HighPassFilter highPass;    // Rumble filter
    AudioUtils::BandPassFilter bandPass;    // String resonance
    AudioUtils::FormantFilter formantFilter;// Enhanced body resonance
    AudioUtils::NonLinearDistortion distortion; // Bow-string interaction
    AudioUtils::Reverb reverb;              // Concert hall simulation
    AudioUtils::PitchShifter pitchShifter;  // Subtle harmonic enrichment
    AudioUtils::Envelope envelope;          // Adaptive ADSR
    double gain;
    std::string name;

    // Refined physics-based core with added bridge resonance
    double celloPhysicsWave(double t, double freq, double bowPressure, double stringTension) const {
        double fundamental = 0.62 * std::sin(2.0 * M_PI * freq * t + bowPressure * 0.16);
        double second = 0.26 * std::sin(2.0 * M_PI * 2.0 * freq * t) * (1.0 - 0.22 * stringTension);
        double third = 0.14 * std::sin(2.0 * M_PI * 3.0 * freq * t) * (1.0 + 0.12 * stringTension);
        double fourth = 0.08 * std::sin(2.0 * M_PI * 4.0 * freq * t) * (0.8 + 0.22 * stringTension);
        double bridge = 0.06 * std::sin(2.0 * M_PI * 0.55 * freq * t) * stringTension * 1.1;
        return fundamental + second + third + fourth + bridge;
    }

    // Enhanced phase jitter with bow-string slip dynamics
    double phaseJitter(double t, double bowVelocity) const {
        return 0.035 * std::sin(0.26 * t + 4.2 * M_PI * whiteNoise()) * bowVelocity +
               0.016 * std::sin(0.08 * t + 8.5 * M_PI * pinkNoise());
    }

public:
    // Constructor with optimized defaults
    CelloGrok3(double gainValue = 0.88, const std::string& instrumentName = "cello_grok3")
        : protector(0.015, 0.96),            // 15ms fade, 96% max gain
          whiteNoise(-0.4, 0.4),
          pinkNoise(0.065),
          brownNoise(0.048),
          lowPass(1620.0),
          highPass(42.0, 0.76),
          bandPass(730.0, 0.97),
          formantFilter(345.0, 1.18),
          distortion(1.32, 0.99, 2.9),
          reverb(0.60, 0.88, 0.52, 0.16),
          pitchShifter(0.996, 0.06),
          envelope(0.026, 0.21, 0.92, 0.38),
          gain(gainValue),
          name(instrumentName)
    {}

    // Generate cello sound with refined dynamics
    double generateWave(double t, double freq, double dur) override {
        freq = std::max(62.0, std::min(920.0, freq));

        double bowPressure = 0.93 + 0.26 * whiteNoise();
        double bowVelocity = 0.89 + 0.32 * pinkNoise();
        bowPressure = std::clamp(bowPressure, 0.75, 1.12);
        bowVelocity = std::clamp(bowVelocity, 0.7, 1.0);
        double stringTension = 0.86 + 0.16 * envFollow.process(bowPressure); // Enhanced tension response

        double env = envelope.process(t, dur);
        if (name == "cello_grok3_pizzicato") {
            envelope.setParams(0.005, 0.11, 0.52, 0.16);
            env = envelope.process(t, dur);
        } else if (name == "cello_grok3_solo") {
            envelope.setParams(0.021, 0.16, 0.96, 0.42);
            env = envelope.process(t, dur);
        }

        double vibratoDepth = (name == "cello_grok3_pizzicato") ? 0.0 : 0.92 * env;
        double vibrato = std::sin(2.0 * M_PI * (5.25 + 0.27 * std::sin(0.125 * t)) * t) * vibratoDepth;

        double core = celloPhysicsWave(t + phaseJitter(t, bowVelocity), freq + vibrato, bowPressure, stringTension);
        double frictionNoise = 0.085 * pinkNoise() * std::exp(-t / 0.032);
        double bodyNoise = 0.055 * brownNoise() * std::exp(-t / 0.095);

        double output = env * bowVelocity * (core + frictionNoise + bodyNoise);

        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(1620.0 + 480.0 * envValue);
        bandPass.setCenterFreq(730.0 + 190.0 * envValue);

        output = highPass.process(output);
        output = bandPass.process(output);
        output = formantFilter.process(output, envValue * 1.1); // Slight boost for richness
        output = lowPass.process(output);
        output = distortion.process(output);
        output = pitchShifter.process(output, freq);
        output = tremolo.process(output, t, 4.3, 0.19);
        output = reverb.process(output);
        output = protector.process(output, t, dur);

        output *= gain * 1.05; // Gentle overall boost for boldness
        if (!std::isfinite(output)) return 0.0;
        return output;
    }
};

// Enhanced FormantFilter with richer resonance
namespace AudioUtils {
class FormantFilter {
    AudioUtils::BandPassFilter band1, band2, band3;
public:
    FormantFilter(double f1 = 345.0, double q = 1.18)
        : band1(f1, q), band2(1260.0, 1.45), band3(2850.0, 1.25) {}
    double process(double x, double env = 1.0) {
        double f1 = band1.process(x);
        double f2 = band2.process(x) * (0.25 + 0.75 * env);
        double f3 = band3.process(x) * (0.15 + 0.85 * env);
        return 0.72 * f1 + 0.18 * f2 + 0.10 * f3; // Adjusted mix for warmth
    }
};
}

static InstrumentRegistrar<CelloGrok3> regCelloGrok3("cello_grok3");
static InstrumentRegistrar<CelloGrok3> regCelloGrok3Solo("cello_grok3_solo");
static InstrumentRegistrar<CelloGrok3> regCelloGrok3Pizzicato("cello_grok3_pizzicato");

} // namespace Instruments

#endif // CELLO_GROK3_H

/*
 * CelloGrok3: The ultimate cello synthesis, blending physics and artistry.
 * - Advanced physics model with bridge resonance for authentic timbre.
 * - Richer formant filter with dynamic modulation for soulful depth.
 * - Subtle harmonic shimmer and bold dynamics for a live feel.
 * - Optimized for expressiveness and ready for any musical journey.
 */