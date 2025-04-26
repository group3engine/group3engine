#ifndef GROUP3ENGINE_AUDIOMANAGER_HPP
#define GROUP3ENGINE_AUDIOMANAGER_HPP

// Ignore all warnings from soloud
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include <soloud.h>
#include <soloud_wav.h>
#include <unordered_map>
#include <string>
#include <filesystem>

#include "spdlog/spdlog.h"

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/// @brief AudioManager is a singleton class that manages audio playback using SoLoud.
class AudioManager {
  private:
    AudioManager() = default;
    ~AudioManager() = default;

  public:
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    /// @brief Get the singleton instance of AudioManager.
    static AudioManager &get() {
        static AudioManager instance;
        return instance;
    }

  public:
    void StartUp();

    void ShutDown();

    /// @brief Add an audio source to the manager.
    void AddAudioSource(const std::string &name, const std::filesystem::path &path) {
        // if the sound already exists, error
        if (mSoundMap.find(name) != mSoundMap.end()) {
            SPDLOG_ERROR("AudioManager: Sound {} already exists", name);
            std::exit(EXIT_FAILURE);
        }
        SoLoud::Wav &wav = mSoundMap[name];
        wav.load(path.c_str());
    }
    /// @brief Set the global volume for all sounds.
    void SetGlobalVolume(float volume) {
        soloud.setGlobalVolume(volume);
    }
    /// @brief Set the volume for a specific sound.
    void SetVolume(const std::string &name, float volume) {
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            it->second.setVolume(volume);
        }
    }
    /// @brief Set the looping state for a specific sound.
    void SetLooping(const std::string &name, bool looping) {
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            it->second.setLooping(looping);
        }
    }
    /// @brief Set the 3D parameters for a specific sound.
    int Play(const std::string &name) {
        int ret = -1;
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            soloud.play(it->second);
        }
        return ret;
    }
    /// @brief Play a sound in 3D space.
    int Play3D(const std::string &name, float posX, float posY, float posZ) {
        int ret = -1;
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            ret = soloud.play3d(it->second, posX, posY, posZ);
        }
        return ret;
    }

    /// @brief Play a sound in the background.
    int PlayBackground(const std::string &name) {
        int ret = -1;
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            ret = soloud.playBackground(it->second);
        }
        return ret;
    }


    /// @brief Set the background music. This will stop any currently playing
    int SetBackgroundMusic(const std::string &name) {
        // Stop any currently playing background music
        if (backgroundMusicHandle != -1) {
            soloud.stop(backgroundMusicHandle);
        }
        auto it = mSoundMap.find(name);
        if (it != mSoundMap.end()) {
            backgroundMusicHandle = soloud.playBackground(it->second);
            soloud.setLooping(backgroundMusicHandle, true);
        }
        else
        {
            backgroundMusicHandle = -1; // Reset handle if not found
        }
        return backgroundMusicHandle;
    }

    /// @brief Stop the currently playing background music.
    void StopBackgroundMusic() {
        if (backgroundMusicHandle != -1) {
            soloud.stop(backgroundMusicHandle);
            backgroundMusicHandle = -1;
        }
    }

    /// @brief set the position of the listener (camera/player)
    void SetListenerPosition(float posX, float posY, float posZ, float dirX, float dirY, float dirZ, float upX, float upY, float upZ) {
        soloud.set3dListenerParameters(posX, posY, posZ, dirX, dirY, dirZ, upX, upY, upZ, 0, 0, 0);
    }

  private:
    SoLoud::Soloud soloud; // SoLoud engine

    int backgroundMusicHandle = -1; // Handle for background music

    std::unordered_map<std::string, SoLoud::Wav> mSoundMap; // Map of sound names to SoLoud::Wav objects
};
#endif // GROUP3ENGINE_AUDIOMANAGER_HPP
