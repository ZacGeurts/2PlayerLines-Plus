#include "audio.h"
#include "instruments.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <mutex>

AudioManager::AudioManager(const GameConfig& config)
    : soundEffectDevice(0), technoLoopDevice(0),
      boopPlaying(false), explosionPlaying(false), laserZapPlaying(false), winnerVoicePlaying(false), technoLoopPlaying(false),
      soundEffectData{0, &config, this},
      technoLoopData{0, &technoLoopPlaying, &config, 0.0f, this, -1},
      technoChannels(2), config(config), soundEffectMutex(), hasReopenedSoundEffectDevice(false) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL audio: %s", SDL_GetError());
        return;
    }

    // Log SDL version and audio driver
    SDL_version ver;
    SDL_GetVersion(&ver);
    SDL_Log("SDL version: %d.%d.%d, Audio driver: %s", ver.major, ver.minor, ver.patch, SDL_GetCurrentAudioDriver());

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.samples = 1024;
    desired.channels = 2;
    desired.callback = nullptr; // No callback for sound effects
    desired.userdata = nullptr;

    // Open sound effect device
    soundEffectDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (soundEffectDevice == 0) {
        SDL_Log("Failed to open sound effect audio device: %s", SDL_GetError());
    } else {
        soundEffectData.deviceId = soundEffectDevice;
        SDL_Log("Sound effect device opened: ID=%u, channels=%d", soundEffectDevice, obtained.channels);
        SDL_PauseAudioDevice(soundEffectDevice, 0); // Start playback
    }

    // Open techno loop device (6 channels for 5.1)
    desired.channels = 6;
    desired.callback = technoLoopCallback;
    desired.userdata = &technoLoopData;
    technoLoopDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (technoLoopDevice == 0) {
        SDL_Log("Failed to open techno loop audio device: %s", SDL_GetError());
    } else {
        technoLoopData.deviceId = technoLoopDevice;
        technoChannels = obtained.channels;
        SDL_Log("Techno loop device opened: ID=%u, channels=%d", technoLoopDevice, technoChannels);
    }

    srand(static_cast<unsigned>(SDL_GetTicks()));
}

AudioManager::~AudioManager() {
    if (soundEffectDevice != 0) {
        SDL_CloseAudioDevice(soundEffectDevice);
        SDL_Log("Closed sound effect device");
    }
    if (technoLoopDevice != 0) {
        SDL_CloseAudioDevice(technoLoopDevice);
        SDL_Log("Closed techno loop device");
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioManager::playBoop(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play boop: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    boopPlaying = true;
    std::vector<int16_t> samples;
    generateBoopSamples(samples, currentTimeSec, 44100 * config.BOOP_DURATION, 2);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 2);
            hasReopenedSoundEffectDevice = true;
        }
    }
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Boop queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue boop: %s", SDL_GetError());
    }
}

void AudioManager::playExplosion(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play explosion: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    explosionPlaying = true;
    std::vector<int16_t> samples;
    generateExplosionSamples(samples, currentTimeSec, 44100 * config.EXPLOSION_DURATION, 2);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 2);
            hasReopenedSoundEffectDevice = true;
        }
    }
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Explosion queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue explosion: %s", SDL_GetError());
    }
}

void AudioManager::playLaserZap(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play laser zap: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    laserZapPlaying = true;
    std::vector<int16_t> samples;
    generateLaserZapSamples(samples, currentTimeSec, 44100 * config.LASER_ZAP_DURATION, 2);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 2);
            hasReopenedSoundEffectDevice = true;
        }
    }
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Laser zap queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue laser zap: %s", SDL_GetError());
    }
}

void AudioManager::playWinnerVoice(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play winner voice: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    winnerVoicePlaying = true;
    std::vector<int16_t> samples;
    generateWinnerVoiceSamples(samples, currentTimeSec, 44100 * config.WINNER_VOICE_DURATION, 2);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 2);
            hasReopenedSoundEffectDevice = true;
        }
    }
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Winner voice queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue winner voice: %s", SDL_GetError());
    }
}

void AudioManager::startTechnoLoop(float currentTimeSec, int songId) {
    if (technoLoopDevice == 0) {
        SDL_Log("Cannot start techno loop: Device not initialized");
        return;
    }
    technoLoopData.t = currentTimeSec;
    technoLoopData.songId = (songId >= -1 && songId <= 4) ? songId : -1;
    technoLoopPlaying = true;
    SDL_PauseAudioDevice(technoLoopDevice, 0);
    SDL_Log("Techno loop started with songId=%d at time=%f", technoLoopData.songId, currentTimeSec);
}

void AudioManager::stopTechnoLoop() {
    technoLoopPlaying = false;
    if (technoLoopDevice != 0) {
        SDL_PauseAudioDevice(technoLoopDevice, 1);
    }
    SDL_Log("Techno loop stopped");
}

bool AudioManager::reopenAudioDevice(SDL_AudioDeviceID& device, BoopAudioData& data, const char* deviceName, int channels) {
    if (device != 0) {
        SDL_CloseAudioDevice(device);
    }
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = channels;
    desired.samples = 1024;
    desired.callback = nullptr; // No callback
    desired.userdata = nullptr;
    device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (device == 0) {
        SDL_Log("Failed to reopen %s audio device: %s", deviceName, SDL_GetError());
        return false;
    }
    data.deviceId = device;
    SDL_Log("%s device reopened: ID=%u, channels=%d", deviceName, device, obtained.channels);
    SDL_PauseAudioDevice(device, 0); // Ensure playback
    return true;
}

bool AudioManager::reopenTechnoAudioDevice(SDL_AudioDeviceID& device, TechnoAudioData& data, SDL_AudioCallback callback, const char* deviceName, int channels) {
    if (device != 0) {
        SDL_CloseAudioDevice(device);
    }
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = channels;
    desired.samples = 1024;
    desired.callback = callback;
    desired.userdata = &data;
    device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (device == 0) {
        SDL_Log("Failed to reopen %s audio device: %s", deviceName, SDL_GetError());
        return false;
    }
    data.deviceId = device;
    technoChannels = obtained.channels;
    SDL_Log("%s device reopened: ID=%u, channels=%d", deviceName, device, obtained.channels);
    return true;
}

void AudioManager::generateBoopSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels) {
    buffer.resize(samples * channels, 0);
    for (int i = 0; i < samples; ++i) {
        float t = i / 44100.0f;
        if (t >= config.BOOP_DURATION) {
            boopPlaying = false;
            break;
        }
        float freq = 880.0f - 400.0f * (t / config.BOOP_DURATION);
        float sample = std::sin(2.0f * M_PI * freq * t) * (1.0f - t / config.BOOP_DURATION) * 0.5f;
        int16_t sample16 = static_cast<int16_t>(32760.0f * std::min(std::max(sample, -0.9f), 0.9f));
        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample16;
        }
    }
    if (!buffer.empty()) {
        SDL_Log("Generated %zu boop samples", buffer.size());
    }
}

void AudioManager::generateExplosionSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels) {
    buffer.resize(samples * channels, 0);
    for (int i = 0; i < samples; ++i) {
        float t = i / 44100.0f;
        if (t >= config.EXPLOSION_DURATION) {
            explosionPlaying = false;
            break;
        }
        float sample = ((float)rand() / RAND_MAX * 2.0f - 1.0f) * std::exp(-3.0f * t) * 0.5f;
        int16_t sample16 = static_cast<int16_t>(32760.0f * std::min(std::max(sample, -0.9f), 0.9f));
        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample16;
        }
    }
    if (!buffer.empty()) {
        SDL_Log("Generated %zu explosion samples", buffer.size());
    }
}

void AudioManager::generateLaserZapSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels) {
    buffer.resize(samples * channels, 0);
    for (int i = 0; i < samples; ++i) {
        float t = i / 44100.0f;
        if (t >= config.LASER_ZAP_DURATION) {
            laserZapPlaying = false;
            break;
        }
        float freq = 1200.0f - 800.0f * (t / config.LASER_ZAP_DURATION);
        float sample = std::sin(2.0f * M_PI * freq * t) * (1.0f - t / config.LASER_ZAP_DURATION) * 0.4f;
        int16_t sample16 = static_cast<int16_t>(32760.0f * std::min(std::max(sample, -0.9f), 0.9f));
        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample16;
        }
    }
    if (!buffer.empty()) {
        SDL_Log("Generated %zu laser zap samples", buffer.size());
    }
}

void AudioManager::generateWinnerVoiceSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels) {
    buffer.resize(samples * channels, 0);
    for (int i = 0; i < samples; ++i) {
        float t = i / 44100.0f;
        if (t >= config.WINNER_VOICE_DURATION) {
            winnerVoicePlaying = false;
            break;
        }
        float sample = std::sin(2.0f * M_PI * 200.0f * t) * std::sin(2.0f * M_PI * 5.0f * t) * 0.4f;
        int16_t sample16 = static_cast<int16_t>(32760.0f * std::min(std::max(sample, -0.9f), 0.9f));
        for (int ch = 0; ch < channels; ++ch) {
            buffer[i * channels + ch] = sample16;
        }
    }
    if (!buffer.empty()) {
        SDL_Log("Generated %zu winner voice samples", buffer.size());
    }
}

void AudioManager::technoLoopCallback(void* userdata, Uint8* stream, int len) {
    TechnoAudioData* data = static_cast<TechnoAudioData*>(userdata);
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / sizeof(int16_t) / data->manager->technoChannels;
    if (!*(data->playing) || data->songId < -1 || data->songId > 4) {
        data->t = 0.0f;
        std::fill(buffer, buffer + samples * data->manager->technoChannels, 0);
        return;
    }

    bool anySoundPlayed = false;
    for (int i = 0; i < samples; ++i) {
        float t = data->t + i / 44100.0f;
        std::vector<float> channelSamples;
        if (data->songId == -1) {
            // Original techno loop
            using namespace Instruments;
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
            int currentCycle = static_cast<int>(t / CYCLE_DURATION);
            if (currentCycle != lastCycle) {
                vocalActive = true;
                lastCycle = currentCycle;
            }

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

            float kickAmp = (beatTime < BEAT_TIME / 2) ? 1.0f : 0.0f;
            float sidechain = 1.0f - 0.5f * kickAmp;

            float kickPan = 0.0f, bassPan = 0.0f, vocalPan = 0.0f;
            float hihatPan = (beat % 2 == 0) ? -0.2f : 0.2f;
            float arpPan = (bar % 2 == 0) ? -0.15f : 0.15f;
            float padPan = -0.3f, guitarPan = 0.3f;

            channelSamples.resize(data->manager->technoChannels, 0.0f);
            float left = 0.0f, right = 0.0f;

            if (section == 0) {
                float kick = kickAmp ? generateKick(beatTime, 50.0f) : 0.0f;
                float hihat = (eighthTime < BEAT_TIME / 4) ? generateHiHat(eighthTime, 1500.0f, false) : 0.0f;
                float snare = (beat == 1 || beat == 3) ? generateSnare(beatTime) : 0.0f;
                float bassFreq = NOTES[CHORD_PROGRESSIONS[0][bar % 4]];
                float bass = generateBass(beatTime, bassFreq) * sidechain;
                float subBass = generateSubBass(beatTime, bassFreq / 2) * sidechain;
                float arpTime = std::fmod(localTime, BEAT_TIME / 4);
                int arpIndex = (static_cast<int>(localTime / (BEAT_TIME / 4)) % ARP_LENGTH);
                float arpFreq = NOTES[ARP_PATTERN[arpIndex]];
                float synthArp = (eighthTime < BEAT_TIME / 4) ? generateSynthArp(arpTime, arpFreq) : 0.0f;
                float leadFreq = NOTES[3];
                if (barTime > BAR_TIME / 2) leadFreq = NOTES[5];
                float leadSynth = (beat % 2 == 1) ? generateLeadSynth(beatTime, leadFreq) : 0.0f;
                float vocal = 0.0f;
                if (vocalActive && bar >= 8 && bar < 12) {
                    float vocalTime = std::fmod(localTime, BEAT_TIME);
                    int vocalIndex = (static_cast<int>(localTime / BEAT_TIME) % VOCAL_LENGTH);
                    int phonemeIndex = PHONEMES[vocalIndex];
                    vocal = generateVocal(vocalTime, VOCAL_NOTES[vocalIndex], phonemeIndex) * 0.5f;
                }
                if (data->manager->technoChannels == 6) {
                    channelSamples[0] = hihat * 0.7f + synthArp * 0.6f;
                    channelSamples[1] = hihat * 0.3f + synthArp * 0.4f;
                    channelSamples[2] = kick + bass * 0.5f + subBass * 0.5f + snare + leadSynth + vocal;
                    channelSamples[3] = kick * 0.5f + subBass * 0.7f;
                    channelSamples[4] = hihat * 0.5f + synthArp * 0.5f;
                    channelSamples[5] = hihat * 0.3f + synthArp * 0.3f;
                } else {
                    left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + snare + bass * (1.0f - bassPan) +
                           subBass * (1.0f - bassPan) + synthArp * (1.0f - arpPan) + leadSynth + vocal * (1.0f - vocalPan);
                    right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + snare + bass * (1.0f + bassPan) +
                            subBass * (1.0f + bassPan) + synthArp * (1.0f + arpPan) + leadSynth + vocal * (1.0f + vocalPan);
                    channelSamples[0] = left;
                    if (data->manager->technoChannels > 1) channelSamples[1] = right;
                }
            } else if (section == 1) {
                float kick = kickAmp ? generateKick(beatTime, 55.0f) : 0.0f;
                float hihat = (eighthTime < BEAT_TIME / 4) ? generateHiHat(eighthTime, 1500.0f, true) : 0.0f;
                float snare = (beat == 1 || beat == 3) ? generateSnare(beatTime) : 0.0f;
                float clap = (beat == 1 || beat == 3) ? generateClap(beatTime) : 0.0f;
                float bassFreq = NOTES[CHORD_PROGRESSIONS[0][bar % 4]];
                float bass = generateBass(beatTime, bassFreq) * sidechain;
                float subBass = generateSubBass(beatTime, bassFreq / 2) * sidechain;
                float arpTime = std::fmod(localTime, BEAT_TIME / 8);
                int arpIndex = (static_cast<int>(localTime / (BEAT_TIME / 8)) % ARP_LENGTH);
                float arpFreq = NOTES[ARP_PATTERN[arpIndex]] * (1.0f + sectionProgress);
                float synthArp = (eighthTime < BEAT_TIME / 4) ? generateSynthArp(arpTime, arpFreq) : 0.0f;
                float guitarFreq = NOTES[5];
                float guitar = (beat % 2 == 1) ? generateGuitar(beatTime, guitarFreq) : 0.0f;
                float noise = (std::fmod(localTime, BEAT_TIME) < BEAT_TIME / 16) ? 0.2f * (rand() % 1000 / 500.0f - 1.0f) * sectionProgress : 0.0f;
                if (data->manager->technoChannels == 6) {
                    channelSamples[0] = hihat * 0.7f + synthArp * 0.6f + guitar * 0.3f;
                    channelSamples[1] = hihat * 0.3f + synthArp * 0.4f + guitar * 0.7f;
                    channelSamples[2] = kick + bass * 0.5f + subBass * 0.5f + snare + clap + noise;
                    channelSamples[3] = kick * 0.5f + subBass * 0.7f;
                    channelSamples[4] = hihat * 0.5f + synthArp * 0.5f + guitar * 0.2f;
                    channelSamples[5] = hihat * 0.3f + synthArp * 0.3f + guitar * 0.4f;
                } else {
                    left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + snare + clap + bass * (1.0f - bassPan) +
                           subBass * (1.0f - bassPan) + synthArp * (1.0f - arpPan) + guitar * (1.0f - guitarPan) + noise;
                    right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + snare + clap + bass * (1.0f + bassPan) +
                            subBass * (1.0f + bassPan) + synthArp * (1.0f + arpPan) + guitar * (1.0f + guitarPan) + noise;
                    channelSamples[0] = left;
                    if (data->manager->technoChannels > 1) channelSamples[1] = right;
                }
            } else if (section == 2) {
                float kick = kickAmp ? generateKick(beatTime, 60.0f) : 0.0f;
                float hihat = (eighthTime < BEAT_TIME / 4) ? generateHiHat(eighthTime, 1500.0f, false) : 0.0f;
                float tom = (beat % 2 == 1) ? generateTom(beatTime, 150.0f) : 0.0f;
                float snare = (localTime > BREAK_DURATION - 0.2f) ? generateSnare(localTime - (BREAK_DURATION - 0.2f)) * 0.6f : 0.0f;
                float padFreq = NOTES[0];
                float pad = generatePad(beatTime, padFreq) * (1.0f - sectionProgress);
                float pianoFreq = NOTES[5];
                float piano = (beat == 3) ? generatePiano(beatTime, pianoFreq) : 0.0f;
                if (data->manager->technoChannels == 6) {
                    channelSamples[0] = hihat * 0.7f + pad * 0.6f;
                    channelSamples[1] = hihat * 0.3f + pad * 0.4f;
                    channelSamples[2] = kick + tom + snare + piano;
                    channelSamples[3] = kick * 0.5f;
                    channelSamples[4] = hihat * 0.5f + pad * 0.5f;
                    channelSamples[5] = hihat * 0.3f + pad * 0.3f;
                } else {
                    left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + tom + snare + pad * (1.0f - padPan) + piano;
                    right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + tom + snare + pad * (1.0f + padPan) + piano;
                    channelSamples[0] = left;
                    if (data->manager->technoChannels > 1) channelSamples[1] = right;
                }
            } else {
                float kick = kickAmp ? generateKick(beatTime, 50.0f) * 0.7f : 0.0f;
                float hihat = (eighthTime < BEAT_TIME / 4) ? generateHiHat(eighthTime, 1500.0f, false) * 0.5f : 0.0f;
                float bass = generateBass(beatTime, NOTES[0]) * sidechain * 0.6f;
                float subBass = generateSubBass(beatTime, NOTES[0] / 2) * sidechain * 0.6f;
                float padFreq = NOTES[1];
                float pad = generatePad(beatTime, padFreq);
                float pianoFreq = NOTES[5];
                float piano = (beat == 2) ? generatePiano(beatTime, pianoFreq) : 0.0f;
                float vocal = 0.0f;
                if (vocalActive && bar >= 4 && bar < 8) {
                    float vocalTime = std::fmod(localTime, BEAT_TIME);
                    int vocalIndex = (static_cast<int>(localTime / BEAT_TIME) % VOCAL_LENGTH);
                    int phonemeIndex = PHONEMES[vocalIndex];
                    vocal = generateVocal(vocalTime, VOCAL_NOTES[vocalIndex], phonemeIndex) * 0.5f;
                }
                if (data->manager->technoChannels == 6) {
                    channelSamples[0] = hihat * 0.7f + pad * 0.6f;
                    channelSamples[1] = hihat * 0.3f + pad * 0.4f;
                    channelSamples[2] = kick + bass * 0.5f + subBass * 0.5f + piano + vocal;
                    channelSamples[3] = kick * 0.5f + subBass * 0.7f;
                    channelSamples[4] = hihat * 0.5f + pad * 0.5f;
                    channelSamples[5] = hihat * 0.3f + pad * 0.3f;
                } else {
                    left = kick * (1.0f - kickPan) + hihat * (1.0f - hihatPan) + bass * (1.0f - bassPan) +
                           subBass * (1.0f - bassPan) + pad * (1.0f - padPan) + piano + vocal * (1.0f - vocalPan);
                    right = kick * (1.0f + kickPan) + hihat * (1.0f + hihatPan) + bass * (1.0f + bassPan) +
                            subBass * (1.0f + bassPan) + pad * (1.0f + padPan) + piano + vocal * (1.0f + vocalPan);
                    channelSamples[0] = left;
                    if (data->manager->technoChannels > 1) channelSamples[1] = right;
                }
            }
        } else {
            // Classical songs (songId 0–4)
            switch (data->songId) {
                case 0: channelSamples = generateSong1(t, data->manager->technoChannels); break;
                case 1: channelSamples = generateSong2(t, data->manager->technoChannels); break;
                case 2: channelSamples = generateSong3(t, data->manager->technoChannels); break;
                case 3: channelSamples = generateSong4(t, data->manager->technoChannels); break;
                case 4: channelSamples = generateSong5(t, data->manager->technoChannels); break;
            }
        }

        if (channelSamples.size() != static_cast<size_t>(data->manager->technoChannels)) {
            SDL_Log("Error: Song %d returned %zu channels, expected %d", data->songId, channelSamples.size(), data->manager->technoChannels);
            std::fill(buffer + i * data->manager->technoChannels, buffer + (i + 1) * data->manager->technoChannels, 0);
            continue;
        }

        for (int ch = 0; ch < data->manager->technoChannels; ++ch) {
            channelSamples[ch] = std::min(std::max(channelSamples[ch], -0.9f), 0.9f);
            buffer[i * data->manager->technoChannels + ch] = static_cast<int16_t>(32760.0f * channelSamples[ch]);
            if (channelSamples[ch] != 0.0f) anySoundPlayed = true;
        }
    }

    if (anySoundPlayed) {
        //SDL_Log("Techno loop callback: Generated non-zero samples for songId=%d", data->songId);
    }
    data->t += samples / 44100.0f;
    if (data->config->TECHNO_LOOP_DURATION < 0) {
        data->t = std::fmod(data->t, 180.0f); // Loop every ~3 minutes
    }
}