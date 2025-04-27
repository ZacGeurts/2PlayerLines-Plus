#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Quantum Beat
// In A minor, 140 BPM, ~3 minutes
float generateSong3(float t, float songTime) {
    using namespace Instruments;
    float sample = 0.0f;

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song3 - Quantum Beat\n");
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
    std::vector<std::tuple<float, float, float>> subBassNotes;
    std::vector<std::tuple<float, float, float>> cymbalNotes;
    std::vector<std::tuple<float, float, float>> fluteNotes;
    std::vector<std::tuple<float, float, float>> violinNotes;

    // SubBass: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        subBassNotes.emplace_back(t, 220.00f, quarter); // A3
    }

    // Cymbal: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        cymbalNotes.emplace_back(t, 0.0f, eighth);
    }

    // Flute: Starts at 15s, eighth-note melody
    for (float t = 15.0f; t < 180.0f; t += quarter) {
        fluteNotes.emplace_back(t + 0.0f * eighth, notes[0], eighth); // A4
        fluteNotes.emplace_back(t + 1.0f * eighth, notes[4], eighth); // E5
    }

    // Violin: Starts at 30s, quarter-note melody
    for (float t = 30.0f; t < 180.0f; t += 4.0f * quarter) {
        violinNotes.emplace_back(t + 0.0f * quarter, notes[2], quarter); // C5
        violinNotes.emplace_back(t + 1.0f * quarter, notes[4], quarter); // E5
        violinNotes.emplace_back(t + 2.0f * quarter, notes[6], quarter); // G5
        violinNotes.emplace_back(t + 3.0f * quarter, notes[0], quarter); // A4
    }

    // Process notes
    float leftSample = 0.0f, rightSample = 0.0f, centerSample = 0.0f, lfeSample = 0.0f, surroundLeftSample = 0.0f, surroundRightSample = 0.0f;

    // SubBass: LFE and center
    for (const auto& [start, freq, dur] : subBassNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateSubBass(noteT, freq, dur) * 0.6f;
            lfeSample += wave * 0.8f;
            centerSample += wave * 0.2f;
        }
    }

    // Cymbal: Surround
    for (const auto& [start, freq, dur] : cymbalNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateCymbal(noteT, dur) * 0.2f;
            surroundLeftSample += wave * 0.5f;
            surroundRightSample += wave * 0.5f;
        }
    }

    // Flute: Front and center
    for (const auto& [start, freq, dur] : fluteNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateFlute(noteT, freq, dur) * 0.35f;
            leftSample += wave * 0.4f;
            rightSample += wave * 0.4f;
            centerSample += wave * 0.2f;
        }
    }

    // Violin: Surround
    for (const auto& [start, freq, dur] : violinNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateViolin(noteT, freq, dur) * 0.3f;
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