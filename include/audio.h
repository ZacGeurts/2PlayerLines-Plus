#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include "types.h" // For GameConfig
#include <mutex>
#include <vector>
#include <atomic>
#include <thread>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

class AudioManager {
public:
    AudioManager(const GameConfig& config);
    ~AudioManager();
    void playBoop(float currentTimeSec);
    void playExplosion(float currentTimeSec);
    void playLaserZap(float currentTimeSec);
    void playWinnerVoice(float currentTimeSec);
    void startBackgroundMusic();
    void stopBackgroundMusic();

private:
    void playSongsSequentially();

    SDL_AudioDeviceID soundEffectDevice; // Stereo device for sound effects
    SDL_AudioDeviceID musicDevice; // 6-channel (5.1) or 2-channel device for music
    bool boopPlaying;
    bool explosionPlaying;
    bool laserZapPlaying;
    bool winnerVoicePlaying;
    struct BoopAudioData {
        SDL_AudioDeviceID deviceId;
        const GameConfig* config;
        AudioManager* manager;
    } soundEffectData;
    const GameConfig& config;
    std::mutex soundEffectMutex;
    bool hasReopenedSoundEffectDevice;
    bool hasReopenedMusicDevice;
    std::atomic<bool> musicPlaying;
    std::thread musicThread;
	int musicChannels; // Added to store actual music device channel count
#ifdef _WIN32
    HANDLE songgenProcess;
#else
    pid_t songgenPid;
#endif
    // Song playback members
    std::vector<std::string> songFiles;
    size_t currentSongIndex;
    bool isFirstRun;

    bool reopenAudioDevice(SDL_AudioDeviceID& device, BoopAudioData& data, const char* deviceName, int preferredChannels, int* obtainedChannels);
    void generateBoopSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateExplosionSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateLaserZapSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateWinnerVoiceSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
};

#endif // AUDIO_H