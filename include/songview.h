// songview.h
// This is not free software and requires royalties for commercial use.
// Contact: https://github.com/ZacGeurts

#ifndef SONGVIEW_H
#define SONGVIEW_H

#include "instruments.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <vector>
#include <string>
#include <fftw3.h>

class SongView {
public:
    SongView(const std::string& filename);
    ~SongView();

    void run();

private:
    struct Note {
        float freq;
        float duration;
        float startTime;
        std::string phoneme;
        bool open;
        float volume;
        float velocity;
    };

    struct Section {
        std::string name;
        float startTime;
        float endTime;
        float progress;
        std::string templateName;
        Section() : startTime(0.0f), endTime(0.0f), progress(0.0f) {}
        Section(const std::string& n, float st, float et, float p, const std::string& tn = "")
            : name(n), startTime(st), endTime(et), progress(p), templateName(tn) {}
    };

    struct Part {
        std::string sectionName;
        std::string instrument;
        float pan;
        float reverbMix;
        bool useReverb;
        float reverbDelay;
        float reverbDecay;
        float reverbMixFactor;
        bool useDistortion;
        float distortionDrive;
        float distortionThreshold;
        std::vector<Note> notes;
        std::vector<std::pair<float, float>> panAutomation;
        std::vector<std::pair<float, float>> volumeAutomation;
        std::vector<std::pair<float, float>> reverbMixAutomation;
    };

    struct SongData {
        float bpm, duration, rootFreq;
        std::string scaleName, title, genres;
        std::vector<Section> sections;
        std::vector<Part> parts;
        int channels;
    };

    enum class ViewMode {
        NOTATION, TABLATURE, WAVEFORM, SPECTROGRAM
    };

    struct PlaybackState {
        struct ActiveNote {
            size_t noteIndex;
            float startTime;
            float endTime;
        };
        float currentTime;
        std::vector<size_t> nextNoteIndices;
        std::vector<AudioUtils::Reverb> reverbs;
        std::vector<AudioUtils::Distortion> distortions;
        std::vector<std::vector<ActiveNote>> activeNotes;
    };

    SongData song;
    SDL_Window* window;
    SDL_GLContext glContext;
    bool running;
    ViewMode currentView;
    float currentTime;
    bool playing;
    SDL_AudioDeviceID audioDevice;
    std::vector<float> waveformData;
    fftw_complex* spectrogramData;
    fftw_plan fftPlan;
    int spectrogramWidth, spectrogramHeight;
    bool showHelp;
    PlaybackState playbackState;

    // Editing state
    struct EditState {
        size_t selectedPart;
        size_t selectedNote;
        bool isEditing;
    } editState;

    void initSDL();
    void initOpenGL();
    void loadSong(const std::string& filename);
    void setupAudio();
    void render();
    void renderNotation();
    void renderTablature();
    void renderWaveform();
    void renderSpectrogram();
    void renderHelp();
    void handleEvents();
    void saveSong();
    void generateWaveform();
    void generateSpectrogram();
    static void audioCallback(void* userdata, Uint8* stream, int len);
    float interpolateAutomation(float t, const std::vector<std::pair<float, float>>& automation, float defaultValue);
    float getTailDuration(const std::string& instrument);
    int getPhonemeIndex(const std::string& phoneme);
};

#endif // SONGVIEW_H