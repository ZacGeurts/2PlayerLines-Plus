// instruments/vocal.h
// vocal instrument
// creates a vocal note with
// float t time
// float freq frequency
// int phoneme for phoneme selection
// float dur duration
// int depth for male/female voice
#ifndef VOCAL_H
#define VOCAL_H

// ai can modify this file as long as float generateWave(float t, float freq, int phoneme, float dur, int depth) { } remains true
// tell it how it should sound better by comparing it to videos you listen to.

#include "instruments.h"

namespace Instruments {

struct VocalState {
    float current_freq = 0.0f;
    float current_dur = 0.0f;
    float start_time = 0.0f;
    bool is_new_note = true;
    float prev_output = 0.0f;
    float prev_time = -1.0f;
};

class Vocal {
    AudioUtils::AudioProtector protector;
    AudioUtils::RandomGenerator rng;
    AudioUtils::Reverb reverb;
    AudioUtils::LowPassFilter filter;
    AudioUtils::Distortion distortion;
    VocalState male_state;
    VocalState female_state;
    float gain; // 0.2f is 20% volume
    float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE; // 44100 default is max supported SDL2.

public:
    Vocal(float gain = 0.2f, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE)
        : protector(0.005f, 0.9f),
          reverb(0.25f, 0.6f, 0.4f),
          filter(4500.0f),
          distortion(2.0f, 0.7f),
          gain(gain) {}          

    float generateWave(float t, float freq, int phoneme, float dur, int depth) {
        VocalState& state = (depth == 1) ? male_state : female_state;
        bool is_new_note = (freq != state.current_freq || t < state.prev_time || 
                           t >= state.start_time + state.current_dur + 1.5f);
        if (is_new_note) {
            state.is_new_note = true;
            state.current_freq = freq;
            state.current_dur = dur;
            state.start_time = t;
        }
        state.prev_time = t;
        float attack = 0.05f, decay = 0.5f, sustain = 0.8f, release = 1.5f;
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
            env_current = sustain * (1.0f - t_rel) * std::exp(-t_rel * 4.0f);
        } else {
            env_current = 0.0f;
        }
        if (state.is_new_note && t_current < 1.5f) {
            float t_prev = t - (state.start_time - crossfade_dur);
            if (t_prev >= 0.0f && t_prev < state.current_dur + 1.5f) {
                if (t_prev < attack) {
                    env_prev = t_prev / attack;
                } else if (t_prev < attack + decay) {
                    env_prev = 1.0f - (t_prev - attack) / decay * (1.0f - sustain);
                } else if (t_prev < state.current_dur) {
                    env_prev = sustain;
                } else if (t_prev < state.current_dur + release) {
                    float t_rel = (t_prev - state.current_dur) / release;
                    env_prev = sustain * (1.0f - t_rel) * std::exp(-t_rel * 4.0f);
                }
            }
        }
        float formant_scale = (depth == 1) ? 1.0f : 1.25f;
        float breath_amount = (depth == 1) ? 0.25f : 0.5f;
        float vibrato_depth = (depth == 1) ? 0.015f : 0.025f;
        float base_freq = freq * ((depth == 1) ? 0.3f : 1.2f);
        if (depth == 1) {
            base_freq = std::max(20.0f, base_freq);
        }
        int selected_phoneme;
        float f1, f2, bw1, bw2;
        if (depth == 1) {
            float freq_normalized = (base_freq - 20.0f) / (90.0f - 20.0f);
            freq_normalized = std::max(0.0f, std::min(1.0f, freq_normalized));
            selected_phoneme = static_cast<int>(freq_normalized * 14);
            switch (selected_phoneme) {
                case 0: f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale; bw1 = 140.0f; bw2 = 180.0f; break;
                case 1: f1 = 600.0f * formant_scale; f2 = 800.0f * formant_scale; bw1 = 150.0f; bw2 = 170.0f; break;
                case 2: f1 = 350.0f * formant_scale; f2 = 1000.0f * formant_scale; bw1 = 130.0f; bw2 = 190.0f; break;
                case 3: f1 = 300.0f * formant_scale; f2 = 700.0f * formant_scale; bw1 = 160.0f; bw2 = 200.0f; break;
                case 4: f1 = 500.0f * formant_scale; f2 = 950.0f * formant_scale; bw1 = 140.0f; bw2 = 180.0f; break;
                case 5: f1 = 400.0f * formant_scale; f2 = 800.0f * formant_scale; bw1 = 130.0f; bw2 = 170.0f; break;
                case 6: f1 = 300.0f * formant_scale; f2 = 600.0f * formant_scale; bw1 = 120.0f; bw2 = 160.0f; break;
                case 7: f1 = 450.0f * formant_scale; f2 = 1100.0f * formant_scale; bw1 = 140.0f; bw2 = 180.0f; break;
                case 8: f1 = 500.0f * formant_scale; f2 = 1000.0f * formant_scale; bw1 = 150.0f; bw2 = 190.0f; break;
                case 9: f1 = 350.0f * formant_scale; f2 = 900.0f * formant_scale; bw1 = 130.0f; bw2 = 170.0f; break;
                case 10: f1 = 250.0f * formant_scale; f2 = 650.0f * formant_scale; bw1 = 160.0f; bw2 = 200.0f; break;
                case 11: f1 = 200.0f * formant_scale; f2 = 700.0f * formant_scale; bw1 = 150.0f; bw2 = 190.0f; break;
                case 12: f1 = 400.0f * formant_scale; f2 = 800.0f * formant_scale; bw1 = 140.0f; bw2 = 180.0f; break;
                case 13: f1 = 450.0f * formant_scale; f2 = 850.0f * formant_scale; bw1 = 150.0f; bw2 = 190.0f; break;
                default: f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale; bw1 = 140.0f; bw2 = 180.0f;
            }
        } else {
            float freq_normalized = (base_freq - 160.0f) / (300.0f - 160.0f);
            freq_normalized = std::max(0.0f, std::min(1.0f, freq_normalized));
            selected_phoneme = static_cast<int>(freq_normalized * 13);
            switch (selected_phoneme) {
                case 0: f1 = 800.0f * formant_scale; f2 = 2000.0f * formant_scale; bw1 = 80.0f; bw2 = 100.0f; break;
                case 1: f1 = 600.0f * formant_scale; f2 = 2700.0f * formant_scale; bw1 = 70.0f; bw2 = 90.0f; break;
                case 2: f1 = 750.0f * formant_scale; f2 = (1800.0f + 200.0f * std::sin(2.0f * M_PI * 0.5f * t)) * formant_scale; bw1 = 90.0f; bw2 = 110.0f; break;
                case 3: f1 = 550.0f * formant_scale; f2 = 2800.0f * formant_scale; bw1 = 70.0f; bw2 = 90.0f; break;
                case 4: f1 = 700.0f * formant_scale; f2 = 2400.0f * formant_scale; bw1 = 80.0f; bw2 = 100.0f; break;
                case 5: f1 = 500.0f * formant_scale; f2 = 1500.0f * formant_scale; bw1 = 90.0f; bw2 = 110.0f; break;
                case 6: f1 = 800.0f * formant_scale; f2 = 1400.0f * formant_scale; bw1 = 90.0f; bw2 = 110.0f; break;
                case 7: f1 = 400.0f * formant_scale; f2 = 900.0f * formant_scale; bw1 = 80.0f; bw2 = 100.0f; break;
                case 8: f1 = 650.0f * formant_scale; f2 = 2000.0f * formant_scale; bw1 = 80.0f; bw2 = 100.0f; break;
                case 9: f1 = 750.0f * formant_scale; f2 = 1600.0f * formant_scale; bw1 = 90.0f; bw2 = 110.0f; break;
                case 10: f1 = 600.0f * formant_scale; f2 = 2900.0f * formant_scale; bw1 = 70.0f; bw2 = 90.0f; break;
                case 11: f1 = 800.0f * formant_scale; f2 = 1800.0f * formant_scale; bw1 = 90.0f; bw2 = 110.0f; break;
                case 12: f1 = 700.0f * formant_scale; f2 = (2000.0f + 300.0f * std::sin(2.0f * M_PI * 0.7f * t)) * formant_scale; bw1 = 80.0f; bw2 = 100.0f; break;
                default: f1 = 700.0f * formant_scale; f2 = 2000.0f * formant_scale; bw1 = 80.0f; bw2 = 100.0f;
            }
        }
        float saw = 0.0f;
        const int num_harmonics = 20;
        for (int i = 1; i <= num_harmonics; ++i) {
            float harmonic_freq = base_freq * i;
            if (harmonic_freq > 20000.0f) break;
            saw += std::sin(2.0f * M_PI * harmonic_freq * t) / i;
        }
        saw *= 1.2f;
        AudioUtils::BandPassFilter formant1_filter(f1, bw1);
        AudioUtils::BandPassFilter formant2_filter(f2, bw2);
        float formant1 = formant1_filter.process(saw) * 0.7f;
        float formant2 = formant2_filter.process(saw) * 0.6f;
        float vocal_current = (depth == 1) ? 
            (0.4f * saw + 0.6f * (formant1 + formant2)) : 
            (0.3f * saw + 0.7f * (formant1 + formant2));
        float breath = rng.generatePinkNoise() * std::exp(-6.0f * t / dur) * breath_amount;
        float vibrato = 1.0f + vibrato_depth * std::sin(2.0f * M_PI * 5.0f * t);
        float output_current = env_current * (vocal_current + breath) * vibrato;
        float output_prev = state.prev_output;
        if (state.is_new_note && t_current < 1.5f) {
            float prev_base_freq = state.current_freq * ((depth == 1) ? 0.3f : 1.2f);
            if (depth == 1) {
                prev_base_freq = std::max(20.0f, prev_base_freq);
            }
            float prev_f1 = f1, prev_f2 = f2, prev_bw1 = bw1, prev_bw2 = bw2;
            float prev_saw = 0.0f;
            for (int i = 1; i <= num_harmonics; ++i) {
                float harmonic_freq = prev_base_freq * i;
                if (harmonic_freq > 20000.0f) break;
                prev_saw += std::sin(2.0f * M_PI * harmonic_freq * t) / i;
            }
            prev_saw *= 1.2f;
            AudioUtils::BandPassFilter prev_formant1_filter(prev_f1, prev_bw1 / prev_f1);
            AudioUtils::BandPassFilter prev_formant2_filter(prev_f2, prev_bw2 / prev_f2);
            float prev_formant1 = prev_formant1_filter.process(prev_saw) * 0.7f;
            float prev_formant2 = prev_formant2_filter.process(prev_saw) * 0.6f;
            float prev_vocal = (depth == 1) ? 
                (0.4f * prev_saw + 0.6f * (prev_formant1 + prev_formant2)) :
                (0.3f * prev_saw + 0.7f * (prev_formant1 + prev_formant2));
            float prev_breath = rng.generatePinkNoise() * std::exp(-6.0f * t / state.current_dur) * breath_amount;
            float prev_vibrato = 1.0f + vibrato_depth * std::sin(2.0f * M_PI * 5.0f * t);
            output_prev = env_prev * (prev_vocal + prev_breath) * prev_vibrato;
        }
        float output;
        if (state.is_new_note && t_current < crossfade_dur) {
            float crossfade_t = t_current / crossfade_dur;
            float smooth_t = 0.5f * (1.0f - std::cos(M_PI * crossfade_t));
            output = (1.0f - smooth_t) * output_prev + smooth_t * output_current;
        } else {
            output = output_current;
            state.is_new_note = false;
        }
        if (depth == 1) {
            output = distortion.process(output);
        }
        output = reverb.process(output);
        output = filter.process(output);
        output *= (depth == 1) ? 2.0f : 1.8f;
        output = std::max(std::min(output, 1.0f), -1.0f);
        state.prev_output = output_current;
        output = protector.process(output, t, dur);
        output *= gain;
        return output;
    }
};

} // namespace Instruments

#endif // VOCAL_H
