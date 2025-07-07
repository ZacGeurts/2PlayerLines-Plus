// instruments.h - generates sound from instruments folder instrument files. Custom RNG.

// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp songgen.h instruments.h and instrument files
// Interested parties can find my contact information at https://github.com/ZacGeurts
// If you make commercial gain, you can do the math and update my Patreon.

// If you want to distribute changes to this file then open a github fork.

// Follow the local law.
// FCC in the USA restricts what frequencies you can broadcast.
// Most, if not all countries restrict frequencies.
// Audible frequencies in the USA are a First Amendment Right. 
// There are some nefarious limitations, like faking a policeman phone call.
// Making sound is a human and biological right.
// Courts ruled we cannot extinct weevils or anything else.

// Maybe there is a usage for frequencies exceeding this many 31415926535897932384626433832795029L
// I may plug it into my simulator to view waveforms exceeding audio.
// What I would hope to find is a useful tool for physicists and data scientists.
// The zoom resolution on a second of audio would be over 22x, so an audio viewer might make sense.
// Instead of just audio wave forms it can be 3d color modeled with all that data.
// I would like to bring my computer to its knees with crazy precision and then speed it up.
/* 
    Far out of this programs audio scope. I cap to 20hz and less than 44100hz (SDL2 maximum and exceeds human hearing).
    We can go below 20hz down to 0hz, but top of the line car stereos might play 8hz with expensive speakers, 12 is expensive too.
    It requires too much voltage to go lower and you would not hear a difference.
    20hz-80hz should be top quality for a subwoofer and it does not try blowing out pc speakers.
    20hz-120hz also a common setting for subwoofers.
*/
// You would need more than a speaker. Fun fact: WiFi is frequencies. Sound you cannot hear.
// Do not restrict emergency communications or damage heart pace makers, etc.
// ---
// Always put hearing safety first. It does not grow back.
// Be kind to pets. Sensitive ears.

// This system requries some skill level to fully enjoy all the features.
//      Basic file management skills with multiple windows
//      Text editing skills. (copy, paste, etc.)
//      A functional AI to paste code back and forth.
//
//      Computer Terminal keyboard skills will be of assistance.
//      We need to be able to write 'make clean' and 'make'
//      Knowing how to program will help with making more complicated changes.
//
// note: the AI should have enough memory to take a full instrument file.
// include telling it what kind of instrument it is that you would Love to hear.
//
// Start with an existing instrument and see if your AI can make it sound better.
// song1.song is designed to be changed to different instruments, so you can test your instrument sound.
// AudioUtils is explained at the bottom of an instrument's .h file, but it is there for the AI.
// Lesson 1: Patience // *takes nap*
//
// All instrument files share a similar template, so they are easily comparable.
// AudioUtils has powerfull tools at an instrument's disposal.
// It should be able to generate every hearable sound using only frequency tones.
//
// If you create a NEW instrument you will need to see the include Register at the very bottom of this file.
//
// To add a new instrument (e.g., steeldrum.h):
//
// 1. Copy and paste an existing instrument from the folder and tell AI you want a steeldrum and paste the code response into a new steeldrum.h.
// 1a. Save the new header file steeldrum.h within the instruments folder.
//
// 2. Update the Register and include at the bottom of this file. Save.
//
// 3. Rebuild the project with 'make clean && make' from a terminal prompt.
// 3b. run 'make clean && make' after every time you have saved a change you would like to test.
// 3c. if you encounter errors from running 'make', paste the errors back at the AI.
//
// 4. Edit the song1.song file with steeldrum to test. (do not edit other .song files)
//
// 5. './songgen song1.song'
// 5a. run './songgen' without specifying a file for the help
//
// Instruments use generateWave(t, freq, dur). Time, Frequency, Duration.
// This is important. We want the AI to create with only these 3 things, and complex math, a flute or a drum or a piano.
// No fancy tricks. I permit every instrument an audio frequency tone generator and those 3 things.
// The AI needs to make a Cello or a Trumpet, with fractions of a second, sound like the thing.
// Sound has time. This looks like a wave. A quarter note is 4 waves per second if they took turns.
// Frequency is how wide or close together the waves are.
// Duration is how long the AI has to make it sound like the thing.
// How does an instrument create the wave when it does it?
// And then for that quarter second make it sound like the thing. 
// pluck, toot, or drum
//
// Except for vocal.
// The Vocal "instrument" uses a special overload,
// Note for Vocal Overload:
//       Do not add vocal, modify existing. (simulates singers).
//     generateWave(t, freq, phoneme, dur, variant), variant 0 is male (vocal_0), variant 1 is female (vocal_1).
//     Both vocal_0 and vocal_1 are registered separately but use the same Vocal class with different variant parameters.
//
// ---
// AudioUtils takes the waves and does filtering, distortion, noise, and gives the AI what it needs to update sound.
// It can sound and look like a heart monitor with a heartbeat.
// It can turn the wave into a laser beam wider than every sound you can hear at once.
// Math should be able to get us audio everywhere between.
// Instrments are like pond ripples from throwing a rock. The water underneath is turbulent as well.
// Unique stones, we are trying to replicate the ripples.
// It should be simple to create beep.h after the steeldrum.h, but I think you can be more creative.
//
// I do not expect computers will be getting slower, so I went overboard in some areas.
// I had AI implement threading already, so the program distributes across processor cores nicely.
// Have fun! Make waves. God Bless.

#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <cmath>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <map>
#include <algorithm>
#include <string>
#include <set>
#include <memory>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <limits>
#include <SDL2/SDL.h>
#include <array>

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#include <cpuid.h>
#define USE_RDRAND
#endif
#if defined(__ARM_NEON)
#include <arm_neon.h>
#define USE_NEON
#endif

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define DEBUG_LOG 0

namespace AudioUtils {
    constexpr float DEFAULT_SAMPLE_RATE = 44100.0f;
    constexpr int BUFFER_SIZE = 1024;
    constexpr int RING_BUFFER_COUNT = 4;

    class RandomGenerator {
    private:
        using FixedPoint = int64_t;
        static constexpr FixedPoint FIXED_POINT_MASK = (1LL << 32) - 1;
        static constexpr size_t STATE_SIZE = 256;
        static_assert(STATE_SIZE % 4 == 0, "STATE_SIZE must be multiple of 4 for SIMD");

        struct ZigguratTables {
            static constexpr size_t TABLE_SIZE = 256;
            std::array<FixedPoint, TABLE_SIZE> x;
            std::array<FixedPoint, TABLE_SIZE> f;
            ZigguratTables() {
                for (size_t i = 0; i < TABLE_SIZE; ++i) {
                    double z = 3.5 * (i + 1) / TABLE_SIZE;
                    x[i] = static_cast<FixedPoint>(z * (1LL << 32));
                    f[i] = static_cast<FixedPoint>(std::exp(-0.5 * z * z) * (1LL << 32));
                }
            }
        };
        static inline const ZigguratTables ziggurat;

        struct MegaMixMaxLite {
            alignas(16) FixedPoint state[STATE_SIZE];
            FixedPoint counter;
            FixedPoint checksum;
            static std::mutex global_counter_mutex;

            static bool has_rdrand() {
#ifdef USE_RDRAND
                uint32_t eax, ebx, ecx, edx;
                __cpuid(1, eax, ebx, ecx, edx);
                return (ecx & (1 << 30)) != 0;
#else
                return false;
#endif
            }

            MegaMixMaxLite() {
                static FixedPoint global_counter = 0xCAFEBABEDEADBEEFLL;
                thread_local static FixedPoint thread_counter = 0xFEEDFACE12345678LL;
                FixedPoint seed;
                {
                    std::lock_guard<std::mutex> lock(global_counter_mutex);
                    int stack_entropy;
                    seed = static_cast<FixedPoint>(reinterpret_cast<uintptr_t>(&stack_entropy)) +
                           thread_counter++ + global_counter++;
                }

                seed = hash_seed(seed);

                if (has_rdrand() && is_rdrand_faster()) {
                    for (size_t i = 0; i < STATE_SIZE; ++i) {
                        state[i] = get_rdrand();
                    }
                } else {
                    for (size_t i = 0; i < STATE_SIZE; ++i) {
                        state[i] = seed + (seed / (1LL << 32)) +
                                   (static_cast<FixedPoint>(i) * 0xDEADBEEFC0FFEE00LL);
                        seed = hash_seed(seed + static_cast<FixedPoint>(i));
                    }
                }
                counter = seed;
                for (int i = 0; i < 8; ++i) { next(); }
                update_checksum();

                if (is_all_zero()) {
                    for (size_t i = 0; i < STATE_SIZE; ++i) {
                        state[i] = static_cast<FixedPoint>(0xBADC0DE123456789LL) +
                                   (static_cast<FixedPoint>(i) * 0xFEEDFACE98765432LL);
                    }
                    counter = 0xCAFE;
                    update_checksum();
                }
            }

            FixedPoint hash_seed(FixedPoint x) {
                x = (x * 0xFF51AFD7ED558CCDLL) >> 32;
                x = (x * 0xC4CEB9FE1A85EC53LL) >> 32;
                return x;
            }

            bool is_all_zero() {
                for (size_t i = 0; i < STATE_SIZE; ++i) {
                    if (state[i] != 0) return false;
                }
                return true;
            }

            void update_checksum() {
                checksum = 0;
                for (size_t i = 0; i < STATE_SIZE; ++i) {
                    checksum += state[i];
                }
            }

            FixedPoint get_rdrand() {
#ifdef USE_RDRAND
                unsigned long long value;
                if (_rdrand64_step(&value)) {
                    return static_cast<FixedPoint>(value);
                }
#endif
                return hash_seed(counter++);
            }

            bool is_rdrand_faster() {
#ifdef USE_RDRAND
                return true;
#else
                return false;
#endif
            }

            FixedPoint next() {
                FixedPoint old_checksum = checksum;
                update_checksum();
                if (old_checksum == checksum && checksum != 0) {
                    *this = MegaMixMaxLite();
                }

#ifdef USE_SSE2
                if (__builtin_cpu_supports("sse2")) {
                    __m128i multiplier = _mm_set1_epi64x(6364136223846793005LL);
                    __m128i counter_vec = _mm_set1_epi64x(counter);
                    for (size_t i = 0; i < STATE_SIZE; i += 4) {
                        __m128i state_vec = _mm_load_si128(reinterpret_cast<__m128i*>(&state[i]));
                        __m128i index_vec = _mm_set_epi64x(i + 3, i + 2);
                        __m128i term = _mm_add_epi64(multiplier, _mm_mullo_epi64(index_vec, _mm_set1_epi64x(123456789)));
                        state_vec = _mm_add_epi64(_mm_mullo_epi64(state_vec, term), counter_vec);
                        _mm_store_si128(reinterpret_cast<__m128i*>(&state[i]), state_vec);
                    }
                } else
#endif
#ifdef USE_NEON
                if (__builtin_cpu_supports("neon")) {
                    int64x2_t multiplier = vdupq_n_s64(6364136223846793005LL);
                    int64x2_t counter_vec = vdupq_n_s64(counter);
                    for (size_t i = 0; i < STATE_SIZE; i += 2) {
                        int64x2_t state_vec = vld1q_s64(&state[i]);
                        int64x2_t index_vec = vdupq_n_s64(i);
                        int64x2_t term = vaddq_s64(multiplier, vmulq_n_s64(index_vec, 123456789));
                        state_vec = vaddq_s64(vmulq_s64(state_vec, term), counter_vec);
                        vst1q_s64(&state[i], state_vec);
                    }
                } else
#endif
                {
                    for (size_t i = 0; i < STATE_SIZE; ++i) {
                        state[i] = (state[i] * (6364136223846793005LL + (static_cast<FixedPoint>(i) * 123456789))) + counter;
                    }
                }

                counter += (1LL << 32);
                FixedPoint mix = state[0] + state[STATE_SIZE - 1];
                for (size_t i = 1; i < STATE_SIZE; ++i) {
                    mix += state[i] + (state[i - 1] >> 23);
                }
                FixedPoint temp = mix >> 32;
                temp = (temp * temp) >> 32;
                mix += temp;
                return mix;
            }
        };

        MegaMixMaxLite prng;
        static constexpr size_t BUFFER_SIZE = 1ULL << 20;
        thread_local static std::vector<FixedPoint> buffer;
        thread_local static size_t buffer_pos;

        void fill_buffer(FixedPoint min = -(1LL << 32), FixedPoint max = (1LL << 32)) {
            try {
                buffer.resize(BUFFER_SIZE);
                buffer_pos = 0;
                for (size_t i = 0; i < BUFFER_SIZE; i += 8) {
                    FixedPoint val = prng.next();
                    for (size_t j = 0; j < 8 && i + j < BUFFER_SIZE; ++j) {
                        FixedPoint raw = val >> 8;
                        buffer[i + j] = min + ((max - min) * raw) / (1LL << 32);
                    }
                }
            } catch (const std::bad_alloc& e) {
                throw std::runtime_error("Failed to allocate buffer: " + std::string(e.what()));
            }
        }

        FixedPoint get_random_fixed_point() {
            if (buffer_pos + 8 > buffer.size()) {
                fill_buffer();
            }
            FixedPoint result = 0;
            for (int i = 0; i < 8; ++i) {
                result = result << 8 | (buffer[buffer_pos++] & 0xFF);
                if (buffer_pos >= buffer.size()) {
                    fill_buffer();
                }
            }
            return result / (1LL << 32);
        }

        uint64_t get_random_uint64() {
            if (buffer_pos + 8 > buffer.size()) {
                fill_buffer();
            }
            uint64_t result = 0;
            for (int i = 0; i < 8; ++i) {
                result = result << 8 | (buffer[buffer_pos++] & 0xFF);
                if (buffer_pos >= buffer.size()) {
                    fill_buffer();
                }
            }
            return result;
        }

    public:
        static constexpr FixedPoint FIXED_POINT_SCALE = 1LL << 32;
        RandomGenerator(FixedPoint min = 0, FixedPoint max = FIXED_POINT_SCALE) {
            fill_buffer(min, max);
        }

        template<typename T, typename U>
        FixedPoint dist(T min, U max) {
            static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>, "Inputs must be numeric");
            FixedPoint min_val = static_cast<FixedPoint>(min) * FIXED_POINT_SCALE;
            FixedPoint max_val = static_cast<FixedPoint>(max) * FIXED_POINT_SCALE;
            if (min_val > max_val) { std::swap(min_val, max_val); }
            min_val = std::max(min_val, -FIXED_POINT_SCALE * 1000);
            max_val = std::min(max_val, FIXED_POINT_SCALE * 1000);
            FixedPoint raw = get_random_fixed_point();
            return min_val + ((max_val - min_val) * raw) / FIXED_POINT_SCALE;
        }

        template<typename T, typename U, typename V, typename W>
        FixedPoint normal_dist(T mean, U stddev, V min, W max) {
            static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U> &&
                          std::is_arithmetic_v<V> && std::is_arithmetic_v<W>, "Inputs must be numeric");
            FixedPoint mean_val = static_cast<FixedPoint>(mean) * FIXED_POINT_SCALE;
            FixedPoint stddev_val = std::max(static_cast<FixedPoint>(stddev) * FIXED_POINT_SCALE, static_cast<FixedPoint>(1LL));
            FixedPoint min_val = static_cast<FixedPoint>(min) * FIXED_POINT_SCALE;
            FixedPoint max_val = static_cast<FixedPoint>(max) * FIXED_POINT_SCALE;
            if (min_val > max_val) { std::swap(min_val, max_val); }

            while (true) {
                uint64_t idx = get_random_uint64() % ZigguratTables::TABLE_SIZE;
                FixedPoint u = get_random_fixed_point();
                FixedPoint x = ziggurat.x[idx];
                if (u < ziggurat.f[idx]) {
                    FixedPoint result = mean_val + (stddev_val * x) / FIXED_POINT_SCALE;
                    return std::max(min_val, std::min(max_val, result));
                }
                if (idx == 0) {
                    FixedPoint z = get_random_fixed_point() * 3;
                    FixedPoint result = mean_val + (stddev_val * z) / FIXED_POINT_SCALE;
                    return std::max(min_val, std::min(max_val, result));
                }
            }
        }

        float random_float() {
            constexpr uint32_t mantissa_bits = 1U << 23;
            uint64_t x = get_random_uint64() & (mantissa_bits - 1);
            return static_cast<float>(x) / mantissa_bits;
        }

        long double random_L() {
            constexpr uint64_t mantissa_bits = 1ULL << 52;
            uint64_t x = get_random_uint64() & (mantissa_bits - 1);
            return static_cast<long double>(x) / mantissa_bits;
        }

        FixedPoint random_fixed_point() {
            return get_random_fixed_point();
        }

        int random_int() {
            return static_cast<int>((get_random_fixed_point() * std::numeric_limits<int>::max()) / FIXED_POINT_SCALE);
        }

        uint64_t random_uint64() {
            return get_random_uint64();
        }

        template<typename T, typename U>
        signed int roll_dice(T min, U max) {
            static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>, "Inputs must be numeric");
            FixedPoint min_val = static_cast<FixedPoint>(min) * FIXED_POINT_SCALE;
            FixedPoint max_val = static_cast<FixedPoint>(max) * FIXED_POINT_SCALE;
            if (min_val > max_val) { std::swap(min_val, max_val); }
            min_val = std::max(min_val, static_cast<FixedPoint>(std::numeric_limits<signed int>::min()) * FIXED_POINT_SCALE);
            max_val = std::min(max_val, static_cast<FixedPoint>(std::numeric_limits<signed int>::max()) * FIXED_POINT_SCALE);
            FixedPoint raw = get_random_fixed_point();
            return static_cast<signed int>((min_val + ((max_val - min_val + FIXED_POINT_SCALE) * raw) / FIXED_POINT_SCALE) / FIXED_POINT_SCALE);
        }
    };

    std::mutex RandomGenerator::MegaMixMaxLite::global_counter_mutex;
    thread_local std::vector<RandomGenerator::FixedPoint> RandomGenerator::buffer;
    thread_local size_t RandomGenerator::buffer_pos = 0;

    // Audio processing classes
    class AudioProtector {
    private:
        float fadeTime, maxGain;
        float currentGain;
    public:
        AudioProtector(float fade = 0.01f, float gain = 0.92f)
            : fadeTime(fade), maxGain(gain), currentGain(gain) {}
        float process(float input, float t = 0.0f, float dur = 0.0f) {
            float output = input * currentGain;
            if (std::abs(output) > maxGain) {
                currentGain = std::max(0.0f, currentGain - (1.0f / (fadeTime * DEFAULT_SAMPLE_RATE)));
            } else {
                currentGain = std::min(maxGain, currentGain + (1.0f / (fadeTime * DEFAULT_SAMPLE_RATE)));
            }
            return std::isfinite(output) ? std::clamp(output, -maxGain, maxGain) : 0.0f;
        }
    };

    class WhiteNoise {
    private:
        RandomGenerator rng;
    public:
        WhiteNoise(float min = -1.0f, float max = 1.0f)
            : rng(static_cast<int64_t>(min * RandomGenerator::FIXED_POINT_SCALE),
                  static_cast<int64_t>(max * RandomGenerator::FIXED_POINT_SCALE)) {}
        float generate() {
            return rng.random_float() * 2.0f - 1.0f;
        }
    };

    class PinkNoise {
    private:
        RandomGenerator rng;
        float b0, b1, b2, b3, b4, b5, b6;
        float amplitude;
    public:
        PinkNoise(float amp = 1.0f) : rng(), b0(0.0f), b1(0.0f), b2(0.0f), b3(0.0f), b4(0.0f), b5(0.0f), b6(0.0f), amplitude(amp) {}
        float operator()() {
            float white = rng.random_float() * 2.0f - 1.0f;
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5327672f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            float output = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f) * amplitude;
            b6 = white * 0.115926f;
            return std::isfinite(output) ? output * 0.11f : 0.0f;
        }
    };

    class BrownNoise {
    private:
        RandomGenerator rng;
        float lastOutput;
        float amplitude;
    public:
        BrownNoise(float amp = 1.0f) : rng(), lastOutput(0.0f), amplitude(amp) {}
        float operator()() {
            float white = rng.random_float() * 2.0f - 1.0f;
            lastOutput = lastOutput + (0.02f * white);
            lastOutput = std::clamp(lastOutput, -1.0f, 1.0f);
            return std::isfinite(lastOutput) ? lastOutput * amplitude : 0.0f;
        }
    };

    class LowPassFilter {
    private:
        float cutoff;
        float x1, x2, y1, y2;
        float b0, b1, b2, a1, a2;
    public:
        LowPassFilter(float freq = 1000.0f)
            : cutoff(freq), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void setCutoff(float freq) {
            cutoff = std::max(20.0f, freq); // Prevent invalid cutoff frequencies
            updateCoefficients();
        }
        void updateCoefficients() {
            float w = 2.0f * M_PI * cutoff / DEFAULT_SAMPLE_RATE;
            float alpha = std::sin(w) / (2.0f * 0.707f);
            float cosw = std::cos(w);
            float b = 1.0f + alpha;
            b0 = (1.0f - cosw) / (2.0f * b);
            b1 = (1.0f - cosw) / b;
            b2 = b0;
            a1 = -2.0f * cosw / b;
            a2 = (1.0f - alpha) / b;
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return std::isfinite(output) ? output : 0.0f;
        }
    };

    class HighPassFilter {
    private:
        float cutoff, q;
        float x1, x2, y1, y2;
        float b0, b1, b2, a1, a2;
    public:
        HighPassFilter(float freq = 100.0f, float qVal = 0.707f)
            : cutoff(freq), q(qVal), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void updateCoefficients() {
            float w = 2.0f * M_PI * cutoff / DEFAULT_SAMPLE_RATE;
            float alpha = std::sin(w) / (2.0f * q);
            float cosw = std::cos(w);
            float b = 1.0f + alpha;
            b0 = (1.0f + cosw) / (2.0f * b);
            b1 = -(1.0f + cosw) / b;
            b2 = b0;
            a1 = -2.0f * cosw / b;
            a2 = (1.0f - alpha) / b;
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return std::isfinite(output) ? output : 0.0f;
        }
    };

    class BandPassFilter {
    private:
        float centerFreq, q;
        float x1, x2, y1, y2;
        float b0, b1, b2, a1, a2;
    public:
        BandPassFilter(float freq = 1000.0f, float qVal = 1.0f)
            : centerFreq(freq), q(qVal), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void setCenterFreq(float freq) {
            centerFreq = std::max(20.0f, freq); // Prevent invalid frequencies
            updateCoefficients();
        }
        void updateCoefficients() {
            float w = 2.0f * M_PI * centerFreq / DEFAULT_SAMPLE_RATE;
            float alpha = std::sin(w) / (2.0f * q);
            float cosw = std::cos(w);
            float b = 1.0f + alpha;
            b0 = alpha / b;
            b1 = 0.0f;
            b2 = -b0;
            a1 = -2.0f * cosw / b;
            a2 = (1.0f - alpha) / b;
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return std::isfinite(output) ? output : 0.0f;
        }
    };

    class Distortion {
    private:
        float drive, threshold, softClip;
    public:
        Distortion(float d = 1.0f, float thresh = 0.7f, float soft = 1.0f)
            : drive(d), threshold(thresh), softClip(soft) {}
        float process(float input) {
            float output = input * drive;
            if (std::abs(output) > threshold) {
                output = std::tanh((output - threshold) * softClip) + threshold * (output > 0 ? 1.0f : -1.0f);
            }
            return std::isfinite(output) ? std::clamp(output, -1.0f, 1.0f) : 0.0f;
        }
        void setDrive(float d) {
            drive = std::max(0.0f, d); // Ensure non-negative drive
        }
    };

    class Reverb {
    private:
        std::vector<float> delayLine;
        size_t writePos;
        float delay, decay, mix, airAbsorption;
    public:
        Reverb(float d = 0.1f, float dec = 0.5f, float m = 0.2f, float air = 0.2f)
            : writePos(0), delay(d), decay(dec), mix(m), airAbsorption(air) {
            size_t delaySamples = static_cast<size_t>(delay * DEFAULT_SAMPLE_RATE);
            delayLine.resize(delaySamples, 0.0f);
        }
        float process(float input) {
            size_t readPos = (writePos + delayLine.size() - static_cast<size_t>(delay * DEFAULT_SAMPLE_RATE)) % delayLine.size();
            float delayed = delayLine[readPos] * decay * (1.0f - airAbsorption);
            float output = input * (1.0f - mix) + delayed * mix;
            delayLine[writePos] = input + delayed * decay;
            writePos = (writePos + 1) % delayLine.size();
            return std::isfinite(output) ? std::clamp(output, -1.0f, 1.0f) : 0.0f;
        }
    };

    class Chorus {
    private:
        std::vector<float> delayLine;
        size_t writePos;
        float depth, rate, mix;
        float phase;
    public:
        Chorus(float d = 0.2f, float r = 0.5f, float m = 0.2f)
            : writePos(0), depth(d), rate(r), mix(m), phase(0.0f) {
            size_t delaySamples = static_cast<size_t>(0.03f * DEFAULT_SAMPLE_RATE);
            delayLine.resize(delaySamples, 0.0f);
        }
        float process(float input) {
            float delayTime = depth * std::sin(2.0f * M_PI * rate * phase);
            size_t delaySamples = static_cast<size_t>(delayTime * DEFAULT_SAMPLE_RATE);
            size_t readPos = (writePos + delayLine.size() - delaySamples) % delayLine.size();
            float output = input * (1.0f - mix) + delayLine[readPos] * mix;
            delayLine[writePos] = input;
            writePos = (writePos + 1) % delayLine.size();
            phase += 1.0f / DEFAULT_SAMPLE_RATE;
            if (phase >= 1.0f) phase -= 1.0f;
            return std::isfinite(output) ? std::clamp(output, -1.0f, 1.0f) : 0.0f;
        }
    };

    class Tremolo {
    private:
        float rate, depth;
        float phase;
    public:
        Tremolo(float r = 5.0f, float d = 0.5f)
            : rate(r), depth(d), phase(0.0f) {}
        float process(float input, float t = 0.0f) {
            float modulation = 1.0f - depth + depth * std::sin(2.0f * M_PI * rate * (phase + t));
            phase += 1.0f / DEFAULT_SAMPLE_RATE;
            if (phase >= 1.0f) phase -= 1.0f;
            float output = input * modulation;
            return std::isfinite(output) ? std::clamp(output, -1.0f, 1.0f) : 0.0f;
        }
    };

    class EnvelopeFollower {
    private:
        float attack, release;
        float envelope;
    public:
        EnvelopeFollower(float a = 0.01f, float r = 0.1f)
            : attack(a), release(r), envelope( carcinomas) {}
        float process(float input) {
            float absInput = std::abs(input);
            float coeff = (absInput > envelope) ? attack : release;
            envelope = envelope + (absInput - envelope) * (1.0f - std::exp(-1.0f / (coeff * DEFAULT_SAMPLE_RATE)));
            return std::isfinite(envelope) ? envelope : 0.0f;
        }
    };

    struct FormantFilter {
        float centerFreq, bandwidth;
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        FormantFilter(float freq, float bw)
            : centerFreq(freq), bandwidth(bw), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void setParameters(float freq, float bw) {
            centerFreq = freq;
            bandwidth = bw;
            updateCoefficients();
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return std::isfinite(output) ? output : 0.0f;
        }
        void updateCoefficients() {
            float r = std::exp(-M_PI * bandwidth / DEFAULT_SAMPLE_RATE);
            float theta = 2.0f * M_PI * centerFreq / DEFAULT_SAMPLE_RATE;
            b0 = 1.0f - r; b1 = 0.0f; b2 = 0.0f;
            a1 = -2.0f * r * std::cos(theta); a2 = r * r;
        }
    };
}

namespace Instruments {
    struct Note {
        long double freq, duration, startTime;
        int phoneme;
        bool open;
        long double volume, velocity;

        Note(long double freq = 440.0L, long double d = 0.0625L, long double s = 0.0L, int p = -1, bool o = false, long double v = 0.5L, long double vel = 0.8L)
            : freq(freq), duration(d), startTime(s), phoneme(p), open(o), volume(v), velocity(vel) {}
    };

    struct Part {
        std::string instrument;
        std::vector<Note> notes;
        bool useReverb;
        long double reverbAirMix, reverbDelay, reverbDecay;
        bool useDistortion;
        long double pan, reverbMix;
        std::string sectionName;
        long double distortionDrive, distortionThreshold;
        std::vector<std::pair<long double, long double>> panAutomation, volumeAutomation, reverbMixAutomation;

        Part() : instrument(""), notes(), useReverb(false), reverbAirMix(0.2L), reverbDelay(0.1L),
                 reverbDecay(0.5L), useDistortion(false), pan(0.0L), reverbMix(0.2L), sectionName(""),
                 distortionDrive(1.5L), distortionThreshold(0.7L), panAutomation(), volumeAutomation(),
                 reverbMixAutomation() {}
    };

    struct Section {
        std::string name, templateName;
        long double startTime, endTime, progress;

        Section() : name(""), templateName(""), startTime(0.0L), endTime(0.0L), progress(0.0L) {}
        Section(std::string n, std::string t, long double start, long double end, long double prog = 0.0L)
            : name(n), templateName(t), startTime(start), endTime(end), progress(prog) {}
    };

    struct WaveguideState {
        std::vector<float> forwardWave, backwardWave;
        size_t delayLineSize;
        size_t writePos;
        float lastFreq;
        float pressure;
        WaveguideState() : delayLineSize(0), writePos(0), lastFreq(0.0f), pressure(0.0f) {}
    };

    struct Song {
        long double duration;
        int channels;
        std::vector<Section> sections;
        std::vector<Part> parts;

        Song() : duration(0.0L), channels(0), sections(), parts() {}
    };

    struct ActiveNote {
        size_t noteIndex;
        float startTime, endTime;
    };

    struct PlaybackState {
        Song song;
        float currentTime;
        size_t currentSectionIdx;
        bool playing;
        std::vector<size_t> nextNoteIndices;
        std::vector<std::vector<ActiveNote>> activeNotes;
        std::vector<AudioUtils::Reverb> reverbs;
        std::vector<AudioUtils::Distortion> distortions;
        PlaybackState() : currentTime(0.0f), currentSectionIdx(0), playing(false) {}
    };

    class Instrument {
    public:
        virtual ~Instrument() = default;
        virtual double generateWave(double t, double freq, double dur) = 0;
        virtual double generateWave(double t, double freq, int phoneme, double dur, int variant) {
            return generateWave(t, freq, dur);
        }
    };

    using InstrumentFactory = std::function<std::unique_ptr<Instrument>()>;
    static std::map<std::string, InstrumentFactory>& getInstrumentRegistry() {
        static std::map<std::string, InstrumentFactory> registry;
        return registry;
    }

    template<typename T>
    struct InstrumentRegistrar {
        InstrumentRegistrar(const std::string& name) {
            getInstrumentRegistry()[name] = []() -> std::unique_ptr<Instrument> {
                return std::make_unique<T>(1.0f);
            };
        }
    };

    float generateInstrumentWave(const std::string& instrument, float t, float freq, float dur, int phoneme = -1, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE) {
        auto& registry = getInstrumentRegistry();
        std::string instName = instrument;
        if (instName.find(".h") != std::string::npos) {
            instName = instName.substr(0, instName.find(".h"));
        }
        auto it = registry.find(instName);
        if (it == registry.end()) {
            std::cerr << "Unknown instrument: " << instName << std::endl;
            return 0.0f;
        }
        auto instrumentPtr = it->second();
        if (!instrumentPtr) {
            std::cerr << "Failed to create instrument: " << instName << std::endl;
            return 0.0f;
        }
        if (instName == "vocal_0" || instName == "vocal_1") {
            int variant = (instName == "vocal_0") ? 0 : 1;
            return static_cast<float>(instrumentPtr->generateWave(t, freq, phoneme, dur, variant));
        }
        return static_cast<float>(instrumentPtr->generateWave(t, freq, dur));
    }

    float interpolateAutomation(float t, const std::vector<std::pair<float, float>>& points, float defaultValue) {
        if (points.empty()) return defaultValue;
        if (t <= points.front().first) return points.front().second;
        if (t >= points.back().first) return points.back().second;
        for (size_t i = 1; i < points.size(); ++i) {
            if (t >= points[i-1].first && t <= points[i].first) {
                float t0 = points[i-1].first, t1 = points[i].first;
                float v0 = points[i-1].second, v1 = points[i].second;
                return v0 + (v1 - v0) * (t - t0) / (t1 - t0);
            }
        }
        return defaultValue;
    }

    size_t countNotesInSection(const Song& song, const Section& section) {
        size_t count = 0;
        for (const auto& part : song.parts) {
            for (const auto& note : part.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    ++count;
                }
            }
        }
        return count;
    }

    std::string getInstrumentsInSection(const Song& song, const Section& section) {
        std::set<std::string> instruments;
        for (const auto& part : song.parts) {
            for (const auto& note : part.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    instruments.insert(part.instrument);
                    break;
                }
            }
        }
        std::string result;
        for (const auto& inst : instruments) {
            result += inst + ", ";
        }
        if (!result.empty()) result.resize(result.size() - 2);
        return result.empty() ? "none" : result;
    }
}

#include "../instruments/banjo.h"
#include "../instruments/bass.h"
#include "../instruments/bell.h"
#include "../instruments/cello.h"
#include "../instruments/clap.h"
#include "../instruments/clarinet.h"
#include "../instruments/cymbal.h"
#include "../instruments/flute.h"
#include "../instruments/guitar.h"
#include "../instruments/hihat.h"
#include "../instruments/kick.h"
#include "../instruments/leadsynth.h"
#include "../instruments/marimba.h"
#include "../instruments/oboe.h"
#include "../instruments/organ.h"
#include "../instruments/pad.h"
#include "../instruments/piano.h"
#include "../instruments/saxophone.h"
#include "../instruments/sitar.h"
#include "../instruments/snare.h"
#include "../instruments/steelguitar.h"
#include "../instruments/subbass.h"
#include "../instruments/syntharp.h"
#include "../instruments/tambourine.h"
#include "../instruments/tom.h"
#include "../instruments/trumpet.h"
#include "../instruments/tuba.h"
#include "../instruments/violin.h"
#include "../instruments/xylophone.h"
#include "../instruments/vocal.h"

#endif // INSTRUMENTS_H