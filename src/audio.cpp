#include "audio.h"
#include "constants.h"
#include <random>
#include <cmath>
#include <cstdio>

void explosionAudioCallback(void* userdata, Uint8* stream, int len) {
    static float t = 0.0f;
    Sint16* buffer = (Sint16*)stream;
    int samples = len / sizeof(Sint16);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> noise(-1.0f, 1.0f);

    for (int i = 0; i < samples; ++i) {
        if (t < EXPLOSION_DURATION) {
            float envelope = exp(-4.0f * t);
            float sample = noise(rng) * envelope * 0.5f;
            buffer[i] = (Sint16)(sample * 32767.0f);
            t += 1.0f / 44100.0f;
        } else {
            buffer[i] = 0;
        }
    }

    if (t >= EXPLOSION_DURATION) t = 0.0f;
}

void boopAudioCallback(void* userdata, Uint8* stream, int len) {
    BoopAudioData* data = (BoopAudioData*)userdata;
    static float t = 0.0f;
    Sint16* buffer = (Sint16*)stream;
    int samples = len / sizeof(Sint16);

    SDL_LockAudioDevice(data->deviceId);
    if (!*(data->playing)) {
        for (int i = 0; i < samples; ++i) {
            buffer[i] = 0;
        }
        SDL_UnlockAudioDevice(data->deviceId);
        return;
    }

    for (int i = 0; i < samples; ++i) {
        if (t < BOOP_DURATION) {
            float freq = 880.0f;
            float envelope = exp(-10.0f * t);
            float sample = sin(2.0f * M_PI * freq * t) * envelope * 0.5f;
            buffer[i] = (Sint16)(sample * 32767.0f);
            t += 1.0f / 44100.0f;
        } else {
            buffer[i] = 0;
            *(data->playing) = false;
            t = 0.0f;
        }
    }
    SDL_UnlockAudioDevice(data->deviceId);
}