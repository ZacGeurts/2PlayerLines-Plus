#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Electric Storm
// In A minor, 140 BPM, ~3 minutes
float generateSong4(float t, float songTime) {
    using namespace Instruments;
    float sample = 0.0f;

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song4 - Electric Storm\n");
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
    std::vector<std::tuple<float, float, float>> tomNotes;
    std::vector<std::tuple<float, float, float>> snareNotes;
    std::vector<std::tuple<float, float, float>> padNotes;
    std::vector<std::tuple<float, float, float>> guitarNotes;

    // Tom: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        tomNotes.emplace_back(t, 220.00f, quarter); // A3
    }

    // Snare: Every eighth note
    for (float t = 0.0f; t < 180.0f; t += eighth) {
        snareNotes.emplace_back(t, 0.0f, eighth);
    }

    // Pad: Starts at 15s, quarter-note chords
    for (float t = 15.0f; t < 180.0f; t += 4.0f * quarter) {
        padNotes.emplace_back(t + 0.0f * quarter, notes[0], quarter); // A4
        padNotes.emplace_back(t + 1.0f * quarter, notes[2], quarter); // C5
        padNotes.emplace_back(t + 2.0f * quarter, notes[4], quarter); // E5
        padNotes.emplace_back(t + 3.0f * quarter, notes[0], quarter); // A4
    }

    // Guitar: Starts at 30s, eighth-note riffs
    for (float t = 30.0f; t < 180.0f; t += quarter) {
        guitarNotes.emplace_back(t + 0.0f * eighth, notes[0], eighth); // A4
        guitarNotes.emplace_back(t + 1.0f * eighth, notes[4], eighth); // E5
    }

    // Process notes
    float leftSample = 0.0f, rightSample = 0.0f, centerSample = 0.0f, lfeSample = 0.0f, surroundLeftSample = 0.0f, surroundRightSample = 0.0f;

    // Tom: LFE and center
    for (const auto& [start, freq, dur] : tomNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateTom(noteT, freq, dur) * 0.5f;
            lfeSample += wave * 0.7f;
            centerSample += wave * 0.3f;
        }
    }

    // Snare: Surround and front
    for (const auto& [start, freq, dur] : snareNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateSnare(noteT, dur) * 0.4f;
            surroundLeftSample += wave * 0.3f;
            surroundRightSample += wave * 0.3f;
            leftSample += wave * 0.2f;
            rightSample += wave * 0.2f;
        }
    }

    // Pad: Surround
    for (const auto& [start, freq, dur] : padNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generatePad(noteT, freq, dur) * 0.25f;
            surroundLeftSample += wave * 0.5f;
            surroundRightSample += wave * 0.5f;
        }
    }

    // Guitar: Front and center
    for (const auto& [start, freq, dur] : guitarNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateGuitar(noteT, freq, dur) * 0.35f;
            leftSample += wave * 0.4f;
            rightSample += wave * 0.4f;
            centerSample += wave * 0.2f;
        }
    }

    // Mix channels
    sample = (leftSample * 0.2f + rightSample * 0.2f + centerSample * 0.3f + lfeSample * 0.2f + surroundLeftSample * 0.05f + surroundRightSample * 0.05f);

    // Clip to prevent distortion
    sample = std::min(std::max(sample, -0.9f), 0.9f);

    return sample;
}