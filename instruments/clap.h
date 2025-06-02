// instruments/clap.h
// clap instrument (single variant)
// creates a clap note with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef CLAP_H
#define CLAP_H

// The sound is tuned for a sharp, bright hand clap with a noisy attack and short sustain

#include "instruments.h"

namespace Instruments {

class Clap : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for clap attack
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for body texture
    AudioUtils::BandPassFilter filter;      // Isolates clap frequency range
    AudioUtils::HighPassFilter highPass;    // Removes low-end for clarity
    AudioUtils::Distortion distortion;      // Adds grit to the clap
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain; // Overall gain for balanced volume

public:
    Clap(double gainValue = 0.8) // Moderate gain for balanced mix
        : protector(0.004, 0.9),       // 4ms fade-out, 90% max gain for tight control
          whiteNoise(-1.0L, 1.0L),     // White noise for clap attack
          pinkNoise(0.2),              // 20% amplitude for body texture
          filter(4500.0, 1.0),         // 4.5kHz center, Q=1.0 for focused clap range
          highPass(1200.0, 0.707),     // 1.2kHz cutoff for clarity
          distortion(1.6, 0.9, 1.3),   // Light distortion: drive=1.6, threshold=0.9, soft=1.3
          envFollow(0.001, 0.025),     // Fast envelope: 1ms attack, 25ms release
          gain(gainValue)              // Volume 0.0 through 1.0
    {}

    // Generate a clap sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to clap range (500Hz to 6kHz for tonal brightness)
        freq = std::max(500.0, std::min(6000.0, freq));

        // Dynamic velocity with random variation
        double velocity = 0.9 + whiteNoise.generate() * 0.1;
        velocity = std::max(0.6, std::min(1.0, velocity));

        // ADSR envelope: sharp attack, short sustain for clap
        double attack = 0.001, decay = 0.03, sustain = 0.1, release = 0.08;
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

        // Pitch envelope for slight brightness sweep
        double pitchEnv = std::exp(-t / 0.05) * 300.0;
        double pitchMod = freq + pitchEnv;

        // Waveforms: Noise-based for clap texture, minimal tonal component
        double noise = 0.6 * whiteNoise.generate(); // Primary clap attack
        double pink = 0.3 * pinkNoise() * std::exp(-t / 0.06); // Body texture
        double tonal = 0.1 * std::sin(2.0 * M_PI * pitchMod * t) * std::exp(-t / 0.04); // Subtle tonal component

        // Combine waveforms
        double output = env * velocity * (noise + pink + tonal);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        filter.setCenterFreq(4000.0 + 2000.0 * envValue); // 4kHz-6kHz for dynamic brightness

        // Apply effects chain
        output = distortion.process(output); // Add grit
        output = filter.process(output);     // Focus clap range
        output = highPass.process(output);   // Remove low-end
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

// Register the clap instrument
static InstrumentRegistrar<Clap> regClap("clap");

} // namespace Instruments

#endif // CLAP_H

/*
 * AudioUtils Namespace Overview
 * ============================
 * The AudioUtils namespace contains a suite of audio processing utilities designed
 * for high-quality instrument synthesis.
 * All utilities operate at DEFAULT_SAMPLE_RATE (44100.0f) and use double precision
 * for numerical stability. They integrate seamlessly with RandomGenerator (using
 * MegaMixMaxLite) for noise generation and are optimized for real-time audio with
 * robust parameter validation and output clamping.
 *
 * Existing Utilities
 * -----------------
 * 1. RandomGenerator
 *    - Purpose: Generates high-quality random numbers for white noise, pink noise,
 *      and uniform distributions, used for velocity variation, string noise, or
 *      stochastic effects. Uses a 262,144-bit state (MegaMixMaxLite) for superior
 *      statistical quality.
 *    - Features:
 *      - White noise: Uniform random values in a specified range.
 *      - Pink noise: 1/f noise for natural string buzz or ambient texture.
 *      - Thread-safe with clock-free seeding.
 *    - Call Example:
 *      RandomGenerator rng(-1.0, 1.0);				// Initialize random generator
 *      int diceroll = rng.roll_d6();				// Roll a 6-sided die
 *		int diceroll = rng.roll_dice(min, max);		// specify range to random
 *
 * 2. HighPassFilter
 *    - Purpose: Removes low frequencies below a cutoff, used for DC blocking or
 *      emphasizing high-end in bass synthesis.
 *    - Features:
 *      - Biquad filter with dynamic cutoff and Q-factor.
 *      - Precomputed coefficients for efficiency.
 *      - Dynamic parameter updates and state reset.
 *    - Call Example:
 *      HighPassFilter hpf(100.0, 0.707);			// 100Hz cutoff, Q=0.707
 *      double output = hpf.process(white);			// Process white noise
 *      hpf.setCutoff(150.0);						// Update cutoff
 *
 * 3. LowPassFilter
 *    - Purpose: Removes high frequencies above a cutoff, used for warming bass tones.
 *    - Features:
 *      - First-order filter with dynamic cutoff.
 *      - Precomputed smoothing factor for efficiency.
 *      - State reset for stability.
 *    - Call Example:
 *      LowPassFilter lpf(250.0); // 250Hz cutoff
 *      double output = lpf.process(pink);			// Process pink noise
 *      lpf.setCutoff(300.0);						// Update cutoff
 *
 * 4. BandPassFilter
 *    - Purpose: Isolates a frequency band, used for resonant bass effects or formant-like tones.
 *    - Features:
 *      - Biquad filter with dynamic center frequency and bandwidth.
 *      - Precomputed coefficients for efficiency.
 *      - Dynamic updates and state reset.
 *    - Call Example:
 *      BandPassFilter bpf(1000.0, 0.5);			// 1kHz center, Q=0.5
 *      double output = bpf.process(white);			// Process white noise
 *      bpf.setCenterFreq(1200.0);					// Update center frequency
 *
 * 5. Reverb
 *    - Purpose: Adds spatial ambiance, simulating room reflections for a live bass sound.
 *    - Features:
 *      - Delay-based reverb with modulated feedback for richness.
 *      - Dynamic delay time, decay, mix, and modulation.
 *      - Buffer resizing and state reset.
 *    - Call Example:
 *      Reverb reverb(0.2, 0.6, 0.4, 0.1);			// 200ms delay, 60% decay, 40% mix, 10% mod
 *      double output = reverb.process(pink);		// Process pink noise
 *      reverb.setParameters(0.3, 0.5, 0.3, 0.05);	// Update parameters
 *
 * 6. Distortion
 *    - Purpose: Adds harmonic grit, emulating bass amp overdrive or string distortion.
 *    - Features:
 *      - Soft clipping with tanh for natural distortion.
 *      - Dynamic drive, threshold, and softness.
 *      - Output clamping for stability.
 *    - Call Example:
 *      Distortion dist(1.8, 0.75, 2.0);			// Drive=1.8, threshold=0.75, soft=2.0
 *      double output = dist.process(white);		// Process white noise
 *      dist.setDrive(2.0);							// Update drive
 *
 * 7. AudioProtector
 *    - Purpose: Ensures clean audio output by removing DC offset, applying fade-out,
 *      soft clipping, and gain limiting.
 *    - Features:
 *      - Integrates HighPassFilter for DC blocking.
 *      - Dynamic fade-out and gain limit.
 *      - State reset for stability.
 *    - Call Example:
 *      AudioProtector protector(0.01, 0.85);		// 10ms fade-out, 85% max gain
 *      double output = protector.process(pink, 0.1, 1.0);	// Process with t=0.1s, dur=1s
 *      protector.setMaxGain(0.9);					// Update max gain
 *
 * 8. BrownNoise
 *    - Purpose: Generates brown (1/f²) noise for deep, rumbly textures like amp hum
 *      or room ambiance, enhancing bass guitar realism.
 *    - Features:
 *      - Integrates RandomGenerator’s white noise for Brownian motion.
 *      - Adjustable amplitude scale.
 *      - State reset to prevent drift.
 *    - Call Example:
 *      BrownNoise brown(0.03);						// 3% amplitude scale
 *      double output = brown.process();			// Generate brown noise sample
 *      brown.reset();								// Reset state
 *
 * 9. Chorus
 *    - Purpose: Thickens bass sound with modulated delay, simulating doubled tracks
 *      or detuning for a richer tone.
 *    - Features:
 *      - Variable delay (5-10ms) with dynamic depth, rate, and mix.
 *      - Linear interpolation for smooth modulation.
 *      - Buffer management and state reset.
 *    - Call Example:
 *      Chorus chorus(0.4, 0.3, 0.25);				// Depth=0.4, rate=0.3Hz, mix=25%
 *      double output = chorus.process(pink);		// Process pink noise
 *      chorus.reset();								// Reset state
 *
 * 10. Tremolo
 *     - Purpose: Adds amplitude pulsing for dynamic bass effects, emulating expressive
 *       playing or amp tremolo.
 *     - Features:
 *       - Sinusoidal amplitude modulation with dynamic rate and depth.
 *       - Lightweight and efficient.
 *     - Call Example:
 *       Tremolo tremolo(3.0, 0.3);					// Rate=3Hz, depth=30%
 *       double output = tremolo.process(white, 0.1); // Process with t=0.1s
 *
 * 11. EnvelopeFollower
 *     - Purpose: Tracks signal amplitude to dynamically control effects (e.g., filter
 *       cutoff), adding responsiveness to bass dynamics.
 *     - Features:
 *       - Adjustable attack and release times.
 *       - Smooth envelope tracking with state reset.
 *     - Call Example:
 *       EnvelopeFollower envFollow(0.005, 0.05);	// 5ms attack, 50ms release
 *       double env = envFollow.process(pink);		// Track amplitude
 *       envFollow.reset();							// Reset state
 *
 * 12. WhiteNoise
 *     - Purpose: Generates white noise (uniform random values) for velocity variation
 *       or stochastic effects in instrument synthesis.
 *     - Features:
 *       - Uses RandomGenerator for high-quality randomness.
 *       - Configurable range [min, max].
 *       - Output clamping for stability.
 *     - Call Example:
 *       RandomGenerator rng(-1.0, 1.0);			// Initialize random generator
 *       WhiteNoise white(rng, -0.15, 0.15);		// ±15% range
 *       double output = white.process();			// Generate white noise sample

 * 13. PinkNoise
 *     - Purpose: Generates pink (1/f) noise for natural string buzz or ambient texture,
 *       enhancing instrument realism.
 *     - Features:
 *       - Three-stage filter on white noise for 1/f spectrum.
 *       - Adjustable amplitude scale.
 *       - State reset for filter stability.
 *     - Call Example:
 *       RandomGenerator rng(-1.0, 1.0);			// Initialize random generator
 *       PinkNoise pink(rng, 0.05);					// 5% amplitude
 *       double output = pink.process();			// Generate pink noise sample
 *       pink.reset();								// Reset state
 *
 * Integration Example
 * ------------------
 * To create a realistic bass guitar sound, chain utilities with RandomGenerator:
 *   RandomGenerator rng(-1.0, 1.0);
 *   LowPassFilter lpf(250.0);
 *   Distortion dist(1.8, 0.75, 2.0);
 *   Chorus chorus(0.4, 0.3, 0.25);
 *   Tremolo tremolo(3.0, 0.3);
 *   EnvelopeFollower envFollow(0.005, 0.05);
 *   BrownNoise brown(0.03);
 *   AudioProtector protector(0.01, 0.85);
 *
 *   double t = 0.1, dur = 1.0;
 *   double sample = rng.generatePinkNoise() + brown.process() * 0.5; // Combine noises
 *   double env = envFollow.process(sample);		// Track amplitude
 *   lpf.setCutoff(150.0 + 100.0 * env);			// Dynamic filter
 *   sample = dist.process(sample);					// Add grit
 *   sample = lpf.process(sample);					// Warm tone
 *   sample = chorus.process(sample);				// Thicken sound
 *   sample = tremolo.process(sample, t);			// Add pulsing
 *   sample = protector.process(sample, t, dur);	// Protect output
 *
 * Notes
 * -----
 * - All utilities use DEFAULT_SAMPLE_RATE (44100.0f) for SDL2 compatibility.
 * - Calculations use double precision for stability, compatible with RandomGenerator’s
 *   long double output (cast to double).
 * - Utilities are thread-safe where applicable (e.g., RandomGenerator) and include
 *   validation and clamping for robustness.
 * - Designed for real-time audio with minimal computational overhead.
 * - Enhances synthesis by adding noise textures, dynamic effects, and
 *   professional polish.
 */