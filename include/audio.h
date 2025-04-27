#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include "types.h" // Include types.h for GameConfig and BoopAudioData

// Song function declarations
float generateSong1(float t, float songTime);
float generateSong2(float t, float songTime);
float generateSong3(float t, float songTime);
float generateSong4(float t, float songTime);
float generateSong5(float t, float songTime);

class AudioManager {
public:
    AudioManager(const GameConfig& config);
    ~AudioManager();
    void playBoop(float currentTimeSec);
    void playExplosion(float currentTimeSec);
    void playLaserZap(float currentTimeSec);
    void playWinnerVoice(float currentTimeSec);
    void startTechnoLoop(float currentTimeSec, int songId = -1); // Added songId parameter
    void stopTechnoLoop();

private:
    static void boopCallback(void* userdata, Uint8* stream, int len);
    static void explosionCallback(void* userdata, Uint8* stream, int len);
    static void laserZapCallback(void* userdata, Uint8* stream, int len);
    static void winnerVoiceCallback(void* userdata, Uint8* stream, int len);
    static void technoLoopCallback(void* userdata, Uint8* stream, int len);

    SDL_AudioDeviceID boopDevice;
    SDL_AudioDeviceID explosionDevice;
    SDL_AudioDeviceID laserZapDevice;
    SDL_AudioDeviceID winnerVoiceDevice;
    SDL_AudioDeviceID technoLoopDevice;
    bool boopPlaying;
    bool explosionPlaying;
    bool laserZapPlaying;
    bool winnerVoicePlaying;
    bool technoLoopPlaying;
    BoopAudioData boopData;
    BoopAudioData explosionData;
    BoopAudioData laserZapData;
    BoopAudioData winnerVoiceData;
    BoopAudioData technoLoopData;
    int technoSongId;
    int technoChannels;
    const GameConfig& config;

    // Helper to reopen audio device safely
    bool reopenAudioDevice(SDL_AudioDeviceID& device, BoopAudioData& data, SDL_AudioCallback callback, const char* deviceName, int channels);
};

#endif // AUDIO_H