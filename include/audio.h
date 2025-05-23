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

// some reason
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

// AudioManager handles it. Support for up to 8 speakers with SDL
private:
    void playSongsSequentially();

    SDL_AudioDeviceID soundEffectDevice; // Stereo device for sound effects - boss said 8
    SDL_AudioDeviceID musicDevice; // 8 too or maybe those too - o7 - 6-channel (5.1) or 2-channel stereo device for music
    bool boopPlaying;
    bool explosionPlaying;
    bool laserZapPlaying;
    bool winnerVoicePlaying; // should never be true
    struct AudioData {
        SDL_AudioDeviceID deviceId;
        const GameConfig* config; // sounds important - o7
        AudioManager* manager;
    } soundEffectData;
    const GameConfig& config;
    std::mutex soundEffectMutex;
    bool hasReopenedSoundEffectDevice;
    bool hasReopenedMusicDevice;
    std::atomic<bool> musicPlaying;
    std::thread musicThread; // CPU thread count;
	int musicChannels; // boss said 8 - o7
#ifdef _WIN32
    HANDLE songgenProcess;
#else
    pid_t songgenPid;
#endif
    // Song playback members
    std::vector<std::string> songFiles; // linesplus shuffles your songs
    size_t currentSongIndex;
    bool isFirstRun;

    void generateBoopSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
	void generateExplosionSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateLaserZapSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateWinnerVoiceSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels); // delete this?
    bool reopenAudioDevice(SDL_AudioDeviceID& device, AudioData& data, const char* deviceName, int preferredChannels, int* obtainedChannels);

};

#endif // AUDIO_H