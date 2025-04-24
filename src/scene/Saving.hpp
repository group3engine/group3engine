//
// Created by thomas on 24/04/25.
//

#ifndef GROUP3ENGINE_SAVING_HPP
#define GROUP3ENGINE_SAVING_HPP
#include <json.hpp>
#include <string>
#include <glm/glm.hpp>
#include <thread>


/// @brief This class is a singleton that can save and load data for a scene.
/// This class is thread safe and will save data to a file in the background.
/// The file is based on the scene name. If you want to save data for multiple scenes, the class must be modified.
/// If you would like to save a datatype that is not supported, the class is trivial to extend.
class Saving {
private:
    Saving();
    ~Saving();
public:
    /// @brief Get the singleton instance of the Saving class
    static Saving &get() {
        static Saving instance;
        return instance;
    }
    /// @brief Save a glm::vec3 to the save file.
    void Save(std::string const &key, glm::vec3 value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = {value.x, value.y, value.z};
        mSaveDataUpToDate = false;
    }
    /// @brief Save a string to the save file.
    void Save(std::string const &key, std::string value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = value;
        mSaveDataUpToDate = false;
    }
    /// @brief Save an int to the save file.
    void Save(std::string const &key, int value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = value;
        mSaveDataUpToDate = false;
    }
    /// @brief Save a float to the save file.
    void Save(std::string const &key, float value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = value;
        mSaveDataUpToDate = false;
    }
    /// @brief Save a bool to the save file.
    void Save(std::string const &key, bool value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = value;
        mSaveDataUpToDate = false;
    }
    /// @brief Save a double to the save file.
    void Save(std::string const &key, double value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = value;
        mSaveDataUpToDate = false;
    }
    /// @brief Save a glm::vec4 to the save file.
    void Save(std::string const &key, glm::vec4 value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = {value.x, value.y, value.z, value.w};
        mSaveDataUpToDate = false;
    }
    /// @brief Save a glm::mat4 to the save file.
    void Save(std::string const &key, glm::mat4 value) {
        std::lock_guard<std::mutex> lock(mMutex);
        mSaveData[key] = {value[0][0], value[0][1], value[0][2], value[0][3],
                          value[1][0], value[1][1], value[1][2], value[1][3],
                          value[2][0], value[2][1], value[2][2], value[2][3],
                          value[3][0], value[3][1], value[3][2], value[3][3]};
        mSaveDataUpToDate = false;
    }
    /// @brief Query if a key exists in the save file.
    bool HasKey(std::string const &key) {
        std::lock_guard<std::mutex> lock(mMutex);
        return mSaveData.contains(key);
    }
    /// @brief Get a value of type T from the save file.
    template<typename T> T Get(std::string const &key);


    void UpdateSceneName(const std::filesystem::path &sceneName);
private:
    std::filesystem::path mSaveFileName{};
    nlohmann::json mSaveData{};
    bool mSaveDataUpToDate = false;
    std::mutex mMutex{};

    void Load();
    void SaveThread();

    std::thread mSaveThread;
    bool mSaveThreadRunning = true;



};


#endif //GROUP3ENGINE_SAVING_HPP
