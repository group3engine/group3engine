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

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

class AudioManager {
  private:
    AudioManager() = default;
    ~AudioManager() = default;

  public:
    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    static AudioManager &get() {
        static AudioManager instance;
        return instance;
    }

  public:
    void StartUp();

    void ShutDown();

    void PlaySound();

    void PlayMainMenuMusic();
    void TryStopMainMenuMusic();

  private:
    SoLoud::Soloud soloud; // SoLoud engine

    // TODO: Remove temporaries
    SoLoud::Wav gWave;      // One wave file

    SoLoud::Wav mMainMenuMusic;
    int mMainMenuMusicHandle = 0;
};
#endif // GROUP3ENGINE_AUDIOMANAGER_HPP
