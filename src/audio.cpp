#include "audio.h"
#include "instruments.h"
#include <cmath>
#include <SDL2/SDL.h>
#include <algorithm>

AudioManager::AudioManager(const GameConfig& config)
    : boopDevice(0), explosionDevice(0), laserZapDevice(0), winnerVoiceDevice(0), technoLoopDevice(0),
      boopPlaying(false), explosionPlaying(false), laserZapPlaying(false), winnerVoicePlaying(false), technoLoopPlaying(false),
      boopData{0, &boopPlaying, &config, 0.0f, nullptr},
      explosionData{0, &explosionPlaying, &config, 0.0f, nullptr},
      laserZapData{0, &laserZapPlaying, &config, 0.0f, nullptr},
      winnerVoiceData{0, &winnerVoicePlaying, &config, 0.0f, nullptr},
      technoLoopData{0, &technoLoopPlaying, &config, 0.0f, this},
      technoSongId(0), technoChannels(2), config(config) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL audio: %s", SDL_GetError());
        return;
    }

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.samples = 1024;

    srand(static_cast<unsigned>(SDL_GetTicks()));

    // Sound effects: Stereo (2 channels)
    desired.channels = 2;
    desired.callback = boopCallback; desired.userdata = &boopData;
    boopDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (boopDevice == 0) SDL_Log("Failed to open boop audio device: %s", SDL_GetError());
    else { boopData.deviceId = boopDevice; SDL_Log("Boop device opened: ID=%u, channels=%d", boopDevice, obtained.channels); }

    desired.callback = explosionCallback; desired.userdata = &explosionData;
    explosionDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (explosionDevice == 0) SDL_Log("Failed to open explosion audio device: %s", SDL_GetError());
    else { explosionData.deviceId = explosionDevice; SDL_Log("Explosion device opened: ID=%u, channels=%d", explosionDevice, obtained.channels); }

    desired.callback = laserZapCallback; desired.userdata = &laserZapData;
    laserZapDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (laserZapDevice == 0) SDL_Log("Failed to open laser zap audio device: %s", SDL_GetError());
    else { laserZapData.deviceId = laserZapDevice; SDL_Log("Laser zap device opened: ID=%u, channels=%d", laserZapDevice, obtained.channels); }

    desired.callback = winnerVoiceCallback; desired.userdata = &winnerVoiceData;
    winnerVoiceDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (winnerVoiceDevice == 0) SDL_Log("Failed to open winner voice audio device: %s", SDL_GetError());
    else { winnerVoiceData.deviceId = winnerVoiceDevice; SDL_Log("Winner voice device opened: ID=%u, channels=%d", winnerVoiceDevice, obtained.channels); }

    // Techno loop: Try 5.1 (6 channels)
    desired.channels = 6;
    desired.callback = technoLoopCallback; desired.userdata = &technoLoopData;
    technoLoopDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (technoLoopDevice == 0) SDL_Log("Failed to open techno loop audio device: %s", SDL_GetError());
    else {
        technoLoopData.deviceId = technoLoopDevice;
        technoChannels = obtained.channels;
        SDL_Log("Techno loop device opened: ID=%u, channels=%d", technoLoopDevice, technoChannels);
    }
}

AudioManager::~AudioManager() {
    if (boopDevice != 0) { SDL_CloseAudioDevice(boopDevice); SDL_Log("Closed boop device"); }
    if (explosionDevice != 0) { SDL_CloseAudioDevice(explosionDevice); SDL_Log("Closed explosion device"); }
    if (laserZapDevice != 0) { SDL_CloseAudioDevice(laserZapDevice); SDL_Log("Closed laser zap device"); }
    if (winnerVoiceDevice != 0) { SDL_CloseAudioDevice(winnerVoiceDevice); SDL_Log("Closed winner voice device"); }
    if (technoLoopDevice != 0) { SDL_CloseAudioDevice(technoLoopDevice); SDL_Log("Closed techno loop device"); }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioManager::boopCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = static_cast<BoopAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100; spec.format = AUDIO_S16SYS; spec.channels = 2; spec.samples = 1024; // Fallback
    static bool loggedError = false;
    SDL_AudioStatus status = SDL_GetAudioDeviceStatus(data->deviceId);
    if (status != SDL_AUDIO_PLAYING || SDL_GetAudioDeviceSpec(static_cast<int>(data->deviceId), 0, &spec) != 0) {
        if (!loggedError) {
            SDL_Log("Boop callback: Invalid device ID=%u, status=%d, error=%s, using fallback spec", data->deviceId, status, SDL_GetError());
            loggedError = true;
        }
    } else {
        loggedError = false; // Reset if spec retrieval succeeds
    }
    int channels = spec.channels;
    int samples = len / sizeof(int16_t) / channels;
    if (!*(data->playing) || (data->config->BOOP_DURATION > 0 && data->t >= data->config->BOOP_DURATION)) {
        *(data->playing) = false; data->t = 0.0f; std::fill(buffer, buffer + samples * channels, 0); return;
    }
    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        float freq = 440.0f + 100.0f * sin(t * 2.0f * M_PI * 5.0f);
        int16_t sample = static_cast<int16_t>(32760.0f * sin(t * 2.0f * M_PI * freq));
        for (int ch = 0; ch < channels; ++ch) buffer[i * channels + ch] = sample;
    }
    data->t += samples / 44100.0f;
}

void AudioManager::explosionCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = static_cast<BoopAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100; spec.format = AUDIO_S16SYS; spec.channels = 2; spec.samples = 1024;
    static bool loggedError = false;
    SDL_AudioStatus status = SDL_GetAudioDeviceStatus(data->deviceId);
    if (status != SDL_AUDIO_PLAYING || SDL_GetAudioDeviceSpec(static_cast<int>(data->deviceId), 0, &spec) != 0) {
        if (!loggedError) {
            SDL_Log("Explosion callback: Invalid device ID=%u, status=%d, error=%s, using fallback spec", data->deviceId, status, SDL_GetError());
            loggedError = true;
        }
    } else {
        loggedError = false;
    }
    int channels = spec.channels;
    int samples = len / sizeof(int16_t) / channels;
    if (!*(data->playing) || (data->config->EXPLOSION_DURATION > 0 && data->t >= data->config->EXPLOSION_DURATION)) {
        *(data->playing) = false; data->t = 0.0f; std::fill(buffer, buffer + samples * channels, 0); return;
    }
    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        int16_t sample = static_cast<int16_t>((rand() % 32760) * (1.0f - t / data->config->EXPLOSION_DURATION));
        for (int ch = 0; ch < channels; ++ch) buffer[i * channels + ch] = sample;
    }
    data->t += samples / 44100.0f;
}

void AudioManager::laserZapCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = static_cast<BoopAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100; spec.format = AUDIO_S16SYS; spec.channels = 2; spec.samples = 1024;
    static bool loggedError = false;
    SDL_AudioStatus status = SDL_GetAudioDeviceStatus(data->deviceId);
    if (status != SDL_AUDIO_PLAYING || SDL_GetAudioDeviceSpec(static_cast<int>(data->deviceId), 0, &spec) != 0) {
        if (!loggedError) {
            SDL_Log("Laser zap callback: Invalid device ID=%u, status=%d, error=%s, using fallback spec", data->deviceId, status, SDL_GetError());
            loggedError = true;
        }
    } else {
        loggedError = false;
    }
    int channels = spec.channels;
    int samples = len / sizeof(int16_t) / channels;
    if (!*(data->playing) || (data->config->LASER_ZAP_DURATION > 0 && data->t >= data->config->LASER_ZAP_DURATION)) {
        *(data->playing) = false; data->t = 0.0f; std::fill(buffer, buffer + samples * channels, 0); return;
    }
    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        float freq = 880.0f - 600.0f * (t / data->config->LASER_ZAP_DURATION);
        int16_t sample = static_cast<int16_t>(32760.0f * sin(t * 2.0f * M_PI * freq) * (1.0f - t / data->config->LASER_ZAP_DURATION));
        for (int ch = 0; ch < channels; ++ch) buffer[i * channels + ch] = sample;
    }
    data->t += samples / 44100.0f;
}

void AudioManager::winnerVoiceCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = static_cast<BoopAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100; spec.format = AUDIO_S16SYS; spec.channels = 2; spec.samples = 1024;
    static bool loggedError = false;
    SDL_AudioStatus status = SDL_GetAudioDeviceStatus(data->deviceId);
    if (status != SDL_AUDIO_PLAYING || SDL_GetAudioDeviceSpec(static_cast<int>(data->deviceId), 0, &spec) != 0) {
        if (!loggedError) {
            SDL_Log("Winner voice callback: Invalid device ID=%u, status=%d, error=%s, using fallback spec", data->deviceId, status, SDL_GetError());
            loggedError = true;
        }
    } else {
        loggedError = false;
    }
    int channels = spec.channels;
    int samples = len / sizeof(int16_t) / channels;
    if (!*(data->playing) || (data->config->WINNER_VOICE_DURATION > 0 && data->t >= data->config->WINNER_VOICE_DURATION)) {
        *(data->playing) = false; data->t = 0.0f; std::fill(buffer, buffer + samples * channels, 0); return;
    }
    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        float freq1 = 200.0f + 100.0f * sin(t * 2.0f * M_PI * 2.0f);
        float freq2 = 800.0f + 200.0f * sin(t * 2.0f * M_PI * 3.0f);
        float amplitude = 0.8f * (1.0f - t / data->config->WINNER_VOICE_DURATION);
        int16_t sample = static_cast<int16_t>(32760.0f * amplitude * (0.5f * sin(t * 2.0f * M_PI * freq1) + 0.5f * sin(t * 2.0f * M_PI * freq2)));
        for (int ch = 0; ch < channels; ++ch) buffer[i * channels + ch] = sample;
    }
    data->t += samples / 44100.0f;
}

void AudioManager::technoLoopCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = static_cast<BoopAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / sizeof(int16_t) / data->manager->technoChannels;
    if (!*(data->playing)) {
        data->t = 0.0f; std::fill(buffer, buffer + samples * data->manager->technoChannels, 0); return;
    }

    int songId = data->manager->technoSongId;

    const float BPM = 130.0f;
    const float BEAT_TIME = 60.0f / BPM;
    const float BAR_TIME = BEAT_TIME * 4;
    const float MAIN_DURATION = BAR_TIME * 26;
    const float BUILD_DURATION = BAR_TIME * 13;
    const float BREAK_DURATION = BAR_TIME * 13;
    const float CHILL_DURATION = BAR_TIME * 13;
    const float CYCLE_DURATION = MAIN_DURATION + BUILD_DURATION + BREAK_DURATION + CHILL_DURATION;

    const float NOTES[] = {130.81f, 155.56f, 174.61f, 196.00f, 233.08f, 261.63f};
    const int ARP_PATTERN[] = {0, 1, 3, 5};
    const int ARP_LENGTH = 4;
    const float VOCAL_NOTES[] = {196.00f, 233.08f, 261.63f, 155.56f, 261.63f, 233.08f};
    const int VOCAL_LENGTH = 6;
    const int PHONEMES[] = {0, 1, 1, 2, 3, 4}; // k, ee, p, o, n, m
    const int CHORD_PROGRESSIONS[5][4] = {
        {0, 1, 3, 4}, {0, 3, 4, 1}, {0, 1, 5, 4}, {0, 3, 1, 4}, {0, 4, 3, 1}
    };

    static bool vocalActive = false;
    static int lastCycle = -1;
    int currentCycle = static_cast<int>(data->t / CYCLE_DURATION);
    if (currentCycle != lastCycle) {
        vocalActive = true;
        lastCycle = currentCycle;
    }

    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        float cycleTime = std::fmod(t, CYCLE_DURATION);
        float localTime, sectionProgress;
        int section;
        if (cycleTime < MAIN_DURATION) {
            section = 0; localTime = cycleTime; sectionProgress = localTime / MAIN_DURATION;
        } else if (cycleTime < MAIN_DURATION + BUILD_DURATION) {
            section = 1; localTime = cycleTime - MAIN_DURATION; sectionProgress = localTime / BUILD_DURATION;
        } else if (cycleTime < MAIN_DURATION + BUILD_DURATION + BREAK_DURATION) {
            section = 2; localTime = cycleTime - (MAIN_DURATION + BUILD_DURATION); sectionProgress = localTime / BREAK_DURATION;
        } else {
            section = 3; localTime = cycleTime - (MAIN_DURATION + BUILD_DURATION + BREAK_DURATION); sectionProgress = localTime / CHILL_DURATION;
        }

        float beatTime = std::fmod(localTime, BEAT_TIME);
        int beat = static_cast<int>(localTime / BEAT_TIME) % 4;
        float barTime = std::fmod(localTime, BAR_TIME);
        int bar = static_cast<int>(localTime / BAR_TIME);
        float eighthTime = std::fmod(localTime, BEAT_TIME / 2);

        float kickAmp = (beatTime < BEAT_TIME / 2) ? std::exp(-12.0f * beatTime) : 0.0f;
        float sidechain = 1.0f - 0.5f * kickAmp;

        float kickPan = 0.0f, bassPan = 0.0f, vocalPan = 0.0f;
        float hihatPan = (beat % 2 == 0) ? -0.2f : 0.2f;
        float arpPan = (bar % 2 == 0) ? -0.15f : 0.15f;
        float padPan = -0.3f, guitarPan = 0.3f;

        float channels[6] = {0.0f};
        float left = 0.0f, right = 0.0f, mono = 0.0f;

        if (section == 0) {
            float kick = (beatTime < BEAT_TIME / 2) ? Instruments::generateKick(beatTime, 50.0f) : 0.0f;
            float hihat = (eighthTime < BEAT_TIME / 4) ? Instruments::generateHiHat(eighthTime, 1500.0f, false) : 0.0f;
            float snare = (beat == 1 || beat == 3) ? Instruments::generateSnare(beatTime) : 0.0f;
            float bassFreq = NOTES[CHORD_PROGRESSIONS[songId][bar % 4]];
            float bass = Instruments::generateBass(beatTime, bassFreq) * sidechain;
            float subBass = Instruments::generateSubBass(beatTime, bassFreq / 2) * sidechain;
            float arpTime = std::fmod(localTime, BEAT_TIME / 4);
            int arpIndex = (static_cast<int>(localTime / (BEAT_TIME / 4)) % ARP_LENGTH);
            float arpFreq = NOTES[ARP_PATTERN[arpIndex]];
            float synthArp = (eighthTime < BEAT_TIME / 4) ? Instruments::generateSynthArp(arpTime, arpFreq) : 0.0f;
            float leadFreq = NOTES[3];
            if (barTime > BAR_TIME / 2) leadFreq = NOTES[5];
            float leadSynth = (beat % 2 == 1) ? Instruments::generateLeadSynth(beatTime, leadFreq) : 0.0f;
            float vocal = 0.0f;
            if (vocalActive && bar >= 8 && bar < 12) {
                float vocalTime = std::fmod(localTime, BEAT_TIME);
                int vocalIndex = (static_cast<int>(localTime / BEAT_TIME) % VOCAL_LENGTH);
                int phonemeIndex = PHONEMES[vocalIndex];
                vocal = Instruments::generateVocal(vocalTime, VOCAL_NOTES[vocalIndex], phonemeIndex) * 0.5f;
            }
            if (data->manager->technoChannels == 6) {
                channels[0] = hihat * 0.7f + synthArp * 0.6f;
                channels[1] = hihat * 0.3f + synthArp * 0.4f;
                channels[2] = kick + bass * 0.5f + subBass * 0.5f + snare + leadSynth + vocal;
                channels[3] = kick * 0.5f + subBass * 0.7f;
                channels[4] = hihat * 0.5f + synthArp * 0.5f;
                channels[5] = hihat * 0.3f + synthArp * 0.3f;
            } else if (data->manager->technoChannels == 2) {
                left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + snare + bass * (1.0f - bassPan) +
                       subBass * (1.0f - bassPan) + synthArp * (1.0f - arpPan) + leadSynth + vocal * (1.0f - vocalPan);
                right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + snare + bass * (1.0f + bassPan) +
                        subBass * (1.0f + bassPan) + synthArp * (1.0f + arpPan) + leadSynth + vocal * (1.0f + vocalPan);
            } else {
                mono = kick + hihat + snare + bass + subBass + synthArp + leadSynth + vocal;
            }
        } else if (section == 1) {
            float kick = (beatTime < BEAT_TIME / 2) ? Instruments::generateKick(beatTime, 55.0f) : 0.0f;
            float hihat = (eighthTime < BEAT_TIME / 4) ? Instruments::generateHiHat(eighthTime, 1500.0f, true) : 0.0f;
            float snare = (beat == 1 || beat == 3) ? Instruments::generateSnare(beatTime) : 0.0f;
            float clap = (beat == 1 || beat == 3) ? Instruments::generateClap(beatTime) : 0.0f;
            float bassFreq = NOTES[CHORD_PROGRESSIONS[songId][bar % 4]];
            float bass = Instruments::generateBass(beatTime, bassFreq) * sidechain;
            float subBass = Instruments::generateSubBass(beatTime, bassFreq / 2) * sidechain;
            float arpTime = std::fmod(localTime, BEAT_TIME / 8);
            int arpIndex = (static_cast<int>(localTime / (BEAT_TIME / 8)) % ARP_LENGTH);
            float arpFreq = NOTES[ARP_PATTERN[arpIndex]] * (1.0f + sectionProgress);
            float synthArp = (eighthTime < BEAT_TIME / 4) ? Instruments::generateSynthArp(arpTime, arpFreq) : 0.0f;
            float guitarFreq = NOTES[5];
            float guitar = (beat % 2 == 1) ? Instruments::generateGuitar(beatTime, guitarFreq) : 0.0f;
            float noise = (std::fmod(localTime, BEAT_TIME) < BEAT_TIME / 16) ? 0.2f * (rand() % 1000 / 500.0f - 1.0f) * sectionProgress : 0.0f;
            if (data->manager->technoChannels == 6) {
                channels[0] = hihat * 0.7f + synthArp * 0.6f + guitar * 0.3f;
                channels[1] = hihat * 0.3f + synthArp * 0.4f + guitar * 0.7f;
                channels[2] = kick + bass * 0.5f + subBass * 0.5f + snare + clap + noise;
                channels[3] = kick * 0.5f + subBass * 0.7f;
                channels[4] = hihat * 0.5f + synthArp * 0.5f + guitar * 0.2f;
                channels[5] = hihat * 0.3f + synthArp * 0.3f + guitar * 0.4f;
            } else if (data->manager->technoChannels == 2) {
                left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + snare + clap + bass * (1.0f - bassPan) +
                       subBass * (1.0f - bassPan) + synthArp * (1.0f - arpPan) + guitar * (1.0f - guitarPan) + noise;
                right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + snare + clap + bass * (1.0f + bassPan) +
                        subBass * (1.0f + bassPan) + synthArp * (1.0f + arpPan) + guitar * (1.0f + guitarPan) + noise;
            } else {
                mono = kick + hihat + snare + clap + bass + subBass + synthArp + guitar + noise;
            }
        } else if (section == 2) {
            float kick = (beatTime < BEAT_TIME / 2) ? Instruments::generateKick(beatTime, 60.0f) : 0.0f;
            float hihat = (eighthTime < BEAT_TIME / 4) ? Instruments::generateHiHat(eighthTime, 1500.0f, false) : 0.0f;
            float tom = (beat % 2 == 1) ? Instruments::generateTom(beatTime, 150.0f) : 0.0f;
            float snare = (localTime > BREAK_DURATION - 0.2f) ? 0.6f * (rand() % 1000 / 500.0f - 1.0f) * std::exp(-20.0f * (localTime - (BREAK_DURATION - 0.2f))) : 0.0f;
            float padFreq = NOTES[0];
            float pad = Instruments::generatePad(beatTime, padFreq) * (1.0f - sectionProgress);
            float pianoFreq = NOTES[5];
            float piano = (beat == 3) ? Instruments::generatePiano(beatTime, pianoFreq) : 0.0f;
            if (data->manager->technoChannels == 6) {
                channels[0] = hihat * 0.7f + pad * 0.6f;
                channels[1] = hihat * 0.3f + pad * 0.4f;
                channels[2] = kick + tom + snare + piano;
                channels[3] = kick * 0.5f;
                channels[4] = hihat * 0.5f + pad * 0.5f;
                channels[5] = hihat * 0.3f + pad * 0.3f;
            } else if (data->manager->technoChannels == 2) {
                left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + tom + snare + pad * (1.0f - padPan) + piano;
                right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + tom + snare + pad * (1.0f + padPan) + piano;
            } else {
                mono = kick + hihat + tom + snare + pad + piano;
            }
        } else {
            float kick = (beatTime < BEAT_TIME / 2) ? Instruments::generateKick(beatTime, 50.0f) * 0.7f : 0.0f;
            float hihat = (eighthTime < BEAT_TIME / 4) ? Instruments::generateHiHat(eighthTime, 1500.0f, false) * 0.5f : 0.0f;
            float bass = Instruments::generateBass(beatTime, NOTES[0]) * sidechain * 0.6f;
            float subBass = Instruments::generateSubBass(beatTime, NOTES[0] / 2) * sidechain * 0.6f;
            float padFreq = NOTES[1];
            float pad = Instruments::generatePad(beatTime, padFreq);
            float pianoFreq = NOTES[5];
            float piano = (beat == 2) ? Instruments::generatePiano(beatTime, pianoFreq) : 0.0f;
            float vocal = 0.0f;
            if (vocalActive && bar >= 4 && bar < 8) {
                float vocalTime = std::fmod(localTime, BEAT_TIME);
                int vocalIndex = (static_cast<int>(localTime / BEAT_TIME) % VOCAL_LENGTH);
                int phonemeIndex = PHONEMES[vocalIndex];
                vocal = Instruments::generateVocal(vocalTime, VOCAL_NOTES[vocalIndex], phonemeIndex) * 0.5f;
            }
            if (data->manager->technoChannels == 6) {
                channels[0] = hihat * 0.7f + pad * 0.6f;
                channels[1] = hihat * 0.3f + pad * 0.4f;
                channels[2] = kick + bass * 0.5f + subBass * 0.5f + piano + vocal;
                channels[3] = kick * 0.5f + subBass * 0.7f;
                channels[4] = hihat * 0.5f + pad * 0.5f;
                channels[5] = hihat * 0.3f + pad * 0.3f;
            } else if (data->manager->technoChannels == 2) {
                left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + bass * (1.0f - bassPan) +
                       subBass * (1.0f - bassPan) + pad * (1.0f - padPan) + piano + vocal * (1.0f - vocalPan);
                right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + bass * (1.0f + bassPan) +
                        subBass * (1.0f + bassPan) + pad * (1.0f + padPan) + piano + vocal * (1.0f + vocalPan);
            } else {
                mono = kick + hihat + bass + subBass + pad + piano + vocal;
            }
        }

        if (data->manager->technoChannels == 6) {
            for (int ch = 0; ch < 6; ++ch) {
                channels[ch] = std::min(std::max(channels[ch], -0.9f), 0.9f);
                buffer[i * 6 + ch] = static_cast<int16_t>(32760.0f * channels[ch]);
            }
        } else if (data->manager->technoChannels == 2) {
            left = std::min(std::max(left, -0.9f), 0.9f);
            right = std::min(std::max(right, -0.9f), 0.9f);
            buffer[i * 2] = static_cast<int16_t>(32760.0f * left);
            buffer[i * 2 + 1] = static_cast<int16_t>(32760.0f * right);
        } else {
            mono = std::min(std::max(mono, -0.9f), 0.9f);
            buffer[i] = static_cast<int16_t>(32760.0f * mono);
        }
    }
    data->t += samples / 44100.0f;
    if (data->config->TECHNO_LOOP_DURATION < 0) data->t = std::fmod(data->t, CYCLE_DURATION);
}

void AudioManager::playBoop(float currentTimeSec) {
    if (boopDevice != 0) {
        SDL_AudioStatus status = SDL_GetAudioDeviceStatus(boopDevice);
        if (status != SDL_AUDIO_PLAYING) {
            SDL_Log("Boop device not playing, status=%d, attempting to restart", status);
            SDL_CloseAudioDevice(boopDevice);
            SDL_AudioSpec desired, obtained;
            SDL_zero(desired);
            desired.freq = 44100;
            desired.format = AUDIO_S16SYS;
            desired.channels = 2;
            desired.samples = 1024;
            desired.callback = boopCallback;
            desired.userdata = &boopData;
            boopDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
            if (boopDevice == 0) {
                SDL_Log("Failed to reopen boop device: %s", SDL_GetError());
                return;
            }
            boopData.deviceId = boopDevice;
            SDL_Log("Reopened boop device: ID=%u, channels=%d", boopDevice, obtained.channels);
        }
        boopData.t = 0.0f; boopPlaying = true; SDL_PauseAudioDevice(boopDevice, 0);
    } else {
        SDL_Log("Boop device not initialized");
    }
}

void AudioManager::playExplosion(float currentTimeSec) {
    if (explosionDevice != 0) {
        SDL_AudioStatus status = SDL_GetAudioDeviceStatus(explosionDevice);
        if (status != SDL_AUDIO_PLAYING) {
            SDL_Log("Explosion device not playing, status=%d, attempting to restart", status);
            SDL_CloseAudioDevice(explosionDevice);
            SDL_AudioSpec desired, obtained;
            SDL_zero(desired);
            desired.freq = 44100;
            desired.format = AUDIO_S16SYS;
            desired.channels = 2;
            desired.samples = 1024;
            desired.callback = explosionCallback;
            desired.userdata = &explosionData;
            explosionDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
            if (explosionDevice == 0) {
                SDL_Log("Failed to reopen explosion device: %s", SDL_GetError());
                return;
            }
            explosionData.deviceId = explosionDevice;
            SDL_Log("Reopened explosion device: ID=%u, channels=%d", explosionDevice, obtained.channels);
        }
        explosionData.t = 0.0f; explosionPlaying = true; SDL_PauseAudioDevice(explosionDevice, 0);
    } else {
        SDL_Log("Explosion device not initialized");
    }
}

void AudioManager::playLaserZap(float currentTimeSec) {
    if (laserZapDevice != 0) {
        SDL_AudioStatus status = SDL_GetAudioDeviceStatus(laserZapDevice);
        if (status != SDL_AUDIO_PLAYING) {
            SDL_Log("Laser zap device not playing, status=%d, attempting to restart", status);
            SDL_CloseAudioDevice(laserZapDevice);
            SDL_AudioSpec desired, obtained;
            SDL_zero(desired);
            desired.freq = 44100;
            desired.format = AUDIO_S16SYS;
            desired.channels = 2;
            desired.samples = 1024;
            desired.callback = laserZapCallback;
            desired.userdata = &laserZapData;
            laserZapDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
            if (laserZapDevice == 0) {
                SDL_Log("Failed to reopen laser zap device: %s", SDL_GetError());
                return;
            }
            laserZapData.deviceId = laserZapDevice;
            SDL_Log("Reopened laser zap device: ID=%u, channels=%d", laserZapDevice, obtained.channels);
        }
        laserZapData.t = 0.0f; laserZapPlaying = true; SDL_PauseAudioDevice(laserZapDevice, 0);
    } else {
        SDL_Log("Laser zap device not initialized");
    }
}

void AudioManager::playWinnerVoice(float currentTimeSec) {
    if (winnerVoiceDevice != 0) {
        SDL_AudioStatus status = SDL_GetAudioDeviceStatus(winnerVoiceDevice);
        if (status != SDL_AUDIO_PLAYING) {
            SDL_Log("Winner voice device not playing, status=%d, attempting to restart", status);
            SDL_CloseAudioDevice(winnerVoiceDevice);
            SDL_AudioSpec desired, obtained;
            SDL_zero(desired);
            desired.freq = 44100;
            desired.format = AUDIO_S16SYS;
            desired.channels = 2;
            desired.samples = 1024;
            desired.callback = winnerVoiceCallback;
            desired.userdata = &winnerVoiceData;
            winnerVoiceDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
            if (winnerVoiceDevice == 0) {
                SDL_Log("Failed to reopen winner voice device: %s", SDL_GetError());
                return;
            }
            winnerVoiceData.deviceId = winnerVoiceDevice;
            SDL_Log("Reopened winner voice device: ID=%u, channels=%d", winnerVoiceDevice, obtained.channels);
        }
        winnerVoiceData.t = 0.0f; winnerVoicePlaying = true; SDL_PauseAudioDevice(winnerVoiceDevice, 0);
    } else {
        SDL_Log("Winner voice device not initialized");
    }
}

void AudioManager::startTechnoLoop(float currentTimeSec) {
    if (technoLoopDevice != 0 && !technoLoopPlaying) {
        technoLoopData.t = 0.0f;
        technoSongId = rand() % 5;
        technoLoopPlaying = true;
        SDL_PauseAudioDevice(technoLoopDevice, 0);
    }
}

void AudioManager::stopTechnoLoop() {
    if (technoLoopDevice != 0 && technoLoopPlaying) {
        technoLoopPlaying = false;
        technoLoopData.t = 0.0f;
        SDL_PauseAudioDevice(technoLoopDevice, 1);
    }
}