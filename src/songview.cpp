// songview.cpp
// This is not free software and requires royalties for commercial use.
// Contact: https://github.com/ZacGeurts

#include "songview.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <GL/glu.h>

SongView::SongView(const std::string& filename) 
    : running(true), currentView(ViewMode::NOTATION), currentTime(0.0f), playing(false), 
      audioDevice(0), spectrogramData(nullptr), fftPlan(nullptr), 
      spectrogramWidth(1024), spectrogramHeight(512), showHelp(false) {
    initSDL();
    initOpenGL();
    loadSong(filename);
    setupAudio();
    generateWaveform();
    generateSpectrogram();
    playbackState.currentTime = 0.0f;
    playbackState.nextNoteIndices = std::vector<size_t>(song.parts.size(), 0);
    playbackState.reverbs = std::vector<AudioUtils::Reverb>(song.parts.size());
    playbackState.distortions = std::vector<AudioUtils::Distortion>(song.parts.size());
    playbackState.activeNotes = std::vector<std::vector<PlaybackState::ActiveNote>>(song.parts.size());
    for (size_t i = 0; i < song.parts.size(); ++i) {
        auto& part = song.parts[i];
        playbackState.reverbs[i] = AudioUtils::Reverb(part.reverbDelay, part.reverbDecay, part.reverbMixFactor);
        playbackState.distortions[i] = AudioUtils::Distortion(part.distortionDrive, part.distortionThreshold);
    }
    editState = {0, 0, false};
}

SongView::~SongView() {
    if (spectrogramData) {
        fftw_free(spectrogramData);
        fftw_destroy_plan(fftPlan);
    }
    if (audioDevice) SDL_CloseAudioDevice(audioDevice);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void SongView::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        throw std::runtime_error("SDL initialization failed");
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    window = SDL_CreateWindow("SongView", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                             1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        throw std::runtime_error("Window creation failed");
    }
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_Log("OpenGL context creation failed: %s", SDL_GetError());
        throw std::runtime_error("OpenGL context creation failed");
    }
    SDL_GL_SetSwapInterval(1);
}

void SongView::initOpenGL() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1280, 0, 720);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SongView::loadSong(const std::string& filename) {
    std::ifstream in(filename);
    if (!in || in.peek() == std::ifstream::traits_type::eof()) {
        SDL_Log("Cannot open song file: %s", filename.c_str());
        throw std::runtime_error("Cannot open song file");
    }

    song.bpm = 120.0f;
    song.rootFreq = 440.0f;
    song.scaleName = "major";
    song.duration = 180.0f;
    song.channels = 6;
    song.title = "Untitled";
    song.genres = "Unknown";

    std::string line;
    Part currentPart;
    bool inPart = false, inNotes = false, inPanAutomation = false, 
         inVolumeAutomation = false, inReverbMixAutomation = false;
    size_t expectedNotes = 0;

    while (std::getline(in, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "Song:") {
            std::getline(ss, song.title);
            song.title.erase(0, song.title.find_first_not_of(" \t"));
        } else if (token == "Section:") {
            Section section;
            ss >> section.name >> section.startTime >> section.endTime;
            std::string progressLabel, templateLabel;
            ss >> progressLabel >> section.progress >> templateLabel >> section.templateName;
            if (std::isfinite(section.startTime) && std::isfinite(section.endTime) && 
                section.startTime >= 0.0f && section.endTime > section.startTime) {
                song.sections.push_back(section);
            }
        } else if (token == "Part:") {
            if (!currentPart.instrument.empty()) song.parts.push_back(currentPart);
            currentPart = Part();
            std::getline(ss, currentPart.sectionName);
            currentPart.sectionName.erase(0, currentPart.sectionName.find_first_not_of(" \t"));
            inPart = true;
            inNotes = inPanAutomation = inVolumeAutomation = inReverbMixAutomation = false;
        } else if (inPart && token == "Instrument:") {
            std::getline(ss, currentPart.instrument);
            currentPart.instrument.erase(0, currentPart.instrument.find_first_not_of(" \t"));
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
        } else if (inPart && token == "ReverbMixFactor:") {
            ss >> currentPart.reverbMixFactor;
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
            Note note;
            std::string phonemeLabel, openLabel, volLabel, velLabel;
            ss >> note.freq >> note.duration >> note.startTime >> phonemeLabel >> note.phoneme 
               >> openLabel >> note.open >> volLabel >> note.volume >> velLabel >> note.velocity;
            if (std::isfinite(note.startTime) && std::isfinite(note.freq) && note.duration > 0.0f) {
                currentPart.notes.push_back(note);
            }
        } else if (inPart && token == "PanAutomation:") {
            size_t expectedPoints;
            ss >> expectedPoints;
            inPanAutomation = true;
            inNotes = inVolumeAutomation = inReverbMixAutomation = false;
        } else if (inPart && inPanAutomation && token == "PanPoint:") {
            float time, value;
            ss >> time >> value;
            currentPart.panAutomation.emplace_back(time, value);
        } else if (inPart && token == "VolumeAutomation:") {
            size_t expectedPoints;
            ss >> expectedPoints;
            inVolumeAutomation = true;
            inNotes = inPanAutomation = inReverbMixAutomation = false;
        } else if (inPart && inVolumeAutomation && token == "VolumePoint:") {
            float time, value;
            ss >> time >> value;
            currentPart.volumeAutomation.emplace_back(time, value);
        } else if (inPart && token == "ReverbMixAutomation:") {
            size_t expectedPoints;
            ss >> expectedPoints;
            inReverbMixAutomation = true;
            inNotes = inPanAutomation = inVolumeAutomation = false;
        } else if (inPart && inReverbMixAutomation && token == "ReverbMixPoint:") {
            float time, value;
            ss >> time >> value;
            currentPart.reverbMixAutomation.emplace_back(time, value);
        }
    }
    if (!currentPart.instrument.empty()) song.parts.push_back(currentPart);
    in.close();

    if (song.sections.empty()) song.sections.emplace_back("Default", 0.0f, song.duration, 0.0f);
    SDL_Log("Loaded song: %s, Title: %s, Parts: %zu, Sections: %zu", 
            filename.c_str(), song.title.c_str(), song.parts.size(), song.sections.size());
}

void SongView::setupAudio() {
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_F32;
    spec.channels = song.channels;
    spec.samples = 1024;
    spec.callback = audioCallback;
    spec.userdata = this;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (audioDevice == 0) {
        SDL_Log("Failed to open audio device: %s, trying stereo", SDL_GetError());
        spec.channels = 2;
        song.channels = 2;
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }
    if (audioDevice == 0) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        throw std::runtime_error("Audio device initialization failed");
    }
}

void SongView::generateWaveform() {
    waveformData.clear();
    float sampleRate = 44100.0f;
    size_t numSamples = static_cast<size_t>(song.duration * sampleRate);
    waveformData.resize(numSamples, 0.0f);

    for (const auto& part : song.parts) {
        for (const auto& note : part.notes) {
            size_t startSample = static_cast<size_t>(note.startTime * sampleRate);
            size_t durationSamples = static_cast<size_t>(note.duration * sampleRate);
            const auto& samples = Instruments::sampleManager.getSample(
                part.instrument, sampleRate, note.freq, note.duration, getPhonemeIndex(note.phoneme), note.open);
            for (size_t i = 0; i < durationSamples && i < samples.size() && startSample + i < numSamples; ++i) {
                waveformData[startSample + i] += samples[i] * note.volume * note.velocity;
            }
        }
    }
}

void SongView::generateSpectrogram() {
    float sampleRate = 44100.0f;
    int windowSize = 2048;
    spectrogramWidth = static_cast<int>(song.duration * sampleRate / (windowSize / 2));
    spectrogramHeight = windowSize / 2;
    spectrogramData = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * windowSize * spectrogramWidth);
    double* in = (double*)fftw_malloc(sizeof(double) * windowSize);
    fftPlan = fftw_plan_dft_r2c_1d(windowSize, in, spectrogramData, FFTW_ESTIMATE);

    for (int t = 0; t < spectrogramWidth; ++t) {
        std::fill(in, in + windowSize, 0.0);
        size_t sampleStart = t * (windowSize / 2);
        for (size_t i = 0; i < static_cast<size_t>(windowSize) && sampleStart + i < waveformData.size(); ++i) {
            in[i] = waveformData[sampleStart + i];
        }
        fftw_execute(fftPlan);
        for (int f = 0; f < spectrogramHeight; ++f) {
            spectrogramData[t * windowSize + f][0] = spectrogramData[f][0];
            spectrogramData[t * windowSize + f][1] = spectrogramData[f][1];
        }
    }
    fftw_free(in);
}

void SongView::run() {
    while (running) {
        handleEvents();
        render();
        SDL_GL_SwapWindow(window);
        SDL_Delay(10);
    }
}

void SongView::render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    switch (currentView) {
        case ViewMode::NOTATION: renderNotation(); break;
        case ViewMode::TABLATURE: renderTablature(); break;
        case ViewMode::WAVEFORM: renderWaveform(); break;
        case ViewMode::SPECTROGRAM: renderSpectrogram(); break;
    }

    // Render playback cursor
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    float x = (currentTime / song.duration) * 1280.0f;
    glVertex2f(x, 0.0f);
    glVertex2f(x, 720.0f);
    glEnd();

    if (showHelp) {
        renderHelp();
    }
}

void SongView::renderNotation() {
    glColor3f(1.0f, 1.0f, 1.0f);
    float yStep = 720.0f / 12; // 12 semitones for one octave
    float xScale = 1280.0f / song.duration;

    for (size_t p = 0; p < song.parts.size(); ++p) {
        const auto& part = song.parts[p];
        for (size_t i = 0; i < part.notes.size(); ++i) {
            const auto& note = part.notes[i];
            float midiNote = 69.0f + 12.0f * log2(note.freq / 440.0f);
            float y = (midiNote - 60.0f) * yStep + 360.0f; // Center around middle C
            float xStart = note.startTime * xScale;
            float xEnd = (note.startTime + note.duration) * xScale;

            if (editState.isEditing && editState.selectedPart == p && editState.selectedNote == i) {
                glColor3f(1.0f, 1.0f, 0.0f); // Highlight selected note
            } else {
                glColor3f(1.0f, 1.0f, 1.0f);
            }

            glBegin(GL_QUADS);
            glVertex2f(xStart, y - 5.0f);
            glVertex2f(xEnd, y - 5.0f);
            glVertex2f(xEnd, y + 5.0f);
            glVertex2f(xStart, y + 5.0f);
            glEnd();
        }
    }
}

void SongView::renderTablature() {
    glColor3f(1.0f, 1.0f, 1.0f);
    float yStep = 720.0f / 6; // 6 strings for guitar-like tablature
    float xScale = 1280.0f / song.duration;

    for (size_t p = 0; p < song.parts.size(); ++p) {
        const auto& part = song.parts[p];
        if (part.instrument != "guitar" && part.instrument != "bass") continue;

        for (size_t i = 0; i < part.notes.size(); ++i) {
            const auto& note = part.notes[i];
            float midiNote = 69.0f + 12.0f * log2(note.freq / 440.0f);
            int fret = static_cast<int>(midiNote - 40.0f); // Simplified fret calculation
            if (fret < 0 || fret > 24) continue;
            int string = fret % 6;
            float y = (5 - string) * yStep + 100.0f;
            float xStart = note.startTime * xScale;

            if (editState.isEditing && editState.selectedPart == p && editState.selectedNote == i) {
                glColor3f(1.0f, 1.0f, 0.0f);
            } else {
                glColor3f(1.0f, 1.0f, 1.0f);
            }

            glBegin(GL_QUADS);
            glVertex2f(xStart, y - 5.0f);
            glVertex2f(xStart + 20.0f, y - 5.0f);
            glVertex2f(xStart + 20.0f, y + 5.0f);
            glVertex2f(xStart, y + 5.0f);
            glEnd();
        }
    }

    // Draw strings
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_LINES);
    for (int i = 0; i < 6; ++i) {
        float y = i * yStep + 100.0f;
        glVertex2f(0.0f, y);
        glVertex2f(1280.0f, y);
    }
    glEnd();
}

void SongView::renderWaveform() {
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINE_STRIP);
    for (size_t i = 0; i < waveformData.size(); ++i) {
        float x = (i / static_cast<float>(waveformData.size())) * 1280.0f;
        float y = 360.0f + waveformData[i] * 200.0f;
        glVertex2f(x, y);
    }
    glEnd();
}

void SongView::renderSpectrogram() {
    glBegin(GL_QUADS);
    for (int t = 0; t < spectrogramWidth; ++t) {
        for (int f = 0; f < spectrogramHeight; ++f) {
            float mag = sqrt(spectrogramData[t * spectrogramWidth + f][0] * spectrogramData[t * spectrogramWidth + f][0] +
                             spectrogramData[t * spectrogramWidth + f][1] * spectrogramData[t * spectrogramWidth + f][1]);
            mag = std::min(mag / 100.0f, 1.0f);
            glColor3f(mag, mag * 0.5f, mag * 0.2f);
            float x = (t / static_cast<float>(spectrogramWidth)) * 1280.0f;
            float y = (f / static_cast<float>(spectrogramHeight)) * 720.0f;
            float dx = 1280.0f / spectrogramWidth;
            float dy = 720.0f / spectrogramHeight;
            glVertex2f(x, y);
            glVertex2f(x + dx, y);
            glVertex2f(x + dx, y + dy);
            glVertex2f(x, y + dy);
        }
    }
    glEnd();
}

void SongView::renderHelp() {
    // Semi-transparent background
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(200.0f, 100.0f);
    glVertex2f(1080.0f, 100.0f);
    glVertex2f(1080.0f, 620.0f);
    glVertex2f(200.0f, 620.0f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    float y = 580.0f;
    const float lineHeight = 40.0f;

    struct { const char* key; const char* desc; } commands[] = {
        {"1", "Notation View"},
        {"2", "Tablature View"},
        {"3", "Waveform View"},
        {"4", "Spectrogram View"},
        {"Space", "Toggle Playback"},
        {"E", "Toggle Edit Mode"},
        {"S", "Save Song"},
        {"H", "Toggle Help"},
        {"Esc", "Quit"},
        {"Left/Right", "Select Note (Edit Mode)"},
        {"Up/Down", "Select Part (Edit Mode)"},
        {"+/-", "Adjust Pitch (Edit Mode)"}
    };

    for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); ++i) {
        glBegin(GL_QUADS);
        glVertex2f(250.0f, y - 5.0f);
        glVertex2f(350.0f, y - 5.0f);
        glVertex2f(350.0f, y + 5.0f);
        glVertex2f(250.0f, y + 5.0f);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(400.0f, y - 5.0f);
        glVertex2f(1000.0f, y - 5.0f);
        glVertex2f(1000.0f, y + 5.0f);
        glVertex2f(400.0f, y + 5.0f);
        glEnd();
        y -= lineHeight;
    }
}

void SongView::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_h:
                    showHelp = !showHelp;
                    break;
                case SDLK_1:
                    currentView = ViewMode::NOTATION;
                    break;
                case SDLK_2:
                    currentView = ViewMode::TABLATURE;
                    break;
                case SDLK_3:
                    currentView = ViewMode::WAVEFORM;
                    break;
                case SDLK_4:
                    currentView = ViewMode::SPECTROGRAM;
                    break;
                case SDLK_SPACE:
                    playing = !playing;
                    SDL_PauseAudioDevice(audioDevice, !playing);
                    break;
                case SDLK_e:
                    if (!song.parts.empty()) {
                        editState.isEditing = !editState.isEditing;
                        editState.selectedPart = std::min(editState.selectedPart, song.parts.size() - 1);
                        editState.selectedNote = std::min(editState.selectedNote, song.parts[editState.selectedPart].notes.size() - 1);
                    }
                    break;
                case SDLK_s:
                    saveSong();
                    break;
                case SDLK_LEFT:
                    if (editState.isEditing && editState.selectedNote > 0) {
                        editState.selectedNote--;
                    }
                    break;
                case SDLK_RIGHT:
                    if (editState.isEditing && editState.selectedNote < song.parts[editState.selectedPart].notes.size() - 1) {
                        editState.selectedNote++;
                    }
                    break;
                case SDLK_UP:
                    if (editState.isEditing && editState.selectedPart > 0) {
                        editState.selectedPart--;
                        editState.selectedNote = 0;
                    }
                    break;
                case SDLK_DOWN:
                    if (editState.isEditing && editState.selectedPart < song.parts.size() - 1) {
                        editState.selectedPart++;
                        editState.selectedNote = 0;
                    }
                    break;
                case SDLK_PLUS:
                case SDLK_EQUALS:
                    if (editState.isEditing && !song.parts.empty()) {
                        auto& note = song.parts[editState.selectedPart].notes[editState.selectedNote];
                        note.freq *= pow(2.0f, 1.0f / 12.0f);
                    }
                    break;
                case SDLK_MINUS:
                    if (editState.isEditing && !song.parts.empty()) {
                        auto& note = song.parts[editState.selectedPart].notes[editState.selectedNote];
                        note.freq /= pow(2.0f, 1.0f / 12.0f);
                    }
                    break;
            }
        }
    }
}

void SongView::saveSong() {
    std::ofstream out("output.song");
    if (!out) {
        SDL_Log("Failed to save song file");
        return;
    }

    out << "Song: " << song.title << "\n";
    out << "Sections: " << song.sections.size() << "\n";
    for (const auto& section : song.sections) {
        out << "Section: " << section.name << " " << section.startTime << " " 
            << section.endTime << " Progress: " << section.progress << " Template: " 
            << section.templateName << "\n";
    }
    out << "Parts: " << song.parts.size() << "\n";
    for (const auto& part : song.parts) {
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
                << " Phoneme: " << note.phoneme << " Open: " << note.open 
                << " Volume: " << note.volume << " Velocity: " << note.velocity << "\n";
        }
        if (!part.panAutomation.empty()) {
            out << "PanAutomation: " << part.panAutomation.size() << "\n";
            for (const auto& point : part.panAutomation) {
                out << "PanPoint: " << point.first << " " << point.second << "\n";
            }
        }
        if (!part.volumeAutomation.empty()) {
            out << "VolumeAutomation: " << part.volumeAutomation.size() << "\n";
            for (const auto& point : part.volumeAutomation) {
                out << "VolumePoint: " << point.first << " " << point.second << "\n";
            }
        }
        if (!part.reverbMixAutomation.empty()) {
            out << "ReverbMixAutomation: " << part.reverbMixAutomation.size() << "\n";
            for (const auto& point : part.reverbMixAutomation) {
                out << "ReverbMixPoint: " << point.first << " " << point.second << "\n";
            }
        }
    }
    out.close();
    SDL_Log("Song saved to output.song");
}

void SongView::audioCallback(void* userdata, Uint8* stream, int len) {
    SongView* viewer = static_cast<SongView*>(userdata);
    float* output = reinterpret_cast<float*>(stream);
    bool isStereo = viewer->song.channels == 2;
    int numChannels = isStereo ? 2 : 6;
    int numSamples = len / sizeof(float) / numChannels;
    float sampleRate = 44100.0f;

    std::fill(output, output + numSamples * numChannels, 0.0f);

    for (int i = 0; i < numSamples; ++i) {
        float t = viewer->playbackState.currentTime + i / sampleRate;
        float L = 0.0f, R = 0.0f, C = 0.0f, LFE = 0.0f, Ls = 0.0f, Rs = 0.0f;

        if (t > viewer->song.duration + 3.0f) {
            viewer->playing = false;
            viewer->currentTime = 0.0f;
            viewer->playbackState.currentTime = 0.0f;
            SDL_PauseAudioDevice(viewer->audioDevice, 1);
            break;
        }

        for (size_t partIdx = 0; partIdx < viewer->song.parts.size(); ++partIdx) {
            auto& part = viewer->song.parts[partIdx];
            auto& nextIdx = viewer->playbackState.nextNoteIndices[partIdx];
            auto& active = viewer->playbackState.activeNotes[partIdx];

            float pan = viewer->interpolateAutomation(t, part.panAutomation, part.pan);
            float volume = viewer->interpolateAutomation(t, part.volumeAutomation, 0.5f);
            float reverbMix = viewer->interpolateAutomation(t, part.reverbMixAutomation, part.reverbMix);

            float leftGain = (pan <= 0.0f) ? 1.0f : 1.0f - pan;
            float rightGain = (pan >= 0.0f) ? 1.0f : 1.0f + pan;
            float surroundGain = 0.5f * (leftGain + rightGain);
            float centerWeight = (part.instrument == "voice") ? 0.8f : 0.3f;
            float lfeWeight = (part.instrument == "subbass" || part.instrument == "kick") ? 0.5f : 0.1f;
            float sideWeight = (part.instrument == "guitar" || part.instrument == "syntharp") ? 0.6f : 0.4f;

            while (nextIdx < part.notes.size() && part.notes[nextIdx].startTime <= t && active.size() < 16) {
                const auto& note = part.notes[nextIdx];
                float tailDuration = viewer->getTailDuration(part.instrument);
                active.push_back({nextIdx, note.startTime, note.startTime + note.duration + tailDuration});
                ++nextIdx;
            }

            for (auto it = active.begin(); it != active.end();) {
                const auto& note = part.notes[it->noteIndex];
                if (t <= it->endTime) {
                    float noteTime = t - note.startTime;
                    size_t sampleIndex = static_cast<size_t>(noteTime * sampleRate);
                    const std::vector<float>& samples = Instruments::sampleManager.getSample(
                        part.instrument, sampleRate, note.freq, note.duration, viewer->getPhonemeIndex(note.phoneme), note.open);
                    float sample = (sampleIndex < samples.size()) ? samples[sampleIndex] : 0.0f;
                    sample *= note.volume * note.velocity * volume;
                    if (part.useDistortion) {
                        sample = viewer->playbackState.distortions[partIdx].process(sample);
                    }
                    if (part.useReverb) {
                        sample = viewer->playbackState.reverbs[partIdx].process(sample * (1.0f - reverbMix)) + 
                                 sample * reverbMix;
                    }

                    L += sample * leftGain * sideWeight;
                    R += sample * rightGain * sideWeight;
                    C += sample * centerWeight;
                    LFE += sample * lfeWeight;
                    Ls += sample * surroundGain * sideWeight;
                    Rs += sample * surroundGain * sideWeight;

                    ++it;
                } else {
                    it = active.erase(it);
                }
            }
        }

        if (isStereo) {
            float L_out = L + 0.707f * C + 0.707f * LFE + 0.5f * Ls;
            float R_out = R + 0.707f * C + 0.707f * LFE + 0.5f * Rs;
            output[i * 2 + 0] = std::max(-1.0f, std::min(1.0f, L_out));
            output[i * 2 + 1] = std::max(-1.0f, std::min(1.0f, R_out));
        } else {
            output[i * 6 + 0] = std::max(-1.0f, std::min(1.0f, L));
            output[i * 6 + 1] = std::max(-1.0f, std::min(1.0f, R));
            output[i * 6 + 2] = std::max(-1.0f, std::min(1.0f, C));
            output[i * 6 + 3] = std::max(-1.0f, std::min(1.0f, LFE));
            output[i * 6 + 4] = std::max(-1.0f, std::min(1.0f, Ls));
            output[i * 6 + 5] = std::max(-1.0f, std::min(1.0f, Rs));
        }
    }

    viewer->playbackState.currentTime += numSamples / sampleRate;
    viewer->currentTime = viewer->playbackState.currentTime;
}

float SongView::interpolateAutomation(float t, const std::vector<std::pair<float, float>>& automation, float defaultValue) {
    if (automation.empty()) return defaultValue;
    if (t <= automation.front().first) return automation.front().second;
    if (t >= automation.back().first) return automation.back().second;
    for (size_t i = 1; i < automation.size(); ++i) {
        if (t >= automation[i-1].first && t < automation[i].first) {
            float t0 = automation[i-1].first, t1 = automation[i].first;
            float v0 = automation[i-1].second, v1 = automation[i].second;
            return v0 + (v1 - v0) * (t - t0) / (t1 - t0);
        }
    }
    return defaultValue;
}

float SongView::getTailDuration(const std::string& instrument) {
    if (instrument == "cymbal") return 2.0f;
    if (instrument == "guitar") return 1.5f;
    if (instrument == "syntharp") return 1.2f;
    if (instrument == "subbass") return 0.8f;
    if (instrument == "kick") return 0.5f;
    if (instrument == "snare") return 0.6f;
    if (instrument == "piano") return 2.0f;
    if (instrument == "violin") return 2.5f;
    if (instrument == "cello") return 3.0f;
    if (instrument == "marimba") return 1.0f;
    if (instrument == "steelguitar") return 1.8f;
    if (instrument == "sitar") return 2.0f;
    return 1.5f;
}

int SongView::getPhonemeIndex(const std::string& phoneme) {
    if (phoneme.empty()) return -1;
    static const std::map<std::string, int> phonemeMap = {
        {"aa", 0}, {"ae", 1}, {"ah", 2}, {"ao", 3}, {"aw", 4},
        {"ay", 5}, {"eh", 6}, {"er", 7}, {"ey", 8}, {"ih", 9},
        {"iy", 10}, {"ow", 11}, {"oy", 12}, {"uh", 13}, {"uw", 14}
    };
    auto it = phonemeMap.find(phoneme);
    return (it != phonemeMap.end()) ? it->second : -1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./songview <filename>.song" << std::endl;
        return 1;
    }

    try {
        SongView viewer(argv[1]);
        viewer.run();
    } catch (const std::exception& e) {
        SDL_Log("Error: %s", e.what());
        return 1;
    }

    return 0;
}