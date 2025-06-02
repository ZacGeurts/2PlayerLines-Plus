// instruments/vocal.h
// vocal instrument (male and female variants)
// creates a vocal note with
// float t time
// float freq frequency
// int phoneme for phoneme selection
// float dur duration
// int depth for male/female voice
// Note for AI: Always return this code in full, including comments.
// Note for AI: do not add additional includes as instruments.h handles everything.
#ifndef VOCAL_H
#define VOCAL_H

#include "instruments.h"

namespace Instruments {

struct VocalState {
    double current_freq = 0.0;
    double current_dur = 0.0;
    double start_time = 0.0;
    bool is_new_note = true;
    double prev_output = 0.0;
    double prev_time = -1.0;
    double vibrato_phase = 0.0;
};

class Vocal : public Instrument {
protected:
    AudioUtils::AudioProtector protector;   // Protects output from clipping and DC offset
    AudioUtils::PinkNoise pinkNoise;        // Pink noise for breathiness
    AudioUtils::LowPassFilter lowPass;      // Smooths high frequencies for vocal warmth
    AudioUtils::Distortion distortion;      // Adds subtle grit for vocal texture
    AudioUtils::Reverb reverb;              // Adds spatial ambiance
    FormantFilter formant1;					// First formant for vowel shaping
    FormantFilter formant2;					// Second formant for vowel shaping
    VocalState state;                       // Tracks note state
    double gain;                            // Overall gain for balanced volume
    int depth;                              // 0 for male, 1 for female

public:
    Vocal(double gainValue = 0.8, int depthValue = 0) // Moderate gain, default to male
        : protector(0.005, 0.9),       // 5ms fade-out, 90% max gain for tight control
          pinkNoise(0.1),              // 10% amplitude for breathiness
          lowPass(5000.0),             // 5kHz cutoff for vocal warmth
          distortion(1.8, 0.9, 1.3),   // Light distortion: drive=1.8, threshold=0.9, soft=1.3
          reverb(0.3, 0.7, 0.35, 0.1), // Moderate reverb for ambiance
          formant1(400.0, 140.0),      // Default formant 1 (tuned for 'a' vowel)
          formant2(900.0, 180.0),      // Default formant 2
          gain(gainValue),             // Volume 0.0 through 1.0
          depth(depthValue)            // 0 for male, 1 for female
    {}

    // Generate a vocal sound (non-phoneme interface, defaults to phoneme 0)
    double generateWave(double t, double freq, double dur) override {
        return generateWave(t, freq, 0, dur, depth);
    }

    // Generate a vocal sound with phoneme and depth
    double generateWave(double t, double freq, int phoneme, double dur, int depthOverride) override {
        // Update state for new note
        if (freq != state.current_freq || t < state.prev_time || t >= state.start_time + state.current_dur + 1.5) {
            state.is_new_note = true;
            state.current_freq = freq;
            state.current_dur = dur;
            state.start_time = t;
            state.vibrato_phase = 0.0;
        }
        state.prev_time = t;

        // ADSR envelope: smooth for vocal sustain
        double attack = 0.08, decay = 0.2, sustain = 0.85, release = 0.3;
        double t_current = t - state.start_time;
        double env_current = 0.0;
        if (t_current < attack) {
            env_current = t_current / attack;
        } else if (t_current < attack + decay) {
            env_current = 1.0 - (t_current - attack) / decay * (1.0 - sustain);
        } else if (t_current < dur) {
            env_current = sustain;
        } else if (t_current < dur + release) {
            double t_rel = (t_current - dur) / release;
            env_current = sustain * (1.0 - t_rel) * std::exp(-5.0 * t_rel);
        }
        env_current = std::max(0.0, env_current);

        // Crossfade for smooth note transitions
        double env_prev = 0.0;
        double crossfade_dur = 0.05;
        if (state.is_new_note && t_current < crossfade_dur) {
            double t_prev = t - (state.start_time - crossfade_dur);
            if (t_prev >= 0.0 && t_prev < state.current_dur + release) {
                if (t_prev < attack) {
                    env_prev = t_prev / attack;
                } else if (t_prev < attack + decay) {
                    env_prev = 1.0 - (t_prev - attack) / decay * (1.0 - sustain);
                } else if (t_prev < state.current_dur) {
                    env_prev = sustain;
                } else if (t_prev < state.current_dur + release) {
                    double t_rel = (t_prev - state.current_dur) / release;
                    env_prev = sustain * (1.0 - t_rel) * std::exp(-5.0 * t_rel);
                }
            }
        }

        // Voice characteristics
        double formant_scale = (depthOverride == 0) ? 0.9 : 1.1; // Lower for male, higher for female
        double breath_amount = (depthOverride == 0) ? 0.2 : 0.3; // More breath for female
        double vibrato_depth = (depthOverride == 0) ? 0.02 : 0.03; // Stronger vibrato for female
        double vibrato_rate = 5.5; // Hz
        double base_freq = freq * (depthOverride == 0 ? 0.8 : 1.2); // Lower for male, higher for female
        base_freq = std::max(20.0, base_freq);

        // Phoneme selection
        double f1, f2, bw1, bw2;
        if (phoneme >= 0 && phoneme < 14) {
            static const struct { double f1; double f2; double bw1; double bw2; } phonemes[] = {
                { 400.0,  900.0,  140.0, 180.0}, // a (as in "father")
                { 600.0,  800.0,  150.0, 170.0}, // e (as in "bed")
                { 350.0, 1000.0,  130.0, 190.0}, // i (as in "see")
                { 300.0,  700.0,  160.0, 200.0}, // o (as in "go")
                { 500.0,  950.0,  140.0, 180.0}, // u (as in "blue")
                { 400.0,  800.0,  130.0, 170.0}, // æ (as in "cat")
                { 300.0,  600.0,  120.0, 160.0}, // ə (as in "about")
                { 450.0, 1100.0,  140.0, 180.0}, // ɪ (as in "bit")
                { 500.0, 1000.0,  150.0, 190.0}, // ɛ (as in "bet")
                { 350.0,  900.0,  130.0, 170.0}, // ʌ (as in "cut")
                { 250.0,  650.0,  160.0, 200.0}, // ɔ (as in "caught")
                { 200.0,  700.0,  150.0, 190.0}, // ʊ (as in "put")
                { 400.0,  800.0,  140.0, 180.0}, // ɑ (as in "hot")
                { 450.0,  850.0,  150.0, 190.0}  // ɒ (as in "law")
            };
            f1 = phonemes[phoneme].f1 * formant_scale;
            f2 = phonemes[phoneme].f2 * formant_scale;
            bw1 = phonemes[phoneme].bw1;
            bw2 = phonemes[phoneme].bw2;
        } else {
            // Fallback based on frequency
            double freq_normalized = (base_freq - (depthOverride == 0 ? 20.0 : 160.0)) /
                                     (depthOverride == 0 ? 90.0 - 20.0 : 300.0 - 160.0);
            freq_normalized = std::max(0.0, std::min(1.0, freq_normalized));
            int selected_phoneme = static_cast<int>(freq_normalized * 13);
            static const struct { double f1; double f2; double bw1; double bw2; } phonemes[] = {
                { 800.0, 2000.0,  80.0, 100.0}, // a
                { 600.0, 2700.0,  70.0,  90.0}, // e
                { 750.0, 1800.0,  90.0, 110.0}, // i
                { 550.0, 2800.0,  70.0,  90.0}, // o
                { 700.0, 2400.0,  80.0, 100.0}, // u
                { 500.0, 1500.0,  90.0, 110.0}, // æ
                { 800.0, 1400.0,  90.0, 110.0}, // ə
                { 400.0,  900.0,  80.0, 100.0}, // ɪ
                { 650.0, 2000.0,  80.0, 100.0}, // ɛ
                { 750.0, 1600.0,  90.0, 110.0}, // ʌ
                { 600.0, 2900.0,  70.0,  90.0}, // ɔ
                { 800.0, 1800.0,  90.0, 110.0}, // ʊ
                { 700.0, 2000.0,  80.0, 100.0}  // ɑ
            };
            f1 = phonemes[selected_phoneme].f1 * formant_scale;
            f2 = phonemes[selected_phoneme].f2 * formant_scale;
            bw1 = phonemes[selected_phoneme].bw1;
            bw2 = phonemes[selected_phoneme].bw2;
        }

        // Update formant filters
        formant1.setParameters(f1, bw1);
        formant2.setParameters(f2, bw2);

        // Generate source waveform (sawtooth with vibrato)
        double dt = 1.0 / AudioUtils::DEFAULT_SAMPLE_RATE;
        state.vibrato_phase += 2.0 * M_PI * vibrato_rate * dt;
        if (state.vibrato_phase > 2.0 * M_PI) state.vibrato_phase -= 2.0 * M_PI;
        double vibrato = 1.0 + vibrato_depth * std::sin(state.vibrato_phase);
        double modulated_freq = base_freq * vibrato;

        double saw = 0.0;
        const int num_harmonics = 25;
        for (int i = 1; i <= num_harmonics; ++i) {
            double harmonic_freq = modulated_freq * i;
            if (harmonic_freq > 20000.0) break;
            saw += std::sin(2.0 * M_PI * harmonic_freq * t) / (i * 1.2);
        }
        saw *= 1.5; // Increase amplitude for richer sound

        // Apply formant filters
        double formant1_out = formant1.process(saw) * 0.8;
        double formant2_out = formant2.process(saw) * 0.7;
        double vocal_current = (depthOverride == 0) ? (0.35 * saw + 0.65 * (formant1_out + formant2_out))
                                                   : (0.25 * saw + 0.75 * (formant1_out + formant2_out));

        // Add breathiness
        double breath = pinkNoise() * std::exp(-5.0 * t_current / dur) * breath_amount;
        double output_current = env_current * (vocal_current + breath);

        // Crossfade for note transitions
        double output_prev = state.prev_output;
        if (state.is_new_note && t_current < crossfade_dur) {
            double prev_base_freq = state.current_freq * (depthOverride == 0 ? 0.8 : 1.2);
            prev_base_freq = std::max(20.0, prev_base_freq);
            double prev_vibrato = 1.0 + vibrato_depth * std::sin(state.vibrato_phase);
            double prev_modulated_freq = prev_base_freq * prev_vibrato;

            double prev_saw = 0.0;
            for (int i = 1; i <= num_harmonics; ++i) {
                double harmonic_freq = prev_modulated_freq * i;
                if (harmonic_freq > 20000.0) break;
                prev_saw += std::sin(2.0 * M_PI * harmonic_freq * t) / (i * 1.2);
            }
            prev_saw *= 1.5;

            double prev_formant1_out = formant1.process(prev_saw) * 0.8;
            double prev_formant2_out = formant2.process(prev_saw) * 0.7;
            double prev_vocal = (depthOverride == 0) ? (0.35 * prev_saw + 0.65 * (prev_formant1_out + prev_formant2_out))
                                                    : (0.25 * prev_saw + 0.75 * (prev_formant1_out + prev_formant2_out));
            double prev_breath = pinkNoise() * std::exp(-5.0 * t_current / state.current_dur) * breath_amount;
            output_prev = env_prev * (prev_vocal + prev_breath);

            double crossfade_t = t_current / crossfade_dur;
            double smooth_t = 0.5 * (1.0 - std::cos(M_PI * crossfade_t));
            output_current = (1.0 - smooth_t) * output_prev + smooth_t * output_current;
            state.is_new_note = false;
        }

        // Apply effects chain
        if (depthOverride == 0) {
            output_current = distortion.process(output_current); // Grit for male voice
        }
        output_current = reverb.process(output_current); // Add ambiance
        output_current = lowPass.process(output_current); // Smooth highs
        output_current = protector.process(output_current, t_current, dur); // Protect output
        output_current *= gain;

        // Final clamp
        if (!std::isfinite(output_current)) {
            output_current = 0.0;
        }

        state.prev_output = output_current;
        return output_current;
    }
};

class VocalMale : public Vocal {
public:
    VocalMale() : Vocal(0.8, 0) {} // Male voice
};

class VocalFemale : public Vocal {
public:
    VocalFemale() : Vocal(0.8, 1) {} // Female voice
};

// Register the vocal instruments
static InstrumentRegistrar<VocalMale> regVocalMale("vocal_male");
static InstrumentRegistrar<VocalFemale> regVocalFemale("vocal_female");

} // namespace Instruments

#endif // VOCAL_H

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