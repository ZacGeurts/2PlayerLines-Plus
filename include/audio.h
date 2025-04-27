#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include "types.h" // For GameConfig and BoopAudioData
#include <mutex>
#include <vector>

// Song function declarations
std::vector<float> generateSong1(float t, int channels);
std::vector<float> generateSong2(float t, int channels);
std::vector<float> generateSong3(float t, int channels);
std::vector<float> generateSong4(float t, int channels);
std::vector<float> generateSong5(float t, int channels);

class AudioManager {
public:
    AudioManager(const GameConfig& config);
    ~AudioManager();
    void playBoop(float currentTimeSec);
    void playExplosion(float currentTimeSec);
    void playLaserZap(float currentTimeSec);
    void playWinnerVoice(float currentTimeSec);
    void startTechnoLoop(float currentTimeSec, int songId = -1);
    void stopTechnoLoop();

private:
    static void technoLoopCallback(void* userdata, Uint8* stream, int len);

    SDL_AudioDeviceID soundEffectDevice;
    SDL_AudioDeviceID technoLoopDevice;
    bool boopPlaying;
    bool explosionPlaying;
    bool laserZapPlaying;
    bool winnerVoicePlaying;
    bool technoLoopPlaying;
    struct BoopAudioData {
        SDL_AudioDeviceID deviceId;
        const GameConfig* config;
        AudioManager* manager;
    } soundEffectData;
    struct TechnoAudioData {
        SDL_AudioDeviceID deviceId;
        bool* playing;
        const GameConfig* config;
        float t;
        AudioManager* manager;
        int songId;
    } technoLoopData;
    int technoChannels;
    const GameConfig& config;
    std::mutex soundEffectMutex;
    bool hasReopenedSoundEffectDevice;

    // Helper to reopen audio device safely
    bool reopenAudioDevice(SDL_AudioDeviceID& device, BoopAudioData& data, const char* deviceName, int channels);
    bool reopenTechnoAudioDevice(SDL_AudioDeviceID& device, TechnoAudioData& data, SDL_AudioCallback callback, const char* deviceName, int channels);

    // Helper to generate sound effect samples
    void generateBoopSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateExplosionSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateLaserZapSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateWinnerVoiceSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
};

#endif // AUDIO_H