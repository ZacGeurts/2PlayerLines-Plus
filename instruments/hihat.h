// instruments/hihat.h
// hi-hat instrument (open and closed variants)
// creates a hi-hat note with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef HIHAT_H
#define HIHAT_H

// The sound is tuned for crisp, metallic hi-hat with sharp attack (closed) or sustained shimmer (open)

#include "instruments.h"

namespace Instruments {

class HiHat : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for metallic texture
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for natural hi-hat shimmer
    AudioUtils::BandPassFilter filter;      // Isolates hi-hat frequency range
    AudioUtils::HighPassFilter highPass;    // Removes low-end for crispness
    AudioUtils::Distortion distortion;      // Adds metallic grit
    AudioUtils::Tremolo tremolo;            // Adds dynamic pulsing for open hi-hat
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain; // Overall gain for balanced volume
    bool isOpen; // Flag to toggle between open and closed hi-hat

public:
    // Constructor: Initialize with gain, open/closed state, and default effect parameters
    HiHat(double gainValue = 0.9, bool open = false) // Higher gain for clarity, default to closed
        : protector(0.004, 0.95),       // 4ms fade-out, 95% max gain for tight control
          whiteNoise(-1.0L, 1.0L),      // White noise for metallic texture
          pinkNoise(0.1),               // 10% amplitude for shimmer
          filter(6000.0, 0.8),          // 6kHz center, Q=0.8 for focused hi-hat range
          highPass(2000.0, 0.707),      // 2kHz cutoff for crispness
          distortion(1.5, 0.9, 1.2),    // Light distortion: drive=1.5, threshold=0.9, soft=1.2
          tremolo(4.0, 0.05),           // Fast, subtle tremolo: rate=4Hz, depth=5%
          envFollow(0.001, 0.02),       // Fast envelope: 1ms attack, 20ms release
          gain(gainValue),              // Volume 0.0 through 1.0
          isOpen(open)                  // Set open or closed hi-hat
    {} // Empty constructor body

    // Set whether hi-hat is open or closed
    void setOpen(bool open) {
        isOpen = open;
    }

    // Generate a hi-hat sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to hi-hat range (3kHz to 8kHz for metallic tone)
        freq = std::max(3000.0, std::min(8000.0, freq));

        // Dynamic velocity with random variation
        double velocity = 0.9 + whiteNoise.generate() * 0.1;
        velocity = std::max(0.6, std::min(1.0, velocity));

        // ADSR envelope: sharp for closed, sustained for open
        double attack = 0.001, decay = isOpen ? 0.1 : 0.01, sustain = isOpen ? 0.4 : 0.1, release = isOpen ? 0.3 : 0.05;
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

        // Pitch envelope for slight metallic sweep
        double pitchEnv = std::exp(-t / (isOpen ? 0.1 : 0.02)) * 500.0; // Faster for closed
        double pitchMod = freq + pitchEnv;

        // Waveforms: Noise-based for hi-hat texture
        double noise = 0.6 * whiteNoise.generate(); // Primary metallic noise
        double pink = 0.3 * pinkNoise() * std::exp(-t / (isOpen ? 0.2 : 0.03)); // Shimmer, longer for open
        double harmonic = 0.1 * std::sin(2.0 * M_PI * pitchMod * t) * std::exp(-t / (isOpen ? 0.15 : 0.02));

        // Combine waveforms
        double output = env * velocity * (noise + pink + harmonic);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        filter.setCenterFreq(5000.0 + 2000.0 * envValue); // 5kHz-7kHz for dynamic brightness

        // Apply effects chain
        output = distortion.process(output); // Add metallic grit
        output = filter.process(output);     // Focus hi-hat range
        output = highPass.process(output);   // Remove low-end
        output = tremolo.process(output, t); // Subtle pulsing, more pronounced for open
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

// Register the hi-hat instruments
static InstrumentRegistrar<HiHat> regHiHatClosed("hihat_closed");
static InstrumentRegistrar<HiHat> regHiHatOpen("hihat_open");

} // namespace Instruments

#endif // HIHAT_H

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