#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Cyber Drift
// In C minor, 128 BPM, ~3 minutes
std::vector<float> generateSong2(float songTime, int channels) {
    using namespace Instruments;
    std::vector<float> samples(channels, 0.0f);

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song2 - Cyber Drift\n");
        hasLogged = true;
    }

    // Fixed tempo: 128 BPM
    float bpm = 128.0f;
    float beatTime = 60.0f / bpm; // 0.46875 seconds
    float quarter = beatTime;
    float sixteenth = quarter / 4.0f;

    // C minor scale (4th octave)
    std::vector<float> notes = {
        261.63f, // C4
        293.66f, // D4
        311.13f, // Eb4
        349.23f, // F4
        392.00f, // G4
        415.30f, // Ab4
        466.16f  // Bb4
    };

    // Note collections
    std::vector<std::tuple<float, float, float>> kickNotes;
    std::vector<std::tuple<float, float, float>> snareNotes;
    std::vector<std::tuple<float, float, float>> synthArpNotes;
    std::vector<std::tuple<float, float, float>> bassNotes;

    // Kick: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        kickNotes.emplace_back(t, 130.81f, quarter); // C3
    }

    // Snare: Every other quarter note (off-beat)
    for (float t = quarter; t < 180.0f; t += 2.0f * quarter) {
        snareNotes.emplace_back(t, 0.0f, quarter);
    }

    // SynthArp: Starts at 10s, sixteenth-note arpeggios
    for (float t = 10.0f; t < 180.0f; t += quarter) {
        synthArpNotes.emplace_back(t + 0.0f * sixteenth, notes[0], sixteenth); // C4
        synthArpNotes.emplace_back(t + 1.0f * sixteenth, notes[2], sixteenth); // Eb4
        synthArpNotes.emplace_back(t + 2.0f * sixteenth, notes[4], sixteenth); // G4
        synthArpNotes.emplace_back(t + 3.0f * sixteenth, notes[2], sixteenth); // Eb4
    }

    // Bass: Starts at 20s, quarter-note bassline
    for (float t = 20.0f; t < 180.0f; t += 4.0f * quarter) {
        bassNotes.emplace_back(t + 0.0f * quarter, notes[0], quarter); // C4
        bassNotes.emplace_back(t + 1.0f * quarter, notes[4], quarter); // G4
        bassNotes.emplace_back(t + 2.0f * quarter, notes[2], quarter); // Eb4
        bassNotes.emplace_back(t + 3.0f * quarter, notes[0], quarter); // C4
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

    // SynthArp: Front and center
    for (const auto& [start, freq, dur] : synthArpNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateSynthArp(noteT, freq, dur) * 0.35f;
            leftSample += wave * 0.4f;
            rightSample += wave * 0.4f;
            centerSample += wave * 0.2f;
        }
    }

    // Bass: LFE and center
    for (const auto& [start, freq, dur] : bassNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateBass(noteT, freq, dur) * 0.4f;
            lfeSample += wave * 0.6f;
            centerSample += wave * 0.3f;
            leftSample += wave * 0.05f;
            rightSample += wave * 0.05f;
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