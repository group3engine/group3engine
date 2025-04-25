#include "AudioManager.hpp"

#include "Utils.hpp"

void AudioManager::StartUp() {
    soloud.init(); // Initialize SoLoud

    // Set up sound sources
    std::filesystem::path soundPath = assetsPath / "audio" / "jump/Post Jump 1.wav";
    gWave.load(soundPath.string().c_str()); // Load a wave

    std::filesystem::path musicPath = assetsPath / "audio" / "music/Sketchbook 2024-11-20.ogg";
    mMainMenuMusic.load(musicPath.string().c_str());
}

void AudioManager::ShutDown() {
    soloud.deinit();
}

void AudioManager::PlaySound() {
    soloud.play(gWave); // Play the wave
}

void AudioManager::PlayMainMenuMusic() {
    mMainMenuMusicHandle = soloud.playBackground(mMainMenuMusic);
    soloud.setLooping(mMainMenuMusicHandle, true);
}

void AudioManager::TryStopMainMenuMusic() {
    if (soloud.isValidVoiceHandle(mMainMenuMusicHandle)) {
        soloud.stop(mMainMenuMusicHandle);
    }
}
