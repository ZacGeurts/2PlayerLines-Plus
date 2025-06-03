// instruments/banjo.h
// banjo instrument
// creates a banjo note with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef BANJO_H
#define BANJO_H

// The sound is tuned for a bright, twangy banjo tone with sharp pluck and resonant body

#include "instruments.h"

namespace Instruments {

class Banjo : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for velocity variation
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for string pluck texture
    AudioUtils::LowPassFilter lowPass;      // Smooths high frequencies for rounded tone
    AudioUtils::HighPassFilter highPass;    // Removes low-end mud
    AudioUtils::BandPassFilter bandPass;    // Emphasizes bright harmonics
    AudioUtils::Distortion distortion;      // Adds twangy grit
    AudioUtils::BrownNoise brownNoise;      // Adds subtle body resonance
    AudioUtils::Reverb reverb;              // Adds spatial ambiance
    AudioUtils::Chorus chorus;              // Thickens sound for string resonance
    AudioUtils::Tremolo tremolo;            // Adds subtle vibrato
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain;                            // Overall gain for balanced volume
    std::string name;                       // Stores instrument name for variant handling

public:
    // Constructor: Initialize with gain and name for variant handling
    Banjo(double gainValue = 0.9, const std::string& instrumentName = "banjo")
        : protector(0.01, 0.92),         // 10ms fade-out, 92% max gain for clean output
          whiteNoise(-0.7L, 0.7L),       // White noise for velocity variation
          pinkNoise(0.1),                // Pink noise for sharp string pluck
          lowPass(3500.0),               // 3.5kHz cutoff for bright, rounded tone
          highPass(150.0, 0.707),        // 150Hz cutoff, Q=0.707 to remove mud
          bandPass(1200.0, 0.8),         // 1.2kHz center, Q=0.8 for harmonic emphasis
          distortion(2.0, 0.85, 1.7),     // Twangy distortion: drive=2.0, threshold=0.85, soft=1.7
          brownNoise(0.04),              // Brown noise for body resonance
          reverb(0.3, 0.65, 0.35, 0.08),  // 300ms delay, 65% decay, 35% mix for open ambiance
          chorus(0.3, 0.5, 0.2),         // Depth=0.3, rate=0.5Hz, mix=20% for string resonance
          tremolo(6.0, 0.15),            // Rate=6Hz, depth=15% for subtle vibrato
          envFollow(0.005, 0.1),         // 5ms attack, 100ms release for dynamic response
          gain(gainValue),
          name(instrumentName)          // Store name for variant handling
    {} // Empty constructor body

    // Generate a banjo sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to banjo range (82Hz to 1.3kHz for E2 to C5)
        freq = std::max(82.0, std::min(1300.0, freq));

        // Dynamic velocity with subtle variation
        double velocity = 0.95 + whiteNoise.generate() * 0.3; // Subtle variation for pluck dynamics
        velocity = std::max(0.75, std::min(1.0, velocity));

        // ADSR envelope for sharp plucked tone
        double attack = 0.005, decay = 0.1, sustain = 0.6, release = 0.15; // Sharp plucked envelope
        if (name == "banjo_bright") {
            attack = 0.003; decay = 0.08; sustain = 0.65; // Brighter, sharper pluck
        } else if (name == "banjo_muted") {
            attack = 0.008; sustain = 0.5; release = 0.1; // Softer, muted pluck
        }
        double env;
        if (t < attack) {
            env = t / attack;
        } else if (t < attack + decay) {
            env = 1.0 - (t - attack) / decay * (1.0 - sustain);
        } else if (t < dur) {
            env = sustain;
        } else {
            env = sustain * std::exp(-(t - dur) / release);
        }
        env = std::max(0.0, env);

        // Pitch envelope for subtle vibrato
        double pitchEnv = std::sin(2.0 * M_PI * 6.0 * t) * 0.6; // 6Hz vibrato
        double pitchMod = freq + pitchEnv;

        // Waveforms: Sawtooth-like with sines and noise for string texture
        double sine1 = 0.5 * std::sin(2.0 * M_PI * pitchMod * t);          // Fundamental
        double sine2 = 0.3 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t);     // 2nd harmonic
        double sine3 = 0.15 * std::sin(2.0 * M_PI * 3.0 * pitchMod * t);    // 3rd harmonic
        double noise = 0.12 * pinkNoise() * std::exp(-t / 0.02);            // String pluck
        double brown = 0.04 * brownNoise() * std::exp(-t / 0.08);          // Body resonance

        // Variant-specific adjustments
        double mixSine1 = 0.5, mixSine2 = 0.3, mixSine3 = 0.15, mixNoise = 0.12, mixBrown = 0.04;
        double lowPassCutoff = 3500.0, bandPassCenter = 1200.0;
        if (name == "banjo_bright") {
            mixSine3 *= 1.4; mixNoise *= 1.3; lowPassCutoff = 4500.0; bandPassCenter = 1500.0; 
            distortion.setDrive(2.3); // Brighter tone
        } else if (name == "banjo_muted") {
            mixSine1 *= 1.2; mixSine3 *= 0.7; lowPassCutoff = 3000.0; bandPassCenter = 1000.0; 
            distortion.setDrive(1.7); // Muted tone
        }

        // Combine waveforms
        double output = env * velocity * (mixSine1 * sine1 + mixSine2 * sine2 + mixSine3 * sine3 + mixNoise * noise + mixBrown * brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(lowPassCutoff - 600.0 * envValue); // Dynamic cutoff for expressiveness
        bandPass.setCenterFreq(bandPassCenter + 300.0 * envValue); // Dynamic harmonic emphasis

        // Apply effects chain
        output = highPass.process(output);     // Remove mud
        output = bandPass.process(output);     // Emphasize harmonics
        output = lowPass.process(output);      // Round tone
        output = distortion.process(output);   // Twangy grit
        output = chorus.process(output);       // String resonance
        output = tremolo.process(output, t);   // Subtle vibrato
        output = reverb.process(output);       // Open ambiance
        output = protector.process(output, t, dur); // Protect output

        // Apply gain
        output *= gain;

        // Final clamp
        if (!std::isfinite(output)) {
            return 0.0;
        }

        return output;
    }
};

// Register banjo variations
static InstrumentRegistrar<Banjo> regBanjo("banjo");
static InstrumentRegistrar<Banjo> regBanjoBright("banjo_bright");
static InstrumentRegistrar<Banjo> regBanjoMuted("banjo_muted");

} // namespace Instruments

#endif // BANJO_H

/* 
 * AudioUtils Namespace
 * ===================
 * Provides utilities for audio processing and synthesis, designed for thread-safety and robustness.
 * Uses DEFAULT_SAMPLE_RATE (44100 Hz) and 8 channels (SDL2 max). All utilities include validation and clamping.
 *
 * 1. RandomGenerator
 *    - Generates high-quality random numbers for noise or stochastic effects.
 *    - Features: Thread-safe, 262,144-bit state (MegaMixMaxLite), clock-free entropy seeding.
 *    - Usage:
 *      static thread_local AudioUtils::RandomGenerator rng;
 *      int roll = rng.roll_d20();				// 1-20
 *      int percent = rng.roll_2d10();			// 0-99
 *      int range = rng.roll_dice(min, max);	// Custom range
 *      long double val = rng.random_L();		// Random long double [0,1)
 *      long double dist = rng.dist(1, 11);		// Random long double [1,11]
 *
 * 2. HighPassFilter
 *    - Removes frequencies below cutoff (e.g., DC blocking, bass high-end emphasis).
 *    - Features: Biquad filter, dynamic cutoff/Q-factor, precomputed coefficients, state reset.
 *    - Usage:
 *      HighPassFilter hpf(100.0L, 0.707L);		// 100Hz cutoff, Q=0.707
 *      long double out = hpf.process(input);	// Process signal
 *      hpf.setCutoff(150.0L);					// Update cutoff
 *
 * 3. LowPassFilter
 *    - Removes frequencies above cutoff (e.g., warming bass tones).
 *    - Features: First-order filter, dynamic cutoff, precomputed smoothing, state reset.
 *    - Usage:
 *      LowPassFilter lpf(250.0L);                // 250Hz cutoff
 *      long double out = lpf.process(input);      // Process signal
 *      lpf.setCutoff(300.0L);                    // Update cutoff
 *
 * 4. BandPassFilter
 *    - Isolates frequency band (e.g., resonant bass effects).
 *    - Features: Biquad filter, dynamic center frequency/bandwidth, precomputed coefficients.
 *    - Usage:
 *      BandPassFilter bpf(1000.0L, 0.5L);        // 1kHz center, Q=0.5
 *      long double out = bpf.process(input);      // Process signal
 *      bpf.setCenterFreq(1200.0L);               // Update center frequency
 *
 * 5. Reverb
 *    - Adds spatial ambiance (room reflections).
 *    - Features: Delay-based, modulated feedback, adjustable delay/decay/mix/modulation.
 *    - Usage:
 *      Reverb reverb(0.2L, 0.6L, 0.4L, 0.1L);    // 200ms delay, 60% decay, 40% mix, 10% mod
 *      long double out = reverb.process(input);   // Process signal
 *      reverb.setParameters(0.3L, 0.5L, 0.3L, 0.05L); // Update parameters
 *
 * 6. Distortion
 *    - Adds harmonic grit (e.g., bass amp overdrive).
 *    - Features: Soft clipping (tanh), dynamic drive/threshold/softness, output clamping.
 *    - Usage:
 *      Distortion dist(1.8L, 0.75L, 2.0L);        // Drive=1.8, threshold=0.75, softness=2.0
 *      long double out = dist.process(input);     // Process signal
 *      dist.setDrive(2.0L);                      // Update drive
 *
 * 7. AudioProtector
 *    - Ensures clean audio via DC blocking, fade-out, soft clipping, gain limiting.
 *    - Features: Integrates HighPassFilter, dynamic fade-out/gain limit, state reset.
 *    - Usage:
 *      AudioProtector protector(0.01L, 0.85L);    // 10ms fade-out, 85% max gain
 *      long double out = protector.process(input, 0.1L, 1.0L); // Process with t=0.1s, dur=1s
 *      protector.setMaxGain(0.9L);               // Update max gain
 *
 * 8. WhiteNoise
 *    - Generates white noise for stochastic effects.
 *    - Features: Uses RandomGenerator, configurable range, output clamping.
 *    - Usage:
 *      WhiteNoise white(-1.0L, 1.0L);            // Range [-1,1]
 *      long double out = white.process();        // Generate noise
 *
 * 9. BrownNoise
 *    - Generates brown (1/f²) noise for deep textures (e.g., amp hum).
 *    - Features: Uses RandomGenerator, adjustable amplitude, state reset.
 *    - Usage:
 *      BrownNoise brown(0.03L);                  // 3% amplitude
 *      long double out = brown.process();        // Generate noise
 *      brown.reset();                            // Reset state
 *
 * 10. Chorus
 *     - Thickens sound with modulated delay (e.g., doubled tracks).
 *     - Features: Variable delay (5-10ms), dynamic depth/rate/mix, state reset.
 *     - Usage:
 *       Chorus chorus(0.4L, 0.3L, 0.25L);       // Depth=0.4, rate=0.3Hz, mix=25%
 *       long double out = chorus.process(input); // Process signal
 *       chorus.reset();                         // Reset state
 *
 * 11. Tremolo
 *     - Adds amplitude pulsing (e.g., expressive bass effects).
 *     - Features: Sinusoidal modulation, dynamic rate/depth.
 *     - Usage:
 *       Tremolo tremolo(3.0L, 0.3L);            // Rate=3Hz, depth=30%
 *       long double out = tremolo.process(input, 0.1L); // Process with t=0.1s
 *
 * 12. EnvelopeFollower
 *     - Tracks signal amplitude for dynamic effect control.
 *     - Features: Adjustable attack/release, smooth tracking, state reset.
 *     - Usage:
 *       EnvelopeFollower env(0.005L, 0.05L);     // 5ms attack, 50ms release
 *       long double out = env.process(input);    // Track amplitude
 *       env.reset();                            // Reset state
 *
 * 13. PinkNoise
 *     - Generates pink (1/f) noise for natural textures (e.g., string buzz).
 *     - Features: Three-stage filtered white noise, adjustable amplitude, state reset.
 *     - Usage:
 *       PinkNoise pink(0.05L);                  // 5% amplitude
 *       long double out = pink.process();       // Generate noise
 *       pink.reset();                           // Reset state
 */