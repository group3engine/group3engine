//
// Created by thomas on 24/04/25.
//

#include <fstream>
#include "Saving.hpp"
#include "spdlog/spdlog.h"

Saving::Saving()
{
    // spin up the thread
    mSaveThread = std::thread(&Saving::SaveThread, this);
}

void Saving::Load()
{
    std::lock_guard<std::mutex> lock(mMutex);
    // load the save file
    std::ifstream file(mSaveFileName);
    if (file.is_open())
    {
        file >> mSaveData;
        file.close();
        // check that the version is correct
        if (mSaveData.contains(VERSIONSTRING))
        {
            if (mSaveData[VERSIONSTRING] != VERSIONNUMBER)
            {
                SPDLOG_ERROR("Save file version mismatch. Expected {}, got {}", VERSIONNUMBER, std::string(mSaveData["VERSION"]));
                mSaveData.clear();
            }
        }
        else
        {
            SPDLOG_ERROR("Save file version missing.");
            mSaveData.clear();
        }
    }
    else
    {
        // clear the save data if the file doesn't exist
        mSaveData.clear();
    }
    mSaveData[VERSIONSTRING] = VERSIONNUMBER;
}

void Saving::SaveThread()
{
    while (mSaveThreadRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(mSaveDataUpToDate)
            continue;
        std::lock_guard<std::mutex> lock(mMutex);
        // save the file
        std::ofstream file(mSaveFileName);
        if (file.is_open())
        {
            file << mSaveData.dump(4);
            file.close();
        }
        mSaveDataUpToDate = true;
    }
}

void Saving::UpdateSceneName(const std::filesystem::path &sceneName)
{

    // Get the user's home directory
    std::filesystem::path homePath;

    #ifdef _WIN32
        homePath = std::getenv("USERPROFILE");
    #else
        homePath = std::getenv("HOME");
    #endif

    // Create path to documents folder/group3engine
    std::filesystem::path savePath = homePath / "Documents" / "group3enginesaves";

    // Create directories if they don't exist
    std::error_code ec;
    if (!std::filesystem::exists(savePath)) {
        std::filesystem::create_directories(savePath, ec);
        if (ec) {
            SPDLOG_ERROR("Failed to create save directory: {}", ec.message());
            return;
        }
    }
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveFileName = savePath / sceneName;
    }
    Load();

}

Saving::~Saving()
{
    mSaveThreadRunning = false;
    if (mSaveThread.joinable())
    {
        mSaveThread.join();
    }
}

template <> glm::vec3 Saving::Get<glm::vec3>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        auto value = mSaveData[key];
        return {value[0], value[1], value[2]};
    }
    return {0.f, 0.f, 0.f};
}
template <> std::string Saving::Get<std::string>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        return mSaveData[key];
    }
    return "";
}
template <> int Saving::Get<int>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        return mSaveData[key];
    }
    return 0;
}
template <> float Saving::Get<float>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        return mSaveData[key];
    }
    return 0.f;
}
template <> bool Saving::Get<bool>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        return mSaveData[key];
    }
    return false;
}
template <> double Saving::Get<double>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        return mSaveData[key];
    }
    return 0.0;
}
template <> glm::vec4 Saving::Get<glm::vec4>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        auto value = mSaveData[key];
        return {value[0], value[1], value[2], value[3]};
    }
    return {0.f, 0.f, 0.f, 0.f};
}
template <> glm::mat4 Saving::Get<glm::mat4>(std::string const &key)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSaveData.contains(key))
    {
        auto value = mSaveData[key];
        return {static_cast<float>(value[0]), static_cast<float>(value[1]), static_cast<float>(value[2]), static_cast<float>(value[3]),
                static_cast<float>(value[4]), static_cast<float>(value[5]), static_cast<float>(value[6]), static_cast<float>(value[7]),
                static_cast<float>(value[8]), static_cast<float>(value[9]), static_cast<float>(value[10]), static_cast<float>(value[11]),
                static_cast<float>(value[12]), static_cast<float>(value[13]), static_cast<float>(value[14]), static_cast<float>(value[15])};
    }
    return {1.f};
}


