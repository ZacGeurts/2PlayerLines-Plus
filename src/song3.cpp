#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Starfield
// In G minor, 132 BPM, ~3 minutes
std::vector<float> generateSong3(float songTime, int channels) {
    using namespace Instruments;
    std::vector<float> samples(channels, 0.0f);

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song3 - Starfield\n");
        hasLogged = true;
    }

    // Fixed tempo: 132 BPM
    float bpm = 132.0f;
    float beatTime = 60.0f / bpm; // 0.45455 seconds
    float quarter = beatTime;
    float sixteenth = quarter / 4.0f;

    // G minor scale (4th octave)
    std::vector<float> notes = {
        392.00f, // G4
        440.00f, // A4
        466.16f, // Bb4
        523.25f, // C5
        587.33f, // D5
        622.25f, // Eb5
        698.46f  // F5
    };

    // Note collections
    std::vector<std::tuple<float, float, float>> kickNotes;
    std::vector<std::tuple<float, float, float>> clapNotes;
    std::vector<std::tuple<float, float, float>> padNotes;
    std::vector<std::tuple<float, float, float>> leadNotes;

    // Kick: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        kickNotes.emplace_back(t, 196.00f, quarter); // G3
    }

    // Clap: Every other quarter note (off-beat)
    for (float t = quarter; t < 180.0f; t += 2.0f * quarter) {
        clapNotes.emplace_back(t, 0.0f, quarter);
    }

    // Pad: Starts at 12s, whole-note chords
    for (float t = 12.0f; t < 180.0f; t += 4.0f * quarter) {
        padNotes.emplace_back(t, notes[0], 4.0f * quarter); // G4
        padNotes.emplace_back(t, notes[2], 4.0f * quarter); // Bb4
        padNotes.emplace_back(t, notes[4], 4.0f * quarter); // D5
    }

    // LeadSynth: Starts at 25s, quarter-note melody
    for (float t = 25.0f; t < 180.0f; t += 4.0f * quarter) {
        leadNotes.emplace_back(t + 0.0f * quarter, notes[0], quarter); // G4
        leadNotes.emplace_back(t + 1.0f * quarter, notes[4], quarter); // D5
        leadNotes.emplace_back(t + 2.0f * quarter, notes[3], quarter); // C5
        leadNotes.emplace_back(t + 3.0f * quarter, notes[2], quarter); // Bb4
    }

    // Process notes
    float leftSample = 0.0f, rightSample = 0.0f, centerSample = 0.0f, lfeSample = 0.0f, surroundLeftSample = 0.0f, surroundRightSample = 0.0f;

    // Kick: LFE and center
    for (const auto& [start, freq, dur] : kickNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateKick(noteT, freq, dur) * 0.5f;
            lfeSample += wave * 0.7f;
            centerSample += wave * 0.3f;
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

    // Pad: Surround and center
    for (const auto& [start, freq, dur] : padNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generatePad(noteT, freq, dur) * 0.3f;
            surroundLeftSample += wave * 0.3f;
            surroundRightSample += wave * 0.3f;
            centerSample += wave * 0.4f;
        }
    }

    // LeadSynth: Front and center
    for (const auto& [start, freq, dur] : leadNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateLeadSynth(noteT, freq, dur) * 0.35f;
            leftSample += wave * 0.4f;
            rightSample += wave * 0.4f;
            centerSample += wave * 0.2f;
        }
    }

    // Assign samples based on channel count
    if (channels == 6) { // 5.1 surround
        samples[0] = std::min(std::max(leftSample, -0.9f), 0.9f);           // Front left
        samples[1] = std::min(std::max(rightSample, -0.9f), 0.9f);          // Front right
        samples[2] = std::min(std::max(centerSample, -0.9f), 0.9f);         // Center
        samples[3] = std::min(std::max(lfeSample, -0.9f), 0.9f);            // LFE
        samples[4] = std::min(std::max(surroundLeftSample, -0.9f), 0.9f);   // Surround left
        samples[5] = std::min(std::max(surroundRightSample, -0.9f), 0.9f);  // Surround right
    } else if (channels == 2) { // Stereo
        samples[0] = std::min(std::max(leftSample + surroundLeftSample * 0.5f + centerSample * 0.5f, -0.9f), 0.9f);
        samples[1] = std::min(std::max(rightSample + surroundRightSample * 0.5f + centerSample * 0.5f, -0.9f), 0.9f);
    } else { // Mono
        samples[0] = std::min(std::max(leftSample * 0.2f + rightSample * 0.2f + centerSample * 0.3f +
                                       lfeSample * 0.2f + surroundLeftSample * 0.05f + surroundRightSample * 0.05f, -0.9f), 0.9f);
    }

    return samples;
}