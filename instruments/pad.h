// instruments/pad.h
// pad instrument
// creates a pad sound with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef PAD_H
#define PAD_H

// The sound is tuned for a lush, ambient pad tone with smooth sustain and warm texture

#include "instruments.h"

namespace Instruments {

class Pad : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for velocity variation
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for subtle texture
    AudioUtils::LowPassFilter lowPass;      // Smooths high frequencies for warmth
    AudioUtils::HighPassFilter highPass;    // Removes subsonic rumble
    AudioUtils::BandPassFilter bandPass;    // Emphasizes harmonic richness
    AudioUtils::Distortion distortion;      // Adds subtle harmonic warmth
    AudioUtils::BrownNoise brownNoise;      // Adds low-end resonance
    AudioUtils::Reverb reverb;              // Adds spacious ambiance
    AudioUtils::Chorus chorus;              // Thickens sound for lush texture
    AudioUtils::Tremolo tremolo;            // Adds gentle vibrato
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain;                            // Overall gain for balanced volume
    std::string name;                       // Stores instrument name for variant handling

public:
    // Constructor: Initialize with gain and name for variant handling
    Pad(double gainValue = 0.8, const std::string& instrumentName = "pad")
        : protector(0.05, 0.9),          // 50ms fade-out, 90% max gain for smooth output
          whiteNoise(-0.5L, 0.5L),       // Subtle white noise for velocity variation
          pinkNoise(0.05),               // Pink noise for subtle texture
          lowPass(2000.0),               // 2kHz cutoff for warm, ambient tone
          highPass(50.0, 0.707),         // 50Hz cutoff, Q=0.707 to remove rumble
          bandPass(800.0, 0.7),          // 800Hz center, Q=0.7 for harmonic richness
          distortion(1.2, 0.95, 2.5),    // Subtle distortion: drive=1.2, threshold=0.95, soft=2.5
          brownNoise(0.04),              // Brown noise for low-end resonance
          reverb(0.8, 0.85, 0.6, 0.2),   // 800ms delay, 85% decay, 60% mix for spacious ambiance
          chorus(0.4, 0.3, 0.3),         // Depth=0.4, rate=0.3Hz, mix=30% for lush texture
          tremolo(4.0, 0.2),             // Rate=4Hz, depth=20% for gentle vibrato
          envFollow(0.05, 0.5),          // 50ms attack, 500ms release for smooth dynamics
          gain(gainValue),
          name(instrumentName)          // Store name for variant handling
    {} // Empty constructor body

    // Generate a pad sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to pad range (110Hz to 1kHz for A2 to C5)
        freq = std::max(110.0, std::min(1000.0, freq));

        // Dynamic velocity with subtle variation
        double velocity = 0.9 + whiteNoise.generate() * 0.3; // Subtle variation for dynamics
        velocity = std::max(0.7, std::min(1.0, velocity));

        // ADSR envelope for smooth, sustained tone
        double attack = 0.5, decay = 0.3, sustain = 0.9, release = 1.0; // Smooth pad envelope
        if (name == "pad_bright") {
            attack = 0.3; decay = 0.2; sustain = 0.95; // Brighter, quicker attack
        } else if (name == "pad_dark") {
            attack = 0.7; sustain = 0.8; release = 1.2; // Darker, slower attack
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

        // Pitch envelope for gentle vibrato
        double pitchEnv = std::sin(2.0 * M_PI * 3.0 * t) * 0.5; // 3Hz vibrato
        double pitchMod = freq + pitchEnv;

        // Waveforms: Sawtooth-like with sines and noise for lush texture
        double sine1 = 0.6 * std::sin(2.0 * M_PI * pitchMod * t);          // Fundamental
        double sine2 = 0.3 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t);     // 2nd harmonic
        double sine3 = 0.15 * std::sin(2.0 * M_PI * 3.0 * pitchMod * t);    // 3rd harmonic
        double noise = 0.05 * pinkNoise() * std::exp(-t / 0.5);             // Subtle texture
        double brown = 0.05 * brownNoise() * std::exp(-t / 0.8);           // Low-end resonance

        // Variant-specific adjustments
        double mixSine1 = 0.6, mixSine2 = 0.3, mixSine3 = 0.15, mixNoise = 0.05, mixBrown = 0.05;
        double lowPassCutoff = 2000.0, bandPassCenter = 800.0;
        if (name == "pad_bright") {
            mixSine2 *= 1.3; mixSine3 *= 1.4; lowPassCutoff = 2500.0; bandPassCenter = 1000.0;
            distortion.setDrive(1.4); // Brighter tone
        } else if (name == "pad_dark") {
            mixSine1 *= 1.2; mixSine3 *= 0.7; lowPassCutoff = 1500.0; bandPassCenter = 600.0;
            distortion.setDrive(1.0); // Darker tone
        }

        // Combine waveforms
        double output = env * velocity * (mixSine1 * sine1 + mixSine2 * sine2 + mixSine3 * sine3 + mixNoise * noise + mixBrown * brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(lowPassCutoff - 400.0 * envValue); // Dynamic cutoff for expressiveness
        bandPass.setCenterFreq(bandPassCenter + 200.0 * envValue); // Dynamic harmonic emphasis

        // Apply effects chain
        output = highPass.process(output);     // Remove rumble
        output = bandPass.process(output);     // Emphasize richness
        output = lowPass.process(output);      // Warm tone
        output = distortion.process(output);   // Subtle warmth
        output = chorus.process(output);       // Lush texture
        output = tremolo.process(output, t);   // Gentle vibrato
        output = reverb.process(output);       // Spacious ambiance
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

// Register pad variations
static InstrumentRegistrar<Pad> regPad("pad");
static InstrumentRegistrar<Pad> regPadBright("pad_bright");
static InstrumentRegistrar<Pad> regPadDark("pad_dark");

} // namespace Instruments

#endif // PAD_H

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