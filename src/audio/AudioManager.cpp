#include "AudioManager.hpp"

#include "Utils.hpp"
#include <json.hpp>
#include <fstream>

void AudioManager::StartUp()
{
    soloud.init(); // Initialize SoLoud

    // Set up sound sources
    std::filesystem::path manifestPath = assetsPath / "audiomanifest.json";
    LoadAudioManifest(manifestPath);
}

void AudioManager::ShutDown() {
    soloud.deinit();
}

void AudioManager::LoadAudioManifest(const std::filesystem::path &manifestPath)
{
    std::ifstream file(manifestPath);
    if (file.is_open())
    {
        nlohmann::json manifest;
        file >> manifest;
        file.close();

        for (const auto &entry : manifest) {
            std::string name = entry["name"];
            std::filesystem::path path = assetsPath / entry["path"];
            AddAudioSource(name, path);
        }
    } else {
        SPDLOG_ERROR("AudioManager: Failed to open manifest file {}", manifestPath.string());
        std::exit(EXIT_FAILURE);
    }
}

void AudioManager::AddAudioSource(const std::string &name, const std::filesystem::path &path)
{
    // if the sound already exists, error
    if (mSoundMap.find(name) != mSoundMap.end()) {
        SPDLOG_ERROR("AudioManager: Sound {} already exists", name);
        std::exit(EXIT_FAILURE);
    }
    SoLoud::Wav &wav = mSoundMap[name];
    wav.load(path.c_str());
}

void AudioManager::SetVolume(const std::string &name, float volume)
{
    auto it = mSoundMap.find(name);
    if (it != mSoundMap.end()) {
        it->second.setVolume(volume);
    }
}

void AudioManager::SetLooping(const std::string &name, bool looping)
{
    auto it = mSoundMap.find(name);
    if (it != mSoundMap.end()) {
        it->second.setLooping(looping);
    }
}

int AudioManager::Play(const std::string &name)
{
    int ret = -1;
    auto it = mSoundMap.find(name);
    if (it != mSoundMap.end()) {
        soloud.play(it->second);
    }
    return ret;
}

int AudioManager::Play3D(const std::string &name, float posX, float posY, float posZ)
{
    int ret = -1;
    auto it = mSoundMap.find(name);
    if (it != mSoundMap.end()) {
        ret = soloud.play3d(it->second, posX, posY, posZ);
    }
    return ret;
}

int AudioManager::PlayBackground(const std::string &name)
{
    int ret = -1;
    auto it = mSoundMap.find(name);
    if (it != mSoundMap.end()) {
        ret = soloud.playBackground(it->second);
    }
    return ret;
}

int AudioManager::SetBackgroundMusic(const std::string &name)
{
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

void AudioManager::StopBackgroundMusic()
{
    if (backgroundMusicHandle != -1) {
        soloud.stop(backgroundMusicHandle);
        backgroundMusicHandle = -1;
    }
}