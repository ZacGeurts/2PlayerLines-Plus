// instruments/leadsynth.h
// lead synth instrument (single variant)
// creates a lead synth note with
// float t time
// float freq frequency
// float dur duration
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef LEADSYNTH_H
#define LEADSYNTH_H

// The sound is tuned for a bold, sustained lead with rich harmonics and subtle vibrato

#include "instruments.h"

namespace Instruments {

class LeadSynth : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::LowPassFilter lowPass;      // Smooths high-end for warmth
    AudioUtils::Distortion distortion;      // Adds harmonic richness
    AudioUtils::Reverb reverb;              // Adds spatial depth
    AudioUtils::Chorus chorus;              // Thickens the lead sound
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain; // Overall gain for balanced volume
    double vibrato_phase; // Tracks vibrato phase

public:
    LeadSynth(double gainValue = 0.9) // Higher gain for prominent lead
        : protector(0.008, 0.85),      // 8ms fade-out, 85% max gain for smooth control
          lowPass(8000.0),             // 8kHz cutoff for warm lead tone
          distortion(1.6, 0.9, 1.3),   // Light distortion: drive=1.6, threshold=0.9, soft=1.3
          reverb(0.2, 0.6, 0.3, 0.1),  // Subtle reverb for ambiance
          chorus(0.4, 0.3, 0.25),      // Chorus for thickness: depth=0.4, rate=0.3Hz, mix=25%
          envFollow(0.005, 0.05),      // Smooth envelope: 5ms attack, 50ms release
          gain(gainValue),             // Volume 0.0 through 1.0
          vibrato_phase(0.0)           // Initialize vibrato phase
    {}

    // Generate a lead synth sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to lead range (200Hz to 5kHz for melodic leads)
        freq = std::max(200.0, std::min(5000.0, freq));

        // Vibrato for expressive lead
        double vibrato_rate = 5.0; // Hz
        double vibrato_depth = 0.02;
        double dt = 1.0 / AudioUtils::DEFAULT_SAMPLE_RATE;
        vibrato_phase += 2.0 * M_PI * vibrato_rate * dt;
        if (vibrato_phase > 2.0 * M_PI) vibrato_phase -= 2.0 * M_PI;
        double vibrato = 1.0 + vibrato_depth * std::sin(vibrato_phase);
        double pitchMod = freq * vibrato;

        // ADSR envelope: smooth attack, long sustain for lead
        double attack = 0.05, decay = 0.2, sustain = 0.9, release = 0.4;
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

        // Waveforms: Sawtooth and square for rich lead tone
        double saw = 0.0, square = 0.0;
        const int num_harmonics = 20;
        for (int i = 1; i <= num_harmonics; ++i) {
            double harmonic_freq = pitchMod * i;
            if (harmonic_freq > 20000.0) break;
            saw += std::sin(2.0 * M_PI * harmonic_freq * t) / i;
            if (i % 2 == 1) square += std::sin(2.0 * M_PI * harmonic_freq * t) / i;
        }
        saw *= 0.5; // Sawtooth component
        square *= 0.3; // Square wave component

        // Combine waveforms
        double output = env * (saw + square);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(6000.0 + 3000.0 * envValue); // 6kHz-9kHz for dynamic brightness

        // Apply effects chain
        output = distortion.process(output); // Add harmonic richness
        output = chorus.process(output);     // Thicken sound
        output = lowPass.process(output);    // Smooth high-end
        output = reverb.process(output);     // Add spatial depth
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

// Register the lead synth instrument
static InstrumentRegistrar<LeadSynth> regLeadSynth("leadsynth");

} // namespace Instruments

#endif // LEADSYNTH_H

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