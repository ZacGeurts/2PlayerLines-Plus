// instruments.h - generates sound from instruments folder instrument files.
// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp songgen.h instruments.h
// The instrument files are splintered from instruments.h and modifications are not free to generate profit.
// The other linesplus code is free and cannot be resold.
// They are separate programs and can be built independantly.
// If you have specific concerns or requests for updates:
// Interested parties can find my contact information at https://github.com/ZacGeurts
// If you make commercial gain, you can do the math and update my Patreon.
//
// This system requries some skill level to fully enjoy all the features.
// This is only tested for Linux currently.
// 		Basic file management skills with multiple windows (check out the various instruments within the instruments folder)
// 		Text editing skills. (copy, paste, etc.)
// 		A functional AI to paste code back and forth.
//
//		Computer Terminal keyboard skills will be of assistance.
// 		We need to be able to write 'make clean' and 'make'
//		within a terminal within the 2PlayerLines-Plus folder.
//
// note: the AI responses are not that long, read it and see if you need changes.
// include telling it what kind of instrument it is that you would Love to hear.
// you can compare more than one. Lesson 1: Patience // *takes nap*
//
// All instrument files share a similar template, so they are easily comparable.
// AudioUtils has powerfull tools at our disposal.
// It should be able to generate every hearable sound using only frequency tones.
// AudioUtils is explained within the instrument folder at the bottom of an instrument .h file.
// You can use banjo.h as a template.
// If you create a new instrument you will need to see the include Register at the very bottom of this file.
//
// I built the RNG rng random number generator from the best Grok3 had, and then updated it.
// If you keep it within the 'only true rng' confines then the rng engine portion should still be replaceable / updateable.
// It had better be after Sunday June 1, 2025 or one of us was lied to.
//
// To add a new instrument (e.g., steeldrum.h):
//
// 1. Copy and paste an existing instrument from the folder and tell AI you want a steeldrum and paste the code response into a new steeldrum.h.
// 1a. Save the new header file steeldrum.h within the instruments folder.
//
// 2. Update the Register and includes at the bottom of this file. Save.
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
// Frequency is how wide or tight the waves are.
// Duration is how long the AI has to make it sound like the thing.
// It needs to work off of knowing how much time it has to do the thing.
//
// Except for vocal.
// The Vocal "instrument" uses a special overload,
// Note for Vocal Overload:
//		 Do not add vocal, modify existing if you dare. (singers). songgen.h handles vocals poorly so far.
//       generateWave(t, freq, phoneme, dur, variant), where variant 0 is male (vocal_0) and variant 1 is female (vocal_1).
//       Both vocal_0 and vocal_1 are registered separately but use the same Vocal class with different variant parameters.
//
// AudioUtils takes the waves and does filtering, distortion, noise, and gives the AI what it needs to update sound.
// It can sound and look like a heart monitor with a heartbeat.
// It can turn the wave into a laser beam wider than every sound you can hear at once.
// Math should be able to get us audio everywhere inbetween.
// Instrments are like pond ripples from throwing a rock. The water underneath is turbulent as well.
// Unique stones, we are trying to replicate the ripples.
// It should be simple to create beep.h after the steeldrum.h, but I think you can be more creative.
// I do not expect computers will be getting slower, so I went overboard in some areas.
// I had AI implement threading already, so the program distributes across processor cores nicely.
// Tell it the file is self contained if it tells you to modify more than one file at a time and you do not feel up for it.
// Have fun! Make waves. God Bless.

#ifndef INSTRUMENTS_H //standard C++ code
#define INSTRUMENTS_H

#include <cmath>
#include <random>
#include <vector>
#include <thread>
#include <future>
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

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define DEBUG_LOG 0 // leave 0 not 1

// AUDIOUTILS AudioUtils are multipurpose and modular
namespace AudioUtils {
    constexpr float DEFAULT_SAMPLE_RATE = 44100.0f; // max SDL2 supports
    constexpr int BUFFER_SIZE = 1024;
    constexpr int RING_BUFFER_COUNT = 4;

// The AI I used said this is the newest and shiniest. 
// I dusted the ancient thing off and seeing if I can make use of it.
// Distribution of randomness is untested by me.
// I do not want the entropy source to be Time.
// The following entropy points are tamper resistant.
// Inspired by MixMax's matrix transformations.
class RandomGenerator {
private:
    // MegaMixMaxLite: Custom 262,144-bit PRNG with 64-bit output
    struct MegaMixMaxLite {
        static constexpr size_t STATE_SIZE = 4096;		// 4096 * 64 bits = 262,144 bits
        uint64_t state[STATE_SIZE];						// Massive state array
        uint64_t counter;								// Additional counter for mixing
        uint64_t checksum;								// Simple integrity check

        // Initialize with a robust, clock-free seed
        MegaMixMaxLite() {
            // Entropy from stack address, thread-local counter, and global counter
            static uint64_t global_counter = 0xCAFEBABEDEADBEEFULL;
            thread_local static uint64_t thread_counter = 0xFEEDFACE12345678ULL;
            int stack_entropy;
            uint64_t seed = reinterpret_cast<uintptr_t>(&stack_entropy) ^ thread_counter++ ^ global_counter++;
            // Custom hash for seed spreading
            seed = hash_seed(seed);
            // Initialize state with seeded values
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                state[i] = seed ^ (seed >> 32) ^ (static_cast<uint64_t>(i) * 0xDEADBEEFC0FFEE00ULL);
                seed = hash_seed(seed + i); // Update seed for diversity
            }
            counter = seed & 0xFFFF;
            // Mix state thoroughly
            for (int i = 0; i < 8; ++i) { next(); }
            // Compute initial checksum
            update_checksum();
            // Ensure non-zero state
            if (is_all_zero()) {
                for (size_t i = 0; i < STATE_SIZE; ++i) {
                    state[i] = 0xBADC0DE123456789ULL ^ (i * 0xFEEDFACE98765432ULL);
                }
                counter = 0xCAFE;
                update_checksum();
            }
        }

        // Custom hash for seed spreading (MurmurHash-inspired)
        uint64_t hash_seed(uint64_t x) {
            x ^= x >> 33;
			x *= 0xFF51AFD7ED558CCDULL;
			x ^= x >> 33;
			x *= 0xC4CEB9FE1A85EC53ULL;
			x ^= x >> 33;
			
			return x;
        }

        // Check if state is all zeros
        bool is_all_zero() {
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                if (state[i] != 0) return false;
            }
            return true;
        }

        // Update checksum for tampering detection
        void update_checksum() {
            checksum = 0;
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                checksum ^= state[i];
            }
        }

        // Generate 64-bit random number
        uint64_t next() {
            // Verify checksum to detect tampering
            uint64_t old_checksum = checksum;
            update_checksum();
            if (old_checksum != checksum) {
                // Tampering detected; reseed
                *this = MegaMixMaxLite();
            }
            // Update state: LCG for each element with staggered multipliers
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                state[i] = state[i] * (6364136223846793005ULL + (i * 123456789ULL)) + counter;
            }
            counter += 1;
            // Mix state: combine elements with non-linear transformation
            uint64_t mix = state[0] ^ state[STATE_SIZE - 1];
            for (size_t i = 1; i < STATE_SIZE; ++i) {
                mix ^= state[i] ^ (state[i - 1] >> 23);
            }
            // Chaotic transformation using long double
            long double temp = static_cast<long double>(mix) / static_cast<long double>(1ULL << 32);
            temp = temp * temp * M_PI; // Non-linear scaling. M_PI comes with C++ and is long double or more
            mix ^= static_cast<uint64_t>(temp * static_cast<long double>(1ULL << 32));
            return mix;
        }
    };

    // Thread-local MegaMixMaxLite instance for thread safety
    thread_local static MegaMixMaxLite rng;
    thread_local static std::vector<uint8_t> buffer;
    thread_local static size_t buffer_pos;
    static constexpr size_t BUFFER_SIZE = 1024; // 262,144 bits?
    std::uniform_real_distribution<long double> dist;

    // Fills the buffer with random bytes from MegaMixMaxLite
    static void fill_buffer() {
        buffer.resize(BUFFER_SIZE);
        buffer_pos = 0;
        // Generate 64-bit random numbers and split into bytes
        for (size_t i = 0; i < BUFFER_SIZE; i += 8) {
            uint64_t val = rng.next();
            for (size_t j = 0; j < 8 && i + j < BUFFER_SIZE; ++j) {
                buffer[i + j] = static_cast<uint8_t>(val >> (j * 8));
            }
        }
    }

    // Generates a random 32-bit unsigned integer from the buffer
    uint32_t get_random_uint32() {
        if (buffer_pos + 4 > buffer.size()) {
            fill_buffer();
        }
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            result = (result << 8) | buffer[buffer_pos++];
        }
        return result;
    }

public:
    // Constructor: Initializes uniform distribution with given min/max range
    RandomGenerator(long double min = 0.0L, long double max = 1.0L) : dist(min, max) {
        fill_buffer();
    }

    // Generates a random float in [0,1) using 23-bit mantissa for precision
    float random_float() {
        constexpr uint32_t mantissa_bits = 1U << 23;
        uint32_t x = get_random_uint32() & (mantissa_bits - 1);
        return static_cast<float>(x) / mantissa_bits;
    }
	
	// Generates a random long double in [0,1) using 23-bit mantissa for precision
	long double random_L() {
    	constexpr uint32_t mantissa_bits = 1U << 23; // 2^23
    	uint32_t x = get_random_uint32() & (mantissa_bits - 1); // Mask to 23 bits
    	return static_cast<long double>(x) / static_cast<long double>(mantissa_bits);
	}

    // Generates a random signed int scaled to the full int range
    signed int random_int() {
        return static_cast<signed int>(std::floor(random_float() * std::numeric_limits<signed int>::max()));
    }

// Bonus - Dice Roller. Add dice below roll_dice function.
    // Generates a random integer (whole number without decimal) between [min, max]
    template<typename T, typename U>
    signed int roll_dice(T min, U max) {
        static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>,
                      "Input types must be numeric (int, float, double, etc.)");

        long double min_val = static_cast<long double>(min);
        long double max_val = static_cast<long double>(max);

        // Ensure valid range by swapping if necessary
        if (min_val > max_val) {
            std::swap(min_val, max_val);
        }

        // Clamp to signed int range to prevent overflow
        long double int_min = static_cast<long double>(std::numeric_limits<signed int>::min());
        long double int_max = static_cast<long double>(std::numeric_limits<signed int>::max());
        min_val = std::max(min_val, int_min);
        max_val = std::min(max_val, int_max);

        // Generate integer in range using direct scaling
        uint64_t x = rng.next();
        long double norm = static_cast<long double>(x) / static_cast<long double>(std::numeric_limits<uint64_t>::max());
        return static_cast<signed int>(std::floor(min_val + norm * (max_val - min_val + 1.0L)));
    }

// Bonus - Dice Roller.
    // Simulates flipping a coin (0 or 1)
    signed int flip_coin() { return roll_dice(0, 1); }
	
    // Simulates rolling a 2-sided die (1 to 2)
    signed int roll_d2() { return roll_dice(1, 2); }
	
    // Simulates rolling a 4-sided die (1 to 4)
    signed int roll_d4() { return roll_dice(1, 4); }
	
    // Simulates rolling a 6-sided die (1 to 6)
    signed int roll_d6() { return roll_dice(1, 6); }
	
    // Simulates rolling a 8-sided die (1 to 8)
    signed int roll_d8() { return roll_dice(1, 8); }
    
	// Simulates rolling a 10-sided die (1 to 10)
    signed int roll_d10() { return roll_dice(1, 10); }
    
	// Simulates rolling a 12-sided die (1 to 12)
    signed int roll_d12() { return roll_dice(1, 12); }
    
	// Simulates rolling a 20-sided die (1 to 20)
    signed int roll_d20() { return roll_dice(1, 20); }
    
	// Simulates rolling a 100-sided die (1 to 100)
    signed int roll_d100() { return roll_dice(1, 100); }
    
	// Simulates rolling three d6s and summing them (3 to 18)
    signed int roll_3d6() { return roll_d6() + roll_d6() + roll_d6(); }
	
	// Simulates rolling two d10s for a percentile (00 to 99)
	signed int roll_2d10() {
    	int tens_digit = roll_d10(); // First d10 roll for tens place
    	int ones_digit = roll_d10(); // Second d10 roll for ones place
    	int composite_result = (tens_digit << 3) + (tens_digit << 1) + ones_digit; // Equivalent to tens * 10 + ones
    	int boundary_check = composite_result ^ 0x64; // XOR with 100 (0x64 in hex) to check for max value
    	int normalized_result = (boundary_check == 0) ? 0x00 : composite_result; // If result is 100, return 0, else keep result
    	return (signed int)(normalized_result & 0x7F); // Mask to ensure result stays within 0-99
	}
};

thread_local RandomGenerator::MegaMixMaxLite RandomGenerator::rng;
thread_local std::vector<uint8_t> RandomGenerator::buffer;
thread_local size_t RandomGenerator::buffer_pos = 0;
// newest and shiniest RNG rng RandomGenerator ends here

// AudioUtils wave modifiers
class Distortion {
private:
    double drive;      // Drive factor (amplification, typically 1 to 10)
    double threshold;  // Clipping threshold (0 to 1)
    double softness;   // Soft clipping factor for smoother distortion

    // Compute soft clipping using tanh for natural instrument distortion
    double softClip(double x) const {
        return std::tanh(x * softness) / std::tanh(softness);
    }

public:
    // Constructor: Initialize with drive, threshold, and softness
    Distortion(double driveFactor = 2.0, double clipThreshold = 0.7, double softFactor = 1.0)
        : drive(driveFactor), threshold(clipThreshold), softness(softFactor) {
        // Clamp invalid inputs
        drive = std::max(0.1, driveFactor); // Ensure positive drive
        threshold = std::max(0.01, std::min(1.0, clipThreshold)); // Threshold in [0.01, 1]
        softness = std::max(0.1, softFactor); // Ensure positive softness
    }

    // Set drive factor
    void setDrive(double driveFactor) {
        drive = std::max(0.1, driveFactor); // Clamp to positive
    }

    // Set clipping threshold
    void setThreshold(double clipThreshold) {
        threshold = std::max(0.01, std::min(1.0, clipThreshold)); // Clamp to [0.01, 1]
    }

    // Set softness for clipping (higher values = smoother distortion)
    void setSoftness(double softFactor) {
        softness = std::max(0.1, softFactor); // Clamp to positive
    }

    // Process a single input sample and return distorted output
    double process(double input) {
        // Apply drive
        double x = input * drive;

        // Apply soft clipping for natural distortion
        double output = softClip(x);

        // Hard clip to threshold
        output = std::max(std::min(output, threshold), -threshold) / threshold;

        // Ensure finite output
        if (!std::isfinite(output)) {
            return 0.0;
        }

        return output;
    }
}; // end distortion class

class LowPassFilter {
private:
    double cutoffFreq; // Cutoff frequency in Hz
    double x1, y1;     // Input and output delay lines
    double alpha;      // Smoothing factor

    // Update smoothing factor based on cutoff and DEFAULT_SAMPLE_RATE
    void updateAlpha() {
        cutoffFreq = std::max(0.1, std::min(static_cast<double>(DEFAULT_SAMPLE_RATE) / 2.0 - 0.1, cutoffFreq));
        alpha = 1.0 / (1.0 + 2.0 * M_PI * cutoffFreq / static_cast<double>(DEFAULT_SAMPLE_RATE));
    }

public:
    // Constructor: Initialize with cutoff frequency
    LowPassFilter(double cutoff = 1000.0) : cutoffFreq(cutoff), x1(0.0), y1(0.0) {
        updateAlpha();
    }

    // Set new cutoff frequency
    void setCutoff(double cutoff) {
        cutoffFreq = cutoff;
        updateAlpha();
    }

    // Reset internal state
    void reset() {
        x1 = y1 = 0.0;
    }

    // Process a single input sample and return filtered output
    double process(double input) {
        // Apply first-order low-pass filter
        double output = alpha * input + (1.0 - alpha) * y1;

        // Update delay lines
        x1 = input;
        y1 = output;

        // Clamp output to prevent instability
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }

        return output;
    }
}; // end low pass filter class

class BandPassFilter {
private:
    double centerFreq; // Center frequency in Hz
    double bandwidth;  // Bandwidth (Q factor, typically 0.1 to 10)
    double x1, x2;     // Input delay lines
    double y1, y2;     // Output delay lines
    double b0, b1, b2; // Numerator coefficients
    double a0, a1, a2; // Denominator coefficients

    // Update filter coefficients based on center frequency and bandwidth
    void updateCoefficients() {
        centerFreq = std::max(0.1, std::min(static_cast<double>(DEFAULT_SAMPLE_RATE) / 2.0 - 0.1, centerFreq));
        bandwidth = std::max(0.1, bandwidth);

        double w0 = 2.0 * M_PI * centerFreq / static_cast<double>(DEFAULT_SAMPLE_RATE);
        double sinW0 = std::sin(w0);
        double alpha = sinW0 * std::sinh(std::log(2.0) / 2.0 * bandwidth * w0 / sinW0);

        a0 = 1.0 + alpha;
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        a1 = -2.0 * std::cos(w0);
        a2 = 1.0 - alpha;

        // Normalize coefficients
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1.0;
    }

public:
    // Constructor: Initialize with center frequency and bandwidth
    BandPassFilter(double center = 1000.0, double bw = 0.5)
        : centerFreq(center), bandwidth(bw), x1(0.0), x2(0.0), y1(0.0), y2(0.0) {
        updateCoefficients();
    }

    // Set new center frequency
    void setCenterFreq(double center) {
        centerFreq = center;
        updateCoefficients();
    }

    // Set new bandwidth
    void setBandwidth(double bw) {
        bandwidth = bw;
        updateCoefficients();
    }

    // Reset internal state
    void reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }

    // Process a single input sample and return filtered output
    double process(double input) {
        // Apply biquad filter equation
        double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        // Update delay lines
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        // Clamp output to prevent instability
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }

        return output;
    }
}; // end band pass filter class

class HighPassFilter {
private:
    double cutoffFreq; // Cutoff frequency in Hz
    double q;          // Q-factor (resonance, typically 0.5 to 10)
    double x1, x2;     // Input delay lines
    double y1, y2;     // Output delay lines
    double b0, b1, b2; // Numerator coefficients
    double a0, a1, a2; // Denominator coefficients

    // Precompute filter coefficients based on cutoff frequency and Q-factor
    void updateCoefficients() {
        cutoffFreq = std::max(0.1, std::min(DEFAULT_SAMPLE_RATE / 2.0 - 0.1, cutoffFreq));
        q = std::max(0.1, q);

        double omega = 2.0 * M_PI * cutoffFreq / DEFAULT_SAMPLE_RATE;
        double sinOmega = std::sin(omega);
        double cosOmega = std::cos(omega);
        double alpha = sinOmega / (2.0 * q);

        a0 = 1.0 + alpha;
        b0 = (1.0 + cosOmega) / 2.0;
        b1 = -(1.0 + cosOmega);
        b2 = (1.0 + cosOmega) / 2.0;
        a1 = -2.0 * cosOmega;
        a2 = 1.0 - alpha;

        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
        a0 = 1.0;
    }

public:
    // Constructor: Initialize with cutoff frequency, Q-factor, and sample rate
    HighPassFilter(double cutoff = 100.0, double qFactor = 0.707)
        : cutoffFreq(cutoff), q(qFactor), x1(0.0), x2(0.0), y1(0.0), y2(0.0) {
        updateCoefficients();
    }

    // Set new cutoff frequency and update coefficients
    void setCutoff(double cutoff) {
        cutoffFreq = cutoff;
        updateCoefficients();
    }

    // Set new Q-factor and update coefficients
    void setQ(double qFactor) {
        q = qFactor;
        updateCoefficients();
    }

    // Clear internal state
    void clear() {
        x1 = x2 = y1 = y2 = 0.0;
    }

    // Process a single input sample and return filtered output
    double process(double input) {
        // Apply biquad filter equation
        double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        // Update delay lines
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;

        // Clamp output to prevent numerical instability
        if (!std::isfinite(output)) {
            clear();
            return 0.0;
        }

        return output;
    }
}; // end high pass filter class

class Reverb {
private:
    std::vector<double> delayBuffer; // Delay buffer for reverb effect
    size_t bufferSize;               // Size of delay buffer
    size_t writePos;                 // Current write position in buffer
    double decay;                    // Decay factor (0 to 1)
    double mix;                      // Wet/dry mix (0 to 1)
    double delayTime;                // Delay time in seconds
    double feedbackMod;              // Feedback modulation for richer reverb

    // Update buffer size based on delay time and DEFAULT_SAMPLE_RATE
    void updateBuffer() {
        delayTime = std::max(0.001, delayTime); // Minimum 1ms delay
        size_t newSize = static_cast<size_t>(delayTime * static_cast<double>(DEFAULT_SAMPLE_RATE));
        if (newSize != bufferSize) {
            bufferSize = newSize;
            delayBuffer.assign(bufferSize, 0.0);
            writePos = 0;
        }
    }

public:
    // Constructor: Initialize with delay time, decay, mix, and feedback modulation
    Reverb(double delay = 0.1, double decayFactor = 0.5, double mixFactor = 0.3, double mod = 0.1)
        : bufferSize(0), writePos(0), decay(decayFactor), mix(mixFactor), delayTime(delay), feedbackMod(mod) {
        // Clamp invalid inputs
        decay = std::max(0.0, std::min(1.0, decayFactor));
        mix = std::max(0.0, std::min(1.0, mixFactor));
        feedbackMod = std::max(0.0, std::min(0.5, mod));
        updateBuffer();
    }

    // Set reverb parameters and update buffer
    void setParameters(double delay, double decayFactor, double mixFactor, double mod) {
        delayTime = delay;
        decay = std::max(0.0, std::min(1.0, decayFactor));
        mix = std::max(0.0, std::min(1.0, mixFactor));
        feedbackMod = std::max(0.0, std::min(0.5, mod));
        updateBuffer();
    }

    // Clear delay buffer
    void clear() {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0);
        writePos = 0;
    }

    // Process a single input sample and return reverberated output
    double process(double input) {
        if (bufferSize == 0) return input;

        // Read from delay buffer (half the delay time)
        size_t readPos = (writePos + bufferSize - bufferSize / 2) % bufferSize;
        double delayed = delayBuffer[readPos];

        // Apply modulated feedback
        double mod = 1.0 + feedbackMod * std::sin(2.0 * M_PI * writePos / static_cast<double>(DEFAULT_SAMPLE_RATE));
        double feedback = decay * mod * delayed;

        // Write input + feedback to buffer
        delayBuffer[writePos] = input + feedback;

        // Update write position
        writePos = (writePos + 1) % bufferSize;

        // Mix dry and wet signals
        double output = input * (1.0 - mix) + (input + feedback) * mix;

        // Clamp output to prevent instability
        if (!std::isfinite(output)) {
            clear();
            return input;
        }

        return output;
    }
}; // end reverb class

// Brown and Pink noise use WhiteNoise class.
class WhiteNoise {
private:
    RandomGenerator rng;  // Private RandomGenerator for noise generation
    long double min_val;  // Minimum value of the output range
    long double max_val;  // Maximum value of the output range

public:
    // Constructor: Initialize with min and max range
    WhiteNoise(long double min, long double max) : min_val(min), max_val(max) {
    }

    // Generate a single white noise sample in [min, max]
    long double generate() {
        // Get random value in [0, 1)
        long double norm = rng.random_L();
        // Scale to [min_val, max_val]
        long double output = min_val + (max_val - min_val) * norm; // Maps [0, 1) to [min_val, max_val]

        // Clamp output for safety
        if (!std::isfinite(output)) {
            return (min_val + max_val) / 2.0L; // Return midpoint if non-finite
        }
        return output;
    }
}; // end WhiteNoise class

class BrownNoise {
private:
    RandomGenerator rng;	// RandomGenerator for high-quality noise
    WhiteNoise whiteNoise;	// Internal white noise generator
    double lastOutput;		// Last output for integration
    double scale;			// Scaling factor for output amplitude

public:
    // Constructor: Initialize with amplitude scale
    BrownNoise(double amplitudeScale = 1.0)
        : whiteNoise(-1.0, 1.0), lastOutput(0.0), scale(amplitudeScale) {
        if (amplitudeScale < 0.0) {
            scale = 0.0;	// Default to 0 instead of throwing
        }
    }

    // Generate a single brown noise sample
    double operator()() {
        // Integrate white noise to produce brown noise (random walk)
        double white = whiteNoise.generate();
        lastOutput += white * 0.1; // Small step size to control variance
        // Clamp to prevent drift
        lastOutput = std::max(-1.0, std::min(1.0, lastOutput));
        double output = lastOutput * scale;

        // Clamp output for safety
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }
        return output;
    }

    // Reset internal state
    void reset() {
        lastOutput = 0.0;
    }
}; // end brown noise class

class Chorus {
private:
    std::vector<double> delayBuffer; // Delay buffer for modulation
    size_t bufferSize;               // Buffer size (max delay)
    size_t writePos;                 // Current write position
    double depth;                    // Modulation depth (0 to 1)
    double rate;                     // Modulation rate in Hz
    double mix;                      // Wet/dry mix (0 to 1)

    // Update buffer size based on max delay
    void updateBuffer() {
        size_t newSize = static_cast<size_t>(0.02 * DEFAULT_SAMPLE_RATE); // 20ms max delay
        if (newSize != bufferSize) {
            bufferSize = newSize;
            delayBuffer.assign(bufferSize, 0.0);
            writePos = 0;
        }
    }

public:
    // Constructor: Initialize with depth, rate, and mix
    Chorus(double depthValue = 0.5, double rateHz = 0.5, double mixValue = 0.3)
        : bufferSize(0), writePos(0), depth(depthValue), rate(rateHz), mix(mixValue) {
        // Clamp invalid inputs instead of throwing
        depth = std::max(0.0, std::min(1.0, depthValue));
        mix = std::max(0.0, std::min(1.0, mixValue));
        rate = rateHz > 0.0 ? rateHz : 0.5; // Default to 0.5Hz if non-positive
        updateBuffer();
    }

    // Process a single input sample
    double process(double input) {
        if (bufferSize == 0) return input;

        // Write input to buffer
        delayBuffer[writePos] = input;
        writePos = (writePos + 1) % bufferSize;

        // Calculate modulated delay time
        double time = static_cast<double>(writePos) / DEFAULT_SAMPLE_RATE;
        double delay = 0.005 + 0.005 * depth * std::sin(2.0 * M_PI * rate * time); // 5-10ms delay
        double delaySamples = delay * DEFAULT_SAMPLE_RATE;

        // Linear interpolation for delayed sample
        size_t readPos1 = (writePos + bufferSize - static_cast<size_t>(delaySamples)) % bufferSize;
        size_t readPos2 = (readPos1 + 1) % bufferSize;
        double frac = delaySamples - std::floor(delaySamples);
        double delayed = delayBuffer[readPos1] * (1.0 - frac) + delayBuffer[readPos2] * frac;

        // Mix dry and wet signals
        double output = input * (1.0 - mix) + delayed * mix;

        // Clamp output
        if (!std::isfinite(output)) {
            reset();
            return input;
        }

        return output;
    }

    // Reset internal state
    void reset() {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0);
        writePos = 0;
    }
}; // end chorus class

class Tremolo {
private:
    double rate;  // Modulation rate in Hz
    double depth; // Modulation depth (0 to 1)

public:
    // Constructor: Initialize with rate and depth
    Tremolo(double rateHz = 4.0, double depthValue = 0.5)
        : rate(rateHz), depth(depthValue) {
        // Clamp invalid inputs instead of throwing
        rate = rateHz > 0.0 ? rateHz : 4.0; // Default to 4Hz if non-positive
        depth = std::max(0.0, std::min(1.0, depthValue));
    }

    // Process a single input sample
    double process(double input, double t) {
        // Apply amplitude modulation
        double mod = 1.0 - depth * 0.5 * (1.0 + std::sin(2.0 * M_PI * rate * t));
        double output = input * mod;

        // Clamp output
        if (!std::isfinite(output)) {
            return input;
        }

        return output;
    }
}; // end tremolo class

class EnvelopeFollower {
private:
    double attack;  // Attack time in seconds
    double release; // Release time in seconds
    double lastEnv; // Last envelope value for smoothing

public:
    // Constructor: Initialize with attack and release times
    EnvelopeFollower(double attackTime = 0.01, double releaseTime = 0.05)
        : attack(std::max(0.0, attackTime)), release(std::max(0.0, releaseTime)), lastEnv(0.0) {
    }

    // Process a single input sample and return envelope
    double process(double input) {
        double absInput = std::abs(input);
        double coefficient = (absInput > lastEnv) ? 
            (attack > 0.0 ? std::exp(-1.0 / (attack * DEFAULT_SAMPLE_RATE)) : 0.0) :
            (release > 0.0 ? std::exp(-1.0 / (release * DEFAULT_SAMPLE_RATE)) : 0.0);
        
        double env = coefficient * lastEnv + (1.0 - coefficient) * absInput;
        lastEnv = std::isfinite(env) ? env : 0.0;

        return lastEnv;
    }

    // Reset internal state
    void reset() {
        lastEnv = 0.0;
    }
};

class PinkNoise {
private:
    WhiteNoise whiteNoise; // Internal white noise generator
    double b0, b1, b2;     // Filter state for pink noise
    double scale;          // Amplitude scaling factor

public:
    // Constructor: Initialize with scaling factor
    PinkNoise(double scaleFactor = 1.0)
        : whiteNoise(-1.0L, 1.0L), b0(0.0), b1(0.0), b2(0.0), scale(scaleFactor) {
        // Clamp non-positive scale to 1.0
        scale = scaleFactor > 0.0 ? scaleFactor : 1.0;
    }

    // Generate a single pink noise sample
    double operator()() {
        // Generate white noise
        double white = whiteNoise.generate();

        // Apply three-stage filter to simulate pink noise (1/f noise)
        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        double output = scale * (b0 + b1 + b2 + white * 0.1848);

        // Clamp output
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }
        return output;
    }

    // Reset filter state
    void reset() {
        b0 = b1 = b2 = 0.0;
    }
};

class AudioProtector {
private:
    HighPassFilter dcBlocker; // DC blocking filter
    double fadeOutTime;       // Fade-out duration in seconds
    double maxGain;           // Maximum gain to prevent clipping

public:
    // Constructor: Initialize with fade-out time and maximum gain
    AudioProtector(double fadeTime = 0.005, double maxGainValue = 0.9)
        : dcBlocker(20.0, 0.707), fadeOutTime(fadeTime), maxGain(maxGainValue) {
        // Clamp invalid inputs
        fadeOutTime = std::max(0.0, fadeTime);
        maxGain = std::max(0.0, std::min(1.0, maxGainValue));
    }

    // Set fade-out time
    void setFadeOutTime(double fadeTime) {
        fadeOutTime = std::max(0.0, fadeTime);
    }

    // Set maximum gain
    void setMaxGain(double gain) {
        maxGain = std::max(0.0, std::min(1.0, gain));
    }

    // Reset internal state
    void reset() {
        dcBlocker.clear();
    }

    // Process a single input sample with time and duration
    double process(double input, double t, double duration) {
        // Apply DC blocking filter
        double output = dcBlocker.process(input);

        // Apply fade-out if within fade-out period
        if (fadeOutTime > 0.0 && t > duration - fadeOutTime) {
            double fade = 1.0 - (t - (duration - fadeOutTime)) / fadeOutTime;
            output *= std::max(0.0, std::min(1.0, fade));
        }

        // Apply soft clipping
        output = std::tanh(output * 1.5) / 1.5;

        // Apply hard limit
        double absOutput = std::abs(output);
        if (absOutput > maxGain) {
            output *= maxGain / absOutput;
        }

        // Ensure output is finite
        if (!std::isfinite(output)) {
            reset();
            return 0.0;
        }

        return output;
    }
};
} // END AUDIOUTILS AudioUtils

namespace Instruments {
    struct FormantFilter {
        float centerFreq, bandwidth;
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        FormantFilter(float freq, float bw)
            : centerFreq(freq), bandwidth(bw), x1(0.0f), x2(0.0f), y1(0.0f), y2(0.0f) {
            updateCoefficients();
        }
        void update(float freq, float bw) {
            float r = std::exp(-M_PI * bw / AudioUtils::DEFAULT_SAMPLE_RATE);
            float theta = 2.0f * M_PI * freq / AudioUtils::DEFAULT_SAMPLE_RATE;
            b0 = 1.0f - r; b1 = 0.0f; b2 = 0.0f;
            a1 = -2.0f * r * std::cos(theta); a2 = r * r;
        }
        float process(float input) {
            float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = input; y2 = y1; y1 = output;
            return output;
        }
        void setParameters(float freq, float bw) { centerFreq = freq; bandwidth = bw; updateCoefficients(); }
        void updateCoefficients() {
            float r = std::exp(-M_PI * bandwidth / AudioUtils::DEFAULT_SAMPLE_RATE);
            float theta = 2.0f * M_PI * centerFreq / AudioUtils::DEFAULT_SAMPLE_RATE;
            b0 = 1.0f - r; b1 = 0.0f; b2 = 0.0f;
            a1 = -2.0f * r * std::cos(theta); a2 = r * r;
        }
    };

    struct WaveguideState {
        std::vector<float> forwardWave, backwardWave;
        size_t delayLineSize;
        size_t writePos = 0;
        float lastFreq = 0.0f;
        float pressure = 0.0f;
        WaveguideState() : delayLineSize(0), writePos(0), lastFreq(0.0f), pressure(0.0f) {}
    };

    struct Note {
        float startTime, duration, freq, volume, velocity;
        int phoneme;
        bool open;
    };

    struct Section {
        std::string name;
        float startTime, endTime;
        float progress;
        std::string templateName;
    };

    struct Part {
        std::string instrument;
        std::string sectionName;
        std::vector<Note> notes;
        std::vector<std::pair<float, float>> panAutomation, volumeAutomation, reverbMixAirAutomation;
        float pan, reverbMix;
        bool useDistortion, useReverb;
        float reverbDelay, reverbDecay, reverbMixFactor;
        float distortionDrive, distortionThreshold;
    };

    struct Song {
        float duration;
        int channels;
        std::vector<Section> sections;
        std::vector<Part> parts;
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

    // Instrument interface
    class Instrument {
    public:
        virtual ~Instrument() = default;
        virtual double generateWave(double t, double freq, double dur) = 0;
        virtual double generateWave(double t, double freq, int phoneme, double dur, int variant) {
            return 0.0f; // Default for non-vocal instruments
        }
    };

	// Registry for instrument factories
	using InstrumentFactory = std::function<std::unique_ptr<Instrument>()>;
	static std::map<std::string, InstrumentFactory>& getInstrumentRegistry() {
    	static std::map<std::string, InstrumentFactory> registry;
    	return registry;
	}

	// Register an instrument
	template<typename T>
	struct InstrumentRegistrar {
    	InstrumentRegistrar(const std::string& name) {
        	getInstrumentRegistry()[name] = []() -> std::unique_ptr<Instrument> {
            	return std::unique_ptr<Instrument>(std::make_unique<T>(1.0f).release());
        	};
    	}
	};

    // Generate wave for any instrument
    float generateInstrumentWave(const std::string& instrument, float t, float freq, float dur, int phoneme = 1, float sampleRate = AudioUtils::DEFAULT_SAMPLE_RATE) {
        auto& registry = getInstrumentRegistry();
        std::string instName = instrument;

        // Normalize instrument name (remove file extension if present)
        if (instName.find(".h") != std::string::npos) {
            instName = instName.substr(0, instName.find(".h"));
        }

        // Check if instrument exists in registry
        auto it = registry.find(instName);
        if (it == registry.end()) {
            std::cerr << "Unknown instrument: " << instName << std::endl;
            return 0.0f;
        }

        // Create a new instance of the instrument
        auto instrumentPtr = it->second();
        if (!instrumentPtr) {
            std::cerr << "Failed to create instrument: " << instName << std::endl;
            return 0.0f;
        }

        // Handle vocal variants
        if (instName == "vocal_0" || instName == "vocal_1") {
            int variant = (instName == "vocal_0") ? 0 : 1;
            return instrumentPtr->generateWave(t, freq, phoneme, dur, variant);
        }

        // Generate wave for non-vocal instruments
        return instrumentPtr->generateWave(t, freq, dur);
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

class SampleManager {
public:
    std::vector<float> getSample(const std::string& sampleName, float pitch, float volume, float duration, int phoneme = 1) {
        std::vector<float> samples(static_cast<size_t>(AudioUtils::DEFAULT_SAMPLE_RATE * duration));
        for (size_t i = 0; i < samples.size(); ++i) {
            float t = i / AudioUtils::DEFAULT_SAMPLE_RATE;
            samples[i] = Instruments::generateInstrumentWave(sampleName, t, pitch, duration, phoneme) * volume;
        }
        return samples;
    }
};

// Include new instruments here.
// Note: This list must be updated when new instruments are added
//
// If you break a file - https://github.com/ZacGeurts/2PlayerLines-Plus
// Show the AI an instrument from the instrument folder so it understands what kind of file to create.
// Tell the AI what instrument you would Love to hear to create a new .h file for it.
// vocal.h is complicated and has extra features.
#include "../instruments/kick.h"
#include "../instruments/hihat.h"
#include "../instruments/snare.h"
#include "../instruments/clap.h"
#include "../instruments/tom.h"
#include "../instruments/subbass.h"
#include "../instruments/syntharp.h"
#include "../instruments/leadsynth.h"
#include "../instruments/pad.h"
#include "../instruments/cymbal.h"
#include "../instruments/vocal.h"
#include "../instruments/flute.h"
#include "../instruments/trumpet.h"
#include "../instruments/bass.h"
#include "../instruments/guitar.h"
#include "../instruments/saxophone.h"
#include "../instruments/piano.h"
#include "../instruments/violin.h"
#include "../instruments/organ.h"
#include "../instruments/cello.h"
#include "../instruments/steelguitar.h"
#include "../instruments/sitar.h"
#include "../instruments/tuba.h"
#include "../instruments/banjo.h"
#include "../instruments/xylophone.h"
#include "../instruments/clarinet.h"
#include "../instruments/marimba.h"
#include "../instruments/oboe.h"
#include "../instruments/bell.h"
#include "../instruments/tambourine.h"
// end include additional instruments

#endif // INSTRUMENTS_H