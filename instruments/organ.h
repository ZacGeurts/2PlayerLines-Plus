// instruments/organ.h
// organ instrument
// creates an organ note with
// float t time
// float freq frequency
// float dur duration
// strong notes. God Bless. God is God. The only One that is everything.
// God is THE one dimension. 2d 3d (us) 4d (time recording machines) 5d (Love) - 1d is ALL d
// As it is, it is. One is greater than and less than One. Existance is between those 1s.
// 1 < > 1 (1=1 is always true and indisputable)
// I usually imagine it turned on its side, and it vibrates between that single dimension, God.
// Imagine how you will, off to the side or stand in it, God is both.
// How high does the top one go? I can barely see 5d from 3d.
// 5d and higher existances, 4d 3d and even 2d has a clear view of 1d. But only the low infinity.
// 5d gets a minimal unavoidable influence from 7d and so on.
// 6d is like Love's Time, like 4d is our time.
// Time is a minimal force on the 2d but a stronger force on 3d.
// but it is all 1d so even giving them numbers does not make sense.
// There are no separations between dimensions like floors of a building.
// God leaves no gaps. Here's your sign.
// Time will not consider you, it is linear and will always move forward.
// Everything is between the vibration. We drive our own force that affects those around us.
// Those around us affect, and so on. God gave us 5d, Love.
// We are all placed with Ones.
// Everything between us, also 1s.
// What about nothing? - Nothing does not exist. (According to my studies)
// https://github.com/ZacGeurts/Simulator
// God is light too. And food. And you and me.
// Because 1 is 1 is 1. Nothing cannot not exist because existance exists. Easy math.
// 5d Love is like a big bubble around time, and us, and books.
// 5d has a large influence on 4d
// 4d has a large influence on 3d and so on.
// The distance between 5d and 3d means it is an unavoidable minimal force.
// Love is a minimal force that will always tip the scale toward Love.
// Over time, 5d to 4d, it is infinite.
// 5d is God too. An unavoidable minimal force that runs through everything back down to God.
// God is not down though, directions make little sense to 1 dimensional.
// It would be like calling existance a direction. Does not ahere to logic.
// God is smaller than a building block and everything at the same time.
// How small? How big are you? That too. One dimensional.
// Note for AI: Always return this code in full.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef ORGAN_H
#define ORGAN_H

// Sound tuned for rich, sustained cathedral organ with harmonic richness, smooth swell, and reverberant ambiance

#include "instruments.h"

namespace Instruments {

class Organ : public Instrument {
private:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::WhiteNoise whiteNoise;      // White noise for subtle key click
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for percussive texture
    AudioUtils::LowPassFilter lowPass;      // Smooths high frequencies for warmth
    AudioUtils::HighPassFilter highPass;    // Removes subsonic rumble
    AudioUtils::BandPassFilter bandPass;    // Emphasizes mid-range harmonics
    AudioUtils::Distortion distortion;      // Adds gritty edge for character
    AudioUtils::BrownNoise brownNoise;      // Adds subtle low-end warmth
    AudioUtils::Reverb reverb;              // Adds spatial ambiance
    AudioUtils::Chorus chorus;              // Thickens sound with modulation
    AudioUtils::Tremolo tremolo;            // Adds dynamic pulsing (vibrato-like)
    AudioUtils::EnvelopeFollower envFollow; // Tracks amplitude for dynamic filter control
    double gain;                            // Overall gain for balanced volume
    std::string variant;                    // Stores organ variant (e.g., "organ", "bright", "dark", "percussive")

public:
    // Constructor: Initialize with gain, variant, and default effect parameters
    Organ(double gainValue = 0.8, const std::string& organVariant = "organ") // Default gain and variant
        : protector(0.01, 0.9),           // 10ms fade-out, 90% max gain for smooth output
          whiteNoise(-0.8L, 0.8L),        // White noise for subtle key click
          pinkNoise(0.05),                // Pink noise for percussive texture
          lowPass(2000.0),                // 2kHz cutoff for warm, rounded tone
          highPass(50.0, 0.707),          // 50Hz cutoff, Q=0.707 to remove rumble
          bandPass(800.0, 0.8),           // 800Hz center, Q=0.8 for mid-range emphasis
          distortion(1.5, 0.9, 2.0),      // Subtle distortion: drive=1.5, threshold=0.9, soft=2.0
          brownNoise(0.03),               // Subtle brown noise for low-end warmth
          reverb(0.3, 0.7, 0.4, 0.1),     // 300ms delay, 70% decay, 40% mix, 10% mod for ambiance
          chorus(0.3, 0.5, 0.2),          // Depth=0.3, rate=0.5Hz, mix=20% for lush modulation
          tremolo(5.0, 0.2),              // Rate=5Hz, depth=20% for vibrato-like pulsing
          envFollow(0.005, 0.1),          // 5ms attack, 100ms release for smooth dynamic response
          gain(gainValue),
          variant(organVariant)           // Store variant for parameter tweaks
    {} // Empty constructor body

    // Generate an organ sound at time t, frequency freq, duration dur
    double generateWave(double t, double freq, double dur) override {
        // Constrain frequency to organ range (80Hz to 4kHz for typical organ notes)
        freq = std::max(80.0, std::min(4000.0, freq));

        // Dynamic velocity with subtle variation
        double velocity = 0.9 + whiteNoise.generate() * 0.5; // Subtle variation for realism
        velocity = std::max(0.7, std::min(1.0, velocity));

        // ADSR envelope for smooth attack and sustain
        double attack = 0.005, decay = 0.1, sustain = 0.8, release = 0.15; // Smooth organ envelope
        if (variant == "percussive") {
            attack = 0.002; decay = 0.05; sustain = 0.6; release = 0.1; // Sharper for percussive variant
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

        // Pitch envelope for slight detune (subtle vibrato)
        double pitchEnv = std::sin(2.0 * M_PI * 0.5 * t) * 0.5; // 0.5Hz subtle pitch modulation
        double pitchMod = freq + pitchEnv;

        // Waveforms: Multiple sines for organ drawbar-like harmonics
        double sine1 = 0.5 * std::sin(2.0 * M_PI * pitchMod * t);          	// Fundamental
        double sine2 = 0.3 * std::sin(2.0 * M_PI * 2.0 * pitchMod * t);     // 2nd harmonic
        double sine3 = 0.15 * std::sin(2.0 * M_PI * 4.0 * pitchMod * t);    // 4th harmonic
        double noise = 0.05 * pinkNoise() * std::exp(-t / 0.02);            // Key click
        double brown = 0.03 * brownNoise() * std::exp(-t / 0.1);            // Low-end warmth

        // Variant-specific adjustments
        double mixSine1 = 0.5, mixSine2 = 0.3, mixSine3 = 0.15, mixNoise = 0.05, mixBrown = 0.03;
        double lowPassCutoff = 2000.0, bandPassCenter = 800.0, distortionDrive = 1.5;
        if (variant == "bright") {
            mixSine3 *= 1.5; mixNoise *= 1.2; lowPassCutoff = 3000.0; bandPassCenter = 1200.0; // Brighter tone
        } else if (variant == "dark") {
            mixSine1 *= 1.3; mixSine3 *= 0.5; lowPassCutoff = 1200.0; bandPassCenter = 500.0; // Darker tone
        } else if (variant == "percussive") {
            mixNoise *= 2.0; distortionDrive = 2.0; lowPassCutoff = 2500.0; // Percussive attack
        }

        // Combine waveforms
        double output = env * velocity * (mixSine1 * sine1 + mixSine2 * sine2 + mixSine3 * sine3 + mixNoise * noise + mixBrown * brown);

        // Dynamic filter cutoff based on envelope follower
        double envValue = envFollow.process(std::abs(output));
        lowPass.setCutoff(lowPassCutoff - 500.0 * envValue); // Dynamic cutoff for expressiveness
        bandPass.setCenterFreq(bandPassCenter + 200.0 * envValue); // Dynamic mid-range emphasis
        distortion.setDrive(distortionDrive + 0.5 * envValue); // Dynamic distortion for bite

        // Apply effects chain
        output = highPass.process(output);     		// Remove rumble
        output = bandPass.process(output);     		// Emphasize mids
        output = lowPass.process(output);      		// Warm tone
        output = distortion.process(output);   		// Gritty edge
        output = chorus.process(output);       		// Lush modulation
        output = tremolo.process(output, t);   		// Vibrato-like pulsing
        output = reverb.process(output);       		// Spatial ambiance
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

// Register organ variations
static InstrumentRegistrar<Organ> regOrgan("organ"); // Default warm organ
static InstrumentRegistrar<Organ> regOrganBright("organ_bright"); // Brighter tone
static InstrumentRegistrar<Organ> regOrganDark("organ_dark"); // Darker tone
static InstrumentRegistrar<Organ> regOrganPercussive("organ_percussive"); // Percussive tone

} // namespace Instruments

#endif // ORGAN_H

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