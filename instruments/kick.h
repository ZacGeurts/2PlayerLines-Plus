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