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
