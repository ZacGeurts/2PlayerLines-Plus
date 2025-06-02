// instruments/kick.h
// kick drum instrument
// creates a kick drum note with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef KICK_H
#define KICK_H

// The sound is tuned for deep, punchy kick drum with sharp attack and low-end thud

#include "instruments.h"

namespace Instruments {

class Kick : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for velocity variation
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for attack texture
    AudioUtils::LowPassFilter filter;       // Smooths high frequencies for warmth
    AudioUtils::Distortion distortion;      // Adds punchy edge
    AudioUtils::BrownNoise brownNoise;      // Adds low-end resonance
    AudioUtils::Chorus chorus;              // Thickens sound with modulation
    AudioUtils::Tremolo tremolo;            // Adds dynamic pulsing
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain; // Overall gain for balanced volume

public:
    // Constructor: Initialize with gain and default effect parameters
    Kick(double gainValue = 0.9)        // Increased gain slightly for more presence
        : protector(0.005, 0.95),       // 5ms fade-out, 95% max gain for tighter control and headroom
          whiteNoise(-1.2L, 1.2L),      // Slightly reduced range for subtler velocity variation
          pinkNoise(0.1),               // Increased to 10% for more pronounced attack texture
          filter(160.0),                // Lowered to 160Hz for deeper, warmer low-end
          distortion(2.0, 0.8, 1.8),    // Adjusted: drive=2.0, threshold=0.8, soft=1.8 for punchier distortion
          brownNoise(0.05),             // Increased to 5% for richer low-end resonance
          chorus(0.2, 0.1, 0.1),        // Slightly stronger chorus: depth=0.2, rate=0.1Hz, mix=10% for thickness
          tremolo(2.0, 0.03),           // Faster, subtler tremolo: rate=2.0Hz, depth=3% for dynamic pulse
          envFollow(0.0015, 0.02),      // Faster envelope: 1.5ms attack, 20ms release for tighter response
          gain(gainValue)               // Volume 0.0 through 1.0
    {} // Empty constructor body
    
    // Generate a kick drum sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to kick range (40Hz to 140Hz for deeper low-end thud)
        freq = std::max(40.0, std::min(140.0, freq));

        // Dynamic velocity with random variation
        double velocity = 0.9 + whiteNoise.generate(); // Slightly lower base velocity for consistency
        velocity = std::max(0.6, std::min(1.0, velocity)); // Tighter range for controlled dynamics

        // ADSR envelope for sharper attack and tighter decay
        double attack = 0.001, decay = 0.035, sustain = 0.2, release = 0.06; // Tighter envelope for punch
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

        // Pitch envelope for dynamic sweep (faster decay from +30Hz to base freq)
        double pitchEnv = std::exp(-t / 0.015) * 30.0; // Faster, stronger sweep for punchier attack
        double pitchMod = freq + pitchEnv;

        // Waveforms: Sine for low-end, slight noise for attack
        double sine = 0.7 * std::sin(2.0 * M_PI * pitchMod * t); // Increased sine for stronger low-end
        double noise = 0.2 * pinkNoise() * std::exp(-t / 0.025); // Tighter noise decay for crisp attack
        double harmonic = 0.1 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t) * std::exp(-t / 0.035); // Slightly stronger harmonic

        // Add pink noise for attack texture and brown noise for resonance
        double pink = 0.15 * pinkNoise() * std::exp(-t / 0.025); // Increased for sharper attack
        double brown = 0.08 * brownNoise(); // Increased for deeper resonance

        // Combine waveforms
        double output = env * velocity * (sine + noise + harmonic + pink + brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        filter.setCutoff(120.0 + 80.0 * envValue); // 120-200Hz cutoff for deeper, dynamic low-end

        // Apply effects chain
        output = distortion.process(output); // Punchy edge
        output = filter.process(output);     // Deep tone
        output = chorus.process(output);     // Thicken sound
        output = tremolo.process(output, t); // Dynamic pulsing
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

// Register the Kick instrument
static InstrumentRegistrar<Kick> regKick("kick");

} // namespace Instruments

#endif // KICK_H

/*
 * AudioUtils Namespace Overview
 * ============================
 * The AudioUtils namespace contains a suite of audio processing utilities designed
 * for high-quality instrument synthesis.
 * All utilities operate at maximum DEFAULT_SAMPLE_RATE (44100.0f)
 *
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
 *      RandomGenerator rng(-1.0L, 1.0L);			// Initialize random generator
 *      int diceroll = rng.roll_d20();				// Roll a 20-sided die
 *      int diceroll = rng.roll_dice(min, max);		// Specify range to random
 *		float numberfloat = rng.random_float();		// Return a float
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
 *      double output = hpf.process(input);			// Process input signal
 *      hpf.setCutoff(150.0);						// Update cutoff
 *
 * 3. LowPassFilter
 *    - Purpose: Removes high frequencies above a cutoff, used for warming bass tones.
 *    - Features:
 *      - First-order filter with dynamic cutoff.
 *      - Precomputed smoothing factor for efficiency.
 *      - State reset for stability.
 *    - Call Example:
 *      LowPassFilter lpf(250.0);					// 250Hz cutoff
 *      double output = lpf.process(input);			// Process input signal
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
 *      double output = bpf.process(input);			// Process input signal
 *      bpf.setCenterFreq(1200.0);					// Update center frequency
 *
 * 5. Reverb
 *    - Purpose: Adds spatial ambiance, simulating room reflections.
 *    - Features:
 *      - Delay-based reverb with modulated feedback for richer sound.
 *      - Adjustable delay time, decay factor, wet/dry mix, and feedback modulation.
 *      - Automatic buffer resizing and state reset for stability.
 *    - Call Example:
 *      Reverb reverb(0.2, 0.6, 0.4, 0.1);			// 200ms delay, 60% decay, 40% mix, 10% mod
 *      double output = reverb.process(input);		// Process input signal
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
 *      double output = dist.process(input);		// Process input signal
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
 *      double output = protector.process(input, 0.1, 1.0);	// Process with t=0.1s, dur=1s
 *      protector.setMaxGain(0.9);					// Update max gain
 *
 * 8. WhiteNoise
 *     - Purpose: Generates white noise for velocity variation
 *       or stochastic effects in instrument synthesis.
 *     - Features:
 *       - Uses RandomGenerator for high-quality randomness.
 *       - Configurable range [min, max].
 *       - Output clamping for stability.
 *     - Call Example:
 *       WhiteNoise white(-1.0L, 1.0L);				// range
 *       double output = white.process();			// Generate white noise
 *
 * 9. BrownNoise
 *    - Purpose: Generates brown (1/f²) noise for deep, rumbly textures like amp hum
 *      or room ambiance, enhancing bass guitar realism.
 *    - Features:
 *      - Integrates RandomGenerator’s white noise for Brownian motion.
 *      - Adjustable amplitude scale.
 *      - State reset to prevent drift.
 *    - Call Example:
 *      BrownNoise brown(0.03);						// 3% amplitude scale
 *      double output = brown.process();			// Generate brown noise
 *		brown.reset();								// Reset state
 *
 * 10. Chorus
 *    - Purpose: Thickens sound with modulated delay, simulating doubled tracks
 *      or detuning for a richer tone.
 *    - Features:
 *      - Variable delay (5-10ms) with dynamic depth, rate, and mix.
 *      - Linear interpolation for smooth modulation.
 *      - Buffer management and state reset.
 *    - Call Example:
 *      Chorus chorus(0.4, 0.3, 0.25);				// Depth=0.4, rate=0.3Hz, mix=25%
 *      double output = chorus.process(input);		// Process input signal
 *      chorus.reset();								// Reset state
 *
 * 11. Tremolo
 *     - Purpose: Adds amplitude pulsing for dynamic bass effects, emulating expressive
 *       playing or amp tremolo.
 *     - Features:
 *       - Sinusoidal amplitude modulation with dynamic rate and depth.
 *       - Lightweight and efficient.
 *     - Call Example:
 *       Tremolo tremolo(3.0, 0.3);					// Rate=3Hz, depth=30%
 *       double output = tremolo.process(input, 0.1); // Process with t=0.1s
 *
 * 12. EnvelopeFollower
 *     - Purpose: Tracks signal amplitude to dynamically control effects (e.g., filter
 *       cutoff), adding responsiveness to bass dynamics.
 *     - Features:
 *       - Adjustable attack and release times for responsive envelope tracking.
 *       - Smooth envelope tracking with stateful smoothing and reset capability.
 *     - Call Example:
 *       EnvelopeFollower envFollow(0.005, 0.05);	// 5ms attack, 50ms release
 *       double env = envFollow.process(input);		// Track amplitude with smoothing
 *       envFollow.reset();							// Reset state to 0
 *
 * 13. PinkNoise
 *     - Purpose: Generates pink (1/f) noise for natural string buzz or ambient texture,
 *       enhancing instrument realism.
 *     - Features:
 *       - Three-stage filter on white noise for 1/f spectrum.
 *       - Adjustable amplitude scale.
 *       - State reset for filter stability.
 *     - Call Example:
 *       PinkNoise pink(0.05);						// 5% amplitude
 *       double output = pink.process();			// Generate pink noise
 *
 * Notes
 * -----
 * - All utilities use DEFAULT_SAMPLE_RATE (44100.0f) for SDL2 maximum available.
 * - Utilities are thread-safe where applicable and include validation and clamping for robustness.
 * - Designed with real-time audio in mind.
 * - Enhances synthesis by adding noise textures, dynamic effects, and
 *   with professional polish.
 */