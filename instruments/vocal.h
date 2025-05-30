// instruments/vocal.h
// Vocal instrument
// Creates a vocal note with
// float t time
// float freq frequency
// int phoneme for phoneme selection
// float dur duration
// int depth for male/female voice
#ifndef VOCAL_H
#define VOCAL_H

#include "instruments.h"

namespace Instruments {

struct VocalState {
    float current_freq = 0.0f;
    float current_dur = 0.0f;
    float start_time = 0.0f;
    bool is_new_note = true;
    float prev_output = 0.0f;
    float prev_time = -1.0f;
    float vibrato_phase = 0.0f;
};

class Vocal : public Instrument {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter;
    AudioUtils::Distortion distortion;
    VocalState male_state;
    VocalState female_state;
    FormantFilter formant1;
    FormantFilter formant2;
    float gain;
    float sampleRate;

public:
    Vocal(float gain = 0.2f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          reverb(0.3f, 0.7f, 0.35f),
          filter(5000.0f),
          distortion(1.8f, 0.75f),
          formant1(400.0f, 140.0f),
          formant2(900.0f, 180.0f),
          gain(gain),
          sampleRate(sampleRate) {}

    float generateWave(float t, float freq, float dur) override {
        return 0.0f; // Non-vocal interface not used
    }

    float generateWave(float t, float freq, int phoneme, float dur, int depth) override {
        VocalState& state = (depth == 0) ? male_state : female_state;
        bool is_new_note = (freq != state.current_freq || t < state.prev_time ||
                           t >= state.start_time + state.current_dur + 1.5f);
        if (is_new_note) {
            state.is_new_note = true;
            state.current_freq = freq;
            state.current_dur = dur;
            state.start_time = t;
            state.vibrato_phase = 0.0f;
        }
        state.prev_time = t;

        // ADSR envelope
        float attack = 0.08f, decay = 0.2f, sustain = 0.85f, release = 0.3f;
        float env_current = 0.0f, env_prev = 0.0f;
        float crossfade_dur = 0.05f;
        float t_current = t - state.start_time;

        if (t_current < attack) {
            env_current = t_current / attack;
        } else if (t_current < attack + decay) {
            env_current = 1.0f - (t_current - attack) / decay * (1.0f - sustain);
        } else if (t_current < dur) {
            env_current = sustain;
        } else if (t_current < dur + release) {
            float t_rel = (t_current - dur) / release;
            env_current = sustain * (1.0f - t_rel) * std::exp(-t_rel * 5.0f);
        } else {
            env_current = 0.0f;
        }

        if (state.is_new_note && t_current < crossfade_dur) {
            float t_prev = t - (state.start_time - crossfade_dur);
            if (t_prev >= 0.0f && t_prev < state.current_dur + release) {
                if (t_prev < attack) {
                    env_prev = t_prev / attack;
                } else if (t_prev < attack + decay) {
                    env_prev = 1.0f - (t_prev - attack) / decay * (1.0f - sustain);
                } else if (t_prev < state.current_dur) {
                    env_prev = sustain;
                } else if (t_prev < state.current_dur + release) {
                    float t_rel = (t_prev - state.current_dur) / release;
                    env_prev = sustain * (1.0f - t_rel) * std::exp(-t_rel * 5.0f);
                }
            }
        }

        // Voice characteristics
        float formant_scale = (depth == 0) ? 0.9f : 1.1f; // Slightly lower for male, higher for female
        float breath_amount = (depth == 0) ? 0.2f : 0.3f;
        float vibrato_depth = (depth == 0) ? 0.02f : 0.03f;
        float vibrato_rate = 5.5f; // Hz
        float base_freq = freq * (depth == 0 ? 0.8f : 1.2f); // Lower for male, higher for female
        base_freq = std::max(20.0f, base_freq);

        // Phoneme selection
        float f1, f2, bw1, bw2;
        if (phoneme >= 0 && phoneme < 14) {
            // Use provided phoneme directly
            static const struct { float f1; float f2; float bw1; float bw2; } phonemes[] = {
                { 400.0f,  900.0f,  140.0f, 180.0f}, // a (as in "father")
                { 600.0f,  800.0f,  150.0f, 170.0f}, // e (as in "bed")
                { 350.0f, 1000.0f,  130.0f, 190.0f}, // i (as in "see")
                { 300.0f,  700.0f,  160.0f, 200.0f}, // o (as in "go")
                { 500.0f,  950.0f,  140.0f, 180.0f}, // u (as in "blue")
                { 400.0f,  800.0f,  130.0f, 170.0f}, // æ (as in "cat")
                { 300.0f,  600.0f,  120.0f, 160.0f}, // ə (as in "about")
                { 450.0f, 1100.0f,  140.0f, 180.0f}, // ɪ (as in "bit")
                { 500.0f, 1000.0f,  150.0f, 190.0f}, // ɛ (as in "bet")
                { 350.0f,  900.0f,  130.0f, 170.0f}, // ʌ (as in "cut")
                { 250.0f,  650.0f,  160.0f, 200.0f}, // ɔ (as in "caught")
                { 200.0f,  700.0f,  150.0f, 190.0f}, // ʊ (as in "put")
                { 400.0f,  800.0f,  140.0f, 180.0f}, // ɑ (as in "hot")
                { 450.0f,  850.0f,  150.0f, 190.0f}  // ɒ (as in "law")
            };
            f1 = phonemes[phoneme].f1 * formant_scale;
            f2 = phonemes[phoneme].f2 * formant_scale;
            bw1 = phonemes[phoneme].bw1;
            bw2 = phonemes[phoneme].bw2;
        } else {
            // Fallback based on frequency
            float freq_normalized = (base_freq - (depth == 0 ? 20.0f : 160.0f)) /
                                    (depth == 0 ? 90.0f - 20.0f : 300.0f - 160.0f);
            freq_normalized = std::max(0.0f, std::min(1.0f, freq_normalized));
            int selected_phoneme = static_cast<int>(freq_normalized * 13);
            static const struct { float f1; float f2; float bw1; float bw2; } phonemes[] = {
                { 800.0f, 2000.0f,  80.0f, 100.0f}, // a
                { 600.0f, 2700.0f,  70.0f,  90.0f}, // e
                { 750.0f, 1800.0f,  90.0f, 110.0f}, // i
                { 550.0f, 2800.0f,  70.0f,  90.0f}, // o
                { 700.0f, 2400.0f,  80.0f, 100.0f}, // u
                { 500.0f, 1500.0f,  90.0f, 110.0f}, // æ
                { 800.0f, 1400.0f,  90.0f, 110.0f}, // ə
                { 400.0f,  900.0f,  80.0f, 100.0f}, // ɪ
                { 650.0f, 2000.0f,  80.0f, 100.0f}, // ɛ
                { 750.0f, 1600.0f,  90.0f, 110.0f}, // ʌ
                { 600.0f, 2900.0f,  70.0f,  90.0f}, // ɔ
                { 800.0f, 1800.0f,  90.0f, 110.0f}, // ʊ
                { 700.0f, 2000.0f,  80.0f, 100.0f}  // ɑ
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
        state.vibrato_phase += 2.0f * M_PI * vibrato_rate / sampleRate;
        if (state.vibrato_phase > 2.0f * M_PI) state.vibrato_phase -= 2.0f * M_PI;
        float vibrato = 1.0f + vibrato_depth * std::sin(state.vibrato_phase);
        float modulated_freq = base_freq * vibrato;

        float saw = 0.0f;
        const int num_harmonics = 25;
        for (int i = 1; i <= num_harmonics; ++i) {
            float harmonic_freq = modulated_freq * i;
            if (harmonic_freq > 20000.0f) break;
            saw += std::sin(2.0f * M_PI * harmonic_freq * t) / (i * 1.2f);
        }
        saw *= 1.5f; // Increase amplitude for richer sound

        // Apply formant filters
        float formant1_out = formant1.process(saw) * 0.8f;
        float formant2_out = formant2.process(saw) * 0.7f;
        float vocal_current = (depth == 0) ? (0.35f * saw + 0.65f * (formant1_out + formant2_out))
                                          : (0.25f * saw + 0.75f * (formant1_out + formant2_out));

        // Add breathiness
        float breath = rng.generatePinkNoise() * std::exp(-5.0f * t_current / dur) * breath_amount;
        float output_current = env_current * (vocal_current + breath);

        // Crossfade for note transitions
        float output_prev = state.prev_output;
        if (state.is_new_note && t_current < crossfade_dur) {
            float prev_base_freq = state.current_freq * (depth == 0 ? 0.8f : 1.2f);
            prev_base_freq = std::max(20.0f, prev_base_freq);
            float prev_vibrato = 1.0f + vibrato_depth * std::sin(state.vibrato_phase);
            float prev_modulated_freq = prev_base_freq * prev_vibrato;

            float prev_saw = 0.0f;
            for (int i = 1; i <= num_harmonics; ++i) {
                float harmonic_freq = prev_modulated_freq * i;
                if (harmonic_freq > 20000.0f) break;
                prev_saw += std::sin(2.0f * M_PI * harmonic_freq * t) / (i * 1.2f);
            }
            prev_saw *= 1.5f;

            float prev_formant1_out = formant1.process(prev_saw) * 0.8f;
            float prev_formant2_out = formant2.process(prev_saw) * 0.7f;
            float prev_vocal = (depth == 0) ? (0.35f * prev_saw + 0.65f * (prev_formant1_out + prev_formant2_out))
                                           : (0.25f * prev_saw + 0.75f * (prev_formant1_out + prev_formant2_out));
            float prev_breath = rng.generatePinkNoise() * std::exp(-5.0f * t_current / state.current_dur) * breath_amount;
            output_prev = env_prev * (prev_vocal + prev_breath);

            float crossfade_t = t_current / crossfade_dur;
            float smooth_t = 0.5f * (1.0f - std::cos(M_PI * crossfade_t));
            output_current = (1.0f - smooth_t) * output_prev + smooth_t * output_current;
            state.is_new_note = false;
        }

        // Apply effects
        if (depth == 0) {
            output_current = distortion.process(output_current);
        }
        output_current = reverb.process(output_current);
        output_current = filter.process(output_current);
        output_current *= (depth == 0) ? 2.2f : 2.0f; // Slightly higher gain for male voice
        output_current = std::max(std::min(output_current, 1.0f), -1.0f);
        output_current = protector.process(output_current, t_current, dur);
        output_current *= gain;

        state.prev_output = output_current;
        return output_current;
    }
};

} // namespace Instruments

#endif // VOCAL_H