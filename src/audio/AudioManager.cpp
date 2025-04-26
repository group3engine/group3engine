#include "AudioManager.hpp"

#include "Utils.hpp"

void AudioManager::StartUp() {
    soloud.init(); // Initialize SoLoud

    // Set up sound sources
    std::filesystem::path soundPath = assetsPath / "audio" / "jump/Post Jump 3.wav";
    AddAudioSource("land", soundPath);
    std::filesystem::path jumpPath = assetsPath / "audio" / "jump/Pre Jump 3.wav";
    AddAudioSource("jump", jumpPath);

    std::filesystem::path musicPath = assetsPath / "audio" / "music/Sketchbook 2024-11-20.ogg";
    AddAudioSource("main_menu_music", musicPath);
    std::filesystem::path arrowPath = assetsPath / "audio" / "arrow/Arrow.wav";
    AddAudioSource("arrow", arrowPath);
}

void AudioManager::ShutDown() {
    soloud.deinit();
}
