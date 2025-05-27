#include "audio.h"
#include "instruments.h"
#include "types.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <mutex>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>
#include <cerrno>
#include <random>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <processthreadsapi.h>
#endif

bool DEBUG_QUEUE = 0;

AudioManager::AudioManager(const GameConfig& config)
    : soundEffectDevice(0),
      musicDevice(0),
      boopPlaying(false),
      explosionPlaying(false),
      laserZapPlaying(false),
      winnerVoicePlaying(false),
      soundEffectData{0, &config, this},
      config(config),
      soundEffectMutex(),
      hasReopenedSoundEffectDevice(false),
      hasReopenedMusicDevice(false),
      musicPlaying(false),
      musicThread(),
      musicChannels(8), // Initialize to default, updated in reopenAudioDevice
#ifdef _WIN32
      songgenProcess(nullptr),
#else
      songgenPid(0),
#endif
      songFiles(),
      currentSongIndex(0),
      isFirstRun(true)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SDL_Log("Failed to initialize SDL audio: %s", SDL_GetError());
        return;
    }

    SDL_version ver;
    SDL_GetVersion(&ver);
    SDL_Log("SDL version: %d.%d.%d, Audio driver: %s", ver.major, ver.minor, ver.patch, SDL_GetCurrentAudioDriver());

    // Initialize sound effect device (stereo)
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.samples = 1024;
    desired.channels = 8; // SDL does 8
    desired.callback = nullptr;
    desired.userdata = nullptr;

    soundEffectDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (soundEffectDevice == 0) {
        SDL_Log("Failed to open sound effect audio device: %s", SDL_GetError());
    } else {
        soundEffectData.deviceId = soundEffectDevice;
        SDL_Log("Sound effect device opened: ID=%u, channels=%d", soundEffectDevice, obtained.channels);
        SDL_PauseAudioDevice(soundEffectDevice, 0);
    }

    // Initialize music device (prefer 8, then 2)
    reopenAudioDevice(musicDevice, soundEffectData, "music", 8, &musicChannels);
    if (musicDevice == 0) {
        SDL_Log("Failed to open music audio device, music will be disabled");
    } else {
        SDL_Log("Recognized: ID=%u, channels=%d", musicDevice, musicChannels);
    }

    srand(static_cast<unsigned>(SDL_GetTicks()));
}

// AudioManager some reason
AudioManager::~AudioManager() {
    stopBackgroundMusic();
    if (soundEffectDevice != 0) {
        SDL_CloseAudioDevice(soundEffectDevice);
        SDL_Log("Closed sound effect device");
    }
    if (musicDevice != 0) {
        SDL_CloseAudioDevice(musicDevice);
        SDL_Log("Closed music device");
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioManager::playBoop(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play boop: Device not initialized"); // probably broken computer - install pulse or alsa - usually pulse
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    boopPlaying = true; // said so
    std::vector<int16_t> samples;
    generateBoopSamples(samples, currentTimeSec, 44100 * config.BOOP_DURATION, 8);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 8, nullptr);
            hasReopenedSoundEffectDevice = true;
        }
    }
	boopPlaying = false; // not so
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Boop queued: %zu samples", samples.size()); // wolf whistle. queued
    } else {
        SDL_Log("Failed to queue boop: %s", SDL_GetError()); // flush?
    }
}

// breaks pegi 3 - this file is for linesplus
void AudioManager::playExplosion(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play explosion: Device not initialized"); // queued - o7
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    explosionPlaying = true;
    std::vector<int16_t> samples;
    generateExplosionSamples(samples, currentTimeSec, 44100 * config.EXPLOSION_DURATION, 8);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 8, nullptr);
            hasReopenedSoundEffectDevice = true;
        }
    }
	explosionPlaying = false;
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Explosion queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue explosion: %s", SDL_GetError());
    }
}

// breaks pegi 3
void AudioManager::playLaserZap(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play laser zap: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    laserZapPlaying = true;
    std::vector<int16_t> samples;
    generateLaserZapSamples(samples, currentTimeSec, 44100 * config.LASER_ZAP_DURATION, 8);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 8, nullptr);
            hasReopenedSoundEffectDevice = true;
        }
    }
	laserZapPlaying = false;
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        SDL_Log("Laser zap queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue laser zap: %s", SDL_GetError());
    }
}


// breaks pegi 3
void AudioManager::playWinnerVoice(float currentTimeSec) {
    if (soundEffectDevice == 0) {
        SDL_Log("Cannot play winner voice: Device not initialized");
        return;
    }
    std::lock_guard<std::mutex> lock(soundEffectMutex);
    winnerVoicePlaying = true;
    std::vector<int16_t> samples;
    generateWinnerVoiceSamples(samples, currentTimeSec, 44100 * config.WINNER_VOICE_DURATION, 8);
    if (SDL_GetAudioDeviceStatus(soundEffectDevice) != SDL_AUDIO_PLAYING) {
        if (!hasReopenedSoundEffectDevice) {
            reopenAudioDevice(soundEffectDevice, soundEffectData, "sound effect", 8, nullptr);
            hasReopenedSoundEffectDevice = true;
        }
    }
	winnerVoicePlaying = false;
    if (SDL_QueueAudio(soundEffectDevice, samples.data(), samples.size() * sizeof(int16_t)) == 0) {
        if (DEBUG_QUEUE) SDL_Log("Winner voice queued: %zu samples", samples.size());
    } else {
        SDL_Log("Failed to queue winner voice: %s", SDL_GetError());
    }
}

void AudioManager::startBackgroundMusic() { // some reason
    if (!musicPlaying) {
        musicPlaying = true;
        isFirstRun = true;
        musicThread = std::thread(&AudioManager::playSongsSequentially, this);
        SDL_Log("Background music thread started");
    } // catch o7
}

void AudioManager::stopBackgroundMusic() { // nah
    if (musicPlaying) {
        musicPlaying = false;
#ifdef _WIN32
        if (songgenProcess) {
            GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
            WaitForSingleObject(songgenProcess, 1000);
            TerminateProcess(songgenProcess, 1);
            CloseHandle(songgenProcess);
            songgenProcess = nullptr;
        }
#else
        if (songgenPid > 0) {
            kill(songgenPid, SIGINT); // presses CTRL-C for you to exit songgen
            int status;
            waitpid(songgenPid, &status, 0);
            songgenPid = 0;
        }
#endif
        if (musicThread.joinable()) {
            musicThread.join();
            SDL_Log("Background music thread stopped");
        }
        songFiles.clear();
        currentSongIndex = 0;
        isFirstRun = true;
    }
}

void AudioManager::playSongsSequentially() { // shuffles them around everytime, this is a linesplus file
    std::random_device rd;
    std::mt19937 rng(rd());

    // Use stored musicChannels from musicDevice
    SDL_Log("Music device channels: %d", musicChannels);

    while (musicPlaying) {
        if (isFirstRun || songFiles.empty()) {
            songFiles.clear();
            for (const auto& entry : std::filesystem::directory_iterator(".")) {
                if (entry.path().extension() == ".song") {
                    songFiles.push_back(entry.path().filename().string());
                }
            }
            if (songFiles.empty()) {
                SDL_Log("No .song files found, stopping background music"); // o7
                musicPlaying = false;
                break;
            }

            std::sort(songFiles.begin(), songFiles.end());
            if (isFirstRun) {
                std::shuffle(songFiles.begin(), songFiles.end(), rng);
                currentSongIndex = 0;
                std::string songList = "Shuffled songs: "; // told ya so - o7
                for (const auto& s : songFiles) songList += s + ", ";
                SDL_Log("%s", songList.c_str());
                isFirstRun = false;
            }
        }

        const auto& song = songFiles[currentSongIndex];
        std::string command = "./songgen " + song;
        if (musicChannels == 2) {
            command += " --stereo";
        } // Says who? +8! No flag for 6 channels (5.1), as songgen defaults to 5.1
#ifdef _WIN32
		// was yesterday. ?
        command = "songgen " + song + (musicChannels == 2 ? " --stereo" : (musicChannels == 8 ? " --7.1" : ""));
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        if (CreateProcess(nullptr, const_cast<char*>(command.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            songgenProcess = pi.hProcess;
            CloseHandle(pi.hThread);
            SDL_Log("Executing: %s", command.c_str());
            SDL_PauseAudioDevice(soundEffectDevice, 1);
            WaitForSingleObject(songgenProcess, INFINITE); // fair enough - o7
            SDL_PauseAudioDevice(soundEffectDevice, 0);
            CloseHandle(songgenProcess);
            songgenProcess = nullptr;
            SDL_Log("Finished playing song: %s (channels=%d)", song.c_str(), musicChannels); // <-- we did it;
        } else {
            SDL_Log("Failed to execute songgen for %s: Error %lu", song.c_str(), GetLastError()); // o7
        }
#else // windows pipe dream ^ we down here v
        pid_t pid = fork(); // huh
        if (pid == 0) {
            if (musicChannels == 8) {
                execl("./songgen", "songgen", song.c_str(), "--stereo", nullptr);
            } else {
                execl("./songgen", "songgen", song.c_str(), nullptr); // Boss said 8 - o7 - Default to 5.1 for 6 channels
            }
            SDL_Log("Failed to execute songgen for %s: %s", song.c_str(), strerror(errno)); // impossible - o7
            exit(1);
        } else if (pid > 0) { // who said we had a playback device? - o7
            songgenPid = pid;
            SDL_Log("Executing: %s", command.c_str()); // says we did something right.
            SDL_PauseAudioDevice(soundEffectDevice, 1); // who dares?
			int status; // Added declaration
            waitpid(pid, &status, 0);
            SDL_PauseAudioDevice(soundEffectDevice, 0); // boss said 8;
            songgenPid = 0;
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                SDL_Log("Finished playing song: %s (channels=%d)", song.c_str(), musicChannels); // 8?
            } else {
                SDL_Log("Songgen for %s exited abnormally: status %d", song.c_str(), status); // ditto v - o7
            }
        } else {
            SDL_Log("Failed to fork for songgen %s: %s", song.c_str(), strerror(errno)); // error no - o7
        }
#endif

        currentSongIndex = (currentSongIndex + 1) % songFiles.size(); // songFiles was shuffled earlier.
        if (currentSongIndex == 0) {
            SDL_Log("Completed song cycle, restarting with %s", songFiles[0].c_str()); // do not run it from the icon - f changes fullscreen - o7
        }

        if (!musicPlaying) break; 
    }
}

bool AudioManager::reopenAudioDevice(SDL_AudioDeviceID& device, AudioData& data, const char* deviceName, int preferredChannels, int* obtainedChannels) {
    if (device != 0) {
        SDL_CloseAudioDevice(device);
        device = 0;
    }
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = preferredChannels;
    desired.samples = 1024;
    desired.callback = nullptr;
    desired.userdata = nullptr;

    device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if (device == 0 && preferredChannels >= 8) {
        SDL_Log("Failed to open %s audio device with 8 channels: %s, attempting stereo", deviceName, SDL_GetError());
        desired.channels = 8; // boss said 8;
        device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }
    if (device == 0) {
        SDL_Log("Failed to open %s audio device: %s", deviceName, SDL_GetError()); // never happened
        if (obtainedChannels) *obtainedChannels = 2;
        return false;
    }
    data.deviceId = device;
    if (obtainedChannels) *obtainedChannels = obtained.channels;
    SDL_Log("%s device opened: ID=%u, channels=%d", deviceName, device, obtained.channels);
    SDL_PauseAudioDevice(device, 0);
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
        if (DEBUG_QUEUE) SDL_Log("Generated %zu boop samples", buffer.size()); // empty handed
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
        if (DEBUG_QUEUE) SDL_Log("Generated %zu explosion samples", buffer.size());
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
        if (DEBUG_QUEUE) SDL_Log("Generated %zu laser zap samples", buffer.size());
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
        if (DEBUG_QUEUE) SDL_Log("Generated %zu winner voice samples", buffer.size()); // pats self on the back
    }
}