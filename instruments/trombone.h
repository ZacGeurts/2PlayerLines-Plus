// instruments/trombone.h
// trombone instrument
// creates a trombone note with
// float t time
// float freq frequency
// float dur duration
#ifndef TROMBONE_H
#define TROMBONE_H

#include "instruments.h"

// Sound tuned for warm, brassy trombone with dynamic lip and bore resonance
// Sample rate assumed DEFAULT_SAMPLE_RATE at playback - SDL2 Maximum

namespace Instruments {

class Trombone : public Instrument {
private:
    AudioProtector protector;   // Protects output from clipping and DC offset
    RandomGenerator rng;        // High-quality noise generator (MegaMixMaxLite)
    WhiteNoise whiteNoise;      // White noise for velocity variation
    PinkNoise pinkNoise;        // Pink noise for breath texture
    LowPassFilter filter;       // Smooths high frequencies for warmth
    Distortion distortion;      // Adds brassy edge
    BrownNoise brownNoise;      // Adds subtle body resonance
    Chorus chorus;              // Thickens sound with modulation
    Tremolo tremolo;            // Adds dynamic pulsing
    EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain;                // Overall gain for balanced volume

public:
    // Constructor: Initialize with gain and default effect parameters
    Trombone(double gainValue = 0.85)
        : protector(0.01, 0.9),          // 10ms fade-out, 90% max gain
          rng(-0.02, 0.02),             // Noise generator
          whiteNoise(),                 // White noise for velocity (±1)
          pinkNoise(0.02),              // Pink noise for breath texture (2%)
          filter(2000.0),               // 2kHz cutoff for warm, brassy tone
          distortion(2.0, 0.8, 2.0),    // Brassy distortion: drive=2.0, threshold=0.8, soft=2.0
          brownNoise(0.01),             // Subtle brown noise for resonance
          chorus(0.25, 0.15, 0.2),      // Light chorus: depth=0.25, rate=0.15Hz, mix=20%
          tremolo(3.0, 0.2),            // Subtle tremolo: rate=3Hz, depth=20%
          envFollow(0.008, 0.06),       // Smooth envelope: 8ms attack, 60ms release
          gain(gainValue) {
        gain = std::max(0.01, std::min(1.0, gainValue)); // Clamp gain
    }

    // Generate a trombone note at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to trombone range (E2 to Bb4: 82.4Hz to 466.2Hz)
        freq = std::max(82.4, std::min(466.2, freq));

        // Dynamic velocity with random variation
        double velocity = 0.95 + whiteNoise();
        velocity = std::max(0.5, std::min(1.0, velocity));

        // ADSR envelope for smooth attack and sustained tone
        double attack = 0.02, decay = 0.1, sustain = 0.85, release = 0.15;
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

        // Vibrato: 5Hz modulation after 0.1s
        double vibratoDepth = t > 0.1 ? 0.005 * (1.0 + 0.3 * t / dur) : 0.0;
        double vibrato = std::sin(2.0 * M_PI * 5.0 * t) * vibratoDepth;
        double pitchMod = freq * (1.0 + vibrato);

        // Waveforms: Sawtooth for brightness, square for brassy character
        double saw = 0.4 * (std::fmod(pitchMod * t, 1.0) - 0.5);
        double square = 0.3 * (std::sin(2.0 * M_PI * pitchMod * t) +
                              0.15 * std::sin(2.0 * M_PI * 3.0 * pitchMod * t));
        double harmonic = 0.1 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t);

        // Add pink noise for breath and brown noise for resonance
        double pink = pinkNoise() * std::exp(-t / 0.1);
        double brown = brownNoise() * std::exp(-t / 0.15);

        // Combine waveforms
        double output = env * velocity * (saw + square + harmonic + pink + brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(output);
        filter.setCutoff(1800.0 + 200.0 * envValue); // 1.8-2kHz cutoff

        // Apply effects chain
        output = distortion.process(output); // Brassy edge
        output = filter.process(output);     // Warm tone
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

// Register the Trombone instrument
static InstrumentRegistrar<Trombone> regTrombone("trombone");

} // namespace Instruments

#endif // TROMBONE_H

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