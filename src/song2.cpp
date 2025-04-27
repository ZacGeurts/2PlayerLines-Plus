#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Cyber Surge
// In A minor, 140 BPM, ~3 minutes
float generateSong2(float t, float songTime) {
    using namespace Instruments;
    float sample = 0.0f;

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song2 - Cyber Surge\n");
        hasLogged = true;
    }

    // Fixed tempo: 140 BPM
    float bpm = 140.0f;
    float beatTime = 60.0f / bpm;
    float quarter = beatTime;
    float eighth = quarter / 2.0f;

    // A minor scale (4th octave)
    std::vector<float> notes = {
        440.00f, // A4
        493.88f, // B4
        523.25f, // C5
        587.33f, // D5
        659.25f, // E5
        698.46f, // F5
        783.99f  // G5
    };

    // Note collections
    std::vector<std::tuple<float, float, float>> bassNotes;
    std::vector<std::tuple<float, float, float>> clapNotes;
    std::vector<std::tuple<float, float, float>> pianoNotes;
    std::vector<std::tuple<float, float, float, int>> vocalNotes;

    // Bass: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        bassNotes.emplace_back(t, 220.00f, quarter); // A3
    }

    // Clap: Every eighth note
    for (float t = 0.0f; t < 180.0f; t += eighth) {
        clapNotes.emplace_back(t, 0.0f, eighth);
    }

    // Piano: Starts at 15s, eighth-note stabs
    for (float t = 15.0f; t < 180.0f; t += quarter) {
        pianoNotes.emplace_back(t + 0.0f * eighth, notes[0], eighth); // A4
        pianoNotes.emplace_back(t + 1.0f * eighth, notes[4], eighth); // E5
    }

    // Vocal: Starts at 30s, quarter-note accents
    for (float t = 30.0f; t < 180.0f; t += 2.0f * quarter) {
        vocalNotes.emplace_back(t, notes[2], quarter, 1); // C5, phoneme 'ee'
    }

    // Process notes
    float leftSample = 0.0f, rightSample = 0.0f, centerSample = 0.0f, lfeSample = 0.0f, surroundLeftSample = 0.0f, surroundRightSample = 0.0f;

    // Bass: LFE and center
    for (const auto& [start, freq, dur] : bassNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateBass(noteT, freq, dur) * 0.6f;
            lfeSample += wave * 0.8f;
            centerSample += wave * 0.2f;
        }
    }

    // Clap: Surround and front
    for (const auto& [start, freq, dur] : clapNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateClap(noteT, dur) * 0.4f;
            surroundLeftSample += wave * 0.3f;
            surroundRightSample += wave * 0.3f;
            leftSample += wave * 0.2f;
            rightSample += wave * 0.2f;
        }
    }

    // Piano: Front and center
    for (const auto& [start, freq, dur] : pianoNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generatePiano(noteT, freq, dur) * 0.35f;
            leftSample += wave * 0.4f;
            rightSample += wave * 0.4f;
            centerSample += wave * 0.2f;
        }
    }

    // Vocal: Surround
    for (const auto& [start, freq, dur, phoneme] : vocalNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateVocal(noteT, freq, phoneme, dur) * 0.3f;
            surroundLeftSample += wave * 0.5f;
            surroundRightSample += wave * 0.5f;
        }
    }

    // Mix channels
    sample = (leftSample * 0.2f + rightSample * 0.2f + centerSample * 0.3f + lfeSample * 0.2f + surroundLeftSample * 0.05f + surroundRightSample * 0.05f);

    // Clip to prevent distortion
    sample = std::min(std::max(sample, -0.9f), 0.9f);

    return sample;
}