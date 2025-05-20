// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp, songgen.h, instruments.h
// The other linesplus code is free and cannot be resold.
// Interested parties can find my contact information at https://github.com/ZacGeurts

#include "songgen.h"
#include "instruments.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <cctype>
#include <signal.h>
#include <SDL2/SDL.h>

// Flag to handle program termination
static volatile bool running = true;

// Signal handler for Ctrl+C
void handleSignal(int) {
    running = false;
}

void printHelp() {
    std::cout << "Generates songs\n";
    std::cout << "Examples:\n";
    std::cout << "  songgen rock\n";
    std::cout << "  songgen jazz\n";
	std::cout << " \n";
	std::cout << "Playback\n";
    std::cout << "  songgen song1.song [--stereo]\n";
    std::cout << "Available genres:\n";
    std::cout << "  classical, jazz, pop, rock, techno, rap, blues, country, folk, reggae,\n";
    std::cout << "  metal, punk, disco, funk, soul, gospel, ambient, edm, latin, hiphop\n";
    std::cout << "Usage:\n";
    std::cout << "  ./songgen [genre]  # Generate a new song\n";
    std::cout << "  ./songgen <filename>.song [--stereo]  # Play an existing song (5.1 or option stereo)\n";
    std::cout << "  ./songgen                            # Show this help message\n";
    std::cout << "\n";
    std::cout << "This makes song1.song if it does not exist then song2.song etc\n";
    std::cout << "Delete song2.song and next song created is song2.song assuming song1 exists\n";
    std::cout << "You can keep only song3.song etc and it will not cause issues with playback with linesplus game\n";
    std::cout << "Missing song numbers are merely skipped.\n";
}

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }
    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(start, end - start);
}

struct SongData {
    float bpm, duration, rootFreq;
    std::string scaleName, title, genres;
    std::vector<SongGen::Section> sections;
    std::vector<SongGen::Part> parts;
    int channels; // Added to store channel count (2 for stereo, 6 for 5.1)
};

SongData parseSongFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) {
        SDL_Log("Error: Cannot open song file: %s", filename.c_str());
        throw std::runtime_error("Cannot open song file: " + filename);
    }
    if (in.peek() == std::ifstream::traits_type::eof()) {
        SDL_Log("Error: Song file %s is empty", filename.c_str());
        throw std::runtime_error("Song file is empty: " + filename);
    }

    SongData song;
    song.bpm = 120.0f;
    song.rootFreq = 440.0f;
    song.scaleName = "major";
    song.duration = 180.0f;
    song.channels = 6;
    song.parts.reserve(7);
    song.sections.reserve(9);

    std::string line;
    SongGen::Part currentPart;
    bool inPart = false, inNotes = false, inPanAutomation = false, inVolumeAutomation = false, inReverbMixAutomation = false;
    size_t lineNumber = 0;
    size_t expectedSections = 0, expectedParts = 0, expectedNotes = 0, expectedPanPoints = 0, expectedVolumePoints = 0, expectedReverbPoints = 0;

    while (std::getline(in, line)) {
        lineNumber++;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        ss >> token;

        try {
            if (token == "Song:") {
                std::getline(ss, song.title);
                song.title = trim(song.title);
            } else if (token == "Genres:") {
                std::getline(ss, song.genres);
                song.genres = trim(song.genres);
            } else if (token == "Sections:") {
                ss >> expectedSections;
            } else if (token == "Section:") {
                SongGen::Section section;
                ss >> section.name >> section.startTime >> section.endTime;
                std::string progressLabel, templateLabel;
                ss >> progressLabel >> section.progress >> templateLabel >> section.templateName;
                if (std::isfinite(section.startTime) && std::isfinite(section.endTime) &&
                    section.startTime >= 0.0f && section.endTime > section.startTime) {
                    song.sections.push_back(section);
                } else {
                    SDL_Log("Invalid section at line %zu: start=%.2f, end=%.2f", lineNumber, section.startTime, section.endTime);
                }
            } else if (token == "Parts:") {
                ss >> expectedParts;
                inPart = false;
                inNotes = inPanAutomation = inVolumeAutomation = inReverbMixAutomation = false;
                if (!currentPart.instrument.empty()) {
                    song.parts.push_back(currentPart);
                    //SDL_Log("Parsed part: %s with %zu notes", currentPart.instrument.c_str(), currentPart.notes.size());
                    currentPart = SongGen::Part();
                }
            } else if (token == "Part:") {
                if (!currentPart.instrument.empty()) {
                    song.parts.push_back(currentPart);
                    //SDL_Log("Parsed part: %s with %zu notes", currentPart.instrument.c_str(), currentPart.notes.size());
                }
                currentPart = SongGen::Part();
                std::getline(ss, currentPart.sectionName);
                currentPart.sectionName = trim(currentPart.sectionName);
                inPart = true;
                inNotes = inPanAutomation = inVolumeAutomation = inReverbMixAutomation = false;
            } else if (inPart && token == "Instrument:") {
                std::getline(ss, currentPart.instrument);
                currentPart.instrument = trim(currentPart.instrument);
            } else if (inPart && token == "Pan:") {
                ss >> currentPart.pan;
            } else if (inPart && token == "ReverbMix:") {
                ss >> currentPart.reverbMix;
            } else if (inPart && token == "UseReverb:") {
                std::string reverbStr;
                ss >> reverbStr;
                currentPart.useReverb = (reverbStr == "true");
            } else if (inPart && token == "ReverbDelay:") {
                ss >> currentPart.reverbDelay;
            } else if (inPart && token == "ReverbDecay:") {
                ss >> currentPart.reverbDecay;
            } else if (inPart && token == "ReverbMixFactor:") {
                ss >> currentPart.reverbMixFactor;
            } else if (inPart && token == "UseDistortion:") {
                std::string distStr;
                ss >> distStr;
                currentPart.useDistortion = (distStr == "true");
            } else if (inPart && token == "DistortionDrive:") {
                ss >> currentPart.distortionDrive;
            } else if (inPart && token == "DistortionThreshold:") {
                ss >> currentPart.distortionThreshold;
            } else if (inPart && token == "Notes:") {
                ss >> expectedNotes;
                inNotes = true;
                inPanAutomation = inVolumeAutomation = inReverbMixAutomation = false;
            } else if (inPart && inNotes && token == "Note:") {
                SongGen::Note note;
                std::string phonemeLabel, openLabel, volLabel, velLabel;
                ss >> note.freq >> note.duration >> note.startTime >> phonemeLabel >> note.phoneme >> openLabel >> note.open >> volLabel >> note.volume >> velLabel >> note.velocity;
                if (std::isfinite(note.startTime) && std::isfinite(note.freq) &&
                    std::isfinite(note.duration) && note.duration > 0.0f) {
                    currentPart.notes.push_back(note);
                } else {
                    SDL_Log("Skipping invalid note at line %zu: start=%.2f, freq=%.2f, duration=%.2f",
                            lineNumber, note.startTime, note.freq, note.duration);
                }
            } else if (inPart && token == "PanAutomation:") {
                ss >> expectedPanPoints;
                inPanAutomation = true;
                inNotes = inVolumeAutomation = inReverbMixAutomation = false;
            } else if (inPart && inPanAutomation && token == "PanPoint:") {
                float time, value;
                ss >> time >> value;
                currentPart.panAutomation.emplace_back(time, value);
            } else if (inPart && token == "VolumeAutomation:") {
                ss >> expectedVolumePoints;
                inVolumeAutomation = true;
                inNotes = inPanAutomation = inReverbMixAutomation = false;
            } else if (inPart && inVolumeAutomation && token == "VolumePoint:") {
                float time, value;
                ss >> time >> value;
                currentPart.volumeAutomation.emplace_back(time, value);
            } else if (inPart && token == "ReverbMixAutomation:") {
                ss >> expectedReverbPoints;
                inReverbMixAutomation = true;
                inNotes = inPanAutomation = inVolumeAutomation = false;
            } else if (inPart && inReverbMixAutomation && token == "ReverbMixPoint:") {
                float time, value;
                ss >> time >> value;
                currentPart.reverbMixAutomation.emplace_back(time, value);
            } else {
                SDL_Log("Unrecognized token '%s' at line %zu", token.c_str(), lineNumber);
            }
        } catch (const std::exception& e) {
            SDL_Log("Error parsing line %zu: %s", lineNumber, e.what());
            continue;
        }
    }

    if (!currentPart.instrument.empty()) {
        song.parts.push_back(currentPart);
        SDL_Log("Parsed final part: %s with %zu notes", currentPart.instrument.c_str(), currentPart.notes.size());
    }
    in.close();

    // Validate parsed data
    if (song.sections.empty()) {
        SDL_Log("No sections parsed, adding default section");
        song.sections.emplace_back("Default", 0.0f, song.duration, 0.0f);
    }
    if (song.parts.empty()) {
        SDL_Log("No parts parsed, song will have no audio");
    }
    if (song.title.empty()) {
        SDL_Log("No title parsed, using default");
        song.title = "Untitled";
    }
    if (song.genres.empty()) {
        SDL_Log("No genres parsed, using default");
        song.genres = "Unknown";
    }

    std::set<std::string> instruments;
    for (const auto& part : song.parts) {
        instruments.insert(part.instrument);
    }
    std::string instrumentList = "";
    for (const auto& inst : instruments) {
        instrumentList += inst + ", ";
    }
    if (!instrumentList.empty()) instrumentList = instrumentList.substr(0, instrumentList.length() - 2);

    SDL_Log("Loaded song: %s", filename.c_str());
    SDL_Log("Metadata:");
    SDL_Log("  Title: %s", song.title.c_str());
    SDL_Log("  Genres: %s", song.genres.c_str());
    SDL_Log("  BPM: %.2f", song.bpm);
    SDL_Log("  Scale: %s", song.scaleName.c_str());
    SDL_Log("  Root Frequency: %.2f Hz", song.rootFreq);
    SDL_Log("  Duration: %.2f seconds", song.duration);
    SDL_Log("  Instruments: %s", instrumentList.c_str());
    SDL_Log("  Parts: %zu, Sections: %zu", song.parts.size(), song.sections.size());
    SDL_Log("CTRL-C to Exit playback.");

    return song;
}

struct PlaybackState {
    SongData song;
    float currentTime;
    bool playing;
    std::vector<size_t> nextNoteIndices;
    std::vector<AudioUtils::Reverb> reverbs;
    std::vector<AudioUtils::Distortion> distortions;
    size_t currentSectionIdx;

    struct ActiveNote {
        size_t noteIndex;
        float startTime;
        float endTime;
    };
    std::vector<std::vector<ActiveNote>> activeNotes;

    PlaybackState(const SongData& s)
        : song(s), currentTime(0.0f), playing(true), nextNoteIndices(s.parts.size(), 0),
          reverbs(s.parts.size()), distortions(s.parts.size()), currentSectionIdx(0),
          activeNotes(s.parts.size()) {
        for (size_t i = 0; i < s.parts.size(); ++i) {
            auto& part = s.parts[i];
            reverbs[i] = AudioUtils::Reverb(part.reverbDelay, part.reverbDecay, part.reverbMixFactor);
            distortions[i] = AudioUtils::Distortion(part.distortionDrive, part.distortionThreshold);
            activeNotes[i].reserve(16);
        }
    }
};

size_t countNotesInSection(const SongData& song, const SongGen::Section& section) {
    size_t noteCount = 0;
    for (const auto& part : song.parts) {
        for (const auto& note : part.notes) {
            if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                ++noteCount;
            }
        }
    }
    return noteCount;
}

std::string getInstrumentsInSection(const SongData& song, const SongGen::Section& section) {
    std::set<std::string> instruments;
    for (const auto& part : song.parts) {
        for (const auto& note : part.notes) {
            if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                instruments.insert(part.instrument);
                break;
            }
        }
    }
    std::string instrumentList = "";
    for (const auto& inst : instruments) {
        instrumentList += inst + ", ";
    }
    if (!instrumentList.empty()) instrumentList = instrumentList.substr(0, instrumentList.length() - 2);
    return instrumentList.empty() ? "None" : instrumentList;
}

float interpolateAutomation(float t, const std::vector<std::pair<float, float>>& automation, float defaultValue) {
    if (automation.empty()) return defaultValue;
    if (t <= automation.front().first) return automation.front().second;
    if (t >= automation.back().first) return automation.back().second;
    for (size_t i = 1; i < automation.size(); ++i) {
        if (t >= automation[i-1].first && t < automation[i].first) {
            float t0 = automation[i-1].first, t1 = automation[i].first;
            float v0 = automation[i-1].second, v1 = automation[i].second;
            return v0 + (v1 - v0) * (t - t0) / (t1 - t0);
        }
    }
    return defaultValue;
}

float getTailDuration(const std::string& instrument) {
    if (instrument == "cymbal") return 2.0f;
    if (instrument == "guitar") return 1.5f;
    if (instrument == "syntharp") return 1.2f;
    if (instrument == "subbass") return 0.8f;
    if (instrument == "kick") return 0.5f;
    if (instrument == "snare") return 0.6f;
    if (instrument == "piano") return 2.0f;
    if (instrument == "violin") return 2.5f;
    if (instrument == "cello") return 3.0f;
    if (instrument == "marimba") return 1.0f;
    if (instrument == "steelguitar") return 1.8f;
    if (instrument == "sitar") return 2.0f;
    return 1.5f; // Default for other instruments
}

void audioCallback(void* userdata, Uint8* stream, int len) {
    PlaybackState* state = static_cast<PlaybackState*>(userdata);
    float* output = reinterpret_cast<float*>(stream);
    bool isStereo = state->song.channels == 2;
    int numChannels = isStereo ? 2 : 6;
    int numSamples = len / sizeof(float) / numChannels;
    float sampleRate = 44100.0f;

    // Determine full playback duration (last section's end time + 5-second fade-out)
    float fullDuration = state->song.sections.empty() ? state->song.duration : state->song.sections.back().endTime;
    fullDuration += 5.0f;

    // Clear output buffer
    std::fill(output, output + static_cast<size_t>(numSamples) * numChannels, 0.0f);

    // Thread-local storage for channel outputs
    unsigned int numThreads = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(state->song.parts.size()));
    if (numThreads == 0) numThreads = 1;
    std::vector<std::vector<float>> threadOutputs(numThreads, std::vector<float>(static_cast<size_t>(numSamples) * numChannels, 0.0f));
    std::mutex outputMutex;

    auto processParts = [&](size_t startIdx, size_t endIdx, size_t threadIdx, float startTime) {
        std::vector<float> localOutput(static_cast<size_t>(numSamples) * numChannels, 0.0f);
        for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
            float t = startTime + i / sampleRate;
            float L = 0.0f, R = 0.0f, C = 0.0f, LFE = 0.0f, Ls = 0.0f, Rs = 0.0f;

            // Apply fade-in and fade-out
            float fadeGain = 1.0f;
            if (t < 5.0f) {
                fadeGain = t / 5.0f;
            } else if (t > fullDuration - 5.0f) {
                fadeGain = (fullDuration - t) / 5.0f;
            }

            // Process parts assigned to this thread
            for (size_t partIdx = startIdx; partIdx < endIdx && partIdx < state->song.parts.size(); ++partIdx) {
                auto& part = state->song.parts[partIdx];
                auto& nextIdx = state->nextNoteIndices[partIdx];
                auto& active = state->activeNotes[partIdx];

                float pan = interpolateAutomation(t, part.panAutomation, part.pan);
                float volume = interpolateAutomation(t, part.volumeAutomation, 0.5f);
                float reverbMix = interpolateAutomation(t, part.reverbMixAutomation, part.reverbMix);

                float leftGain = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
                float rightGain = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
                float surroundGain = 0.5f * (leftGain + rightGain);
                float centerWeight = (part.instrument == "voice") ? 0.8f : 0.3f;
                float lfeWeight = (part.instrument == "subbass" || part.instrument == "kick") ? 0.5f : 0.1f;
                float sideWeight = (part.instrument == "guitar" || part.instrument == "syntharp") ? 0.6f : 0.4f;

                while (nextIdx < part.notes.size() && part.notes[nextIdx].startTime <= t && active.size() < 16) {
                    const auto& note = part.notes[nextIdx];
                    float tailDuration = getTailDuration(part.instrument);
                    active.push_back({nextIdx, note.startTime, note.startTime + note.duration + tailDuration});
                    ++nextIdx;
                }

                for (auto it = active.begin(); it != active.end();) {
                    const auto& note = part.notes[it->noteIndex];
                    if (t <= it->endTime) {
                        float noteTime = t - note.startTime;
                        size_t sampleIndex = static_cast<size_t>(noteTime * sampleRate);
                        const std::vector<float>& samples = Instruments::sampleManager.getSample(
                            part.instrument, sampleRate, note.freq, note.duration, note.phoneme, note.open);
                        float sample = (sampleIndex < samples.size()) ? samples[sampleIndex] : 0.0f;
                        if (samples.empty()) {
                            SDL_Log("Warning: Empty sample for instrument %s at note %zu", part.instrument.c_str(), it->noteIndex);
                        }
                        sample *= note.volume * note.velocity * volume * fadeGain;
                        if (part.useDistortion) {
                            sample = state->distortions[partIdx].process(sample);
                        }
                        if (part.useReverb) {
                            sample = state->reverbs[partIdx].process(sample * (1.0f - reverbMix)) + sample * reverbMix;
                        }

                        L += sample * leftGain * sideWeight;
                        R += sample * rightGain * sideWeight;
                        C += sample * centerWeight;
                        LFE += sample * lfeWeight;
                        Ls += sample * surroundGain * sideWeight;
                        Rs += sample * surroundGain * sideWeight;

                        ++it;
                    } else {
                        it = active.erase(it);
                    }
                }
            }

            // Store in local output
            if (isStereo) {
                float L_out = L + 0.707f * C + 0.707f * LFE + 0.5f * Ls;
                float R_out = R + 0.707f * C + 0.707f * LFE + 0.5f * Rs;
                localOutput[i * 2 + 0] = std::max(-1.0f, std::min(1.0f, L_out));
                localOutput[i * 2 + 1] = std::max(-1.0f, std::min(1.0f, R_out));
            } else {
                localOutput[i * 6 + 0] = std::max(-1.0f, std::min(1.0f, L));
                localOutput[i * 6 + 1] = std::max(-1.0f, std::min(1.0f, R));
                localOutput[i * 6 + 2] = std::max(-1.0f, std::min(1.0f, C));
                localOutput[i * 6 + 3] = std::max(-1.0f, std::min(1.0f, LFE));
                localOutput[i * 6 + 4] = std::max(-1.0f, std::min(1.0f, Ls));
                localOutput[i * 6 + 5] = std::max(-1.0f, std::min(1.0f, Rs));
            }
        }

        // Accumulate into thread output
        std::lock_guard<std::mutex> lock(outputMutex);
        for (size_t i = 0; i < static_cast<size_t>(numSamples) * numChannels; ++i) {
            threadOutputs[threadIdx][i] += localOutput[i];
        }
    };

    // Handle section logging (single-threaded to avoid race conditions)
    for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
        float t = state->currentTime + i / sampleRate;
        if (t > fullDuration || !running) {
            state->playing = false;
            break;
        }
        if (state->currentSectionIdx < state->song.sections.size()) {
            const auto& section = state->song.sections[state->currentSectionIdx];
            if (t >= section.startTime) {
                size_t noteCount = countNotesInSection(state->song, section);
                std::string instruments = getInstrumentsInSection(state->song, section);
                SDL_Log("Playing Section %s with %zu notes at timestamp %.2f, Instruments: %s",
                        section.name.c_str(), noteCount, section.startTime, instruments.c_str());
                state->currentSectionIdx++;
            }
        }
    }

    // Launch threads
    std::vector<std::thread> threads;
    size_t partsPerThread = (state->song.parts.size() + numThreads - 1) / numThreads;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startIdx = t * partsPerThread;
        size_t endIdx = std::min(startIdx + partsPerThread, state->song.parts.size());
        if (startIdx < state->song.parts.size()) {
            threads.emplace_back(processParts, startIdx, endIdx, t, state->currentTime);
        }
    }

    // Join threads
    for (auto& t : threads) {
        t.join();
    }

    // Sum thread outputs
    for (const auto& tOutput : threadOutputs) {
        for (size_t i = 0; i < static_cast<size_t>(numSamples) * numChannels; ++i) {
            output[i] += tOutput[i];
        }
    }

    state->currentTime += numSamples / sampleRate;
}

void playSong(const std::string& filename, bool forceStereo) {
    SongData song = parseSongFile(filename);

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return;
    }

    signal(SIGINT, handleSignal);

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_F32;
    spec.channels = forceStereo ? 2 : 6;
    spec.samples = 1024;
    spec.callback = audioCallback;

    song.channels = spec.channels; // Store channel count in song for callback
    PlaybackState state(song);
    spec.userdata = &state;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (device == 0 && !forceStereo) {
        SDL_Log("Failed to open 5.1 audio device: %s, attempting stereo", SDL_GetError());
        spec.channels = 2;
        song.channels = 2;
        device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }
    if (device == 0) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    SDL_Log("Playing song %s with %d channels", filename.c_str(), spec.channels);
    SDL_PauseAudioDevice(device, 0);

    SDL_Event event;
    while (state.playing && running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }
        SDL_Delay(10);
    }

    SDL_CloseAudioDevice(device);
    SDL_Quit();
    SDL_Log("Playback stopped: %s at timestamp %.2f", running ? "Song completed" : "User interrupted", state.currentTime);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp();
        return 0;
    }

    std::map<std::string, SongGen::Genre> genreMap = {
        {"classical", SongGen::CLASSICAL}, {"jazz", SongGen::JAZZ}, {"pop", SongGen::POP},
        {"rock", SongGen::ROCK}, {"techno", SongGen::TECHNO}, {"rap", SongGen::RAP},
        {"blues", SongGen::BLUES}, {"country", SongGen::COUNTRY}, {"folk", SongGen::FOLK},
        {"reggae", SongGen::REGGAE}, {"metal", SongGen::METAL}, {"punk", SongGen::PUNK},
        {"disco", SongGen::DISCO}, {"funk", SongGen::FUNK}, {"soul", SongGen::SOUL},
        {"gospel", SongGen::GOSPEL}, {"ambient", SongGen::AMBIENT}, {"edm", SongGen::EDM},
        {"latin", SongGen::LATIN}, {"hiphop", SongGen::HIPHOP}
    };

    // Check if the first argument is a .song file
    if (argc >= 2 && argv[1][0] != '-' && std::string(argv[1]).find(".song") != std::string::npos) {
        bool forceStereo = (argc >= 3 && std::string(argv[2]) == "--stereo");
        playSong(argv[1], forceStereo);
        return 0;
    }

    // Handle song generation
    std::vector<SongGen::Genre> genres;
    for (int i = 1; i < argc && genres.size() < 3; ++i) {
        std::string genreStr(argv[i]);
        std::transform(genreStr.begin(), genreStr.end(), genreStr.begin(), ::tolower);
        auto it = genreMap.find(genreStr);
        if (it != genreMap.end()) {
            genres.push_back(it->second);
        } else {
            std::cerr << "Unknown genre: " << genreStr << std::endl;
            printHelp();
            return 1;
        }
    }

    if (genres.empty()) {
        printHelp();
        return 1;
    }

    SongGen::MusicGenerator generator;
    std::string filename = "song1.song";
    int songNum = 1;
    while (std::ifstream(filename)) {
        songNum++;
        filename = "song" + std::to_string(songNum) + ".song";
    }

    try {
        // Generate the song with the first genre
        auto [title, parts, sections] = generator.generateSong(genres[0]);
        // Save the generated song to the file
		std::string genreStr(argv[1]);
		std::string genreStrUpper = genreStr;
		std::transform(genreStrUpper.begin(), genreStrUpper.end(), genreStrUpper.begin(), ::toupper);
        generator.saveToFile(title, genreStrUpper, parts, sections, filename);
        // Verify the generated file
        std::ifstream checkFile(filename);
        if (!checkFile || checkFile.peek() == std::ifstream::traits_type::eof()) {
            SDL_Log("Generated song file %s is empty or invalid", filename.c_str());
            std::cerr << "Error: Generated song file is empty or invalid" << std::endl;
            return 1;
        }
        checkFile.close();
        std::cout << "Generated song: " << filename << std::endl;
    } catch (const std::exception& e) {
        SDL_Log("Error generating song: %s", e.what());
        std::cerr << "Error generating song: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}