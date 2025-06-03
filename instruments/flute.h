// instruments/flute.h
// flute instrument
// creates a flute sound with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef FLUTE_H
#define FLUTE_H

// The sound is tuned for a breathy, airy flute tone with smooth sustain and gentle attack

#include "instruments.h"

namespace Instruments {

class Flute : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for velocity variation
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for breathy attack
    AudioUtils::LowPassFilter lowPass;      // Smooths high frequencies for warmth
    AudioUtils::HighPassFilter highPass;    // Removes subsonic rumble
    AudioUtils::BandPassFilter bandPass;    // Emphasizes airy harmonics
    AudioUtils::Distortion distortion;      // Adds subtle harmonic warmth
    AudioUtils::BrownNoise brownNoise;      // Adds low-end resonance
    AudioUtils::Reverb reverb;              // Adds spatial ambiance
    AudioUtils::Chorus chorus;              // Thickens sound for ensemble feel
    AudioUtils::Tremolo tremolo;            // Adds expressive vibrato
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain;                            // Overall gain for balanced volume
    std::string name;                       // Stores instrument name for variant handling

public:
    // Constructor: Initialize with gain and name for variant handling
    Flute(double gainValue = 0.85, const std::string& instrumentName = "flute")
        : protector(0.02, 0.9),          // 20ms fade-out, 90% max gain for smooth output
          whiteNoise(-0.6L, 0.6L),       // Subtle white noise for velocity variation
          pinkNoise(0.1),                // Pink noise for breathy attack
          lowPass(3000.0),               // 3kHz cutoff for warm, airy tone
          highPass(100.0, 0.707),        // 100Hz cutoff, Q=0.707 to remove rumble
          bandPass(1500.0, 0.8),         // 1.5kHz center, Q=0.8 for airy harmonics
          distortion(1.3, 0.95, 2.0),    // Subtle distortion: drive=1.3, threshold=0.95, soft=2.0
          brownNoise(0.03),              // Brown noise for low-end resonance
          reverb(0.5, 0.75, 0.5, 0.15),  // 500ms delay, 75% decay, 50% mix for hall ambiance
          chorus(0.3, 0.5, 0.2),         // Depth=0.3, rate=0.5Hz, mix=20% for ensemble feel
          tremolo(5.0, 0.15),            // Rate=5Hz, depth=15% for expressive vibrato
          envFollow(0.02, 0.2),          // 20ms attack, 200ms release for smooth dynamics
          gain(gainValue),
          name(instrumentName)          // Store name for variant handling
    {} // Empty constructor body

    // Generate a flute sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to flute range (261Hz to 2kHz for C4 to B5)
        freq = std::max(261.0, std::min(2000.0, freq));

        // Dynamic velocity with subtle variation
        double velocity = 0.9 + whiteNoise.generate() * 0.3; // Subtle variation for dynamics
        velocity = std::max(0.7, std::min(1.0, velocity));

        // ADSR envelope for smooth, breathy sustain
        double attack = 0.05, decay = 0.2, sustain = 0.85, release = 0.3; // Smooth flute envelope
        if (name == "flute_bright") {
            attack = 0.03; decay = 0.15; sustain = 0.9; // Brighter, quicker attack
        } else if (name == "flute_muted") {
            attack = 0.08; sustain = 0.7; release = 0.25; // Softer, muted tone
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

        // Pitch envelope for expressive vibrato
        double pitchEnv = std::sin(2.0 * M_PI * 5.0 * t) * 0.6; // 5Hz vibrato
        double pitchMod = freq + pitchEnv;

        // Waveforms: Sine-heavy with noise for breathy texture
        double sine1 = 0.7 * std::sin(2.0 * M_PI * pitchMod * t);          // Fundamental
        double sine2 = 0.2 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t);     // 2nd harmonic
        double sine3 = 0.1 * std::sin(2.0 * M_PI * 3.0 * pitchMod * t);     // 3rd harmonic
        double noise = 0.15 * pinkNoise() * std::exp(-t / 0.05);            // Breathy attack
        double brown = 0.03 * brownNoise() * std::exp(-t / 0.2);           // Low-end resonance

        // Variant-specific adjustments
        double mixSine1 = 0.7, mixSine2 = 0.2, mixSine3 = 0.1, mixNoise = 0.15, mixBrown = 0.03;
        double lowPassCutoff = 3000.0, bandPassCenter = 1500.0;
        if (name == "flute_bright") {
            mixSine2 *= 1.3; mixSine3 *= 1.4; lowPassCutoff = 3500.0; bandPassCenter = 1800.0;
            distortion.setDrive(1.5); // Brighter tone
        } else if (name == "flute_muted") {
            mixSine1 *= 1.2; mixSine3 *= 0.6; lowPassCutoff = 2500.0; bandPassCenter = 1200.0;
            distortion.setDrive(1.1); // Muted tone
        }

        // Combine waveforms
        double output = env * velocity * (mixSine1 * sine1 + mixSine2 * sine2 + mixSine3 * sine3 + mixNoise * noise + mixBrown * brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(lowPassCutoff - 500.0 * envValue); // Dynamic cutoff for expressiveness
        bandPass.setCenterFreq(bandPassCenter + 200.0 * envValue); // Dynamic harmonic emphasis

        // Apply effects chain
        output = highPass.process(output);     // Remove rumble
        output = bandPass.process(output);     // Emphasize harmonics
        output = lowPass.process(output);      // Warm tone
        output = distortion.process(output);   // Subtle warmth
        output = chorus.process(output);       // Ensemble feel
        output = tremolo.process(output, t);   // Expressive vibrato
        output = reverb.process(output);       // Hall ambiance
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

// Register flute variations
static InstrumentRegistrar<Flute> regFlute("flute");
static InstrumentRegistrar<Flute> regFluteBright("flute_bright");
static InstrumentRegistrar<Flute> regFluteMuted("flute_muted");

} // namespace Instruments

#endif // FLUTE_H

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