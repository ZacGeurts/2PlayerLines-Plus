#include "audio.h"
#include <cmath>
#include <cstdlib>

void AudioManager::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioCallbackData* data = static_cast<AudioCallbackData*>(userdata);
    AudioManager* manager = data->audioManager;
    const GameConfig* config = data->config;
    Sint16* buffer = (Sint16*)stream;
    int samples = len / sizeof(Sint16);
    float dt = samples / 44100.0f; // Delta time for entire buffer

    SDL_LockAudioDevice(manager->audioDevice);
    for (int i = 0; i < samples; ++i) {
        float sample = 0.0f;
        float time = i / 44100.0f; // Time within buffer for waveform

        // Explosion sound
        if (manager->explosionState.active) {
            float t = manager->explosionState.t + time;
            if (t < config->EXPLOSION_DURATION) {
                float explosionSample = 0.7f * (static_cast<float>(rand() % 65536 - 32768) / 32768.0f); // Reduced amplitude
                explosionSample += 0.3f * sin(2.0f * M_PI * 50.0f * t); // Deeper boom
                explosionSample *= exp(-2.0f * t); // Smooth decay
                sample += explosionSample;
            } else {
                manager->explosionState.active = false;
                SDL_Log("Explosion sound stopped at t=%f", t);
            }
        }

        // Boop sound
        if (manager->boopState.active && manager->boopPlaying) {
            float t = manager->boopState.t + time;
            if (t < config->BOOP_DURATION) {
                float boopSample = 0.4f * sin(2.0f * M_PI * 200.0f * t); // Softer frequency
                boopSample += 0.2f * sin(2.0f * M_PI * 400.0f * t); // Harmonic
                boopSample *= exp(-2.0f * t / config->BOOP_DURATION); // Exponential fade
                sample += boopSample;
            } else {
                manager->boopPlaying = false;
                manager->boopState.active = false;
                SDL_Log("Boop sound stopped at t=%f", t);
            }
        }

        // Laser zap sound
        if (manager->laserZapState.active) {
            float t = manager->laserZapState.t + time;
            if (t < config->LASER_ZAP_DURATION) {
                float laserSample = 0.4f * sin(2.0f * M_PI * (800.0f - 500.0f * t / config->LASER_ZAP_DURATION)); // Crisper sweep
                laserSample += 0.2f * sin(2.0f * M_PI * 1600.0f * t); // Bright harmonic
                laserSample *= exp(-5.0f * t); // Sharp decay
                sample += laserSample;
            } else {
                manager->laserZapState.active = false;
                SDL_Log("Laser zap sound stopped at t=%f", t);
            }
        }

        // Normalize to prevent clipping
        sample = std::max(-0.8f, std::min(0.8f, sample)); // Reduced range for overlap
        buffer[i] = static_cast<Sint16>(sample * 32767.0f);
    }

    // Update time for next buffer
    if (manager->explosionState.active) manager->explosionState.t += dt;
    if (manager->boopState.active) manager->boopState.t += dt;
    if (manager->laserZapState.active) manager->laserZapState.t += dt;

    // Pause device only if all sounds are done
    if (!manager->explosionState.active && !manager->boopState.active && !manager->laserZapState.active) {
        SDL_PauseAudioDevice(manager->audioDevice, 1);
        SDL_Log("Audio device paused: no active sounds");
    }
    SDL_UnlockAudioDevice(manager->audioDevice);
}

AudioManager::AudioManager(const GameConfig& config) 
    : config(config), 
      audioDevice(0), 
      boopPlaying(false), 
      explosionState{false, 0.0f, 0.0f}, 
      boopState{false, 0.0f, 0.0f}, 
      laserZapState{false, 0.0f, 0.0f},
      callbackData(nullptr) {
    // Initialize single audio device
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 8192;
    desired.callback = audioCallback;
    callbackData = new AudioCallbackData{&config, this};
    desired.userdata = callbackData;

    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (audioDevice == 0) {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
        delete callbackData;
        callbackData = nullptr;
    } else {
        boopData.deviceId = audioDevice;
        boopData.playing = &boopPlaying;
        boopData.config = &config;
        boopData.t = 0.0f;
        SDL_PauseAudioDevice(audioDevice, 1);
        SDL_Log("Audio device opened successfully");
    }
}

AudioManager::~AudioManager() {
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        delete callbackData;
        callbackData = nullptr;
    }
}

void AudioManager::playExplosion(float currentTimeSec) {
    if (audioDevice != 0) {
        SDL_Log("Playing explosion audio at time %f", currentTimeSec);
        explosionState.active = true;
        explosionState.startTime = currentTimeSec;
        explosionState.t = 0.0f;
        SDL_PauseAudioDevice(audioDevice, 0);
    } else {
        SDL_Log("Cannot play explosion: audio device not initialized");
    }
}

void AudioManager::playBoop(float currentTimeSec) {
    if (audioDevice != 0 && !boopPlaying) {
        SDL_Log("Playing boop audio at time %f", currentTimeSec);
        boopPlaying = true;
        boopState.active = true;
        boopState.startTime = currentTimeSec;
        boopState.t = 0.0f;
        SDL_PauseAudioDevice(audioDevice, 0);
    } else {
        SDL_Log("Boop audio skipped: playing=%d, deviceInitialized=%d",
                boopPlaying, audioDevice != 0);
    }
}

void AudioManager::playLaserZap(float currentTimeSec) {
    if (audioDevice != 0) {
        SDL_Log("Playing laser zap audio at time %f", currentTimeSec);
        laserZapState.active = true;
        laserZapState.startTime = currentTimeSec;
        laserZapState.t = 0.0f;
        SDL_PauseAudioDevice(audioDevice, 0);
    } else {
        SDL_Log("Cannot play laser zap: audio device not initialized");
    }
}

void AudioManager::stopAll() {
    if (audioDevice != 0) {
        SDL_PauseAudioDevice(audioDevice, 1);
        explosionState.active = false;
        explosionState.t = 0.0f;
        boopPlaying = false;
        boopState.active = false;
        boopState.t = 0.0f;
        laserZapState.active = false;
        laserZapState.t = 0.0f;
        SDL_Log("Stopped all audio");
    }
}