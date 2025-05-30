#ifndef AUDIO_H
#define AUDIO_H

#include <SDL2/SDL.h>
#include <mutex>
#include <vector>
#include <atomic>
#include <thread>
#include <string>

#ifdef _WIN32
#include <processthreadsapi.h>
#else
#include <sys/types.h>
#endif
#include "types.h"

namespace Game {

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
    void playInstrument(const std::string& instrument, float freq, float dur, float currentTimeSec); // Added

private:
    void playSongsSequentially();
    void generateInstrumentSamples(std::vector<int16_t>& buffer, const std::string& instrument, float freq, float dur, float startTime, int channels); // Added

    struct AudioData {
        SDL_AudioDeviceID deviceId{0};
        const GameConfig* config{nullptr};
        AudioManager* manager{nullptr};
    };

    SDL_AudioDeviceID soundEffectDevice{0};
    SDL_AudioDeviceID musicDevice{0};
    bool boopPlaying{false};
    bool explosionPlaying{false};
    bool laserZapPlaying{false};
    bool winnerVoicePlaying{false};
    AudioData soundEffectData{0, nullptr, nullptr};
    const GameConfig& config;
    std::mutex soundEffectMutex;
    bool hasReopenedSoundEffectDevice{false};
    bool hasReopenedMusicDevice{false};
    std::atomic<bool> musicPlaying{false};
    std::thread musicThread;
    int musicChannels{8};
    int soundEffectChannels{8};
#ifdef _WIN32
    HANDLE songgenProcess{nullptr};
#else
    pid_t songgenPid{0};
#endif
    std::vector<std::string> songFiles;
    size_t currentSongIndex{0};
    bool isFirstRun{true};

    void generateBoopSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateExplosionSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateLaserZapSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    void generateWinnerVoiceSamples(std::vector<int16_t>& buffer, float startTime, int samples, int channels);
    bool reopenAudioDevice(SDL_AudioDeviceID& device, AudioData& data, const char* deviceName, int preferredChannels, int* obtainedChannels);
};

} // namespace Game

#endif // AUDIO_H