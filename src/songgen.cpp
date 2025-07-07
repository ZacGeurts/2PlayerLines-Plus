// songgen.cpp - plays songgen.h .song files with instruments.h sound generation.
// Binds the two files together and is the command ./songgen

// Registering your instrument is at the bottom of instruments.h
// you can include instruments.h in your programs but be clear it is not free software.

// This is not free software and requires royalties for use for profit.
// Royalties are required for songgen.cpp songgen.h instruments.h and instrument files
// Interested parties can find my contact information at https://github.com/ZacGeurts
// If you can make profit financially, you can do the math and update my Patreon.
// https://patreon/ZacharyGeurts

// Follow the local law.
// FCC in the USA restricts what frequencies you can broadcast.
// Most, if not all countries restrict frequencies.
// Audible frequencies in the USA are a First Amendment Right. 
// There are some nefarious limitations, like faking a policeman phone call, but making sound is a human right.
/* 	
	Far out of this scope. I cap to 20hz and less than 44100hz (SDL2 maximum and exceeds human hearing).
 	We can go below 20hz down to 0hz, but top of the line car stereos might hit 8hz-12hz with expensive equipment.
 	20hz-80hz (20hz-120hz also common) should be top quality for a subwoofer and it does not try blowing out pc speakers.
*/
// You would need more than a speaker. Fun fact: WiFi is frequencies. Sound you cannot hear.Or sound is WiFi that you can.
// Do not restrict emergency communications or damage heart pace makers, etc.
// Always put hearing safety first. It does not grow back.
// Be kind to pets. - Dana White
// They have sensitive ears.

// SongGen is songen.h
// AudioUtils and rng is in instruments.h
// songen.cpp does not use a namespace.
// songgen.cpp uses songen.h for song files and instruments.h for playback.
// Neither talk to songgen.cpp.
// instruments.h talks to nobody but the ../instruments/ folder.
// songgen.h talks to instruments.h to get the instrument availability registry.
// this does not yet mean that it will add your new instruments to songs.
// instrument files would need a bit more depth, but because I want fun things to be user modifiable I kept it simplier to start.

// skip this part to avoid coder headache.
// you would need note range, and what genres it plays with, tempo, and pretty much every map from songgen in there and
// then guhhhh, just do not if you have not been modifiing this code for a month.
// you would need to include songgen.h and every instrument would need to be updated or it would be down again until then
// I would rather leave the code out and leave something a band student can read.

// ---
// oh yeah, did I meantion yet that this is a project you can have fun with AI.
// See if you can make a new instrument in the instrument folder with the files you downloaded.

// Copy the entire code from a file and paste it into AI and tell it what fantastic instrument you would like to hear. Maybe a twinkle.
// The bottom part tells the AI about all the AudioUtils audio tools available, so it can create your instrument.
// From there it should be able to create every hearable sound. RandomGenerator is for audio noise.
// Use an existing instrument and save it to a new file with the name you want, without spaces. (kazoo or your favorite)
// Come back and I will show you how to register it.

// Other than song1.song do not modify other .song files. You were warned to avoid my liability.

// You can replace the instrument name in song1.song with your newly named instrument.
// I do not recommend modifiing songgen.cpp, songgen.h, instruments.h, other then registering your new instrument.

// Registering your instrument is at the bottom of instruments.h
// Save your work. Backup anything that works that you cannot afford to lose.
// If you break something then you have to download the file again.

// run make clean before every update.
// run make
// hopefully it worked. If not, copy and paste the error back at the AI.
// ./songgen song1.song

// standard c++ code follows. SDL2 runs the show. 8 channel 0-44100hz - tops out beyond hearing and speakers able to tweeter it.
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
// #include <random> // nope

bool IS_STEREO = false; // leave false, playback adjusts itself.

// Flag to handle program termination
static volatile sig_atomic_t running = true;

// Signal handler for Ctrl+C
void handleSignal(int sig) {
    running = false;
    SDL_Log("Received signal %d, shutting down...", sig);
}

// ./songgen
void printHelp() {
    std::cout << "Generates songs\n";
    std::cout << "Usage:\n";
    std::cout << "  ./songgen jazz\n";
    std::cout << " \n";
    std::cout << "Playback\n";
    std::cout << "  ./songgen song1.song\n";
	std::cout << "\n";
    std::cerr << "Available Genres: ";
    bool first = true;
	
	// ask songgen.h for all of the current genres.
    for (const auto& [genre, name] : SongGen::MusicGenerator::genreNames) {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::cerr << (first ? "" : ", ") << lowerName;
        first = false;
    }
    std::cerr << "\n";
    std::cout << "Songgen numbers your songs as you create them.\n";
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
    long double bpm, duration, rootFreq;
    std::string scaleName, title, genres;
    std::vector<SongGen::Section> sections;
    std::vector<SongGen::Part> parts;
    int channels; // SDL2 supports up to 8 channels so we use 8. (7.1 audio)
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

    SongData song; // temporary settings - updates when song is loaded
    song.bpm = 120.0L;
    song.rootFreq = 440.0L;
    song.scaleName = "major";
    song.duration = 180.0L;
    song.channels = 8; // 7.1 surround
    song.parts.reserve(20000);
    song.sections.reserve(20000);

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
            } else if (token == "Genre:") {
                std::getline(ss, song.genres);
                song.genres = trim(song.genres);
            } else if (token == "BPM:") {
                ss >> song.bpm;
                if (!std::isfinite(song.bpm) || song.bpm <= 0.0f) {
                    SDL_Log("Invalid BPM at line %zu: %.2Lf, using default 120.0", lineNumber, song.bpm);
                    song.bpm = 120.0L;
                }
            } else if (token == "Scale:") {
                ss >> song.scaleName;
            } else if (token == "RootFrequency:") {
                ss >> song.rootFreq;
                if (!std::isfinite(song.rootFreq) || song.rootFreq <= 0.0f) {
                    SDL_Log("Invalid RootFrequency at line %zu: %.2Lf, using default 440.0", lineNumber, song.rootFreq);
                    song.rootFreq = 440.0L;
                }
            } else if (token == "Duration:") {
                ss >> song.duration;
                if (!std::isfinite(song.duration) || song.duration <= 0.0f) {
                    SDL_Log("Invalid Duration at line %zu: %.2Lf, using default 180.0", lineNumber, song.duration);
                    song.duration = 180.0L;
                }
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
                    SDL_Log("Invalid section at line %zu: start=%.2Lf, end=%.2Lf", lineNumber, section.startTime, section.endTime);
                }
            } else if (token == "Parts:") {
                ss >> expectedParts;
                inPart = false;
                inNotes = inPanAutomation = inVolumeAutomation = inReverbMixAutomation = false;
                if (!currentPart.instrument.empty()) {
                    song.parts.push_back(currentPart);
                    SDL_Log("Parsed part: %s with %zu notes", currentPart.instrument.c_str(), currentPart.notes.size());
                    currentPart = SongGen::Part();
                }
            } else if (token == "Part:") {
                if (!currentPart.instrument.empty()) {
                    song.parts.push_back(currentPart);
                    SDL_Log("Parsed part: %s with %zu notes", currentPart.instrument.c_str(), currentPart.notes.size());
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
                    SDL_Log("Skipping invalid note at line %zu: start=%.2Lf, freq=%.2Lf, duration=%.2Lf",
                            lineNumber, note.startTime, note.freq, note.duration);
                }
            } else if (inPart && token == "PanAutomation:") {
                ss >> expectedPanPoints;
                inPanAutomation = true;
                inNotes = inVolumeAutomation = inReverbMixAutomation = false;
            } else if (inPart && inPanAutomation && token == "PanPoint:") {
                long double time, value;
                ss >> time >> value;
                currentPart.panAutomation.emplace_back(time, value);
            } else if (inPart && token == "VolumeAutomation:") {
                ss >> expectedVolumePoints;
                inVolumeAutomation = true;
                inNotes = inPanAutomation = inReverbMixAutomation = false;
            } else if (inPart && inVolumeAutomation && token == "VolumePoint:") {
                long double time, value;
                ss >> time >> value;
                currentPart.volumeAutomation.emplace_back(time, value);
            } else if (inPart && token == "ReverbMixAutomation:") {
                ss >> expectedReverbPoints;
                inReverbMixAutomation = true;
                inNotes = inPanAutomation = inVolumeAutomation = false;
            } else if (inPart && inReverbMixAutomation && token == "ReverbMixPoint:") {
                long double time, value;
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

    if (song.sections.empty()) {
        SDL_Log("No sections parsed, adding default section");
        song.sections.emplace_back("Default", 0.0f, song.duration, 0.0f, "");
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

    static const std::set<std::string> validScales = {
        "major", "minor", "dorian", "mixolydian", "blues", "pentatonic_minor",
        "harmonic_minor", "whole_tone", "chromatic"
    };
    if (validScales.find(song.scaleName) == validScales.end()) {
        SDL_Log("Invalid scale '%s', defaulting to 'major'", song.scaleName.c_str());
        song.scaleName = "major";
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

// ---
    SDL_Log("Loaded song: %s", filename.c_str());
    SDL_Log("Metadata:");
    SDL_Log("  Title: %s", song.title.c_str());
    SDL_Log("  Genre: %s", song.genres.c_str());
    SDL_Log("  BPM: %.2Lf", song.bpm);
    SDL_Log("  Scale: %s", song.scaleName.c_str());
    SDL_Log("  Root Frequency: %.2Lf Hz", song.rootFreq);
    SDL_Log("  Duration: %.2Lf seconds", song.duration);
    SDL_Log("  Instruments: %s", instrumentList.c_str());
    SDL_Log("  Parts: %zu, Sections: %zu", song.parts.size(), song.sections.size());
    SDL_Log("CTRL-C to Exit playback.");

    return song;
}

// ---
struct PlaybackState {
    SongData song;
    long double currentTime;
    bool playing;
    std::vector<size_t> nextNoteIndices;
	// AudioUtils means instruments.h
    std::vector<AudioUtils::Reverb> reverbs;
    std::vector<AudioUtils::Distortion> distortions;
    size_t currentSectionIdx;

    struct ActiveNote {
        size_t noteIndex;
        long double startTime;
        long double endTime;
    };
    std::vector<std::vector<ActiveNote>> activeNotes;

    PlaybackState(const SongData& s)
        : song(s), currentTime(0.0f), playing(true), nextNoteIndices(s.parts.size(), 0),
          reverbs(s.parts.size()), distortions(s.parts.size()), currentSectionIdx(0),
          activeNotes(s.parts.size()) {
        for (size_t i = 0; i < s.parts.size(); ++i) {
            auto& part = s.parts[i];
            reverbs[i] = AudioUtils::Reverb(part.reverbDelay, part.reverbDecay, part.reverbMix);
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

void audioCallback(void* userdata, Uint8* stream, int len) {
    PlaybackState* state = static_cast<PlaybackState*>(userdata);
    float* output = reinterpret_cast<float*>(stream);
    bool isStereo = state->song.channels == 2;
    int numChannels = isStereo ? 2 : 8;
    int numSamples = len / sizeof(float) / numChannels;

    long double fullDuration = state->song.sections.empty() ? state->song.duration : state->song.sections.back().endTime;
    fullDuration += 5.0L; // 5s for reverb/decay

    if (!state->playing || state->currentTime > fullDuration || !running) {
        state->playing = false;
        std::fill(output, output + numSamples * numChannels, 0.0L);
        SDL_Log("Audio callback stopped: time=%.2Lf, duration=%.2Lf, running=%d", state->currentTime, fullDuration, running);
        return;
    }

    // Pre-allocated per-thread output buffers
    thread_local static std::vector<long double> localOutput;
    localOutput.resize(static_cast<size_t>(numSamples) * numChannels, 0.0L);

    // Initialize thread pool once
    static unsigned int numThreads = std::min(std::thread::hardware_concurrency(), static_cast<unsigned int>(state->song.parts.size()));
    if (numThreads == 0) numThreads = 1;
    std::call_once(pool_init_flag, [&]() {
        thread_pool.reserve(numThreads);
    });

    std::mutex outputMutex;
    auto processParts = [&](size_t startIdx, size_t endIdx, long double startTime) {
        size_t activeNoteCount = 0;
        std::fill(localOutput.begin(), localOutput.end(), 0.0L);

        for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
            long double t = startTime + i / static_cast<long double>(AudioUtils::DEFAULT_SAMPLE_RATE);
            long double L = 0.0L, R = 0.0L, C = 0.0L, LFE = 0.0L, Ls = 0.0L, Rs = 0.0L, Lsb = 0.0L, Rsb = 0.0L;

            long double fadeGain = 1.0L;
            if (t < 5.0L) {
                fadeGain = t / 5.0L;
            } else if (t > fullDuration - 5.0L) {
                fadeGain = (fullDuration - t) / 5.0L;
            }

            for (size_t partIdx = startIdx; partIdx < endIdx && partIdx < state->song.parts.size(); ++partIdx) {
                auto& part = state->song.parts[partIdx];
                auto& nextIdx = state->nextNoteIndices[partIdx];
                auto& active = state->activeNotes[partIdx];

                long double pan = Instruments::interpolateAutomation(t, part.panAutomation, part.pan);
                long double volume = Instruments::interpolateAutomation(t, part.volumeAutomation, 0.5L);
                long double reverbMix = Instruments::interpolateAutomation(t, part.reverbMixAutomation, part.reverbMix);

                long double leftGain = (pan <= 0.0L) ? 1.0L : 1.0L - pan;
                long double rightGain = (pan >= 0.0L) ? 1.0L : 1.0L + pan;
                long double surroundGain = 0.5L * (leftGain + rightGain);
                long double centerWeight = (part.instrument == "vocal") ? 0.8L : 0.3L;
                long double lfeWeight = (part.instrument == "subbass" || part.instrument == "kick") ? 0.5L : 0.1L;
                long double sideWeight = (part.instrument == "guitar" || part.instrument == "syntharp") ? 0.6L : 0.4L;

                while (nextIdx < part.notes.size() && part.notes[nextIdx].startTime <= t && active.size() < 16) {
                    const auto& note = part.notes[nextIdx];
                    active.push_back({nextIdx, note.startTime, note.startTime + note.duration});
                    ++nextIdx;
                }

                for (auto it = active.begin(); it != active.end();) {
                    const auto& note = part.notes[it->noteIndex];
                    if (t <= it->endTime) {
                        long double noteTime = t - note.startTime;
                        long double sample;
                        if (part.instrument == "vocal_0" || part.instrument == "vocal_1") {
                            sample = Instruments::generateInstrumentWave(
                                part.instrument, noteTime, note.freq, note.duration, AudioUtils::DEFAULT_SAMPLE_RATE);
                            sample *= (note.open ? 1.0L : 0.8L);
                        } else {
                            // Incremental waveform generation using long double buffer
                            if (buffer_pos >= AudioUtils::BUFFER_SIZE) {
                                AudioUtils::RandomGenerator::fill_buffer();
                            }
                            sample = buffer[buffer_pos++]; // Use precomputed waveform or noise
                            sample *= note.volume * note.velocity * volume * fadeGain;
                        }
                        if (part.useDistortion) {
                            sample = state->distortions[partIdx].process(sample);
                        }
                        if (part.useReverb) {
                            sample = state->reverbs[partIdx].process(sample * (1.0L - reverbMix)) + sample * reverbMix;
                        }

                        L += sample * leftGain * sideWeight;
                        R += sample * rightGain * sideWeight;
                        C += sample * centerWeight;
                        LFE += sample * lfeWeight;
                        Ls += sample * surroundGain * sideWeight;
                        Rs += sample * surroundGain * sideWeight;
                        Lsb += sample * surroundGain * sideWeight;
                        Rsb += sample * surroundGain * sideWeight;

                        ++activeNoteCount;
                        ++it;
                    } else {
                        it = active.erase(it);
                    }
                }
            }

			// Stereo speakers combine 7.1 channels into two with standard downmix coefficients
			if (isStereo) {
    			long double L_out = L + 0.707L * C + 0.707L * LFE + 0.5L * Ls + 0.5L * Lsb;
    			long double R_out = R + 0.707L * C + 0.707L * LFE + 0.5L * Rs + 0.5L * Rsb;
    			localOutput[i * 2 + 0] = std::max(-1.0L, std::min(1.0L, L_out));
    			localOutput[i * 2 + 1] = std::max(-1.0L, std::min(1.0L, R_out));
			} else {
    			localOutput[i * 8 + 0] = std::max(-1.0L, std::min(1.0L, L));
    			localOutput[i * 8 + 1] = std::max(-1.0L, std::min(1.0L, R));
    			localOutput[i * 8 + 2] = std::max(-1.0L, std::min(1.0L, C));
    			localOutput[i * 8 + 3] = std::max(-1.0L, std::min(1.0L, LFE));
    			localOutput[i * 8 + 4] = std::max(-1.0L, std::min(1.0L, Ls));
    			localOutput[i * 8 + 5] = std::max(-1.0L, std::min(1.0L, Rs));
    			localOutput[i * 8 + 6] = std::max(-1.0L, std::min(1.0L, Lsb));
    			localOutput[i * 8 + 7] = std::max(-1.0L, std::min(1.0L, Rsb));
			}
        }

        if (activeNoteCount == 0 && threadIdx == 0) {
            SDL_Log("No active notes at time %.2Lf", startTime);
        }

        std::lock_guard<std::mutex> lock(outputMutex);
        for (size_t i = 0; i < static_cast<size_t>(numSamples) * numChannels; ++i) {
            threadOutputs[threadIdx][i] += localOutput[i];
        }
    };

    for (size_t i = 0; i < static_cast<size_t>(numSamples); ++i) {
        long double t = state->currentTime + i / AudioUtils::DEFAULT_SAMPLE_RATE;
        if (t > fullDuration || !running) {
            state->playing = false;
            break;
        }
        if (state->currentSectionIdx < state->song.sections.size()) {
            const auto& section = state->song.sections[state->currentSectionIdx];
            if (t >= section.startTime) {
                size_t noteCount = countNotesInSection(state->song, section);
                std::string instruments = getInstrumentsInSection(state->song, section);
                SDL_Log("Playing Section %s with %zu notes at timestamp %.2Lf, Instruments: %s",
                        section.name.c_str(), noteCount, section.startTime, instruments.c_str());
                state->currentSectionIdx++;
            }
        }
    }

    std::vector<std::thread> threads;
    size_t partsPerThread = (state->song.parts.size() + numThreads - 1) / numThreads;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startIdx = t * partsPerThread;
        size_t endIdx = std::min(startIdx + partsPerThread, state->song.parts.size());
        if (startIdx < state->song.parts.size()) {
            threads.emplace_back(processParts, startIdx, endIdx, t, state->currentTime);
        }
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    for (const auto& tOutput : threadOutputs) {
        for (size_t i = 0; i < static_cast<size_t>(numSamples) * numChannels; ++i) {
            output[i] += tOutput[i];
        }
    }

    state->currentTime += numSamples / AudioUtils::DEFAULT_SAMPLE_RATE;

    // Debug output to check if samples are being generated
    long double maxSample = 0.0L;
    for (int i = 0; i < numSamples * numChannels; ++i) {
        maxSample = std::max(maxSample, std::abs(output[i]));
    }
    if (maxSample > 0.0f) {
        SDL_Log("Generated %d samples at time %.2Lf, max amplitude: %.4Lf", numSamples, state->currentTime, maxSample);
    }
}

// ---
void playSong(const std::string& filename) {
    SongData song;
    try {
        song = parseSongFile(filename);
    } catch (const std::exception& e) {
        SDL_Log("Failed to parse song file: %s", e.what());
        return;
    }

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return;
    }

    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = AudioUtils::DEFAULT_SAMPLE_RATE; // 41200 Hz
    want.format = AUDIO_F32; // 32-bit float samples
    want.channels = forceStereo ? 2 : 8; // Try 8 channels, fallback to stereo
    want.samples = 2048; // Larger buffer to reduce callback frequency
    want.callback = audioCallback;

    song.channels = want.channels;
    PlaybackState state(song);
    want.userdata = &state;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (device == 0 && !forceStereo) {
        SDL_Log("Failed 7.1 audio: %s, trying stereo", SDL_GetError());
        want.channels = 2;
        song.channels = 2;
        device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }
    if (device == 0) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    // Pre-fill long double buffer if used in audioCallback
    try {
        fill_buffer(); // From previous code, ensure buffer is ready
    } catch (const std::runtime_error& e) {
        SDL_Log("Failed to pre-fill buffer: %s", e.what());
        SDL_CloseAudioDevice(device);
        SDL_Quit();
        return;
    }

    SDL_Log("Playing song %s with %d channels", filename.c_str(), have.channels);
    SDL_PauseAudioDevice(device, 0);

    const Uint32 timeout = SDL_GetTicks() + static_cast<Uint32>(song.duration * 1000.0L + 5000); // 5s buffer
    SDL_Event event;
    while (state.playing && running && SDL_GetTicks() < timeout) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
                SDL_Log("User requested exit");
            }
        }
        // No SDL_Delay; audio callback drives timing
    }

    state.playing = false;
    SDL_PauseAudioDevice(device, 1);
    SDL_CloseAudioDevice(device);
    SDL_Quit();
    SDL_Log("Playback stopped: %s at timestamp %.2Lf", running ? "Song completed" : "User interrupted", state.currentTime);
}

void logGenres() {
    std::string genreList = "Loaded genres: ";
    bool first = true;
    for (const auto& [genre, name] : SongGen::MusicGenerator::genreNames) {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        genreList += (first ? "" : ", ") + lowerName;
        first = false;
    }
    SDL_Log("%s", genreList.c_str());
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp();
        return 0;
    }
	
	playSong(argv[1], IS_STEREO); // argv[1],0 is 8 channel and argv[1],1 forces stereo

    // Log loaded genres
    logGenres();

    // Create reverse genre map for string-to-Genre lookup from songgen.h
    std::map<std::string, SongGen::Genre> genreLookup;
    for (const auto& [genre, name] : SongGen::MusicGenerator::genreNames) {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        genreLookup[lowerName] = genre;
    }

    // Parse genres
    std::vector<SongGen::Genre> genres;
    for (int i = 1; i < argc && genres.size() < 3; ++i) {
        std::string genreStr(argv[i]);
        std::transform(genreStr.begin(), genreStr.end(), genreStr.begin(), ::tolower);
        auto it = genreLookup.find(genreStr);
        if (it != genreLookup.end()) {
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
    while (std::ifstream(filename)) { // while song name is used, pick the next song number
        songNum++;
        filename = "song" + std::to_string(songNum) + ".song";
    }

    try {
        auto [title, parts, sections] = generator.generateSong(genres[0]);

        long double bpm = generator.getGenreBPM().find(genres[0]) != generator.getGenreBPM().end() ?
            generator.getGenreBPM().at(genres[0]) : 120.0L;
        std::string scale = "major";
        if (generator.getGenreScales().find(genres[0]) != generator.getGenreScales().end() &&
            !generator.getGenreScales().at(genres[0]).empty()) {
            AudioUtils::RandomGenerator rng;
            std::uniform_int_distribution<size_t> dist(0, generator.getGenreScales().at(genres[0]).size() - 1);
            scale = generator.getGenreScales().at(genres[0])[rng.random_fixed_point()];
        }
        long double rootFrequency = 440.0L;
        long double duration = 180.0L;
        if (!sections.empty()) {
            duration = std::max_element(sections.begin(), sections.end(),
                [](const auto& a, const auto& b) { return a.endTime < b.endTime; })->endTime;
        }

        std::string genreStr = SongGen::MusicGenerator::genreNames.at(genres[0]);
        std::string genreStrUpper = genreStr;
        std::transform(genreStrUpper.begin(), genreStrUpper.end(), genreStrUpper.begin(), ::toupper);
        generator.saveToFile(title, genreStrUpper, bpm, scale, rootFrequency, duration, parts, sections, filename);

        std::ifstream checkFile(filename);
        if (!checkFile || checkFile.peek() == std::ifstream::traits_type::eof()) {
            SDL_Log("Generated song file %s is empty or invalid", filename.c_str());
            std::cerr << "Error: Generated song file is empty or invalid" << std::endl;
            return 1;
        }
        checkFile.close();
        std::cout << "Generated song: " << filename << std::endl;
		SDL_Log("*** Generated song file %s ***", filename.c_str();
    } catch (const std::exception& e) {
        SDL_Log("Error generating song: %s", e.what());
        std::cerr << "Error generating song: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}