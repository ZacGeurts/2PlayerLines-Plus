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
// There are some nefarious limitations, like faking a policeman phone call, but making sound is a human right.
// Maybe there is a usage for frequencies exceeding this many 31415926535897932384626433832795029L
/* 	
	Far out of this scope. I cap to 20hz and less than 44100hz (SDL2 maximum and exceeds human hearing).
 	We can go below 20hz down to 0hz, but top of the line car stereos might hit 8hz-12hz with expensive equipment.
 	It requires too much voltage to go lower and you would not hear a difference.
 	20hz-80hz should be top quality for a subwoofer and it does not try blowing out pc speakers.
*/
// You would need more than a speaker. Fun fact: WiFi is frequencies. Sound you cannot hear.
// Do not restrict emergency communications or damage heart pace makers, etc.
// ---
// Always put hearing safety first. It does not grow back.
// Be kind to pets. Sensitive ears.


// This system requries some skill level to fully enjoy all the features.
// This is only tested for Linux currently.
// 		Basic file management skills with multiple windows
// 		Text editing skills. (copy, paste, etc.)
// 		A functional AI to paste code back and forth.
//
//		Computer Terminal keyboard skills will be of assistance.
// 		We need to be able to write 'make clean' and 'make'
//		Knowing how to program will help with making more complicated changes.
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
//		 Do not add vocal, modify existing. (simulates singers).
//       generateWave(t, freq, phoneme, dur, variant), variant 0 is male (vocal_0), variant 1 is female (vocal_1).
//       Both vocal_0 and vocal_1 are registered separately but use the same Vocal class with different variant parameters.
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

#ifndef INSTRUMENTS_H //standard C++ code
#define INSTRUMENTS_H

#include <cmath>
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
// #include <random> // nope

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#define DEBUG_LOG 0 // leave 0 not 1

// fast tab is searching for --- and nexting.

// AUDIOUTILS AudioUtils are multipurpose and modular
namespace AudioUtils {
    constexpr float DEFAULT_SAMPLE_RATE = 44100.0f; // max SDL2 supports
    constexpr int BUFFER_SIZE = 1024;
    constexpr int RING_BUFFER_COUNT = 4;

// -----------------------------
// The AI I used said this is the newest and shiniest. 
// I dusted the ancient thing off and seeing if I can make use of it.
// Distribution of randomness is untested by me.
// I do not want the entropy source to be Time.
// The following entropy points are tamper resistant.
// Inspired by MixMax's matrix transformations.
class RandomGenerator {
private:
	// Custom 262,144-bit PRNG with long double state for high-precision waveform data
    struct MegaMixMaxLite {
        static constexpr size_t STATE_SIZE = 4096; // 4096 elements for state array
        long double state[STATE_SIZE];            // Long double state for precision
        long double counter;                      // Counter for state mixing
        long double checksum;                     // Checksum for tampering detection

        // Initialize with robust, clock-free seed
        MegaMixMaxLite() {
            static long double global_counter = 0xCAFEBABEDEADBEEFULL; // Global seed
            thread_local static long double thread_counter = 0xFEEDFACE12345678ULL; // Thread-specific
            int stack_entropy;
            long double seed = static_cast<long double>(reinterpret_cast<uintptr_t>(&stack_entropy))
                             + thread_counter++ + global_counter++; // Use + instead of ^
            seed = hash_seed(seed); // Spread seed

            // Initialize state array
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                state[i] = seed
                         + (seed / static_cast<long double>(1ULL << 32)) // Use + instead of ^
                         + (static_cast<long double>(i) * 0xDEADBEEFC0FFEE00ULL);
                seed = hash_seed(seed + static_cast<long double>(i));
            }
            counter = seed;
            for (int i = 0; i < 8; ++i) { next(); } // Initial mixing
            update_checksum();

            // Ensure non-zero state
            if (is_all_zero()) {
                for (size_t i = 0; i < STATE_SIZE; ++i) {
                    state[i] = static_cast<long double>(0xBADC0DE123456789ULL)
                             + (static_cast<long double>(i) * 0xFEEDFACE98765432ULL); // Use +
                }
                counter = 0xCAFE;
                update_checksum();
            }
        }

        // MurmurHash-inspired seed spreading
        long double hash_seed(long double x) {
            x = x * static_cast<long double>(0xFF51AFD7ED558CCDULL);
            x = x * static_cast<long double>(0xC4CEB9FE1A85EC53ULL);
            return x;
        }

        // Check for all-zero state
        bool is_all_zero() {
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                if (state[i] != 0.0L) return false;
            }
            return true;
        }

        // Update checksum
        void update_checksum() {
            checksum = 0.0L;
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                checksum += state[i];
            }
        }

        // Generate long double random number
        long double next() {
            long double old_checksum = checksum;
            update_checksum();
            if (old_checksum != checksum) {
                *this = MegaMixMaxLite(); // Reseed
            }
            for (size_t i = 0; i < STATE_SIZE; ++i) {
                state[i] = state[i] * (static_cast<long double>(6364136223846793005ULL)
                                + (static_cast<long double>(i) * 123456789.0L))
                         + counter;
            }
            counter += 1.0L;
            long double mix = state[0] + state[STATE_SIZE - 1];
            for (size_t i = 1; i < STATE_SIZE; ++i) {
                mix += state[i] + (state[i - 1] / static_cast<long double>(1ULL << 23));
            }
            // Chaotic transformation
            long double temp = mix / static_cast<long double>(1ULL << 32);
            temp = temp * temp * static_cast<long double>(M_PI);
            mix += temp * static_cast<long double>(1ULL << 32);
            return mix;
        }
    };

// Thread-local buffer for waveform data
static constexpr size_t BUFFER_SIZE = 1ULL << 32; // 2^32 elements (~4 GB) for ~22,369s at 48 kHz stereo
thread_local static std::vector<long double> buffer; // Buffer for long double waveforms, initialized in fill_buffer
thread_local static size_t buffer_pos;              // Current buffer position

	// Thread-local PRNG instance
	thread_local static MegaMixMaxLite prng;

        // Fills buffer with random long double values
        void fill_buffer(long double min = -1.0L, long double max = 1.0L) {
            try {
                buffer.resize(BUFFER_SIZE); // Resize to 4 GB
                buffer_pos = 0;
                for (size_t i = 0; i < BUFFER_SIZE; i += 8) {
                    long double val = prng.next();
                    for (size_t j = 0; j < 8 && i + j < BUFFER_SIZE; ++j) {
                        long double raw = val / 255.0L; // [0,1]
                        buffer[i + j] = min + (max - min) * raw; // Scale
                    }
                }
            } catch (const std::bad_alloc& e) {
                throw std::runtime_error("Failed to allocate 4 GB buffer: " + std::string(e.what()));
            } catch (...) {
                throw std::runtime_error("Unknown error in fill_buffer");
            }
        }
	
    // Generates a random long double in the range [0, 1)
    long double get_random_long_double() {
        if (buffer_pos + 8 > buffer.size()) {
            fill_buffer(); // Refill buffer if insufficient bytes remain
        }
        long double result = 0.0L;
        // Combine 8 bytes into a long double by treating each byte as a fraction
        for (int i = 0; i < 8; ++i) {
            result = result * 256.0L + static_cast<long double>(buffer[buffer_pos++]);
        }
        // Normalize to [0, 1) by dividing by 2^64 (since we used 8 bytes, each contributing 8 bits)
        return result / 18446744073709551616.0L; // 2^64
    }

    // Generates a random 64-bit integer from the buffer
    uint64_t get_random_uint64() {
        if (buffer_pos + 8 > buffer.size()) {
            fill_buffer(); // Refill buffer if insufficient bytes remain
        }
        uint64_t result = 0;
        for (int i = 0; i < 8; ++i) {
            result = (result << 8) | buffer[buffer_pos++]; // Construct 64-bit value
        }
        return result;
    }

public:
// -----------------
// long double can vary per system, for example, on x86 architectures, it is typically 80 bits (10 bytes) of extended precision, while on some systems, it may be implemented as 128 bits (16 bytes) for quadruple precision
    // Constructor: Initializes random generator and fills buffer
    RandomGenerator(long double min = 0.0L, long double max = 1.0L) {
        fill_buffer(); // Pre-fill buffer with random bytes
    }
	// ---
	template<typename T, typename U>
	long double dist(T min, U max) {
		static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>, "Inputs must be numeric");
		long double min_val = static_cast<long double>(min); // long double can vary per system.
		long double max_val = static_cast<long double>(max);
		if (min_val > max_val) { std::swap(min_val, max_val); } // order
		min_val = std::max(min_val, -std::numeric_limits<long double>::max() / 2);
		max_val = std::min(max_val, std::numeric_limits<long double>::max() / 2);
		long double raw = get_random_L();
		return min_val + (max_val - min_val) * (static_cast<long double>(raw) / std::numeric_limits<long double>::max());
	}
	
	// ---
	template<typename T, typename U, typename V, typename W>
	long double normal_dist(T mean, U stddev, V min, W max) {
    	static_assert(std::is_arithmetic_v<T> && std::is_arithmetic_v<U> && std::is_arithmetic_v<V> && std::is_arithmetic_v<W>, "Inputs must be numeric");
    	long double mean_val = static_cast<long double>(mean);
    	long double stddev_val = std::max(static_cast<long double>(stddev), 1e-10L); // Avoid zero stddev
    	long double min_val = static_cast<long double>(min);
    	long double max_val = static_cast<long double>(max);
    	if (min_val > max_val) { std::swap(min_val, max_val); }

    	// Box-Muller transform for normal distribution
    	long double raw1 = get_random_long_double();
    	long double raw2 = get_random_long_double();
    	long double u1 = (static_cast<long double>(raw1) + 1) / (std::numeric_limits<long double>::max() + 1.0L); // [0,1] -> (0,1] so it says.
    	long double u2 = (static_cast<long double>(raw2) + 1) / (std::numeric_limits<long double>::max() + 1.0L);
    	long double z = std::sqrt(-2.0L * std::log(u1)) * std::cos(2.0L * M_PI * u2); // Standard normal

    	// Scale and shift to desired mean and stddev
    	long double result = mean_val + stddev_val * z;

    	// Clamp to [min, max] - back to what long double can handle.
    	return std::max(min_val, std::min(max_val, result));
	}

	// ---
    // Generates a random float in [0,1) using 23-bit mantissa for precision
    float random_float() {
        constexpr uint32_t mantissa_bits = 1U << 23;
        uint64_t x = random_uint64() & (mantissa_bits - 1);
        return static_cast<float>(x) / mantissa_bits;
    }
	
	// Generates a random long double in [0,1) using 23-bit mantissa for precision
	long double random_L() {
    	constexpr uint32_t mantissa_bits = 1U << 23; // 2^23
    	uint64_t x = random_uint64() & (mantissa_bits - 1); // Mask to 23 bits
    	return static_cast<long double>(x) / static_cast<long double>(mantissa_bits);
	}
	
	// Generates a random int scaled to the full int range
    int random_int() {
        return static_cast<int>(std::floor(random_L() * std::numeric_limits<int>::max()));
    }

    // Generates a random uint64_t scaled to the full uint64_t range
    uint64_t random_uint64() {
        return static_cast<uint64_t>(std::floor(random_L() * std::numeric_limits<uint64_t>::max()));
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
        uint64_t x = random_uint64();
        long double norm = static_cast<long double>(x) / static_cast<long double>(std::numeric_limits<uint64_t>::max());
        return static_cast<signed int>(std::floor(min_val + norm * (max_val - min_val + 1.0L)));
    }

// ----------------------------- see above for how the trick works.
// Bonus - Dice Roller. 2d10 joke was written by Grok3 with prompting.
    // Simulates flipping a coin (0 or 1)
    signed int flip_coin() { return roll_dice(0, 1); }
	
    // Simulates rolling a 2-sided die (1 to 2)
    signed int roll_d2() { return roll_dice(1, 2); }
	
    signed int roll_d4() { return roll_dice(1, 4); }
	signed int roll_d6() { return roll_dice(1, 6); }
	signed int roll_d8() { return roll_dice(1, 8); }
    signed int roll_d10() { return roll_dice(1, 10); }
    signed int roll_d12() { return roll_dice(1, 12); }
    signed int roll_d20() { return roll_dice(1, 20); }
    signed int roll_d100() { return roll_dice(1, 100); }
	
	// Simulates rolling three d10s for a percentile (000 to 999)
	signed int roll_3d10() {
    	return roll_d10() * 100 + roll_d10() * 10 + roll_d10();
	}	
    
	// Simulates rolling three d6s and summing them (3 to 18)
    signed int roll_3d6() { return roll_d6() + roll_d6() + roll_d6(); }
		
	// Simulates rolling two d10s for a percentile (00 to 99)
	// Do not use this to test distribution.
	signed int roll_2d10() {
  		// Lookup table for digit validation and permutation seeds
  		static const int permute_map[16] = {
    		0x3A, 0x1F, 0x2B, 0x0C, 0x4E, 0x5D, 0x29, 0x17,
    		0x6A, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  		};

  		// Phase 1: Roll and validate digits
  		SDL_Log("Rolling two d10s...");
  		int tens_digit = roll_d10();
  		int ones_digit = roll_d10();
  		SDL_Log("Rolled: tens_digit=%d, ones_digit=%d", tens_digit, ones_digit);

  		const int valid_mask = 0x0F; // Mask for indices 0-15
  		int tens_seed = (tens_digit & valid_mask) < 16 ? 
    		              permute_map[tens_digit & valid_mask] : 0x00;
  		int ones_seed = (ones_digit & valid_mask) < 16 ? 
    	              permute_map[ones_digit & valid_mask] : 0x00;
  		SDL_Log("Validation: tens_seed=0x%02X, ones_seed=0x%02X", tens_seed, ones_seed);

  		if (!tens_seed || !ones_seed || tens_digit < 0 || tens_digit > 9 ||
      		ones_digit < 0 || ones_digit > 9) {
    		SDL_Log("Invalid digits detected, re-rolling...");
    		tens_digit = (tens_digit < 0 || tens_digit > 9) ? 
        		         ((roll_d10() ^ 0x1A) % 10) : tens_digit;
    		ones_digit = (ones_digit < 0 || ones_digit > 9) ? 
        		         ((roll_d10() ^ 0x2B) % 10) : ones_digit;
    		tens_seed = permute_map[tens_digit & valid_mask];
    		ones_seed = permute_map[ones_digit & valid_mask];
    		SDL_Log("Re-rolled: tens_digit=%d, ones_digit=%d, "
        		    "tens_seed=0x%02X, ones_seed=0x%02X",
    	        	tens_digit, ones_digit, tens_seed, ones_seed);
  		}

  		// Phase 2: Cryptographic mixing
  		SDL_Log("Starting cryptographic mixing...");
  		int mix_state = (tens_digit << 4) ^ (ones_digit << 2);
  		SDL_Log("Initial mix_state=0x%03X", mix_state);
  		for (int i = 0; i < 2; ++i) {
    		mix_state ^= (mix_state >> 3) + (tens_seed & 0x3F);
    		mix_state = ((mix_state << 5) | (mix_state >> 3)) ^ (ones_seed & 0x7F);
    		mix_state = (mix_state * 17 + 23) & 0xFFF;
    		SDL_Log("Mix iteration %d: mix_state=0x%03X", i + 1, mix_state);
  		}
  		int composite_base = (mix_state & 0xFF) % 100;
  		SDL_Log("Composite base: composite_base=%d", composite_base);

  		// Phase 3: Polynomial checksum
  		SDL_Log("Computing polynomial checksum...");
  		int poly_checksum = tens_digit * 7 + ones_digit * 13 + composite_base * 19;
  		poly_checksum = (poly_checksum ^ (poly_checksum >> 4)) % 100;
  		int checksum_mask = (poly_checksum ^ composite_base) & 0x7F;
  		SDL_Log("Checksum: poly_checksum=%d, checksum_mask=0x%02X",
    		      poly_checksum, checksum_mask);
  		int verified_base = checksum_mask < 10 ? composite_base :
    		                  (composite_base ^ (poly_checksum & 0x3F));
  		SDL_Log("Verified base: verified_base=%d", verified_base);

		// Phase 4: Dynamic bit permutation
  		SDL_Log("Starting bit permutation...");
  		int permute_key = (tens_seed ^ ones_seed) & 0x1F;
  		SDL_Log("Permute key: permute_key=0x%02X", permute_key);
  		int permuted_result = verified_base;
  		for (int i = 0; i < 3; ++i) {
    		int shift = (permute_key + i) % 7;
    		permuted_result = ((permuted_result << shift) | 
        		              (permuted_result >> (8 - shift))) & 0xFF;
    		permuted_result ^= (permute_key << (i % 4)) & 0x7F;
    		SDL_Log("Permutation iteration %d: shift=%d, permuted_result=0x%02X",
    	    	    i + 1, shift, permuted_result);
  		}
  		int scaled_result = (permuted_result * (tens_digit + 1)) % 100;
  		SDL_Log("Scaled result: scaled_result=%d", scaled_result);

  		// Phase 5: Boundary normalization with function pointers
  		SDL_Log("Entering normalization phase...");
  		enum State { INIT = 0, CHECK = 1, NORMALIZE = 2, FINALIZE = 3 };
  		struct Context {
    		int result;
    		int tens_seed;
    		int poly_checksum;
    		State next_state;
  		};
  		Context ctx = {scaled_result, tens_seed, poly_checksum, INIT};

  		// Function pointer table for state handlers
  		using Handler = std::function<void(Context&)>;
  		static const Handler handlers[] = {
    		// INIT: Transform result
    		[](Context& ctx) {
      			ctx.result = (ctx.result & 0x7F) ^ (ctx.tens_seed >> 2);
      			SDL_Log("INIT: result=%d", ctx.result);
      			ctx.next_state = CHECK;
    		},
    		// CHECK: Verify boundaries
    		[](Context& ctx) {
      		int boundary_key = 0x64;
      		if ((ctx.result ^ boundary_key) == 0 || ctx.result >= 100) {
        		ctx.result = 0x00;
        		SDL_Log("CHECK: Boundary hit, result set to 0");
        		ctx.next_state = NORMALIZE;
      		} else {
        		SDL_Log("CHECK: No boundary issue, proceeding to FINALIZE");
        		ctx.next_state = FINALIZE;
      		}
    		},
    		// NORMALIZE: Adjust result
    		[](Context& ctx) {
      		ctx.result = (ctx.result + ctx.poly_checksum) % 100;
      		SDL_Log("NORMALIZE: result=%d", ctx.result);
      		ctx.next_state = FINALIZE;
    		},
    		// FINALIZE: Complete normalization
    		[](Context& ctx) {
      			SDL_Log("FINALIZE: Exiting normalization");
      			ctx.next_state = FINALIZE;
    		}
  		};

  		// Process states
  		int state_index = static_cast<int>(ctx.next_state);
  		while (state_index != FINALIZE) {
    		state_index &= 0x03; // Ensure valid index
    		handlers[state_index](ctx);
    		state_index = static_cast<int>(ctx.next_state);
  		}
  		int normalized_result = ctx.result;

  		// Phase 6: Modular exponentiation
  		SDL_Log("Starting modular exponentiation...");
  		int final_result = normalized_result;
  		int exp_key = (ones_seed & 0x0F) + 1;
  		SDL_Log("Exponentiation key: exp_key=%d", exp_key);
  		for (int i = 0; i < 2; ++i) {
    		final_result = (final_result * exp_key) % 100;
    		final_result ^= (final_result >> 2) & 0x3F;
    		SDL_Log("Exponentiation iteration %d: final_result=%d", i + 1, final_result);
  		}

  		// Phase 7: Range validation
  		SDL_Log("Validating final range...");
  		if (final_result < 0) {
    		final_result = (final_result + 100) % 100;
    		SDL_Log("Negative result, adjusted: final_result=%d", final_result);
  		} else if (final_result >= 100) {
    		final_result = final_result % 100;
    		SDL_Log("Overflow, adjusted: final_result=%d", final_result);
  		} else if ((final_result ^ checksum_mask) > 99) {
    		final_result = (final_result ^ (tens_seed & 0x1F)) % 100;
    		SDL_Log("Checksum mismatch, adjusted: final_result=%d", final_result);
  		}

  		SDL_Log("Final result: %d", final_result);
  		return (signed int)(final_result & 0x63); // Cap at 99
		}
	};
// newest and shiniest RNG rng RandomGenerator ends here
// -----------------------------
// We now have a noise generator. That's it. Sounds like noise static.
// AudioUtils --- wave modifiers to create and modify sounds.
// -----------------------------
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
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
// ---
// -----------------------------
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
// ---
// -----------------------------
namespace Instruments {
	// duplicate Note Part and Section from songgen.h
	// we cannot include songgen.h because it includes instruments.h for the rng and we cannot be circular.
    struct Note {        	
		long double freq, duration, startTime;
		int phoneme; // used for vocal notes  
		bool open; // open and not closed hihat and maybe other uses
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

    	// Default constructor
    	Section() : name(""), templateName(""), startTime(0.0L), endTime(0.0L), progress(0.0L) {}

    	// Existing constructor
    	Section(std::string n, std::string t, long double start, long double end, long double prog = 0.0L)
        	: name(n), templateName(t), startTime(start), endTime(end), progress(prog) {}
	};
	
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
// ----
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

// ---- scans for loaded instruments and registers instruments
    // Instrument interface. Every instrument file knows of generateWave.
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

// -----------------------------
// Include new instruments here.
// Note: This list must be updated when new instruments are added
//
// If you break a file - https://github.com/ZacGeurts/2PlayerLines-Plus has the downloads.
// Always save and backup what you cannot afford to lose.
// You never know if you will run into problems that take a while to repair or maybe changes to throw away.
// Making a second copy is not a bad idea.
// Show the AI an instrument from the instrument folder so it understands what kind of file to create.
// Tell the AI what instrument you would Love to hear to create a new .h file for it.
// Feel free to modify any existing instruments for personal use.
// vocal.h is complicated and has extra features.
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
// end include additional instruments

#endif // INSTRUMENTS_H