#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include <cmath>
#include <random>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <map>
#include <string>
#include <fstream>
#include <stdexcept>
#include <SDL2/SDL.h>

// Debug logging control: set to 1 to enable instrument logging, 0 to disable
#define DEBUG_LOG 0

// Utility functions for noise (used in precomputation)
namespace AudioUtils {
    class RandomGenerator {
        std::mt19937 rng;
        std::uniform_real_distribution<float> dist;
    public:
        RandomGenerator() : rng(std::random_device{}()), dist(-1.0f, 1.0f) {}
        float generateWhiteNoise() { return dist(rng); }
        float generatePinkNoise() {
            static float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
            float white = dist(rng);
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            return 0.2f * (b0 + b1 + b2 + white * 0.1848f);
        }
    };
}

namespace Instruments {
    struct InstrumentSample {
        float freq;
        float dur;
        std::vector<float> samples;
    };

    // Forward declarations for instrument classes
    class SampleManager;
    class Instrument;
    class PrecomputedInstrument;
    class HiHat;
    class Vocal;

    // Instrument generation functions (used for precomputation, not runtime)
    static float generateKickWave(float t, float freq, float dur) {
        float env = std::exp(-5.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    static float generateHiHatWave(float t, float freq, bool open, float dur) {
        AudioUtils::RandomGenerator rng;
        float env = std::exp(-10.0f * t / (open ? dur : dur * 0.5f));
        return env * rng.generateWhiteNoise();
    }

    static float generateSnareWave(float t, float dur) {
        AudioUtils::RandomGenerator rng;
        float env = std::exp(-8.0f * t / dur);
        return env * rng.generatePinkNoise();
    }

    static float generateClapWave(float t, float dur) {
        AudioUtils::RandomGenerator rng;
        float env = std::exp(-12.0f * t / dur);
        return env * rng.generateWhiteNoise();
    }

    static float generateTomWave(float t, float freq, float dur) {
        float env = std::exp(-6.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    static float generateBassWave(float t, float freq, float dur) {
        float env = std::exp(-4.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    static float generateSubBassWave(float t, float freq, float dur) {
        float env = std::exp(-3.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    static float generateSynthArpWave(float t, float freq, float dur) {
        float env = std::exp(-8.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.5f * std::sin(4.0f * M_PI * freq * t));
    }

    static float generateLeadSynthWave(float t, float freq, float dur) {
        float env = std::exp(-6.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.3f * std::sin(6.0f * M_PI * freq * t));
    }

    static float generatePadWave(float t, float freq, float dur) {
        float env = std::exp(-2.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.4f * std::sin(3.0f * M_PI * freq * t));
    }

    static float generateGuitarWave(float t, float freq, float dur) {
        float env = std::exp(-5.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.2f * std::sin(4.0f * M_PI * freq * t));
    }

    static float generatePianoWave(float t, float freq, float dur) {
        float env = std::exp(-4.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.3f * std::sin(4.0f * M_PI * freq * t));
    }

    static float generateVocalWave(float t, float freq, int phoneme, float dur) {
        float env = std::exp(-4.0f * t / dur);
        float formant = (phoneme % 7) * 100.0f + 500.0f;
        return env * std::sin(2.0f * M_PI * freq * t) * std::sin(2.0f * M_PI * formant * t);
    }

    static float generateFluteWave(float t, float freq, float dur) {
        float env = std::exp(-3.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    static float generateTrumpetWave(float t, float freq, float dur) {
        float env = std::exp(-3.5f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.3f * std::sin(4.0f * M_PI * freq * t));
    }

    static float generateViolinWave(float t, float freq, float dur) {
        float env = std::exp(-2.5f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.2f * std::sin(3.0f * M_PI * freq * t));
    }

    static float generateCelloWave(float t, float freq, float dur) {
        float env = std::exp(-2.0f * t / dur);
        return env * (std::sin(2.0f * M_PI * freq * t) + 0.3f * std::sin(2.5f * M_PI * freq * t));
    }

    static float generateCymbalWave(float t, float dur) {
        AudioUtils::RandomGenerator rng;
        float env = std::exp(-10.0f * t / dur);
        return env * rng.generateWhiteNoise();
    }

    static float generateMarimbaWave(float t, float freq, float dur) {
        float env = std::exp(-6.0f * t / dur);
        return env * std::sin(2.0f * M_PI * freq * t);
    }

    // Precomputation functions
    static void precomputeInstrument(const std::string& name, float freq, float dur, std::vector<float>& samples, float (*generate)(float, float, float)) {
        const float sampleRate = 44100.0f;
        size_t numSamples = static_cast<size_t>(dur * sampleRate);
        samples.resize(numSamples, 0.0f);
        for (size_t i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;
            samples[i] = generate(t, freq, dur);
        }
    }

    static void precomputeInstrument(const std::string& name, float freq, float dur, std::vector<float>& samples, float (*generate)(float, float)) {
        const float sampleRate = 44100.0f;
        size_t numSamples = static_cast<size_t>(dur * sampleRate);
        samples.resize(numSamples, 0.0f);
        for (size_t i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;
            samples[i] = generate(t, dur);
        }
    }

    static void precomputeHiHat(const std::string& name, float freq, float dur, bool open, std::vector<float>& samples) {
        const float sampleRate = 44100.0f;
        size_t numSamples = static_cast<size_t>(dur * sampleRate);
        samples.resize(numSamples, 0.0f);
        for (size_t i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;
            samples[i] = generateHiHatWave(t, freq, open, dur);
        }
    }

    static void precomputeVocal(const std::string& name, float freq, float dur, int phoneme, std::vector<float>& samples) {
        const float sampleRate = 44100.0f;
        size_t numSamples = static_cast<size_t>(dur * sampleRate);
        samples.resize(numSamples, 0.0f);
        for (size_t i = 0; i < numSamples; ++i) {
            float t = i / sampleRate;
            samples[i] = generateVocalWave(t, freq, phoneme, dur);
        }
    }

    // Generate instruments.dat if missing
    static void generateInstrumentsDat(const std::string& filename) {
        std::ifstream check(filename, std::ios::binary);
        bool regenerate = true;
        if (check.is_open()) {
            uint32_t version;
            check.read(reinterpret_cast<char*>(&version), sizeof(version));
            if (version == 1) {
                regenerate = false;
            }
            check.close();
        }

        if (!regenerate) {
            SDL_Log("instruments.dat is up-to-date, skipping precalculation");
            return;
        }

        SDL_Log("Generating instruments.dat...");

        std::vector<float> freqs = {
            50.0f, 60.0f, 100.0f, 180.0f, 220.0f, 440.0f, 493.88f, 523.25f,
            587.33f, 659.25f, 698.46f, 783.99f, 8000.0f
        };
        std::vector<float> durs = {0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 4.0f};
        std::vector<int> phonemes = {0, 1, 2, 3, 4, 5, 6};

        std::vector<InstrumentSample> allData;

        auto addInstrument = [&](const std::string& name, auto generate, bool useFreq = true) {
            for (float dur : durs) {
                if (useFreq) {
                    for (float freq : freqs) {
                        InstrumentSample data;
                        data.freq = freq;
                        data.dur = dur;
                        precomputeInstrument(name, freq, dur, data.samples, generate);
                        allData.push_back(std::move(data));
                    }
                } else {
                    InstrumentSample data;
                    data.freq = 0.0f;
                    data.dur = dur;
                    precomputeInstrument(name, 0.0f, dur, data.samples, generate);
                    allData.push_back(std::move(data));
                }
            }
        };

        addInstrument("kick", generateKickWave);
        for (float dur : durs) {
            for (bool open : {false, true}) {
                InstrumentSample data;
                data.freq = 0.0f;
                data.dur = dur;
                precomputeHiHat(open ? "hihat_open" : "hihat_closed", 0.0f, dur, open, data.samples);
                allData.push_back(std::move(data));
            }
        }
        addInstrument("snare", generateSnareWave, false);
        addInstrument("clap", generateClapWave, false);
        addInstrument("tom", generateTomWave);
        addInstrument("bass", generateBassWave);
        addInstrument("subbass", generateSubBassWave);
        addInstrument("syntharp", generateSynthArpWave);
        addInstrument("leadsynth", generateLeadSynthWave);
        addInstrument("pad", generatePadWave);
        addInstrument("guitar", generateGuitarWave);
        addInstrument("piano", generatePianoWave);
        for (float dur : durs) {
            for (float freq : freqs) {
                for (int phoneme : phonemes) {
                    InstrumentSample data;
                    data.freq = freq;
                    data.dur = dur;
                    precomputeVocal("vocal_" + std::to_string(phoneme), freq, dur, phoneme, data.samples);
                    allData.push_back(std::move(data));
                }
            }
        }
        addInstrument("flute", generateFluteWave);
        addInstrument("trumpet", generateTrumpetWave);
        addInstrument("violin", generateViolinWave);
        addInstrument("cello", generateCelloWave);
        addInstrument("cymbal", generateCymbalWave, false);
        addInstrument("marimba", generateMarimbaWave);

        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            SDL_Log("Error: Could not open %s for writing", filename.c_str());
            throw std::runtime_error("Error: Could not open instruments.dat for writing");
        }

        uint32_t version = 1;
        uint32_t numEntries = allData.size();
        out.write(reinterpret_cast<char*>(&version), sizeof(version));
        out.write(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));

        for (const auto& data : allData) {
            std::string name;
            if (data.dur == 0.125f && data.freq == 0.0f) name = "hihat_closed";
            else if (data.dur == 0.125f && data.freq == 0.0f) name = "hihat_open";
            else if (data.dur == 0.25f && data.freq == 0.0f) name = "snare";
            else if (data.dur == 1.0f && data.freq == 0.0f) name = "cymbal";
            else if (data.dur == 0.25f && data.freq == 0.0f) name = "clap";
            else if (data.freq == 50.0f) name = "subbass";
            else if (data.freq == 60.0f) name = "kick";
            else if (data.freq == 100.0f) name = "tom";
            else if (data.freq == 180.0f) name = "bass";
            else if (data.freq == 220.0f) name = "guitar";
            else if (data.freq == 440.0f) name = "piano";
            else if (data.freq == 493.88f) name = "flute";
            else if (data.freq == 523.25f) name = "trumpet";
            else if (data.freq == 587.33f) name = "violin";
            else if (data.freq == 659.25f) name = "cello";
            else if (data.freq == 698.46f) name = "marimba";
            else if (data.freq == 783.99f) name = "syntharp";
            else if (data.freq == 8000.0f) name = "leadsynth";
            else name = "vocal_" + std::to_string(static_cast<int>(data.freq / 100.0f) % 7);
            uint32_t nameLen = name.size();
            out.write(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            out.write(name.c_str(), nameLen);
            float freq = data.freq;
            float dur = data.dur;
            out.write(reinterpret_cast<char*>(&freq), sizeof(freq));
            out.write(reinterpret_cast<char*>(&dur), sizeof(dur));
            uint32_t numSamples = data.samples.size();
            out.write(reinterpret_cast<char*>(&numSamples), sizeof(numSamples));
            out.write(reinterpret_cast<const char*>(data.samples.data()), numSamples * sizeof(float));
        }

        out.close();
        SDL_Log("Generated %s with %u entries", filename.c_str(), numEntries);
    }

    class SampleManager {
        std::map<std::string, std::vector<InstrumentSample>> samples;
        std::vector<float> freqs = {
            50.0f, 60.0f, 100.0f, 180.0f, 220.0f, 440.0f, 493.88f, 523.25f,
            587.33f, 659.25f, 698.46f, 783.99f, 8000.0f
        };
        std::vector<float> durs = {0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 4.0f};

        float interpolate(float t, const InstrumentSample& sample, float dur) {
            float sampleRate = 44100.0f;
            float index = t * sampleRate;
            size_t idx = static_cast<size_t>(index);
            float frac = index - idx;
            if (idx + 1 >= sample.samples.size()) return 0.0f;
            return sample.samples[idx] * (1.0f - frac) + sample.samples[idx + 1] * frac;
        }

        const InstrumentSample* findClosest(const std::string& name, float freq, float dur) {
            auto it = samples.find(name);
            if (it == samples.end()) return nullptr;
            const InstrumentSample* closest = nullptr;
            float minDist = std::numeric_limits<float>::max();
            for (const auto& sample : it->second) {
                float freqDist = (sample.freq == 0.0f || freq == 0.0f) ? 0.0f : std::abs(sample.freq - freq);
                float durDist = std::abs(sample.dur - dur);
                float dist = freqDist / 8000.0f + durDist / 4.0f;
                if (dist < minDist) {
                    minDist = dist;
                    closest = &sample;
                }
            }
            return closest;
        }

    public:
        SampleManager() {
            const std::string filename = "instruments.dat";
            std::ifstream in(filename, std::ios::binary);
            if (!in) {
                SDL_Log("instruments.dat not found, generating...");
                try {
                    generateInstrumentsDat(filename);
                } catch (const std::exception& e) {
                    SDL_Log("Failed to generate instruments.dat: %s", e.what());
                    throw std::runtime_error("Error: Could not generate instruments.dat");
                }
                in.open(filename, std::ios::binary);
                if (!in) {
                    SDL_Log("Error: Could not open %s after generation", filename.c_str());
                    throw std::runtime_error("Error: Could not open instruments.dat");
                }
            }

            uint32_t version, numEntries;
            in.read(reinterpret_cast<char*>(&version), sizeof(version));
            in.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
            if (version != 1) {
                in.close();
                SDL_Log("Unsupported instruments.dat version, regenerating...");
                try {
                    generateInstrumentsDat(filename);
                } catch (const std::exception& e) {
                    SDL_Log("Failed to generate instruments.dat: %s", e.what());
                    throw std::runtime_error("Error: Could not generate instruments.dat");
                }
                in.open(filename, std::ios::binary);
                if (!in) {
                    SDL_Log("Error: Could not open %s after regeneration", filename.c_str());
                    throw std::runtime_error("Error: Could not open instruments.dat");
                }
                in.read(reinterpret_cast<char*>(&version), sizeof(version));
                in.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
            }

            for (uint32_t i = 0; i < numEntries; ++i) {
                uint32_t nameLen;
                in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
                std::string name(nameLen, '\0');
                in.read(&name[0], nameLen);
                InstrumentSample sample;
                in.read(reinterpret_cast<char*>(&sample.freq), sizeof(sample.freq));
                in.read(reinterpret_cast<char*>(&sample.dur), sizeof(sample.dur));
                uint32_t numSamples;
                in.read(reinterpret_cast<char*>(&numSamples), sizeof(numSamples));
                sample.samples.resize(numSamples);
                in.read(reinterpret_cast<char*>(sample.samples.data()), numSamples * sizeof(float));
                samples[name].push_back(std::move(sample));
            }
            in.close();
            SDL_Log("Loaded instruments.dat with %u entries", numEntries);
        }

        float getSample(const std::string& name, float t, float freq, float dur) {
            const auto* sample = findClosest(name, freq, dur);
            if (!sample) {
                //SDL_Log("Warning: No precomputed sample for %s, freq=%.2f, dur=%.3f", name.c_str(), freq, dur);
                return 0.0f;
            }
            if (t >= sample->dur) return 0.0f;
            return interpolate(t, *sample, dur);
        }

        float getHiHatSample(float t, float freq, bool open, float dur) {
            std::string name = open ? "hihat_open" : "hihat_closed";
            return getSample(name, t, freq, dur);
        }

        float getVocalSample(float t, float freq, int phoneme, float dur) {
            std::string name = "vocal_" + std::to_string(phoneme);
            return getSample(name, t, freq, dur);
        }
    };

    static SampleManager sampleManager;

    // Thread pool for parallel processing
    class ThreadPool {
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> stop;

    public:
        ThreadPool(size_t threads = std::thread::hardware_concurrency()) : stop(false) {
            for (size_t i = 0; i < threads; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queueMutex);
                            condition.wait(lock, [this] { return stop || !tasks.empty(); });
                            if (stop && tasks.empty()) return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~ThreadPool() {
            stop = true;
            condition.notify_all();
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }

        template<class F>
        auto enqueue(F&& f) -> std::future<decltype(f())> {
            using return_type = decltype(f());
            auto task = std::make_shared<std::packaged_task<return_type()>>(std::forward<F>(f));
            std::future<return_type> res = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                tasks.emplace([task]() { (*task)(); });
            }
            condition.notify_one();
            return res;
        }
    };

    // Base class for instruments
    class Instrument {
    protected:
        std::string name;
        AudioUtils::RandomGenerator rng;
    public:
        Instrument(const std::string& n) : name(n) {}
        virtual ~Instrument() = default;
        virtual float generateSample(float t, float freq, float dur, int phoneme = 0, bool open = false) = 0;
        virtual void generateBlock(float t, float freq, float dur, int phoneme, bool open, std::vector<float>& output, size_t start, size_t end) = 0;
    };

    // Generic instrument using precomputed samples
    class PrecomputedInstrument : public Instrument {
    public:
        PrecomputedInstrument(const std::string& n) : Instrument(n) {}
        float generateSample(float t, float freq, float dur, int phoneme = 0, bool open = false) override {
            return sampleManager.getSample(name, t, freq, dur);
        }
        void generateBlock(float t, float freq, float dur, int phoneme, bool open, std::vector<float>& output, size_t start, size_t end) override {
            for (size_t i = start; i < end; ++i) {
                float sampleTime = t + i * (1.0f / 44100.0f);
                output[i] = generateSample(sampleTime, freq, dur, phoneme, open);
            }
        }
    };

    // HiHat instrument (handles open/closed)
    class HiHat : public Instrument {
    public:
        HiHat() : Instrument("hihat") {}
        float generateSample(float t, float freq, float dur, int phoneme = 0, bool open = false) override {
            if (DEBUG_LOG && t <= 1.0f / 44100.0f) {
                SDL_Log("HiHat (%s): freq=%.2f Hz, dur=%.3f s, time=%.3f s", open ? "open" : "closed", freq, dur, t);
            }
            return sampleManager.getHiHatSample(t, freq, open, dur);
        }
        void generateBlock(float t, float freq, float dur, int phoneme, bool open, std::vector<float>& output, size_t start, size_t end) override {
            for (size_t i = start; i < end; ++i) {
                float sampleTime = t + i * (1.0f / 44100.0f);
                output[i] = generateSample(sampleTime, freq, dur, phoneme, open);
            }
        }
    };

    // Vocal instrument (handles phonemes)
    class Vocal : public Instrument {
    public:
        Vocal() : Instrument("vocal") {}
        float generateSample(float t, float freq, float dur, int phoneme = 0, bool open = false) override {
            if (DEBUG_LOG && t <= 1.0f / 44100.0f) {
                SDL_Log("Vocal (phoneme=%d): freq=%.2f Hz, dur=%.3f s, time=%.3f s", phoneme, freq, dur, t);
            }
            return sampleManager.getVocalSample(t, freq, phoneme, dur);
        }
        void generateBlock(float t, float freq, float dur, int phoneme, bool open, std::vector<float>& output, size_t start, size_t end) override {
            for (size_t i = start; i < end; ++i) {
                float sampleTime = t + i * (1.0f / 44100.0f);
                output[i] = generateSample(sampleTime, freq, dur, phoneme, open);
            }
        }
    };

    // Parallel instrument processor
    class ParallelInstrumentProcessor {
        ThreadPool pool;
        static constexpr size_t BLOCK_SIZE = 1024;
    public:
        ParallelInstrumentProcessor() : pool(std::thread::hardware_concurrency()) {}

        void processInstruments(
            const std::vector<std::tuple<std::unique_ptr<Instrument>, float, float, float, int, bool>>& instruments,
            float t, std::vector<float>& output) {
            if (output.size() < BLOCK_SIZE) output.resize(BLOCK_SIZE, 0.0f);
            std::vector<std::future<std::vector<float>>> futures;

            for (const auto& [instr, freq, dur, start, phoneme, open] : instruments) {
                if (t >= start && t < start + dur) {
                    float noteT = t - start;
                    futures.push_back(pool.enqueue([&, noteT, freq, dur, phoneme, open]() {
                        std::vector<float> block(BLOCK_SIZE, 0.0f);
                        instr->generateBlock(noteT, freq, dur, phoneme, open, block, 0, BLOCK_SIZE);
                        return block;
                    }));
                }
            }

            for (auto& future : futures) {
                auto block = future.get();
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    output[i] += block[i];
                }
            }
        }
    };

    // Inline functions for song compatibility (use precomputed samples)
    inline float generateKick(float t, float freq = 60.0f, float dur = 0.25f) {
        static PrecomputedInstrument kick("kick");
        return kick.generateSample(t, freq, dur);
    }

    inline float generateHiHat(float t, float freq = 0.0f, bool open = false, float dur = 0.125f) {
        static HiHat hihat;
        return hihat.generateSample(t, freq, dur, 0, open);
    }

    inline float generateSnare(float t, float dur = 0.25f) {
        static PrecomputedInstrument snare("snare");
        return snare.generateSample(t, 0.0f, dur);
    }

    inline float generateClap(float t, float dur = 0.25f) {
        static PrecomputedInstrument clap("clap");
        return clap.generateSample(t, 0.0f, dur);
    }

    inline float generateTom(float t, float freq = 100.0f, float dur = 0.5f) {
        static PrecomputedInstrument tom("tom");
        return tom.generateSample(t, freq, dur);
    }

    inline float generateBass(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument bass("bass");
        return bass.generateSample(t, freq, dur);
    }

    inline float generateSubBass(float t, float freq, float dur = 1.0f) {
        static PrecomputedInstrument subbass("subbass");
        return subbass.generateSample(t, freq, dur);
    }

    inline float generateSynthArp(float t, float freq, float dur = 0.0625f) {
        static PrecomputedInstrument syntharp("syntharp");
        return syntharp.generateSample(t, freq, dur);
    }

    inline float generateLeadSynth(float t, float freq, float dur = 0.25f) {
        static PrecomputedInstrument leadsynth("leadsynth");
        return leadsynth.generateSample(t, freq, dur);
    }

    inline float generatePad(float t, float freq, float dur = 4.0f) {
        static PrecomputedInstrument pad("pad");
        return pad.generateSample(t, freq, dur);
    }

    inline float generateGuitar(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument guitar("guitar");
        return guitar.generateSample(t, freq, dur);
    }

    inline float generatePiano(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument piano("piano");
        return piano.generateSample(t, freq, dur);
    }

    inline float generateVocal(float t, float freq, int phoneme, float dur = 0.5f) {
        static Vocal vocal;
        return vocal.generateSample(t, freq, dur, phoneme);
    }

    inline float generateFlute(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument flute("flute");
        return flute.generateSample(t, freq, dur);
    }

    inline float generateTrumpet(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument trumpet("trumpet");
        return trumpet.generateSample(t, freq, dur);
    }

    inline float generateViolin(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument violin("violin");
        return violin.generateSample(t, freq, dur);
    }

    inline float generateCello(float t, float freq, float dur = 0.5f) {
        static PrecomputedInstrument cello("cello");
        return cello.generateSample(t, freq, dur);
    }

    inline float generateCymbal(float t, float dur = 1.0f) {
        static PrecomputedInstrument cymbal("cymbal");
        return cymbal.generateSample(t, 0.0f, dur);
    }

    inline float generateMarimba(float t, float freq, float dur = 0.25f) {
        static PrecomputedInstrument marimba("marimba");
        return marimba.generateSample(t, freq, dur);
    }
}

std::vector<float> generateSong1(float songTime, int channels);
std::vector<float> generateSong2(float songTime, int channels);
std::vector<float> generateSong3(float songTime, int channels);
std::vector<float> generateSong4(float songTime, int channels);
std::vector<float> generateSong5(float songTime, int channels);

#endif // INSTRUMENTS_H