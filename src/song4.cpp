#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Orbital Pulse
// In F minor, 136 BPM, ~3 minutes
std::vector<float> generateSong4(float songTime, int channels) {
    using namespace Instruments;
    std::vector<float> samples(channels, 0.0f);

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song4 - Orbital Pulse\n");
        hasLogged = true;
    }

    // Fixed tempo: 136 BPM
    float bpm = 136.0f;
    float beatTime = 60.0f / bpm; // 0.44118 seconds
    float quarter = beatTime;
    float sixteenth = quarter / 4.0f;

    // F minor scale (4th octave)
    std::vector<float> notes = {
        349.23f, // F4
        392.00f, // G4
        415.30f, // Ab4
        466.16f, // Bb4
        523.25f, // C5
        554.37f, // Db5
        622.25f  // Eb5
    };

    // Note collections
    std::vector<std::tuple<float, float, float>> kickNotes;
    std::vector<std::tuple<float, float, float>> snareNotes;
    std::vector<std::tuple<float, float, float>> synthArpNotes;
    std::vector<std::tuple<float, float, float>> pianoNotes;

    // Kick: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        kickNotes.emplace_back(t, 174.61f, quarter); // F3
    }

    // Snare: Every other quarter note (off-beat)
    for (float t = quarter; t < 180.0f; t += 2.0f * quarter) {
        snareNotes.emplace_back(t, 0.0f, quarter);
    }

    // SynthArp: Starts at 15s, sixteenth-note arpeggios
    for (float t = 15.0f; t < 180.0f; t += quarter) {
        synthArpNotes.emplace_back(t + 0.0f * sixteenth, notes[0], sixteenth); // F4
        synthArpNotes.emplace_back(t + 1.0f * sixteenth, notes[2], sixteenth); // Ab4
        synthArpNotes.emplace_back(t + 2.0f * sixteenth, notes[4], sixteenth); // C5
        synthArpNotes.emplace_back(t + 3.0f * sixteenth, notes[2], sixteenth); // Ab4
    }

    // Piano: Starts at 30s, quarter-note melody
    for (float t = 30.0f; t < 180.0f; t += 4.0f * quarter) {
        pianoNotes.emplace_back(t + 0.0f * quarter, notes[0], quarter); // F4
        pianoNotes.emplace_back(t + 1.0f * quarter, notes[4], quarter); // C5
        pianoNotes.emplace_back(t + 2.0f * quarter, notes[3], quarter); // Bb4
        pianoNotes.emplace_back(t + 3.0f * quarter, notes[2], quarter); // Ab4
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