// This is not free software and requires royalties for commercial use.
// Royalties are required for songgen.cpp songgen.h and instruments.h
// The other linesplus code is free and cannot be resold.
// Interested parties can find my contact information at https://github.com/ZacGeurts

#ifndef SONGGEN_H
#define SONGGEN_H

#include "instruments.h"
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#include <SDL2/SDL.h>

namespace SongGen {
    enum Genre {
        CLASSICAL, JAZZ, POP, ROCK, TECHNO, RAP, BLUES, COUNTRY, FOLK, REGGAE,
        METAL, PUNK, DISCO, FUNK, SOUL, GOSPEL, AMBIENT, EDM, LATIN, HIPHOP
    };

    struct Note {
        float freq;
        float duration;
        float startTime;
        int phoneme;
        bool open;
        float volume;
        float velocity;
        Note(float f = 440.0f, float d = 0.0625f, float s = 0.0f, int p = -1, bool o = false, float v = 0.5f, float vel = 0.8f)
            : freq(f), duration(d), startTime(s), phoneme(p), open(o), volume(v), velocity(vel) {}
    };

    struct Part {
        std::vector<Note> notes;
        std::string instrument;
        float pan;
        float reverbMix;
        std::string sectionName;
        std::vector<std::pair<float, float>> panAutomation;
        std::vector<std::pair<float, float>> volumeAutomation;
        std::vector<std::pair<float, float>> reverbMixAutomation;
        bool useReverb;
        float reverbDelay;
        float reverbDecay;
        float reverbMixFactor;
        bool useDistortion;
        float distortionDrive;
        float distortionThreshold;
        Part() : pan(0.0f), reverbMix(0.2f), sectionName(""), useReverb(false), reverbDelay(0.1f),
                 reverbDecay(0.5f), reverbMixFactor(0.2f), useDistortion(false), distortionDrive(1.5f),
                 distortionThreshold(0.7f) {}
    };

    struct Section {
        std::string name;
        float startTime;
        float endTime;
        float progress;
        std::string templateName;
        Section(std::string n = "", float s = 0.0f, float e = 0.0f, float p = 0.0f, std::string t = "")
            : name(n), startTime(s), endTime(e), progress(p), templateName(t) {}
    };

    class MusicGenerator {
    public:
std::tuple<std::string, std::vector<Part>, std::vector<Section>> generateSong(Genre g, float totalDur = -1.0f, float rootFreq = 440.0f, float bpm = 0.0f) {
    ::SDL_Log("Starting song generation for genre %s, requested duration %.2f seconds", genreNames[g].c_str(), totalDur);
    // Set random totalDur between 180 and 300 seconds if not specified
    if (totalDur < 0.0f) {
        std::uniform_real_distribution<float> durDist(180.0f, 300.0f);
        totalDur = durDist(rng);
        ::SDL_Log("No duration specified, randomly set to %.2f seconds", totalDur);
    }
    // Validate and clamp totalDur to 180–300 seconds
    if (!std::isfinite(totalDur) || totalDur < 180.0f || totalDur > 300.0f) {
        ::SDL_Log("Invalid totalDur %.2f, clamping to range 180.0–300.0 seconds", totalDur);
        totalDur = std::clamp(totalDur, 180.0f, 300.0f);
    }
    if (!std::isfinite(rootFreq) || rootFreq <= 0.0f) {
        ::SDL_Log("Invalid rootFreq %.2f, setting to 440.0 Hz", rootFreq);
        rootFreq = 440.0f;
    }
    if (!std::isfinite(bpm) || bpm <= 0.0f) {
        bpm = genreBPM[g];
        ::SDL_Log("Invalid or unspecified BPM, using genre default %.2f", bpm);
    }

    sectionTemplates.clear();
    chordProgressions.clear();
    melodyMotif.clear();

    std::string title = generateTitle();
    std::vector<Part> parts;
    std::vector<Section> sections;
    float t = 0.0f;
    float sectionDur = 32.0f * (60.0f / bpm);
    if (sectionDur > totalDur / 6.0f) {
        sectionDur = totalDur / 6.0f;
        ::SDL_Log("Section duration adjusted to %.2f seconds to fit total duration", sectionDur);
    }
    const std::vector<std::string> scaleNames = genreScales[g];
    std::string scaleName = scaleNames[rng() % scaleNames.size()];
    ::SDL_Log("Selected scale: %s", scaleName.c_str());

    // Define base section plan
    std::vector<std::tuple<std::string, std::string, float>> sectionPlan = {
        {"Intro", "Intro", 0.0f},
        {"Verse1", "Verse", 0.2f},
        {"Chorus1", "Chorus", 0.4f},
        {"Verse2", "Verse", 0.6f},
        {"Chorus2", "Chorus", 0.8f},
        {"Outro", "Outro", 1.0f}
    };
    if (g == CLASSICAL || g == AMBIENT) {
        sectionPlan = {
            {"Intro", "Intro", 0.0f},
            {"PartA", "Verse", 0.25f},
            {"PartB", "Chorus", 0.5f},
            {"PartA2", "Verse", 0.75f},
            {"Outro", "Outro", 1.0f}
        };
    } else if (g == JAZZ || g == BLUES) {
        sectionPlan = {
            {"Intro", "Intro", 0.0f},
            {"Head1", "Verse", 0.2f},
            {"Solo", "Chorus", 0.4f},
            {"Head2", "Verse", 0.6f},
            {"Outro", "Outro", 0.8f}
        };
    }

    // Estimate total duration of base plan
    float basePlanDur = 0.0f;
    for (const auto& [name, templateName, progress] : sectionPlan) {
        float dur = (name == "Intro" || name == "Outro") ? sectionDur * 0.5f : sectionDur;
        basePlanDur += dur;
    }

    // Add tuplets, probably
	std::vector<std::tuple<std::string, std::string, float>> extendedPlan = sectionPlan;
    if (totalDur > basePlanDur * 1.2f) {
        int extraSections = static_cast<int>((totalDur - basePlanDur) / sectionDur);
        int verseCount = 2, chorusCount = 2;
        for (int i = 0; i < extraSections; ++i) {
            if (i % 2 == 0) {
                std::string name = "Verse" + std::to_string(++verseCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Verse", 0.6f + i * 0.1f});
            } else {
                std::string name = "Chorus" + std::to_string(++chorusCount);
                extendedPlan.insert(extendedPlan.end() - 1, {name, "Chorus", 0.8f + i * 0.1f});
            }
        }
    }

    // Generate sections
    for (size_t i = 0; i < extendedPlan.size() && t < totalDur; ++i) {
        const auto& [name, templateName, progress] = extendedPlan[i];
        float endTime = t + (name == "Intro" ? sectionDur * 0.5f : name == "Outro" ? sectionDur * 0.5f : sectionDur);
        if (endTime > totalDur) endTime = totalDur;
        sections.emplace_back(name, t, endTime, progress, templateName);
        ::SDL_Log("Added section %s: %.2f to %.2f seconds", name.c_str(), t, endTime);
        t = endTime;
    }

    // Adjust final section to exactly match totalDur
    if (!sections.empty() && sections.back().endTime < totalDur) {
        sections.back().endTime = totalDur;
        ::SDL_Log("Adjusted final section %s end time to %.2f seconds", sections.back().name.c_str(), totalDur);
    }

    float beat = 60.0f / bpm;

    // Determine intro style (e.g., vocal-only for certain genres)
    bool vocalOnlyIntro = (g == GOSPEL || g == SOUL || g == POP || g == RAP || g == HIPHOP) && rng() % 3 == 0;
    ::SDL_Log("Intro style: %s", vocalOnlyIntro ? "Vocal-only" : "Standard");

// Select instruments per section
std::map<std::string, std::vector<std::string>> sectionInstruments;
const auto& availableInstruments = genreInstruments[g];
for (const auto& section : sections) {
    std::vector<std::string> insts;
    if (section.name == "Intro" && vocalOnlyIntro) {
        insts.push_back((rng() % 2) ? "vocal_0" : "vocal_1");
    } else {
        // Base instruments for all sections
        insts.push_back(availableInstruments[rng() % availableInstruments.size()]); // Melody-like
        insts.push_back("bass"); // Always include bass for harmonic foundation
        // Add genre-specific instruments
        if (section.templateName == "Chorus") {
            if (g == EDM || g == TECHNO || g == AMBIENT) insts.push_back("subbass");
            if (g == CLASSICAL || g == AMBIENT || g == GOSPEL) insts.push_back("pad");
            insts.push_back(availableInstruments[rng() % availableInstruments.size()]); // Extra for chorus
        } else if (section.templateName == "Verse" || section.templateName == "Solo" || section.templateName == "Head") {
            if (g == ROCK || g == PUNK || g == METAL || g == COUNTRY || g == FOLK || g == REGGAE) insts.push_back("guitar");
            if (g == JAZZ || g == BLUES) insts.push_back("saxophone");
        }
        // Add percussion for non-intro sections
        if (section.name != "Intro" && (g == ROCK || g == PUNK || g == METAL || g == DISCO || g == FUNK || g == EDM || g == TECHNO || g == LATIN || g == REGGAE)) {
            insts.push_back("kick");
            insts.push_back("snare");
            insts.push_back((g == ROCK || g == METAL) ? "cymbal" : "hihat_closed");
        } else if (section.name != "Intro" && (g == JAZZ || g == BLUES)) {
            insts.push_back("hihat_closed");
            insts.push_back("snare");
        }
        // Add vocal for specific genres and sections
        if ((g == RAP || g == HIPHOP || g == GOSPEL || g == SOUL || (g == POP && rng() % 2)) && section.templateName != "Intro") {
            insts.push_back((rng() % 2) ? "vocal_0" : "vocal_1");
        }
    }
    // Remove duplicates while preserving order
    std::vector<std::string> uniqueInsts;
    std::set<std::string> seen;
    for (const auto& inst : insts) {
        if (seen.insert(inst).second) uniqueInsts.push_back(inst);
    }
    sectionInstruments[section.name] = uniqueInsts;
    ::SDL_Log("Section %s instruments: %s", section.name.c_str(), std::accumulate(uniqueInsts.begin(), uniqueInsts.end(), std::string(),
        [](const std::string& a, const std::string& b) { return a.empty() ? b : a + ", " + b; }).c_str());
}

// Generate parts based on section instruments
for (const auto& section : sections) {
    const auto& insts = sectionInstruments[section.name];
    for (const auto& inst : insts) {
        if (inst.find("vocal") != std::string::npos) {
            Part vocal = generateVocal(g, scaleName, rootFreq, totalDur, sections, bpm);
            vocal.instrument = inst;
            vocal.notes.erase(std::remove_if(vocal.notes.begin(), vocal.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                vocal.notes.end());
            parts.push_back(vocal);
        } else if (inst == "bass" || inst == "subbass") {
            Part bass = generateBass(g, scaleName, rootFreq, totalDur, sections, bpm);
            bass.instrument = inst;
            bass.notes.erase(std::remove_if(bass.notes.begin(), bass.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                bass.notes.end());
            parts.push_back(bass);
        } else if (inst == "guitar") {
            Part guitar = generateGuitar(g, scaleName, rootFreq, totalDur, sections, bpm);
            guitar.instrument = inst;
            guitar.notes.erase(std::remove_if(guitar.notes.begin(), guitar.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                guitar.notes.end());
            parts.push_back(guitar);
        } else if (inst == "kick" || inst == "snare" || inst == "cymbal" || inst == "hihat_closed" || inst == "hihat_open" || inst == "clap") {
            Part rhythm = generateRhythm(g, totalDur, beat, bpm, inst, sections);
            rhythm.notes.erase(std::remove_if(rhythm.notes.begin(), rhythm.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                rhythm.notes.end());
            parts.push_back(rhythm);
        } else if (inst == "syntharp" || inst == "leadsynth" || (inst == "piano" && (g == Genre::EDM || g == Genre::TECHNO || g == Genre::CLASSICAL))) {
            Part arp = generateArpeggio(g, scaleName, rootFreq, totalDur, sections, bpm);
            arp.instrument = inst;
            arp.notes.erase(std::remove_if(arp.notes.begin(), arp.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                arp.notes.end());
            parts.push_back(arp);
		} else if (inst == "piano") {
            Part piano = generatePiano(g, scaleName, rootFreq, totalDur, sections, bpm);
            piano.instrument = inst;
            piano.notes.erase(std::remove_if(piano.notes.begin(), piano.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                piano.notes.end());
            parts.push_back(piano);
        } else if (inst == "pad" || inst == "strings" || inst == "organ") {
            Part harmony = generateHarmony(g, scaleName, rootFreq, totalDur, sections, bpm);
            harmony.instrument = inst;
            harmony.notes.erase(std::remove_if(harmony.notes.begin(), harmony.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                harmony.notes.end());
            parts.push_back(harmony);
        } else {
            Part melody = generateMelody(g, scaleName, rootFreq, totalDur, sections, bpm);
            melody.instrument = inst;
            melody.notes.erase(std::remove_if(melody.notes.begin(), melody.notes.end(),
                [section](const Note& n) { return n.startTime < section.startTime || n.startTime >= section.endTime; }),
                melody.notes.end());
            parts.push_back(melody);
        }
    }
}

    // Apply volume adjustments
    std::vector<std::string> percussionInstruments = {"kick", "snare", "cymbal", "hihat_closed", "hihat_open", "clap"};
    for (auto& part : parts) {
        // Check if part is percussion
        bool isPercussion = std::find(percussionInstruments.begin(), percussionInstruments.end(), part.instrument) != percussionInstruments.end();

        // Adjust note volumes
        for (auto& note : part.notes) {
            if (isPercussion) {
                note.volume = std::min(1.0f, note.volume * 1.5f); // 150% for percussion
                ::SDL_Log("Increased volume to %.2f for percussion note in part %s", note.volume, part.sectionName.c_str());
            }
            note.volume = std::min(1.0f, note.volume * 0.75f); // 75% for all parts
            ::SDL_Log("Reduced volume to %.2f for note in part %s", note.volume, part.sectionName.c_str());
        }

        // Adjust volume automation
        for (auto& [time, value] : part.volumeAutomation) {
            if (isPercussion) {
                value = std::min(1.0f, value * 1.5f); // 150% for percussion
                ::SDL_Log("Increased volume automation to %.2f at t=%.2f for percussion part %s", value, time, part.sectionName.c_str());
            }
            value = std::min(1.0f, value * 0.75f); // 75% for all parts
            ::SDL_Log("Reduced volume automation to %.2f at t=%.2f for part %s", value, time, part.sectionName.c_str());
        }

        // Apply fade-in and fade-out
        // Fade-in: 0 to 5 seconds, volume from 0 to adjusted initial volume
        float initialVolume = part.volumeAutomation.empty() ? (isPercussion ? 0.5f * 1.5f * 0.75f : 0.5f * 0.75f) : part.volumeAutomation[0].second;
        part.volumeAutomation.insert(part.volumeAutomation.begin(), {{0.0f, 0.0f}, {5.0f, initialVolume}});

        // Fade-out: last 5 seconds, volume from last value to 0        
        float lastVolume = initialVolume;
        for (const auto& [time, value] : part.volumeAutomation) {
            if (time <= totalDur && time > totalDur - 5.0f) {
                lastVolume = value;
            }
        }
        part.volumeAutomation.emplace_back(totalDur - 5.0f, lastVolume);
        part.volumeAutomation.emplace_back(totalDur, 0.0f);

        // Sort volume automation to ensure chronological order
        std::sort(part.volumeAutomation.begin(), part.volumeAutomation.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        // Log the number of notes before processing
        ::SDL_Log("Before processing part %s: %zu notes", part.sectionName.c_str(), part.notes.size());

        // Sort and clean notes
        std::sort(part.notes.begin(), part.notes.end(),
                  [](const Note& a, const Note& b) {
                      return std::isfinite(a.startTime) && std::isfinite(b.startTime) ? a.startTime < b.startTime : false;
                  });
        part.notes.erase(
            std::remove_if(part.notes.begin(), part.notes.end(),
                [totalDur](const Note& n) {
                    bool invalid = n.startTime >= totalDur || !std::isfinite(n.freq) ||
                                   !std::isfinite(n.startTime) || !std::isfinite(n.duration);
                    if (invalid) {
                        ::SDL_Log("Removing invalid note: start=%.2f, freq=%.2f, duration=%.2f",
                                  n.startTime, n.freq, n.duration);
                    }
                    return invalid;
                }),
            part.notes.end()
        );

        // Log the number of notes after processing
        ::SDL_Log("After processing part %s: %zu notes", part.sectionName.c_str(), part.notes.size());
    }

    // Check total notes and truncate if necessary
    size_t totalNotes = std::accumulate(parts.begin(), parts.end(), size_t(0),
        [](size_t sum, const Part& p) { return sum + p.notes.size(); });
    if (totalNotes > 5000) {
        ::SDL_Log("Warning: Total notes %zu exceeds safe limit, truncating", totalNotes);
        for (auto& part : parts) {
            if (part.notes.size() > 1000) {
                part.notes.resize(1000);
                ::SDL_Log("Truncated part %s to 1000 notes", part.sectionName.c_str());
            }
        }
        totalNotes = std::accumulate(parts.begin(), parts.end(), size_t(0),
            [](size_t sum, const Part& p) { return sum + p.notes.size(); });
    }

    ::SDL_Log("Song generation complete: %zu parts, %zu sections, total notes %zu",
              parts.size(), sections.size(), totalNotes);

    return {title, parts, sections};
}

        void saveToFile(const std::string& title, const std::vector<Part>& parts, const std::vector<Section>& sections, const std::string& filename) {
            ::SDL_Log("Saving song '%s' to file %s", title.c_str(), filename.c_str());
            std::ofstream out(filename);
            if (!out.is_open()) {
                ::SDL_Log("Failed to open file %s for writing", filename.c_str());
                return;
            }

            out << "Song: " << title << "\n";
            out << "Sections: " << sections.size() << "\n";
            for (const auto& section : sections) {
                out << "Section: " << section.name << " " << section.startTime << " " << section.endTime
                    << " Progress: " << section.progress << " Template: " << section.templateName << "\n";
            }
            out << "Parts: " << parts.size() << "\n";
            for (const auto& part : parts) {
                out << "Part: " << part.sectionName << "\n";
                out << "Instrument: " << part.instrument << "\n";
                out << "Pan: " << part.pan << "\n";
                out << "ReverbMix: " << part.reverbMix << "\n";
                out << "UseReverb: " << (part.useReverb ? "true" : "false") << "\n";
                out << "ReverbDelay: " << part.reverbDelay << "\n";
                out << "ReverbDecay: " << part.reverbDecay << "\n";
                out << "ReverbMixFactor: " << part.reverbMixFactor << "\n";
                out << "UseDistortion: " << (part.useDistortion ? "true" : "false") << "\n";
                out << "DistortionDrive: " << part.distortionDrive << "\n";
                out << "DistortionThreshold: " << part.distortionThreshold << "\n";
                out << "Notes: " << part.notes.size() << "\n";
                for (const auto& note : part.notes) {
                    out << "Note: " << note.freq << " " << note.duration << " " << note.startTime
                        << " Phoneme: " << note.phoneme << " Open: " << (note.open ? "true" : "false")
                        << " Volume: " << note.volume << " Velocity: " << note.velocity << "\n";
                }
                out << "PanAutomation: " << part.panAutomation.size() << "\n";
                for (const auto& [time, value] : part.panAutomation) {
                    out << "PanPoint: " << time << " " << value << "\n";
                }
                out << "VolumeAutomation: " << part.volumeAutomation.size() << "\n";
                for (const auto& [time, value] : part.volumeAutomation) {
                    out << "VolumePoint: " << time << " " << value << "\n";
                }
                out << "ReverbMixAutomation: " << part.reverbMixAutomation.size() << "\n";
                for (const auto& [time, value] : part.reverbMixAutomation) {
                    out << "ReverbMixPoint: " << time << " " << value << "\n";
                }
            }
            out.close();
            ::SDL_Log("Song saved successfully to %s", filename.c_str());
        }

    private:
        std::mt19937 rng{std::random_device{}()};
        const float sampleRate = 44100.0f;
        const std::vector<float> durations = {
            0.0284091f, 0.0625f, 0.073864f, 0.125f, 0.136364f, 0.147726f, 0.210226f,
            0.25f, 0.272727f, 0.460224f, 0.5f, 0.886364f, 1.0f
        };
        const std::map<std::string, std::vector<float>> scales = {
            {"major", {0, 2, 4, 5, 7, 9, 11}},
            {"minor", {0, 2, 3, 5, 7, 8, 10}},
            {"dorian", {0, 2, 3, 5, 7, 9, 10}},
            {"mixolydian", {0, 2, 4, 5, 7, 9, 10}},
            {"blues", {0, 3, 5, 6, 7, 10}},
            {"pentatonic_minor", {0, 3, 5, 7, 10}},
            {"harmonic_minor", {0, 2, 3, 5, 7, 8, 11}},
            {"whole_tone", {0, 2, 4, 6, 8, 10}},
            {"chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}}
        };

        std::map<Genre, std::vector<std::string>> genreScales = {
            {CLASSICAL, {"major", "minor", "harmonic_minor"}}, {JAZZ, {"dorian", "mixolydian", "blues", "chromatic"}},
            {POP, {"major", "minor"}}, {ROCK, {"major", "minor", "blues"}}, {TECHNO, {"minor", "pentatonic_minor", "whole_tone"}},
            {RAP, {"minor", "pentatonic_minor"}}, {BLUES, {"blues", "pentatonic_minor"}}, {COUNTRY, {"major", "minor"}},
            {FOLK, {"major", "minor", "dorian"}}, {REGGAE, {"minor", "dorian"}}, {METAL, {"minor", "harmonic_minor", "pentatonic_minor"}},
            {PUNK, {"major", "minor"}}, {DISCO, {"major", "minor"}}, {FUNK, {"minor", "pentatonic_minor", "dorian"}},
            {SOUL, {"major", "minor", "blues"}}, {GOSPEL, {"major", "blues"}}, {AMBIENT, {"minor", "dorian", "major", "whole_tone"}},
            {EDM, {"minor", "pentatonic_minor", "major"}}, {LATIN, {"major", "minor", "dorian"}}, {HIPHOP, {"minor", "pentatonic_minor", "blues"}}
        };

        std::map<Genre, std::vector<std::string>> genreInstruments = {
            {CLASSICAL, {"violin", "cello", "flute", "piano", "trumpet", "organ"}},
            {JAZZ, {"piano", "trumpet", "saxophone", "bass", "hihat_closed", "snare", "cymbal"}},
            {POP, {"guitar", "bass", "piano", "kick", "snare", "syntharp", "leadsynth"}},
            {ROCK, {"guitar", "bass", "kick", "snare", "cymbal", "leadsynth"}},
            {TECHNO, {"kick", "hihat_closed", "syntharp", "subbass", "leadsynth", "pad"}},
            {RAP, {"kick", "snare", "hihat_closed", "bass", "vocal_0", "vocal_1"}},
            {BLUES, {"guitar", "bass", "hihat_closed", "snare", "piano", "saxophone"}},
            {COUNTRY, {"guitar", "bass", "kick", "snare", "steelguitar", "violin", "piano"}},
            {FOLK, {"guitar", "violin", "flute", "sitar", "marimba"}},
            {REGGAE, {"bass", "guitar", "kick", "hihat_open", "piano", "organ"}},
            {METAL, {"guitar", "bass", "kick", "snare", "cymbal", "leadsynth"}},
            {PUNK, {"guitar", "bass", "kick", "snare", "cymbal"}},
            {DISCO, {"bass", "guitar", "kick", "hihat_closed", "clap", "syntharp"}},
            {FUNK, {"bass", "guitar", "kick", "snare", "hihat_closed", "saxophone"}},
            {SOUL, {"piano", "bass", "kick", "snare", "guitar", "saxophone", "vocal_0"}},
            {GOSPEL, {"piano", "bass", "kick", "snare", "vocal_0", "vocal_1", "organ"}},
            {AMBIENT, {"pad", "piano", "subbass", "leadsynth", "flute"}},
            {EDM, {"kick", "hihat_closed", "syntharp", "subbass", "leadsynth", "pad"}},
            {LATIN, {"guitar", "bass", "kick", "clap", "marimba", "trumpet"}},
            {HIPHOP, {"kick", "snare", "hihat_closed", "bass", "vocal_0", "syntharp"}}
        };

        std::map<Genre, float> genreBPM = {
            {CLASSICAL, 80.0f}, {JAZZ, 100.0f}, {POP, 120.0f}, {ROCK, 130.0f}, {TECHNO, 140.0f}, {RAP, 90.0f},
            {BLUES, 100.0f}, {COUNTRY, 110.0f}, {FOLK, 100.0f}, {REGGAE, 80.0f}, {METAL, 150.0f}, {PUNK, 160.0f},
            {DISCO, 120.0f}, {FUNK, 110.0f}, {SOUL, 100.0f}, {GOSPEL, 90.0f}, {AMBIENT, 70.0f}, {EDM, 130.0f},
            {LATIN, 110.0f}, {HIPHOP, 95.0f}
        };

        std::map<Genre, std::string> genreNames = {
            {CLASSICAL, "Classical"}, {JAZZ, "Jazz"}, {POP, "Pop"}, {ROCK, "Rock"}, {TECHNO, "Techno"},
            {RAP, "Rap"}, {BLUES, "Blues"}, {COUNTRY, "Country"}, {FOLK, "Folk"}, {REGGAE, "Reggae"},
            {METAL, "Metal"}, {PUNK, "Punk"}, {DISCO, "Disco"}, {FUNK, "Funk"}, {SOUL, "Soul"},
            {GOSPEL, "Gospel"}, {AMBIENT, "Ambient"}, {EDM, "EDM"}, {LATIN, "Latin"}, {HIPHOP, "Hip-Hop"}
        };

        std::map<Genre, std::vector<float>> genreDurationWeights = {
            {CLASSICAL, {0.0f, 0.0f, 0.0f, 0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.15f, 0.2f, 0.2f, 0.17f}},
            {JAZZ, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {POP, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {ROCK, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {TECHNO, {0.2f, 0.25f, 0.25f, 0.15f, 0.1f, 0.05f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},
            {RAP, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {BLUES, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {COUNTRY, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {FOLK, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {REGGAE, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {METAL, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {PUNK, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {DISCO, {0.1f, 0.15f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f, 0.0f}},
            {FUNK, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {SOUL, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {GOSPEL, {0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.12f, 0.15f, 0.12f, 0.1f, 0.08f, 0.05f, 0.1f}},
            {AMBIENT, {0.0f, 0.0f, 0.0f, 0.01f, 0.02f, 0.03f, 0.05f, 0.07f, 0.1f, 0.15f, 0.2f, 0.2f, 0.17f}},
            {EDM, {0.2f, 0.25f, 0.25f, 0.15f, 0.1f, 0.05f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},
            {LATIN, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}},
            {HIPHOP, {0.05f, 0.1f, 0.1f, 0.15f, 0.15f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f, 0.05f, 0.0f, 0.0f}}
        };

        const std::vector<float> availableFreqs = {
            50.0f, 60.0f, 100.0f, 180.0f, 220.0f, 440.0f, 493.88f, 523.25f,
            587.33f, 659.25f, 698.46f, 783.99f, 880.0f, 987.77f, 1046.50f
        };
		
		   const std::vector<float> saxFreqs = {
        	138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 184.99f, 195.99f, 207.65f, 220.00f, 233.08f, 246.94f, 261.63f,
        	277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f, 523.25f,
        	554.37f, 587.33f, 622.25f, 659.26f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f
    	};
		
		const std::vector<float> pianoFreqs = {
        	27.50f, 29.14f, 30.87f, 32.70f, 34.65f, 36.71f, 38.89f, 41.20f, 43.65f, 46.25f, 49.00f, 51.91f,
        	55.00f, 58.27f, 61.74f, 65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, 103.83f,
        	110.00f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f,
        	220.00f, 233.08f, 246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f,
        	440.00f, 466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f, 659.25f, 698.46f, 739.99f, 783.99f, 830.61f,
        	880.00f, 932.33f, 987.77f, 1046.50f, 1108.73f, 1174.66f, 1244.51f, 1318.51f, 1396.91f, 1479.98f, 1567.98f, 1661.22f,
        	1760.00f, 1864.66f, 1975.53f, 2093.00f, 2217.46f, 2349.32f, 2489.02f, 2637.02f, 2793.83f, 2959.96f, 3135.96f, 3322.44f,
        	3520.00f, 3729.31f, 3951.07f, 4186.01f
    	};

        std::vector<Note> melodyMotif;
        std::map<std::string, Part> sectionTemplates;
        std::map<std::string, std::vector<int>> chordProgressions;

    float getClosestFreq(float target, const std::vector<float>& freqPool) {
        if (!std::isfinite(target) || target <= 0.0f) {
            ::SDL_Log("Invalid frequency target %.2f, returning %.2f Hz", target, freqPool[0]);
            return freqPool[0];
        }
        float closest = freqPool[0];
        float minDiff = std::abs(target - closest);
        for (float freq : freqPool) {
            float diff = std::abs(target - freq);
            if (diff < minDiff) {
                minDiff = diff;
                closest = freq;
            }
        }
        return closest;
    }

    // Overload for default frequency pool
    float getClosestFreq(float target) {
        return getClosestFreq(target, availableFreqs);
    }

        float snapToBeatGrid(float time, float bpm) {
            float sixteenthNote = 60.0f / (bpm * 4);
            return std::round(time / sixteenthNote) * sixteenthNote;
        }

        std::string generateTitle() {
            ::SDL_Log("Generating song title");
            std::vector<std::string> adjectives = {"Cosmic", "Epic", "Mystic", "Vibrant", "Ethereal", "Sonic", "Radiant", "Dreamy", "Galactic", "Infinite"};
            std::vector<std::string> nouns = {"Journey", "Wave", "Pulse", "Horizon", "Echo", "Symphony", "Orbit", "Dream", "Tide", "Spark"};
            std::uniform_int_distribution<> adjDist(0, adjectives.size() - 1);
            std::uniform_int_distribution<> nounDist(0, nouns.size() - 1);
            return adjectives[adjDist(rng)] + " " + nouns[nounDist(rng)];
        }

        float getRandomDuration(Genre g, float sectionProgress, float bpm) {
            if (!std::isfinite(bpm) || bpm <= 0.0f) {
                ::SDL_Log("Invalid BPM %.2f, using 120.0", bpm);
                bpm = 120.0f;
            }
            const auto& weights = genreDurationWeights.at(g);
            std::vector<float> adjustedWeights = weights;
            if (sectionProgress > 0.4f && sectionProgress < 0.6f) {
                if (adjustedWeights.size() > 10) adjustedWeights[10] += 0.05f;
                if (adjustedWeights.size() > 12) adjustedWeights[12] += 0.05f;
            }
            std::discrete_distribution<> dist(adjustedWeights.begin(), adjustedWeights.end());
            float duration = durations[dist(rng)];
            float minDur = 60.0f / (bpm * 16);
            return std::max(minDur, duration);
        }

        std::vector<std::vector<int>> getChordProgressions(const std::string& scaleName, Genre g) {
            std::vector<std::vector<int>> progs;
            if (scaleName == "major") progs = {{1, 5, 6, 4}, {1, 4, 5, 1}, {1, 6, 4, 5}, {1, 2, 5, 4}, {1, 3, 6, 4}, {2, 5, 1, 4}};
            else if (scaleName == "minor") progs = {{6, 4, 1, 5}, {6, 3, 4, 5}, {6, 7, 1, 4}, {6, 2, 5, 3}, {6, 1, 4, 7}, {3, 6, 4, 5}};
            else if (scaleName == "dorian") progs = {{2, 7, 1, 4}, {2, 5, 6, 7}, {2, 4, 7, 1}, {2, 1, 4, 5}};
            else if (scaleName == "mixolydian") progs = {{5, 1, 4, 7}, {5, 6, 1, 4}, {5, 3, 6, 7}, {5, 4, 1, 6}};
            else if (scaleName == "blues") progs = {{1, 4, 1, 5}, {1, 5, 4, 1}, {1, 4, 5, 1}, {1, 4, 1, 4}};
            else if (scaleName == "harmonic_minor") progs = {{1, 6, 3, 5}, {1, 4, 6, 7}, {1, 5, 6, 3}, {1, 7, 3, 6}};
            else if (scaleName == "whole_tone") progs = {{1, 3, 5, 1}, {1, 4, 2, 5}};
            else progs = {{1, 4, 5, 4}};

            if (g == JAZZ || g == BLUES) progs.push_back({2, 5, 1, 6});
            else if (g == CLASSICAL) progs.push_back({1, 6, 2, 5});
            else if (g == GOSPEL || g == SOUL) progs.push_back({1, 4, 6, 5});
            else if (g == POP || g == ROCK) progs.push_back({1, 5, 4, 6});
            else if (g == METAL) progs.push_back({1, 7, 4, 5});
            else if (g == LATIN) progs.push_back({1, 4, 2, 5});
            else if (g == AMBIENT) progs.push_back({1, 3, 5, 4});
            return progs;
        }

        std::vector<float> buildChord(int degree, const std::string& scaleName, float rootFreq, Genre g, int inversion = 0) {
            if (!std::isfinite(rootFreq) || rootFreq <= 0.0f) {
                ::SDL_Log("Invalid rootFreq %.2f in buildChord, using 440.0 Hz", rootFreq);
                rootFreq = 440.0f;
            }
            const auto& intervals = scales.at(scaleName);
            rootFreq = getClosestFreq(rootFreq);
            std::vector<float> chord;
            int baseIdx = (degree - 1 + intervals.size()) % intervals.size(); // Ensure non-negative index
            std::vector<int> chordIntervals = {0, 4, 7}; // Basic triad
            for (int offset : chordIntervals) {
                int noteIdx = (baseIdx + offset) % intervals.size();
                float freq = rootFreq * std::pow(2.0f, intervals[noteIdx] / 12.0f);
                chord.push_back(getClosestFreq(freq));
            }
            if (degree == 7 && scaleName == "major") {
                int noteIdx = (baseIdx + 3) % intervals.size();
                float freq = rootFreq * std::pow(2.0f, intervals[noteIdx] / 12.0f);
                chord[1] = getClosestFreq(freq);
            }
            if ((g == JAZZ || g == GOSPEL) && rng() % 3 == 0) {
                int noteIdx = (baseIdx + 11) % intervals.size();
                float freq = rootFreq * std::pow(2.0f, intervals[noteIdx] / 12.0f);
                chord.push_back(getClosestFreq(freq));
            }
            if (g == METAL && degree == 1) {
                chord = {chord[0], chord[2]}; // Power chord for metal
            }
            if (inversion > 0) {
                for (int i = 0; i < inversion && !chord.empty(); ++i) {
                    chord.erase(chord.begin());
                    float nextFreq = chord[0] * 2.0f;
                    if (nextFreq > availableFreqs.back()) nextFreq = availableFreqs.back();
                    chord.push_back(getClosestFreq(nextFreq));
                }
            }
            return chord;
        }

        std::vector<Note> generateMotif(Genre g, const std::string& scaleName, float rootFreq, float bpm) {
            std::vector<Note> motif;
            const auto& intervals = scales.at(scaleName);
            float t = 0.0f;
            float motifDur = 60.0f / bpm;
            int numNotes = (g == JAZZ || g == BLUES) ? 3 : 4;
            float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f));

            for (int i = 0; i < numNotes && t < motifDur; ++i) {
                Note note;
                note.startTime = t;
                note.duration = getRandomDuration(g, 0.5f, bpm) / 2.0f;
                note.freq = currentFreq;
                note.volume = 0.5f;
                note.velocity = 0.8f + 0.1f * (rng() % 100) / 100.0f;
                motif.push_back(note);
                t += note.duration;

                int step = (rng() % 2) ? 1 : -1;
                size_t currentIdx = 0;
                for (size_t j = 0; j < intervals.size(); ++j) {
                    float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                    if (std::abs(currentFreq - freq) < 1e-3) {
                        currentIdx = j;
                        break;
                    }
                }
                currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f));
            }
            return motif;
        }

        Part varyPart(const Part& original, float timeOffset, float intensity = 1.0f, bool transpose = false, float transposeSemitones = 0.0f) {
            Part varied = original;
            varied.notes.clear();
            varied.panAutomation.clear();
            varied.volumeAutomation.clear();
            varied.reverbMixAutomation.clear();

            for (const auto& note : original.notes) {
                Note newNote = note;
                newNote.startTime += timeOffset;
                newNote.volume *= intensity;
                newNote.velocity *= intensity;
                if (transpose) {
                    newNote.freq = getClosestFreq(newNote.freq * std::pow(2.0f, transposeSemitones / 12.0f));
                }
                if (rng() % 3 == 0) {
                    newNote.duration *= (0.9f + 0.2f * (rng() % 100) / 100.0f);
                }
                varied.notes.push_back(newNote);
            }

            for (const auto& [time, value] : original.panAutomation) {
                varied.panAutomation.emplace_back(time + timeOffset, value);
            }
            for (const auto& [time, value] : original.volumeAutomation) {
                varied.volumeAutomation.emplace_back(time + timeOffset, value * intensity);
            }
            for (const auto& [time, value] : original.reverbMixAutomation) {
                varied.reverbMixAutomation.emplace_back(time + timeOffset, value);
            }

            return varied;
        }

        Part generateMelody(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
            Part melody;
            melody.instrument = (g == ROCK) ? "guitar" : genreInstruments[g][rng() % genreInstruments[g].size()];
            melody.pan = (rng() % 2) ? 0.3f : -0.3f;
            melody.reverbMix = (g == AMBIENT || g == CLASSICAL) ? 0.4f : (g == JAZZ || g == SOUL) ? 0.3f : 0.2f;
            melody.sectionName = "Melody";
            melody.useReverb = (g == AMBIENT || g == CLASSICAL || g == JAZZ || g == ROCK || rng() % 2);
            melody.reverbDelay = 0.05f;
            melody.reverbDecay = 0.4f;
            melody.reverbMixFactor = melody.reverbMix;
            melody.useDistortion = (g == ROCK || g == METAL || g == PUNK || rng() % 3 == 0);
            melody.distortionDrive = 1.5f;
            melody.distortionThreshold = 0.7f;

            const float restProb = (g == CLASSICAL || g == AMBIENT) ? 0.35f : 0.3f;
            const float ornamentProb = (g == CLASSICAL || g == JAZZ) ? 0.1f : 0.05f;
            const float motifProb = (g == CLASSICAL || g == POP || g == ROCK) ? 0.3f : 0.2f;
            melody.notes.reserve(500);
            melody.panAutomation.reserve(36);
            melody.volumeAutomation.reserve(36);
            melody.reverbMixAutomation.reserve(36);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                float baseVol = (section.templateName == "Chorus") ? 0.5f : 0.4f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, melody.pan + (rng() % 20 - 10) / 100.0f));
                    float vol = std::max(baseVol, std::min(1.0f, baseVol + (rng() % 20) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, melody.reverbMix + (rng() % 10) / 100.0f));
                    melody.panAutomation.emplace_back(t, pan);
                    melody.volumeAutomation.emplace_back(t, vol);
                    melody.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            const auto& intervals = scales.at(scaleName);
            float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f));
            std::vector<float> stepProbs = (g == POP || g == ROCK || g == COUNTRY || g == SOUL || g == GOSPEL) ?
                                           std::vector<float>{0.5f, 0.3f, 0.15f, 0.05f} :
                                           (g == JAZZ || g == BLUES) ? std::vector<float>{0.4f, 0.3f, 0.2f, 0.1f} :
                                           (g == CLASSICAL) ? std::vector<float>{0.3f, 0.3f, 0.3f, 0.1f} :
                                           std::vector<float>{0.5f, 0.3f, 0.15f, 0.05f};
            std::discrete_distribution<> stepDist(stepProbs.begin(), stepProbs.end());
            float chromaticProb = (g == ROCK) ? 0.05f : (g == JAZZ || g == BLUES) ? 0.25f : (g == CLASSICAL) ? 0.15f : (g == GOSPEL || g == SOUL) ? 0.2f : 0.05f;
            float arpeggioProb = (g == ROCK) ? 0.1f : (g == CLASSICAL || g == JAZZ || g == EDM) ? 0.3f : 0.1f;

            melodyMotif = generateMotif(g, scaleName, rootFreq, bpm);

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Melody") != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.1f : 1.0f;
                    bool transpose = (section.name == "Verse2" && rng() % 2);
                    float transposeSemitones = transpose ? 2.0f : 0.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Melody"], section.startTime, intensity, transpose, transposeSemitones);
                    melody.notes.insert(melody.notes.end(), varied.notes.begin(), varied.notes.end());
                    melody.panAutomation.insert(melody.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    melody.volumeAutomation.insert(melody.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    melody.reverbMixAutomation.insert(melody.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused melody template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                float sectionDur = sectionEnd - t;
                size_t maxNotes = static_cast<size_t>(sectionDur * (g == ROCK || g == EDM ? 4.0f : 3.0f));
                size_t sectionNoteCount = 0;
                float phraseDur = 4.0f * (60.0f / bpm);
                float phraseStart = t;

                while (t < sectionEnd && sectionNoteCount < maxNotes) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting melody generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    if (rng() / static_cast<float>(rng.max()) < restProb) {
                        t += getRandomDuration(g, section.progress, bpm);
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    bool useMotif = (rng() / static_cast<float>(rng.max()) < motifProb) && (t + 60.0f / bpm <= sectionEnd);
                    if (useMotif) {
                        for (const auto& motifNote : melodyMotif) {
                            if (sectionNoteCount >= maxNotes) break;
                            Note note = motifNote;
                            note.startTime = snapToBeatGrid(t + motifNote.startTime, bpm);
                            note.duration = std::min(motifNote.duration, sectionEnd - note.startTime);
                            note.volume = 0.4f + 0.1f * section.progress;
                            note.velocity = 0.8f + 0.2f * (rng() % 100) / 100.0f;
                            note.phoneme = melody.instrument.find("vocal") != std::string::npos ? rng() % 7 : -1;
                            note.open = melody.instrument.find("hihat") != std::string::npos ? (rng() % 2) : false;
                            melody.notes.push_back(note);
                            sectionNoteCount++;
                        }
                        t += 60.0f / bpm;
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                        note.duration = 0.0625f;
                    }
                    note.volume = 0.4f + 0.1f * section.progress;
                    note.velocity = 0.8f + 0.2f * (rng() % 100) / 100.0f;

                    if (rng() / static_cast<float>(rng.max()) < ornamentProb && note.duration > 0.125f) {
                        Note graceNote = note;
                        graceNote.duration = note.duration * 0.25f;
                        graceNote.startTime = note.startTime - graceNote.duration;
                        size_t currentIdx = 0;
                        for (size_t j = 0; j < intervals.size(); ++j) {
                            float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                            if (std::abs(currentFreq - freq) < 1e-3) {
                                currentIdx = j;
                                break;
                            }
                        }
                        currentIdx = (currentIdx + 1) % intervals.size();
                        graceNote.freq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f));
                        graceNote.volume *= 0.7f;
                        if (std::isfinite(graceNote.freq) && graceNote.startTime >= section.startTime) {
                            melody.notes.push_back(graceNote);
                            sectionNoteCount++;
                        }
                    }

                    if (static_cast<int>(rng() % 100) < static_cast<int>(arpeggioProb * 100)) {
                        auto chord = buildChord((sectionIdx % 4) + 1, scaleName, rootFreq, g, rng() % 2);
                        if (chord.empty() || !std::all_of(chord.begin(), chord.end(), [](float f) { return std::isfinite(f); })) {
                            ::SDL_Log("Invalid chord frequencies in melody, using 440.0 Hz");
                            note.freq = 440.0f;
                            invalidFreqCount++;
                        } else {
                            note.freq = chord[rng() % chord.size()];
                        }
                    } else if (static_cast<int>(rng() % 100) < static_cast<int>(chromaticProb * 100)) {
                        size_t currentIdx = 0;
                        for (size_t j = 0; j < availableFreqs.size(); ++j) {
                            if (std::abs(currentFreq - availableFreqs[j]) < 1e-3) {
                                currentIdx = j;
                                break;
                            }
                        }
                        int dir = (rng() % 2) ? 1 : -1;
                        currentIdx = (currentIdx + dir + availableFreqs.size()) % availableFreqs.size();
                        note.freq = availableFreqs[currentIdx];
                        if (!std::isfinite(note.freq)) {
                            note.freq = 440.0f;
                            invalidFreqCount++;
                        }
                    } else {
                        int step = stepDist(rng);
                        int dir = (rng() % 2) ? 1 : -1;
                        size_t currentIdx = 0;
                        for (size_t j = 0; j < intervals.size(); ++j) {
                            float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                            if (std::abs(currentFreq - freq) < 1e-3) {
                                currentIdx = j;
                                break;
                            }
                        }
                        currentIdx = (currentIdx + dir * (step + 1)) % intervals.size();
                        currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f));
                        note.freq = currentFreq;
                        if (!std::isfinite(note.freq)) {
                            note.freq = 440.0f;
                            invalidFreqCount++;
                        }
                    }

                    note.phoneme = melody.instrument.find("vocal") != std::string::npos ? rng() % 7 : -1;
                    note.open = melody.instrument.find("hihat") != std::string::npos ? (rng() % 2) : false;
                    melody.notes.push_back(note);
                    sectionNoteCount++;
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);

                    if (t >= phraseStart + phraseDur) {
                        if (rng() % 2) {
                            Note endNote = note;
                            endNote.startTime = snapToBeatGrid(t, bpm);
                            endNote.duration = 60.0f / bpm;
                            endNote.volume *= 0.9f;
                            if (endNote.startTime + endNote.duration <= sectionEnd) {
                                melody.notes.push_back(endNote);
                                sectionNoteCount++;
                            }
                            t += endNote.duration;
                        }
                        phraseStart = t;
                    }
                }
                ::SDL_Log("Generated %zu notes for melody in section %s", sectionNoteCount, section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = melody;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : melody.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : melody.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : melody.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : melody.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Melody"] = templatePart;
                    ::SDL_Log("Stored melody template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated melody with total %zu notes, %zu invalid frequencies encountered", melody.notes.size(), invalidFreqCount);
            return melody;
        }

        Part generateRhythm(Genre g, float totalDur, float beat, float bpm, const std::string& instrument, const std::vector<Section>& sections) {
            Part rhythm;
            rhythm.instrument = instrument;
            rhythm.pan = (g == ROCK && instrument == "snare") ? 0.2f : 0.0f;
            rhythm.reverbMix = (g == ROCK) ? 0.15f : 0.3f;
            rhythm.sectionName = "Rhythm";
            rhythm.useReverb = (g == ROCK || rng() % 2);
            rhythm.reverbDelay = 0.1f;
            rhythm.reverbDecay = 0.5f;
            rhythm.reverbMixFactor = rhythm.reverbMix;
            rhythm.useDistortion = (g == ROCK && instrument == "kick") || rng() % 4 == 0;
            rhythm.distortionDrive = 1.2f;
            rhythm.distortionThreshold = 0.9f;

            const size_t maxNotesPerSection = 100;
            rhythm.notes.reserve(maxNotesPerSection * sections.size());
            rhythm.panAutomation.reserve(36);
            rhythm.volumeAutomation.reserve(36);
            rhythm.reverbMixAutomation.reserve(36);

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, rhythm.pan + (rng() % 10 - 5) / 100.0f));
                    float vol = std::max(0.5f, std::min(1.0f, 0.5f + (rng() % 10) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, rhythm.reverbMix + (rng() % 5) / 100.0f));
                    rhythm.panAutomation.emplace_back(t, pan);
                    rhythm.volumeAutomation.emplace_back(t, vol);
                    rhythm.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            std::vector<float> pattern;
            float syncopationProb = (g == JAZZ || g == FUNK || g == LATIN || g == REGGAE) ? 0.4f : 0.2f;
            if (g == ROCK && instrument == "kick") pattern = {0.0f, 2 * beat};
            else if (g == ROCK && instrument == "snare") pattern = {beat, 3 * beat};
            else if (g == ROCK && instrument == "cymbal") pattern = {0.0f, beat, 2 * beat, 3 * beat};
            else if (g == JAZZ || g == BLUES) pattern = {0.0f, 2 * beat / 3, beat, 5 * beat / 3};
            else if (g == REGGAE) pattern = {beat / 2, 3 * beat / 2};
            else if (g == LATIN) pattern = {0.0f, beat / 2, beat, 1.25f * beat, 1.75f * beat};
            else if (g == FUNK || g == DISCO) pattern = {0.0f, beat / 4, 3 * beat / 4, beat, 3 * beat / 2};
            else if (g == GOSPEL) pattern = {0.0f, beat / 2, beat, 1.25f * beat, 1.5f * beat};
            else pattern = {0.0f, beat / 2, beat, 3 * beat / 2};

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Rhythm_" + instrument) != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.1f : 1.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Rhythm_" + instrument], section.startTime, intensity);
                    rhythm.notes.insert(rhythm.notes.end(), varied.notes.begin(), varied.notes.end());
                    rhythm.panAutomation.insert(rhythm.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    rhythm.volumeAutomation.insert(rhythm.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    rhythm.reverbMixAutomation.insert(rhythm.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused rhythm template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                size_t sectionNoteCount = 0;

                while (t < sectionEnd && sectionNoteCount < maxNotesPerSection) {
                    for (float offset : pattern) {
                        if (t + offset >= sectionEnd) break;
                        if (sectionNoteCount >= maxNotesPerSection) break;
                        Note note;
                        note.startTime = snapToBeatGrid(t + offset, bpm);
                        note.duration = getRandomDuration(g, section.progress, bpm);
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                            note.duration = 0.0625f;
                        }
                        note.freq = (instrument == "kick") ? 50.0f : (instrument == "snare") ? 180.0f : (instrument == "cymbal") ? 220.0f : 220.0f;
                        note.volume = 0.5f;
                        note.velocity = (offset == 0.0f || offset == beat) ? 0.9f : 0.7f + 0.2f * (rng() % 100) / 100.0f;
                        note.open = instrument.find("hihat") != std::string::npos ? (offset > beat / 2) : false;
                        note.phoneme = -1;
                        rhythm.notes.push_back(note);
                        sectionNoteCount++;

                        if (rng() / static_cast<float>(rng.max()) < syncopationProb && offset < 3 * beat / 2) {
                            Note syncNote = note;
                            syncNote.startTime = snapToBeatGrid(t + offset + beat / 4, bpm);
                            syncNote.velocity *= 0.8f;
                            if (syncNote.startTime < sectionEnd) {
                                rhythm.notes.push_back(syncNote);
                                sectionNoteCount++;
                            }
                        }
                    }
                    t += beat * 4.0f;
                    t = snapToBeatGrid(t, bpm);
                }
                ::SDL_Log("Generated %zu notes for rhythm (%s) in section %s", sectionNoteCount, instrument.c_str(), section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = rhythm;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : rhythm.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : rhythm.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : rhythm.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : rhythm.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Rhythm_" + instrument] = templatePart;
                    ::SDL_Log("Stored rhythm template %s_%s with %zu notes", templateName.c_str(), instrument.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated rhythm with total %zu notes for instrument %s", rhythm.notes.size(), instrument.c_str());
            return rhythm;
        }

    Part generateSaxophone(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
        ::SDL_Log("Generating saxophone for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
        Part saxophone;
        saxophone.instrument = "saxophone";
        saxophone.pan = (rng() % 2) ? 0.2f : -0.2f; // Slight panning for stereo presence
        saxophone.reverbMix = (g == JAZZ || g == BLUES || g == SOUL) ? 0.3f : (g == AMBIENT) ? 0.4f : 0.25f;
        saxophone.sectionName = "Saxophone";
        saxophone.useReverb = (g == JAZZ || g == BLUES || g == SOUL || g == AMBIENT || rng() % 2);
        saxophone.reverbDelay = 0.12f;
        saxophone.reverbDecay = 0.6f;
        saxophone.reverbMixFactor = saxophone.reverbMix;
        saxophone.useDistortion = (g == FUNK || g == ROCK || rng() % 4 == 0); // Subtle distortion for edge in some genres
        saxophone.distortionDrive = 1.4f;
        saxophone.distortionThreshold = 0.75f;

        const float restProb = (g == JAZZ || g == BLUES) ? 0.45f : (g == FUNK || g == SOUL) ? 0.35f : 0.3f;
        const float legatoProb = (g == JAZZ || g == BLUES || g == SOUL) ? 0.6f : 0.3f; // Legato for smooth phrasing
        const float stabProb = (g == FUNK || g == LATIN) ? 0.5f : 0.2f; // Short, rhythmic stabs
        const float improvProb = (g == JAZZ || g == BLUES) ? 0.4f : 0.1f; // Improvisational flourishes
        saxophone.notes.reserve(400); // Fewer notes than guitar due to breath constraints
        saxophone.panAutomation.reserve(36);
        saxophone.volumeAutomation.reserve(36);
        saxophone.reverbMixAutomation.reserve(36);

        size_t invalidFreqCount = 0;
        const size_t maxInvalidFreqs = 100;

        // Automation for dynamic transitions
        for (const auto& section : sections) {
            float t = section.startTime;
            float end = section.endTime;
            float step = (end - t) / 4.0f;
            for (int i = 0; i < 4 && t < end; ++i) {
                float pan = std::max(-1.0f, std::min(1.0f, saxophone.pan + (rng() % 10 - 5) / 100.0f));
                float vol = std::max(0.45f, std::min(1.0f, 0.45f + (rng() % 15) / 100.0f));
                float rev = std::max(0.0f, std::min(1.0f, saxophone.reverbMix + (rng() % 10) / 100.0f));
                saxophone.panAutomation.emplace_back(t, pan);
                saxophone.volumeAutomation.emplace_back(t, vol);
                saxophone.reverbMixAutomation.emplace_back(t, rev);
                t += step;
            }
        }

        const auto& intervals = scales.at(scaleName);
        float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f), saxFreqs);

        for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
            const auto& section = sections[sectionIdx];
            std::string templateName = section.templateName;

            // Reuse templates for Verse/Chorus
            if (sectionTemplates.find(templateName + "_Saxophone") != sectionTemplates.end() &&
                (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo")) {
                float intensity = (section.name == "Chorus2" || section.name == "Solo" || section.name == "Verse2") ? 1.15f : 1.0f;
                bool transpose = (section.name == "Chorus2" && rng() % 3 == 0);
                float transposeSemitones = transpose ? 2.0f : 0.0f;
                Part varied = varyPart(sectionTemplates[templateName + "_Saxophone"], section.startTime, intensity, transpose, transposeSemitones);
                saxophone.notes.insert(saxophone.notes.end(), varied.notes.begin(), varied.notes.end());
                saxophone.panAutomation.insert(saxophone.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                saxophone.volumeAutomation.insert(saxophone.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                saxophone.reverbMixAutomation.insert(saxophone.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                ::SDL_Log("Reused saxophone template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                continue;
            }

            float t = section.startTime;
            float sectionEnd = section.endTime;
            float sectionDur = sectionEnd - t;
            size_t maxNotes = static_cast<size_t>(sectionDur * (g == JAZZ || g == BLUES || g == FUNK ? 3.5f : 2.5f));
            size_t sectionNoteCount = 0;
            float phraseDur = 4.0f * (60.0f / bpm); // Typical phrase length
            float phraseStart = t;

            // Get chord progression
            std::vector<int> prog;
            if (chordProgressions.find(templateName) != chordProgressions.end()) {
                prog = chordProgressions[templateName];
            } else {
                auto progs = getChordProgressions(scaleName, g);
                prog = progs[rng() % progs.size()];
                if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                    chordProgressions[templateName] = prog;
                }
            }
            size_t chordIdx = 0;

            while (t < sectionEnd && sectionNoteCount < maxNotes) {
                if (invalidFreqCount >= maxInvalidFreqs) {
                    ::SDL_Log("Aborting saxophone generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                    break;
                }
                if (rng() / static_cast<float>(rng.max()) < restProb) {
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    continue;
                }

                bool useLegato = rng() / static_cast<float>(rng.max()) < legatoProb;
                bool useStab = !useLegato && rng() / static_cast<float>(rng.max()) < stabProb;
                bool useImprov = !useLegato && !useStab && rng() / static_cast<float>(rng.max()) < improvProb;

                if (useStab) {
                    // Short, rhythmic stabs (e.g., for FUNK or LATIN)
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = 60.0f / (bpm * 4); // Sixteenth-note stabs
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    float targetFreq = chord[rng() % chord.size()];
                    // Ensure frequency is in saxophone range
                    while (targetFreq > 880.0f) targetFreq /= 2.0f;
                    while (targetFreq < 138.59f) targetFreq *= 2.0f;
                    note.freq = getClosestFreq(targetFreq, saxFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid saxophone frequency at t=%.2f, using 138.59 Hz", t);
                        note.freq = 138.59f; // Default to low B♭
                        invalidFreqCount++;
                    }
                    note.volume = 0.5f + 0.1f * section.progress;
                    note.velocity = 0.9f; // Strong attack for stabs
                    note.phoneme = -1;
                    note.open = false;
                    saxophone.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;
                    chordIdx++;
                } else if (useImprov && (g == JAZZ || g == BLUES)) {
                    // Improvisational flourish: short run of notes
                    int numNotes = 3 + (rng() % 3); // 3–5 notes
                    float runDur = 60.0f / (bpm * 2); // Half-note run
                    float noteDur = runDur / numNotes;
                    size_t currentIdx = 0;
                    for (size_t j = 0; j < intervals.size(); ++j) {
                        float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                        if (std::abs(currentFreq - freq) < 1e-3) {
                            currentIdx = j;
                            break;
                        }
                    }

                    for (int i = 0; i < numNotes && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = noteDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                        int step = (rng() % 2) ? 1 : -1;
                        currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                        float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                        // Ensure frequency is in saxophone range
                        while (targetFreq > 880.0f) targetFreq /= 2.0f;
                        while (targetFreq < 138.59f) targetFreq *= 2.0f;
                        note.freq = getClosestFreq(targetFreq, saxFreqs);
                        currentFreq = note.freq;

                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid saxophone frequency at t=%.2f, using 138.59 Hz", t);
                            note.freq = 138.59f;
                            invalidFreqCount++;
                        }
                        note.volume = 0.45f + 0.1f * section.progress;
                        note.velocity = 0.7f + 0.2f * (rng() % 100) / 100.0f;
                        note.phoneme = -1;
                        note.open = false;
                        saxophone.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                } else {
                    // Melodic line, potentially legato
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = useLegato ? getRandomDuration(g, section.progress, bpm) * 1.5f : getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                    size_t currentIdx = 0;
                    for (size_t j = 0; j < intervals.size(); ++j) {
                        float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                        if (std::abs(currentFreq - freq) < 1e-3) {
                            currentIdx = j;
                            break;
                        }
                    }
                    int step = (rng() % 3) - 1; // -1, 0, or 1 for smooth melodic movement
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                    // Ensure frequency is in saxophone range
                    while (targetFreq > 880.0f) targetFreq /= 2.0f;
                    while (targetFreq < 138.59f) targetFreq *= 2.0f;
                    currentFreq = note.freq = getClosestFreq(targetFreq, saxFreqs);

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid saxophone frequency at t=%.2f, using 138.59 Hz", t);
                        note.freq = 138.59f;
                        invalidFreqCount++;
                    }
                    note.volume = 0.45f + 0.1f * section.progress;
                    note.velocity = useLegato ? 0.7f + 0.15f * (rng() % 100) / 100.0f : 0.85f + 0.15f * (rng() % 100) / 100.0f;
                    note.phoneme = -1;
                    note.open = false;
                    saxophone.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;

                    // Add breath articulation (simulated as a softer grace note)
                    if (useLegato && rng() % 3 == 0 && note.duration > 0.125f) {
                        Note graceNote = note;
                        graceNote.duration = note.duration * 0.2f;
                        graceNote.startTime = note.startTime - graceNote.duration;
                        graceNote.volume *= 0.6f;
                        graceNote.velocity *= 0.8f;
                        if (graceNote.startTime >= section.startTime) {
                            saxophone.notes.push_back(graceNote);
                            sectionNoteCount++;
                        }
                    }
                }

                if (t >= phraseStart + phraseDur) {
                    phraseStart = t;
                    if (rng() % 2) {
                        t += 60.0f / bpm; // Occasional breath pause
                        t = snapToBeatGrid(t, bpm);
                    }
                    chordIdx++;
                }
            }
            ::SDL_Log("Generated %zu notes for saxophone in section %s", sectionNoteCount, section.name.c_str());

            // Store template for Verse/Chorus/Solo
            if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                Part templatePart = saxophone;
                templatePart.notes.clear();
                templatePart.panAutomation.clear();
                templatePart.volumeAutomation.clear();
                templatePart.reverbMixAutomation.clear();
                for (const auto& note : saxophone.notes) {
                    if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                        Note templateNote = note;
                        templateNote.startTime -= section.startTime;
                        templatePart.notes.push_back(templateNote);
                    }
                }
                for (const auto& [time, value] : saxophone.panAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.panAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : saxophone.volumeAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : saxophone.reverbMixAutomation) {
                    if (time >= section.startTime && time < section.endTime) {
                        templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                sectionTemplates[templateName + "_Saxophone"] = templatePart;
                ::SDL_Log("Stored saxophone template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
            }
        }
        ::SDL_Log("Generated saxophone with total %zu notes, %zu invalid frequencies encountered", saxophone.notes.size(), invalidFreqCount);
        return saxophone;
    }

    Part generatePiano(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
        ::SDL_Log("Generating piano for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
        Part piano;
        piano.instrument = "piano";
        piano.pan = 0.0f; // Centered for acoustic piano realism
        piano.reverbMix = (g == CLASSICAL || g == AMBIENT) ? 0.4f : (g == JAZZ || g == BLUES) ? 0.3f : 0.25f;
        piano.sectionName = "Piano";
        piano.useReverb = (g == CLASSICAL || g == JAZZ || g == BLUES || g == AMBIENT || rng() % 2);
        piano.reverbDelay = 0.15f; // Slightly longer for concert hall effect
        piano.reverbDecay = 0.7f;
        piano.reverbMixFactor = piano.reverbMix;
        piano.useDistortion = false; // Rarely used for acoustic piano
        piano.distortionDrive = 0.0f;
        piano.distortionThreshold = 0.0f;

        const float restProb = (g == CLASSICAL || g == AMBIENT) ? 0.4f : (g == JAZZ || g == BLUES) ? 0.35f : 0.3f;
        const float chordProb = (g == CLASSICAL || g == JAZZ || g == POP) ? 0.5f : (g == BLUES || g == ROCK) ? 0.4f : 0.3f;
        const float arpeggioProb = (g == CLASSICAL || g == AMBIENT) ? 0.4f : (g == JAZZ) ? 0.3f : 0.2f;
        const float pedalProb = (g == CLASSICAL || g == AMBIENT || g == JAZZ) ? 0.7f : 0.4f; // Sustain pedal simulation
        piano.notes.reserve(600); // More notes due to chords and arpeggios
        piano.panAutomation.reserve(36);
        piano.volumeAutomation.reserve(36);
        piano.reverbMixAutomation.reserve(36);

        size_t invalidFreqCount = 0;
        const size_t maxInvalidFreqs = 100;

        // Automation for dynamic transitions
        for (const auto& section : sections) {
            float t = section.startTime;
            float end = section.endTime;
            float step = (end - t) / 4.0f;
            for (int i = 0; i < 4 && t < end; ++i) {
                float pan = std::max(-0.1f, std::min(0.1f, piano.pan + (rng() % 5 - 2) / 100.0f)); // Subtle pan for realism
                float vol = std::max(0.4f, std::min(1.0f, 0.4f + (rng() % 20) / 100.0f)); // Wider dynamic range
                float rev = std::max(0.0f, std::min(1.0f, piano.reverbMix + (rng() % 10) / 100.0f));
                piano.panAutomation.emplace_back(t, pan);
                piano.volumeAutomation.emplace_back(t, vol);
                piano.reverbMixAutomation.emplace_back(t, rev);
                t += step;
            }
        }

        const auto& intervals = scales.at(scaleName);
        float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f), pianoFreqs);

        for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
            const auto& section = sections[sectionIdx];
            std::string templateName = section.templateName;

            // Reuse templates for Verse/Chorus
            if (sectionTemplates.find(templateName + "_Piano") != sectionTemplates.end() &&
                (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo")) {
                float intensity = (section.name == "Chorus2" || section.name == "Solo" || section.name == "Verse2") ? 1.2f : 1.0f;
                bool transpose = (section.name == "Chorus2" && rng() % 3 == 0);
                float transposeSemitones = transpose ? 2.0f : 0.0f;
                Part varied = varyPart(sectionTemplates[templateName + "_Piano"], section.startTime, intensity, transpose, transposeSemitones);
                piano.notes.insert(piano.notes.end(), varied.notes.begin(), varied.notes.end());
                piano.panAutomation.insert(piano.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                piano.volumeAutomation.insert(piano.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                piano.reverbMixAutomation.insert(piano.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                ::SDL_Log("Reused piano template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                continue;
            }

            float t = section.startTime;
            float sectionEnd = section.endTime;
            float sectionDur = sectionEnd - t;
            size_t maxNotes = static_cast<size_t>(sectionDur * (g == CLASSICAL || g == JAZZ ? 5.0f : 3.5f));
            size_t sectionNoteCount = 0;
            float measureDur = 4.0f * (60.0f / bpm); // One measure
            float measureStart = t;

            // Get chord progression
            std::vector<int> prog;
            if (chordProgressions.find(templateName) != chordProgressions.end()) {
                prog = chordProgressions[templateName];
            } else {
                auto progs = getChordProgressions(scaleName, g);
                prog = progs[rng() % progs.size()];
                if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                    chordProgressions[templateName] = prog;
                }
            }
            size_t chordIdx = 0;

            while (t < sectionEnd && sectionNoteCount < maxNotes) {
                if (invalidFreqCount >= maxInvalidFreqs) {
                    ::SDL_Log("Aborting piano generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                    break;
                }
                if (rng() / static_cast<float>(rng.max()) < restProb) {
                    t += getRandomDuration(g, section.progress, bpm);
                    t = snapToBeatGrid(t, bpm);
                    continue;
                }

                bool useChord = rng() / static_cast<float>(rng.max()) < chordProb;
                bool useArpeggio = !useChord && rng() / static_cast<float>(rng.max()) < arpeggioProb;
                bool usePedal = rng() / static_cast<float>(rng.max()) < pedalProb;

                if (useChord) {
                    // Play a chord (root position or inversion, rootless for JAZZ)
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, rng() % 3); // Random inversion
                    if (g == JAZZ && rng() % 2) {
                        // Rootless voicing: remove root, add 7th or 9th
                        if (!chord.empty()) chord.erase(chord.begin());
                        float seventh = rootFreq * std::pow(2.0f, (intervals[(prog[chordIdx % prog.size()] + 6) % intervals.size()] + 12) / 12.0f);
                        while (seventh > 1046.50f) seventh /= 2.0f; // Keep in middle register
                        if (std::find(chord.begin(), chord.end(), seventh) == chord.end()) chord.push_back(seventh);
                    }
                    float chordDur = getRandomDuration(g, section.progress, bpm) * (usePedal ? 2.0f : 1.0f);
                    if (t + chordDur > sectionEnd) chordDur = sectionEnd - t;
                    if (!std::isfinite(chordDur) || chordDur <= 0.0f) chordDur = 0.0625f;

                    for (float freq : chord) {
                        // Ensure frequency is in piano range
                        while (freq > 4186.01f) freq /= 2.0f;
                        while (freq < 27.5f) freq *= 2.0f;
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = chordDur;
                        note.freq = getClosestFreq(freq, pianoFreqs);
                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid piano frequency at t=%.2f, using 261.63 Hz", t);
                            note.freq = 261.63f; // Default to middle C
                            invalidFreqCount++;
                        }
                        note.volume = 0.4f + 0.15f * section.progress;
                        note.velocity = 0.6f + 0.3f * (rng() % 100) / 100.0f; // Varied velocity for human feel
                        note.phoneme = -1;
                        note.open = usePedal;
                        piano.notes.push_back(note);
                        sectionNoteCount++;
                    }
                    t += chordDur;
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else if (useArpeggio) {
                    // Arpeggiated chord
                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                    int numNotes = 4 + (rng() % 3); // 4–6 notes
                    float arpDur = 60.0f / (bpm * 2); // Half-note arpeggio
                    float noteDur = arpDur / numNotes;
                    for (int i = 0; i < numNotes && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = noteDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                        float targetFreq = chord[i % chord.size()];
                        while (targetFreq > 1046.50f) targetFreq /= 2.0f; // Keep in middle register
                        while (targetFreq < 27.5f) targetFreq *= 2.0f;
                        note.freq = getClosestFreq(targetFreq, pianoFreqs);
                        if (!std::isfinite(note.freq)) {
                            ::SDL_Log("Invalid piano frequency at t=%.2f, using 261.63 Hz", t);
                            note.freq = 261.63f;
                            invalidFreqCount++;
                        }
                        note.volume = 0.35f + 0.1f * section.progress;
                        note.velocity = 0.5f + 0.3f * (rng() % 100) / 100.0f;
                        note.phoneme = -1;
                        note.open = usePedal;
                        piano.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                    chordIdx++;
                } else {
                    // Melodic line
                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = getRandomDuration(g, section.progress, bpm);
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                    size_t currentIdx = 0;
                    for (size_t j = 0; j < intervals.size(); ++j) {
                        float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                        if (std::abs(currentFreq - freq) < 1e-3) {
                            currentIdx = j;
                            break;
                        }
                    }
                    int step = (rng() % 5) - 2; // -2 to 2 for varied melodic movement
                    currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                    float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                    while (targetFreq > 2093.00f) targetFreq /= 2.0f; // Keep melody in upper-middle register
                    while (targetFreq < 130.81f) targetFreq *= 2.0f;
                    note.freq = getClosestFreq(targetFreq, pianoFreqs);
                    currentFreq = note.freq;

                    if (!std::isfinite(note.freq)) {
                        ::SDL_Log("Invalid piano frequency at t=%.2f, using 261.63 Hz", t);
                        note.freq = 261.63f;
                        invalidFreqCount++;
                    }
                    note.volume = 0.4f + 0.1f * section.progress;
                    note.velocity = 0.6f + 0.3f * (rng() % 100) / 100.0f; // Expressive velocity
                    note.phoneme = -1;
                    note.open = usePedal && note.duration > 0.125f;
                    piano.notes.push_back(note);
                    t += note.duration;
                    t = snapToBeatGrid(t, bpm);
                    sectionNoteCount++;

                    // Add occasional two-note harmony (e.g., thirds or sixths)
                    if (rng() % 3 == 0 && note.duration > 0.125f) {
                        Note harmonyNote = note;
                        int harmonyStep = (g == CLASSICAL || g == POP) ? 2 : 4; // Third or sixth
                        size_t harmonyIdx = (currentIdx + harmonyStep) % intervals.size();
                        float harmonyFreq = rootFreq * std::pow(2.0f, intervals[harmonyIdx] / 12.0f);
                        while (harmonyFreq > 2093.00f) harmonyFreq /= 2.0f;
                        while (harmonyFreq < 130.81f) harmonyFreq *= 2.0f;
                        harmonyNote.freq = getClosestFreq(harmonyFreq, pianoFreqs);
                        if (!std::isfinite(harmonyNote.freq)) {
                            ::SDL_Log("Invalid piano harmony frequency at t=%.2f, skipping", t);
                            invalidFreqCount++;
                            continue;
                        }
                        harmonyNote.volume *= 0.8f; // Softer harmony
                        piano.notes.push_back(harmonyNote);
                        sectionNoteCount++;
                    }
                }

                if (t >= measureStart + measureDur) {
                    measureStart = t;
                    chordIdx++;
                    if (rng() % 4 == 0) {
                        t += 60.0f / (bpm * 2); // Occasional pause for phrasing
                        t = snapToBeatGrid(t, bpm);
                    }
                }
            }
            ::SDL_Log("Generated %zu notes for piano in section %s", sectionNoteCount, section.name.c_str());

            // Store template for Verse/Chorus/Solo
            if (templateName == "Verse" || templateName == "Chorus" || templateName == "Solo") {
                Part templatePart = piano;
                templatePart.notes.clear();
                templatePart.panAutomation.clear();
                templatePart.volumeAutomation.clear();
                templatePart.reverbMixAutomation.clear();
                for (const auto& note : piano.notes) {
                    if (note.startTime >= section.startTime && note.startTime < sectionEnd) {
                        Note templateNote = note;
                        templateNote.startTime -= section.startTime;
                        templatePart.notes.push_back(templateNote);
                    }
                }
                for (const auto& [time, value] : piano.panAutomation) {
                    if (time >= section.startTime && time < sectionEnd) {
                        templatePart.panAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : piano.volumeAutomation) {
                    if (time >= section.startTime && time < sectionEnd) {
                        templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                for (const auto& [time, value] : piano.reverbMixAutomation) {
                    if (time >= section.startTime && time < sectionEnd) {
                        templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                    }
                }
                sectionTemplates[templateName + "_Piano"] = templatePart;
                ::SDL_Log("Stored piano template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
            }
        }
        ::SDL_Log("Generated piano with total %zu notes, %zu invalid frequencies encountered", piano.notes.size(), invalidFreqCount);
        return piano;
    }

Part generateGuitar(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
    ::SDL_Log("Generating guitar for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
    Part guitar;
    guitar.instrument = "guitar";
    guitar.pan = (rng() % 2) ? 0.3f : -0.3f; // Slight panning for stereo width
    guitar.reverbMix = (g == AMBIENT || g == CLASSICAL) ? 0.35f : (g == ROCK || g == BLUES) ? 0.25f : 0.2f;
    guitar.sectionName = "Guitar";
    guitar.useReverb = (g == AMBIENT || g == CLASSICAL || g == ROCK || g == BLUES || rng() % 2);
    guitar.reverbDelay = 0.1f;
    guitar.reverbDecay = 0.5f;
    guitar.reverbMixFactor = guitar.reverbMix;
    guitar.useDistortion = (g == ROCK || g == METAL || g == PUNK || rng() % 3 == 0);
    guitar.distortionDrive = 2.0f;
    guitar.distortionThreshold = 0.6f;

    const float restProb = (g == CLASSICAL || g == AMBIENT) ? 0.35f : (g == JAZZ || g == BLUES) ? 0.4f : 0.3f;
    const float arpeggioProb = (g == CLASSICAL || g == FOLK || g == AMBIENT) ? 0.5f : (g == JAZZ || g == BLUES) ? 0.3f : 0.2f;
    const float strumProb = (g == ROCK || g == PUNK || g == COUNTRY || g == FOLK) ? 0.6f : 0.1f;
    guitar.notes.reserve(500);
    guitar.panAutomation.reserve(36);
    guitar.volumeAutomation.reserve(36);
    guitar.reverbMixAutomation.reserve(36);

    size_t invalidFreqCount = 0;
    const size_t maxInvalidFreqs = 100;

    // Define guitar-specific frequency pool (82 Hz to ~1318 Hz, covering E2 to E6)
    const std::vector<float> guitarFreqs = {
        // Open strings and up to 24th fret per string
        82.41f, 87.31f, 92.50f, 98.00f, 103.83f, 110.00f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f, 261.63f, 277.18f, 293.66f, 311.13f, // E2 to B3 (Low E string)
        110.00f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, // A2 to E4 (A string)
        146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 196.00f, 207.65f, 220.00f, 233.08f, 246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f, 523.25f, 554.37f, // D3 to A4 (D string)
        196.00f, 207.65f, 220.00f, 233.08f, 246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f, 659.25f, 698.46f, 739.99f, // G3 to D5 (G string)
        246.94f, 261.63f, 277.18f, 293.66f, 311.13f, 329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f, 659.25f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, // B3 to F5 (B string)
        329.63f, 349.23f, 369.99f, 392.00f, 415.30f, 440.00f, 466.16f, 493.88f, 523.25f, 554.37f, 587.33f, 622.25f, 659.25f, 698.46f, 739.99f, 783.99f, 830.61f, 880.00f, 932.33f, 987.77f, 1046.50f, 1108.73f, 1174.66f, 1244.51f // E4 to E6 (High E string)
    };

    // Automation for dynamic transitions
    for (const auto& section : sections) {
        float t = section.startTime;
        float end = section.endTime;
        float step = (end - t) / 4.0f;
        for (int i = 0; i < 4 && t < end; ++i) {
            float pan = std::max(-1.0f, std::min(1.0f, guitar.pan + (rng() % 10 - 5) / 100.0f));
            float vol = std::max(0.4f, std::min(1.0f, 0.4f + (rng() % 15) / 100.0f));
            float rev = std::max(0.0f, std::min(1.0f, guitar.reverbMix + (rng() % 10) / 100.0f));
            guitar.panAutomation.emplace_back(t, pan);
            guitar.volumeAutomation.emplace_back(t, vol);
            guitar.reverbMixAutomation.emplace_back(t, rev);
            t += step;
        }
    }

    for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
        const auto& section = sections[sectionIdx];
        std::string templateName = section.templateName;

        // Reuse templates for Verse/Chorus
        if (sectionTemplates.find(templateName + "_Guitar") != sectionTemplates.end() &&
            (templateName == "Verse" || templateName == "Chorus")) {
            float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.2f : 1.0f;
            bool transpose = (section.name == "Chorus2" && rng() % 3 == 0);
            float transposeSemitones = transpose ? 2.0f : 0.0f;
            Part varied = varyPart(sectionTemplates[templateName + "_Guitar"], section.startTime, intensity, transpose, transposeSemitones);
            guitar.notes.insert(guitar.notes.end(), varied.notes.begin(), varied.notes.end());
            guitar.panAutomation.insert(guitar.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
            guitar.volumeAutomation.insert(guitar.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
            guitar.reverbMixAutomation.insert(guitar.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
            ::SDL_Log("Reused guitar template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
            continue;
        }

        float t = section.startTime;
        float sectionEnd = section.endTime;
        float sectionDur = sectionEnd - t;
        size_t maxNotes = static_cast<size_t>(sectionDur * (g == ROCK || g == PUNK || g == METAL ? 4.0f : 3.0f));
        size_t sectionNoteCount = 0;

        // Get chord progression
        std::vector<int> prog;
        if (chordProgressions.find(templateName) != chordProgressions.end()) {
            prog = chordProgressions[templateName];
        } else {
            auto progs = getChordProgressions(scaleName, g);
            prog = progs[rng() % progs.size()];
            if (templateName == "Verse" || templateName == "Chorus") {
                chordProgressions[templateName] = prog;
            }
        }
        size_t chordIdx = 0;

        bool useArpeggio = (rng() / static_cast<float>(rng.max()) < arpeggioProb);
        bool useStrum = (rng() / static_cast<float>(rng.max()) < strumProb && !useArpeggio);

        while (t < sectionEnd && sectionNoteCount < maxNotes) {
            if (invalidFreqCount >= maxInvalidFreqs) {
                ::SDL_Log("Aborting guitar generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                break;
            }
            if (rng() / static_cast<float>(rng.max()) < restProb && !useStrum) {
                t += getRandomDuration(g, section.progress, bpm);
                t = snapToBeatGrid(t, bpm);
                continue;
            }

            if (useStrum) {
                // Strumming pattern: full chord on beat
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, rng() % 2);
                float strumTime = snapToBeatGrid(t, bpm);
                float strumDur = 60.0f / (bpm * 2); // Half-note strums
                if (strumTime + strumDur > sectionEnd) strumDur = sectionEnd - strumTime;

                for (size_t i = 0; i < chord.size() && sectionNoteCount < maxNotes; ++i) {
                    float freq = chord[i];
                    // Ensure frequency is in guitar range
                    while (freq > 1318.0f) freq /= 2.0f;
                    while (freq < 82.0f) freq *= 2.0f;
                    freq = getClosestFreq(freq, guitarFreqs);

                    if (!std::isfinite(freq)) {
                        ::SDL_Log("Invalid guitar frequency at t=%.2f, using 82.41 Hz", strumTime);
                        freq = 82.41f; // Default to E2
                        invalidFreqCount++;
                    }

                    Note note;
                    note.startTime = strumTime;
                    note.duration = strumDur;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;
                    note.freq = freq;
                    note.volume = (g == ROCK || g == METAL || g == PUNK) ? 0.55f : 0.45f + 0.1f * section.progress;
                    note.velocity = (std::fmod(strumTime, 4 * 60.0f / bpm) < 0.1f) ? 0.9f : 0.8f + 0.15f * (rng() % 100) / 100.0f;
                    note.phoneme = -1;
                    note.open = false;
                    guitar.notes.push_back(note);
                    sectionNoteCount++;
                }
                t += strumDur;
                t = snapToBeatGrid(t, bpm);
                if (strumDur >= 0.25f) chordIdx++;
            } else if (useArpeggio) {
                // Arpeggio pattern: sequential chord tones
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, rng() % 2);
                float arpDur = 60.0f / (bpm * 4); // Sixteenth-note arpeggios
                std::vector<size_t> order(chord.size());
                std::iota(order.begin(), order.end(), 0);
                if (g != CLASSICAL) std::shuffle(order.begin(), order.end(), rng);

                for (size_t i : order) {
                    if (t >= sectionEnd || sectionNoteCount >= maxNotes) break;
                    float freq = chord[i];
                    // Ensure frequency is in guitar range
                    while (freq > 1318.0f) freq /= 2.0f;
                    while (freq < 82.0f) freq *= 2.0f;
                    freq = getClosestFreq(freq, guitarFreqs);

                    if (!std::isfinite(freq)) {
                        ::SDL_Log("Invalid guitar frequency at t=%.2f, using 82.41 Hz", t);
                        freq = 82.41f;
                        invalidFreqCount++;
                    }

                    Note note;
                    note.startTime = snapToBeatGrid(t, bpm);
                    note.duration = arpDur;
                    if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                    if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;
                    note.freq = freq;
                    note.volume = 0.4f + 0.1f * section.progress;
                    note.velocity = 0.7f + 0.2f * (rng() % 100) / 100.0f;
                    note.phoneme = -1;
                    note.open = false;
                    guitar.notes.push_back(note);
                    t += note.duration;
                    sectionNoteCount++;
                }
                chordIdx++;
                t = snapToBeatGrid(t, bpm);
            } else {
                // Melodic single-note line
                Note note;
                note.startTime = snapToBeatGrid(t, bpm);
                note.duration = getRandomDuration(g, section.progress, bpm);
                if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                if (!std::isfinite(note.duration) || note.duration <= 0.0f) note.duration = 0.0625f;

                const auto& intervals = scales.at(scaleName);
                static float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f), guitarFreqs);
                size_t currentIdx = 0;
                for (size_t j = 0; j < intervals.size(); ++j) {
                    float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                    if (std::abs(currentFreq - freq) < 1e-3) {
                        currentIdx = j;
                        break;
                    }
                }
                int step = (rng() % 3) - 1; // -1, 0, or 1 for smooth melodic movement
                currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                // Ensure frequency is in guitar range
                while (targetFreq > 1318.0f) targetFreq /= 2.0f;
                while (targetFreq < 82.0f) targetFreq *= 2.0f;
                currentFreq = note.freq = getClosestFreq(targetFreq, guitarFreqs);

                if (!std::isfinite(note.freq)) {
                    ::SDL_Log("Invalid guitar frequency at t=%.2f, using 82.41 Hz", t);
                    note.freq = 82.41f;
                    invalidFreqCount++;
                }
                note.volume = 0.45f + 0.1f * section.progress;
                note.velocity = 0.8f + 0.15f * (rng() % 100) / 100.0f;
                note.phoneme = -1;
                note.open = false;
                guitar.notes.push_back(note);
                t += note.duration;
                t = snapToBeatGrid(t, bpm);
                sectionNoteCount++;
            }
        }
        ::SDL_Log("Generated %zu notes for guitar in section %s", sectionNoteCount, section.name.c_str());

        // Store template for Verse/Chorus
        if (templateName == "Verse" || templateName == "Chorus") {
            Part templatePart = guitar;
            templatePart.notes.clear();
            templatePart.panAutomation.clear();
            templatePart.volumeAutomation.clear();
            templatePart.reverbMixAutomation.clear();
            for (const auto& note : guitar.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    Note templateNote = note;
                    templateNote.startTime -= section.startTime;
                    templatePart.notes.push_back(templateNote);
                }
            }
            for (const auto& [time, value] : guitar.panAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.panAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : guitar.volumeAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : guitar.reverbMixAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                }
            }
            sectionTemplates[templateName + "_Guitar"] = templatePart;
            ::SDL_Log("Stored guitar template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
        }
    }
    ::SDL_Log("Generated guitar with total %zu notes, %zu invalid frequencies encountered", guitar.notes.size(), invalidFreqCount);
    return guitar;
}

Part generateBass(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
    ::SDL_Log("Generating bass for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
    Part bass;
    bass.instrument = (g == ROCK || g == PUNK || g == METAL || g == FUNK || g == BLUES) ? "bass" : 
                      (g == TECHNO || g == EDM || g == AMBIENT) ? "subbass" : "bass";
    bass.pan = 0.0f; // Centered for low-end presence
    bass.reverbMix = (g == AMBIENT || g == TECHNO || g == EDM) ? 0.25f : 0.15f;
    bass.sectionName = "Bass";
    bass.useReverb = (g == AMBIENT || g == TECHNO || g == EDM || rng() % 2);
    bass.reverbDelay = 0.15f;
    bass.reverbDecay = 0.5f;
    bass.reverbMixFactor = bass.reverbMix;
    bass.useDistortion = (g == ROCK || g == METAL || g == PUNK || rng() % 4 == 0);
    bass.distortionDrive = 1.5f;
    bass.distortionThreshold = 0.8f;

    const float restProb = (g == JAZZ || g == BLUES) ? 0.4f : 0.3f;
    const float walkingProb = (g == JAZZ || g == BLUES) ? 0.65f : (g == FUNK) ? 0.5f : 0.15f;
    bass.notes.reserve(250);
    bass.panAutomation.reserve(36);
    bass.volumeAutomation.reserve(36);
    bass.reverbMixAutomation.reserve(36);

    size_t invalidFreqCount = 0;
    const size_t maxInvalidFreqs = 100;

    // Define bass-specific frequency pool (40–200 Hz, covering E1 to ~G3)
    const std::vector<float> bassFreqs = {
        41.20f, 43.65f, 46.25f, 49.00f, 51.91f, 55.00f, 58.27f, 61.74f, // E1 to B1
        65.41f, 69.30f, 73.42f, 77.78f, 82.41f, 87.31f, 92.50f, 98.00f, // C2 to B2
        103.83f, 110.00f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, // C3 to G3
        164.81f, 174.61f, 185.00f, 196.00f // G#3 to B3
    };

    // Automation for smooth transitions
    for (const auto& section : sections) {
        float t = section.startTime;
        float end = section.endTime;
        float step = (end - t) / 4.0f;
        for (int i = 0; i < 4 && t < end; ++i) {
            float pan = std::max(-1.0f, std::min(1.0f, bass.pan + (rng() % 5 - 2.5f) / 100.0f));
            float vol = std::max(0.45f, std::min(1.0f, 0.45f + (rng() % 10) / 100.0f));
            float rev = std::max(0.0f, std::min(1.0f, bass.reverbMix + (rng() % 5) / 100.0f));
            bass.panAutomation.emplace_back(t, pan);
            bass.volumeAutomation.emplace_back(t, vol);
            bass.reverbMixAutomation.emplace_back(t, rev);
            t += step;
        }
    }

    for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
        const auto& section = sections[sectionIdx];
        std::string templateName = section.templateName;

        // Reuse templates for Verse/Chorus
        if (sectionTemplates.find(templateName + "_Bass") != sectionTemplates.end() &&
            (templateName == "Verse" || templateName == "Chorus")) {
            float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.15f : 1.0f;
            Part varied = varyPart(sectionTemplates[templateName + "_Bass"], section.startTime, intensity);
            bass.notes.insert(bass.notes.end(), varied.notes.begin(), varied.notes.end());
            bass.panAutomation.insert(bass.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
            bass.volumeAutomation.insert(bass.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
            bass.reverbMixAutomation.insert(bass.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
            ::SDL_Log("Reused bass template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
            continue;
        }

        float t = section.startTime;
        float sectionEnd = section.endTime;
        float sectionDur = sectionEnd - t;
        size_t maxNotes = static_cast<size_t>(sectionDur * (g == FUNK || g == JAZZ || g == BLUES ? 4.0f : 2.0f));
        size_t sectionNoteCount = 0;
        bool useWalking = (rng() / static_cast<float>(rng.max()) < walkingProb);

        // Get chord progression
        std::vector<int> prog;
        if (chordProgressions.find(templateName) != chordProgressions.end()) {
            prog = chordProgressions[templateName];
        } else {
            auto progs = getChordProgressions(scaleName, g);
            prog = progs[rng() % progs.size()];
            if (templateName == "Verse" || templateName == "Chorus") {
                chordProgressions[templateName] = prog;
            }
        }
        size_t chordIdx = 0;

        while (t < sectionEnd && sectionNoteCount < maxNotes) {
            if (invalidFreqCount >= maxInvalidFreqs) {
                ::SDL_Log("Aborting bass generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                break;
            }
            if (rng() / static_cast<float>(rng.max()) < restProb && !useWalking) {
                t += getRandomDuration(g, section.progress, bpm);
                t = snapToBeatGrid(t, bpm);
                continue;
            }

            Note note;
            note.startTime = snapToBeatGrid(t, bpm);
            note.duration = useWalking ? 60.0f / (bpm * 4) : getRandomDuration(g, section.progress, bpm);
            if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
            if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                note.duration = 0.0625f;
            }

            // Select frequency
            if (useWalking) {
                // Walking bass: step-wise motion within scale
                const auto& intervals = scales.at(scaleName);
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                float rootNote = chord[0];
                size_t currentIdx = 0;
                for (size_t j = 0; j < intervals.size(); ++j) {
                    float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                    if (std::abs(rootNote - freq) < 1e-3) {
                        currentIdx = j;
                        break;
                    }
                }
                int step = (rng() % 3) - 1; // -1, 0, or 1 for smooth transitions
                currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                float targetFreq = rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f);
                // Ensure frequency is in bass range
                while (targetFreq > 200.0f) targetFreq /= 2.0f;
                while (targetFreq < 40.0f) targetFreq *= 2.0f;
                note.freq = getClosestFreq(targetFreq, bassFreqs);
            } else {
                // Root note or chord tone, biased toward root
                auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, 0);
                float targetFreq = chord[rng() % chord.size()];
                // Prefer root note 70% of the time
                if (rng() / static_cast<float>(rng.max()) < 0.7f) targetFreq = chord[0];
                // Ensure frequency is in bass range
                while (targetFreq > 200.0f) targetFreq /= 2.0f;
                while (targetFreq < 40.0f) targetFreq *= 2.0f;
                note.freq = getClosestFreq(targetFreq, bassFreqs);
                // Emphasize downbeats in ROCK/EDM
                if ((g == ROCK || g == EDM || g == METAL) && std::fmod(note.startTime, 4 * 60.0f / bpm) < 0.1f) {
                    note.velocity = 0.95f;
                    note.volume = 0.55f;
                }
            }

            if (!std::isfinite(note.freq)) {
                ::SDL_Log("Invalid bass frequency at t=%.2f, using 41.20 Hz", t);
                note.freq = 41.20f; // Default to E1
                invalidFreqCount++;
            }
            note.volume = (g == ROCK || g == METAL || g == EDM) ? 0.5f : 0.45f + 0.1f * section.progress;
            note.velocity = 0.85f + 0.15f * (rng() % 100) / 100.0f;
            note.phoneme = -1;
            note.open = false;
            bass.notes.push_back(note);
            t += note.duration;
            t = snapToBeatGrid(t, bpm);
            sectionNoteCount++;
            if (!useWalking && note.duration >= 0.25f) chordIdx++;
        }
        ::SDL_Log("Generated %zu notes for bass in section %s", sectionNoteCount, section.name.c_str());

        // Store template for Verse/Chorus
        if (templateName == "Verse" || templateName == "Chorus") {
            Part templatePart = bass;
            templatePart.notes.clear();
            templatePart.panAutomation.clear();
            templatePart.volumeAutomation.clear();
            templatePart.reverbMixAutomation.clear();
            for (const auto& note : bass.notes) {
                if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                    Note templateNote = note;
                    templateNote.startTime -= section.startTime;
                    templatePart.notes.push_back(templateNote);
                }
            }
            for (const auto& [time, value] : bass.panAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.panAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : bass.volumeAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                }
            }
            for (const auto& [time, value] : bass.reverbMixAutomation) {
                if (time >= section.startTime && time < section.endTime) {
                    templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                }
            }
            sectionTemplates[templateName + "_Bass"] = templatePart;
            ::SDL_Log("Stored bass template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
        }
    }
    ::SDL_Log("Generated bass with total %zu notes, %zu invalid frequencies encountered", bass.notes.size(), invalidFreqCount);
    return bass;
}

        Part generateArpeggio(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
            Part arp;
            arp.instrument = (g == CLASSICAL) ? "piano" : (g == EDM || g == TECHNO) ? "syntharp" : "leadsynth";
            arp.pan = (rng() % 2) ? 0.4f : -0.4f;
            arp.reverbMix = (g == AMBIENT || g == EDM || g == TECHNO) ? 0.5f : 0.3f;
            arp.sectionName = "Arpeggio";
            arp.useReverb = true;
            arp.reverbDelay = 0.1f;
            arp.reverbDecay = 0.6f;
            arp.reverbMixFactor = arp.reverbMix;
            arp.useDistortion = (g == EDM || g == TECHNO || rng() % 4 == 0);
            arp.distortionDrive = 1.3f;
            arp.distortionThreshold = 0.8f;

            const float restProb = 0.3f;
            arp.notes.reserve(500);
            arp.panAutomation.reserve(36);
            arp.volumeAutomation.reserve(36);
            arp.reverbMixAutomation.reserve(36);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, arp.pan + (rng() % 10 - 5) / 100.0f));
                    float vol = std::max(0.3f, std::min(1.0f, 0.3f + (rng() % 10) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, arp.reverbMix + (rng() % 10) / 100.0f));
                    arp.panAutomation.emplace_back(t, pan);
                    arp.volumeAutomation.emplace_back(t, vol);
                    arp.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Arpeggio") != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.05f : 1.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Arpeggio"], section.startTime, intensity);
                    arp.notes.insert(arp.notes.end(), varied.notes.begin(), varied.notes.end());
                    arp.panAutomation.insert(arp.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    arp.volumeAutomation.insert(arp.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    arp.reverbMixAutomation.insert(arp.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused arpeggio template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                size_t maxNotes = static_cast<size_t>((sectionEnd - t) * 4.0f);
                size_t sectionNoteCount = 0;

                std::vector<int> prog;
                if (chordProgressions.find(templateName) != chordProgressions.end()) {
                    prog = chordProgressions[templateName];
                } else {
                    auto progs = getChordProgressions(scaleName, g);
                    prog = progs[rng() % progs.size()];
                    if (templateName == "Verse" || templateName == "Chorus") {
                        chordProgressions[templateName] = prog;
                    }
                }

                float arpDur = (g == EDM || g == TECHNO) ? 60.0f / (bpm * 4) : 60.0f / (bpm * 2);
                size_t chordIdx = 0;

                while (t < sectionEnd && sectionNoteCount < maxNotes) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting arpeggio generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    if (rng() / static_cast<float>(rng.max()) < restProb) {
                        t += arpDur;
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    auto chord = buildChord(prog[chordIdx % prog.size()], scaleName, rootFreq, g, rng() % 2);
                    if (chord.empty() || !std::all_of(chord.begin(), chord.end(), [](float f) { return std::isfinite(f); })) {
                        ::SDL_Log("Invalid chord frequencies in arpeggio, skipping");
                        t += arpDur;
                        invalidFreqCount++;
                        continue;
                    }

                    std::vector<size_t> order(chord.size());
                    std::iota(order.begin(), order.end(), 0);
                    if (g != CLASSICAL) std::shuffle(order.begin(), order.end(), rng);

                    for (size_t i : order) {
                        if (t >= sectionEnd || sectionNoteCount >= maxNotes) break;
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = arpDur;
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                            note.duration = 0.0625f;
                        }
                        note.freq = chord[i];
                        note.volume = 0.3f + 0.1f * section.progress;
                        note.velocity = 0.7f + 0.2f * (rng() % 100) / 100.0f;
                        note.phoneme = -1;
                        note.open = false;
                        arp.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    chordIdx++;
                    t = snapToBeatGrid(t, bpm);
                }
                ::SDL_Log("Generated %zu notes for arpeggio in section %s", sectionNoteCount, section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = arp;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : arp.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : arp.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : arp.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : arp.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Arpeggio"] = templatePart;
                    ::SDL_Log("Stored arpeggio template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated arpeggio with total %zu notes, %zu invalid frequencies encountered", arp.notes.size(), invalidFreqCount);
            return arp;
        }

        Part generateHarmony(Genre g, const std::string& scaleName, float rootFreq, float totalDur,
                             const std::vector<Section>& sections, float bpm) {
            ::SDL_Log("Generating harmony for genre %s, scale %s", genreNames[g].c_str(), scaleName.c_str());
            Part harmony;
            harmony.sectionName = "Harmony";
            harmony.instrument = (g == CLASSICAL) ? "strings" : (g == AMBIENT) ? "pad" : "organ";
            harmony.pan = 0.0f;
            harmony.reverbMix = (g == AMBIENT || g == CLASSICAL) ? 0.5f : 0.2f;
            harmony.useReverb = (g == AMBIENT || g == CLASSICAL || g == GOSPEL);
            harmony.reverbDelay = 0.1f;
            harmony.reverbDecay = 0.9f;
            harmony.reverbMixFactor = 0.5f;
            harmony.useDistortion = (g == ROCK && rng() % 2);
            harmony.distortionDrive = 2.0f;
            harmony.distortionThreshold = 0.3f;

            harmony.notes.reserve(1000);
            harmony.panAutomation.reserve(200);
            harmony.volumeAutomation.reserve(200);
            harmony.reverbMixAutomation.reserve(200);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;
            float beat = 60.0f / bpm;

            for (const auto& section : sections) {
                std::string templateName = section.templateName;
                auto it = sectionTemplates.find(templateName + "_Harmony");
                if (it != sectionTemplates.end() && (templateName == "Verse" || templateName == "Chorus")) {
                    ::SDL_Log("Using template %s for section %s", (templateName + "_Harmony").c_str(), section.name.c_str());
                    Part varied = varyPart(it->second, section.startTime, 1.0f);
                    harmony.notes.insert(harmony.notes.end(), varied.notes.begin(), varied.notes.end());
                    harmony.panAutomation.insert(harmony.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    harmony.volumeAutomation.insert(harmony.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    harmony.reverbMixAutomation.insert(harmony.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    continue;
                }

                float t = section.startTime;
                float sectionDur = section.endTime - section.startTime;
                size_t sectionNoteCount = 0;
                size_t maxNotes = static_cast<size_t>(sectionDur * 0.5f);
                maxNotes = std::min(maxNotes, harmony.notes.capacity() - harmony.notes.size());

                std::vector<int> prog;
                if (chordProgressions.find(templateName) != chordProgressions.end()) {
                    prog = chordProgressions[templateName];
                } else {
                    auto progs = getChordProgressions(scaleName, g);
                    prog = progs[rng() % progs.size()];
                    if (templateName == "Verse" || templateName == "Chorus") {
                        chordProgressions[templateName] = prog;
                    }
                }

                float chordDur = beat * 4.0f;
                if (g == CLASSICAL || g == AMBIENT) chordDur *= 2.0f;
                size_t chordIdx = 0;

                while (t < section.endTime && sectionNoteCount < maxNotes && harmony.notes.size() < harmony.notes.capacity()) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting harmony generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    int degree = prog[chordIdx % prog.size()];
                    int inversion = chordIdx % 3; // Cycle through inversions for variety
                    std::vector<float> chord = buildChord(degree, scaleName, rootFreq, g, inversion);

                    for (float freq : chord) {
                        if (!std::isfinite(freq)) {
                            ::SDL_Log("Invalid chord frequency %.2f at t=%.2f, skipping", freq, t);
                            invalidFreqCount++;
                            continue;
                        }
                        Note note(freq, chordDur, t);
                        note.volume = 0.4f + 0.1f * section.progress;
                        note.velocity = 0.6f + 0.2f * (rng() % 100) / 100.0f;
                        harmony.notes.push_back(note);
                        sectionNoteCount++;
                    }

                    float pan = (g == CLASSICAL || g == AMBIENT) ? 0.0f : (rng() % 2 ? -0.2f : 0.2f);
                    if (harmony.panAutomation.size() < harmony.panAutomation.capacity()) {
                        harmony.panAutomation.emplace_back(t, pan);
                    }
                    if (harmony.volumeAutomation.size() < harmony.volumeAutomation.capacity()) {
                        harmony.volumeAutomation.emplace_back(t, 0.4f + 0.1f * section.progress);
                    }
                    if (harmony.reverbMixAutomation.size() < harmony.reverbMixAutomation.capacity()) {
                        harmony.reverbMixAutomation.emplace_back(t, harmony.reverbMix);
                    }

                    t += chordDur;
                    chordIdx++;
                }

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart;
                    templatePart.instrument = harmony.instrument;
                    templatePart.pan = harmony.pan;
                    templatePart.reverbMix = harmony.reverbMix;
                    templatePart.useReverb = harmony.useReverb;
                    templatePart.reverbDelay = harmony.reverbDelay;
                    templatePart.reverbDecay = harmony.reverbDecay;
                    templatePart.reverbMixFactor = harmony.reverbMixFactor;
                    templatePart.useDistortion = harmony.useDistortion;
                    templatePart.distortionDrive = harmony.distortionDrive;
                    templatePart.distortionThreshold = harmony.distortionThreshold;

                    for (const auto& note : harmony.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : harmony.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : harmony.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : harmony.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Harmony"] = templatePart;
                    ::SDL_Log("Stored harmony template %s with %zu notes", (templateName + "_Harmony").c_str(), templatePart.notes.size());
                }
            }

            ::SDL_Log("Generated harmony with total %zu notes, %zu invalid frequencies encountered", harmony.notes.size(), invalidFreqCount);
            return harmony;
        }

        Part generateVocal(Genre g, const std::string& scaleName, float rootFreq, float totalDur, const std::vector<Section>& sections, float bpm) {
            Part vocal;
            vocal.instrument = (rng() % 2) ? "vocal_0" : "vocal_1";
            vocal.pan = (rng() % 2) ? 0.2f : -0.2f;
            vocal.reverbMix = (g == GOSPEL || g == SOUL) ? 0.4f : 0.3f;
            vocal.sectionName = "Vocal";
            vocal.useReverb = true;
            vocal.reverbDelay = 0.15f;
            vocal.reverbDecay = 0.6f;
            vocal.reverbMixFactor = vocal.reverbMix;
            vocal.useDistortion = false;

            const float restProb = (g == RAP || g == HIPHOP) ? 0.5f : 0.4f;
            const float phraseProb = (g == GOSPEL || g == SOUL) ? 0.7f : 0.5f;
            vocal.notes.reserve(300);
            vocal.panAutomation.reserve(36);
            vocal.volumeAutomation.reserve(36);
            vocal.reverbMixAutomation.reserve(36);

            size_t invalidFreqCount = 0;
            const size_t maxInvalidFreqs = 100;

            for (const auto& section : sections) {
                float t = section.startTime;
                float end = section.endTime;
                float step = (end - t) / 4.0f;
                for (int i = 0; i < 4 && t < end; ++i) {
                    float pan = std::max(-1.0f, std::min(1.0f, vocal.pan + (rng() % 10 - 5) / 100.0f));
                    float vol = std::max(0.5f, std::min(1.0f, 0.5f + (rng() % 10) / 100.0f));
                    float rev = std::max(0.0f, std::min(1.0f, vocal.reverbMix + (rng() % 10) / 100.0f));
                    vocal.panAutomation.emplace_back(t, pan);
                    vocal.volumeAutomation.emplace_back(t, vol);
                    vocal.reverbMixAutomation.emplace_back(t, rev);
                    t += step;
                }
            }

            const auto& intervals = scales.at(scaleName);
            float currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[rng() % intervals.size()] / 12.0f));

            for (size_t sectionIdx = 0; sectionIdx < sections.size(); ++sectionIdx) {
                const auto& section = sections[sectionIdx];
                std::string templateName = section.templateName;

                if (sectionTemplates.find(templateName + "_Vocal") != sectionTemplates.end() &&
                    (templateName == "Verse" || templateName == "Chorus")) {
                    float intensity = (section.name == "Chorus2" || section.name == "Verse2") ? 1.1f : 1.0f;
                    bool transpose = (section.name == "Chorus2" && rng() % 2);
                    float transposeSemitones = transpose ? 2.0f : 0.0f;
                    Part varied = varyPart(sectionTemplates[templateName + "_Vocal"], section.startTime, intensity, transpose, transposeSemitones);
                    vocal.notes.insert(vocal.notes.end(), varied.notes.begin(), varied.notes.end());
                    vocal.panAutomation.insert(vocal.panAutomation.end(), varied.panAutomation.begin(), varied.panAutomation.end());
                    vocal.volumeAutomation.insert(vocal.volumeAutomation.end(), varied.volumeAutomation.begin(), varied.volumeAutomation.end());
                    vocal.reverbMixAutomation.insert(vocal.reverbMixAutomation.end(), varied.reverbMixAutomation.begin(), varied.reverbMixAutomation.end());
                    ::SDL_Log("Reused vocal template %s for section %s with %zu notes", templateName.c_str(), section.name.c_str(), varied.notes.size());
                    continue;
                }

                float t = section.startTime;
                float sectionEnd = section.endTime;
                size_t maxNotes = static_cast<size_t>((sectionEnd - t) * 3.0f);
                size_t sectionNoteCount = 0;
                float phraseDur = 4.0f * 60.0f / bpm;
                float phraseStart = t;

                while (t < sectionEnd && sectionNoteCount < maxNotes) {
                    if (invalidFreqCount >= maxInvalidFreqs) {
                        ::SDL_Log("Aborting vocal generation for section %s: too many invalid frequencies (%zu)", section.name.c_str(), invalidFreqCount);
                        break;
                    }
                    if (rng() / static_cast<float>(rng.max()) < restProb) {
                        t += getRandomDuration(g, section.progress, bpm);
                        t = snapToBeatGrid(t, bpm);
                        continue;
                    }

                    bool usePhrase = rng() / static_cast<float>(rng.max()) < phraseProb;
                    int numNotes = usePhrase ? 3 + (rng() % 3) : 1;

                    for (int i = 0; i < numNotes && t < sectionEnd && sectionNoteCount < maxNotes; ++i) {
                        Note note;
                        note.startTime = snapToBeatGrid(t, bpm);
                        note.duration = getRandomDuration(g, section.progress, bpm);
                        if (note.startTime + note.duration > sectionEnd) note.duration = sectionEnd - note.startTime;
                        if (!std::isfinite(note.duration) || note.duration <= 0.0f) {
                            note.duration = 0.0625f;
                        }
                        note.volume = 0.5f + 0.1f * section.progress;
                        note.velocity = 0.8f + 0.2f * (rng() % 100) / 100.0f;
                        note.phoneme = rng() % 7;
                        note.open = false;

                        size_t currentIdx = 0;
                        for (size_t j = 0; j < intervals.size(); ++j) {
                            float freq = rootFreq * std::pow(2.0f, intervals[j] / 12.0f);
                            if (std::abs(currentFreq - freq) < 1e-3) {
                                currentIdx = j;
                                break;
                            }
                        }
                        int step = (rng() % 2) ? 1 : -1;
                        currentIdx = (currentIdx + step + intervals.size()) % intervals.size();
                        currentFreq = getClosestFreq(rootFreq * std::pow(2.0f, intervals[currentIdx] / 12.0f));
                        note.freq = currentFreq;
                        if (!std::isfinite(note.freq)) {
                            note.freq = 440.0f;
                            invalidFreqCount++;
                        }
                        vocal.notes.push_back(note);
                        t += note.duration;
                        sectionNoteCount++;
                    }
                    t = snapToBeatGrid(t, bpm);
                    if (t >= phraseStart + phraseDur) {
                        phraseStart = t;
                        if (rng() % 2) {
                            t += 60.0f / bpm;
                            t = snapToBeatGrid(t, bpm);
                        }
                    }
                }
                ::SDL_Log("Generated %zu notes for vocal in section %s", sectionNoteCount, section.name.c_str());

                if (templateName == "Verse" || templateName == "Chorus") {
                    Part templatePart = vocal;
                    templatePart.notes.clear();
                    templatePart.panAutomation.clear();
                    templatePart.volumeAutomation.clear();
                    templatePart.reverbMixAutomation.clear();
                    for (const auto& note : vocal.notes) {
                        if (note.startTime >= section.startTime && note.startTime < section.endTime) {
                            Note templateNote = note;
                            templateNote.startTime -= section.startTime;
                            templatePart.notes.push_back(templateNote);
                        }
                    }
                    for (const auto& [time, value] : vocal.panAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.panAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : vocal.volumeAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.volumeAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    for (const auto& [time, value] : vocal.reverbMixAutomation) {
                        if (time >= section.startTime && time < section.endTime) {
                            templatePart.reverbMixAutomation.emplace_back(time - section.startTime, value);
                        }
                    }
                    sectionTemplates[templateName + "_Vocal"] = templatePart;
                    ::SDL_Log("Stored vocal template %s with %zu notes", templateName.c_str(), templatePart.notes.size());
                }
            }
            ::SDL_Log("Generated vocal with total %zu notes, %zu invalid frequencies encountered", vocal.notes.size(), invalidFreqCount);
            return vocal;
        }
    };
}
#endif // SONGGEN_H