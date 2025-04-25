#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include "types.h"

class AudioManager {
public:
    AudioManager(const GameConfig& config);
    ~AudioManager();

    void playExplosion(float currentTimeSec);
    void playBoop(float currentTimeSec);
    void playLaserZap(float currentTimeSec);
    void stopAll();

private:
    struct SoundState {
        bool active;
        float startTime;
        float t; // Track time for each sound
    };

    struct AudioCallbackData {
        const GameConfig* config;
        AudioManager* audioManager; // Access to sound states
    };

    static void audioCallback(void* userdata, Uint8* stream, int len);

    const GameConfig& config;
    SDL_AudioDeviceID audioDevice; // Single audio device
    BoopAudioData boopData;
    bool boopPlaying;
    SoundState explosionState;
    SoundState boopState;
    SoundState laserZapState;
    AudioCallbackData* callbackData; // Store callback data
};

#endif // AUDIO_H