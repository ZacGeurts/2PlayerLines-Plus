#include "instruments.h"
#include <vector>
#include <cmath>
#include <cstdio>

// Techno: Dark Horizon
// In A minor, 140 BPM, ~3 minutes
float generateSong5(float t, float songTime) {
    using namespace Instruments;
    float sample = 0.0f;

    // Log message when song starts
    static bool hasLogged = false;
    if (songTime < 0.01f && !hasLogged) {
        printf("Song5 - Dark Horizon\n");
        hasLogged = true;
    }

    // Fixed tempo: 140 BPM
    float bpm = 140.0f;
    float beatTime = 60.0f / bpm;
    float quarter = beatTime;
    float eighth = quarter / 2.0f;
    float sixteenth = quarter / 4.0f;

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
    std::vector<std::tuple<float, float, float>> kickNotes;
    std::vector<std::tuple<float, float, float>> clapNotes;
    std::vector<std::tuple<float, float, float>> synthArpNotes;
    std::vector<std::tuple<float, float, float>> marimbaNotes;

    // Kick: Every quarter note
    for (float t = 0.0f; t < 180.0f; t += quarter) {
        kickNotes.emplace_back(t, 220.00f, quarter); // A3
    }

    // Clap: Every other quarter note
    for (float t = quarter; t < 180.0f; t += 2.0f * quarter) {
        clapNotes.emplace_back(t, 0.0f, quarter);
    }

    // SynthArp: Starts at 15s, sixteenth-note arpeggios
    for (float t = 15.0f; t < 180.0f; t += quarter) {
        synthArpNotes.emplace_back(t + 0.0f * sixteenth, notes[0], sixteenth); // A4
        synthArpNotes.emplace_back(t + 1.0f * sixteenth, notes[2], sixteenth); // C5
        synthArpNotes.emplace_back(t + 2.0f * sixteenth, notes[4], sixteenth); // E5
        synthArpNotes.emplace_back(t + 3.0f * sixteenth, notes[2], sixteenth); // C5
    }

    // Marimba: Starts at 30s, eighth-note melody
    for (float t = 30.0f; t < 180.0f; t += quarter) {
        marimbaNotes.emplace_back(t + 0.0f * eighth, notes[0], eighth); // A4
        marimbaNotes.emplace_back(t + 1.0f * eighth, notes[4], eighth); // E5
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

    // Marimba: Front and center
    for (const auto& [start, freq, dur] : marimbaNotes) {
        if (songTime >= start && songTime < start + dur) {
            float noteT = songTime - start;
            float wave = generateMarimba(noteT, freq, dur) * 0.3f;
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