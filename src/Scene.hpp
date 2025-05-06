#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

#include "Buffer.hpp"
#include "Camera.hpp"
#include "Context.hpp"
#include "Entity.hpp"
#include "Image.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"
#include "LightManager.hpp"


#include "ImGuiRenderer.hpp"

#include "Config.hpp"

#include "SignalSystem.hpp"

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#include "DebugRendererImp.h"
#endif // JPH_DEBUG_RENDERER

class NetworkEntitiesManager;

struct CameraTransform {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::mat4 inverseProjection;
    alignas(16) glm::mat4 inverseView;
    alignas(16) glm::vec4 cameraPosition;
    alignas(8) glm::vec2 viewportSize;
    alignas(4) float fov;
    alignas(4) float nearPlane;
    alignas(4) float farPlane;
};

enum class Override { ACTIVE, INACTIVE };

struct ActivePlayerCountOverride {
    Override override = Override::INACTIVE;
    size_t playerCount = 1;
};

class Scene {
  private:
    Scene() = default;
    ~Scene() = default;

  public:
    Scene(const Scene &) = delete;
    Scene &operator=(const Scene &) = delete;

    static Scene &get() {
        static Scene instance;
        return instance;
    }

  public:
    Scene *GetActiveScene() const { return mCurrentScene; }

    void LoadGLTF(const std::filesystem::path &aFilepath, size_t playerCount);

    void DrawOpaque(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void DrawAlphaMasked(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

    void DrawShadowMap(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex = 0);
    void DrawSkinned(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex= 0);
    void AddLightSource(Light& LightSource);

    void Update(double aDeltaTime);
    void UpdateUi(double aDeltaTime);

    void Awake();

    void Load(const std::filesystem::path &filePath, size_t playerCount);
    void Unload();

    void StartUp(Context *context, MaterialManager *materialManager,
                 MeshManager *meshManager, TextureManager *textureManager);
    void ShutDown();

    TextureManager *GetTextureManager() const { return mTextureManager; }


    std::vector<Entity *>& GetEntities() { return m_Entities; }

    const std::filesystem::path &GetSceneFilename() { return mSceneFilename; }

    const std::vector<Camera *> &GetCameras() const { return mCameras; }

    const std::vector<Buffer> &GetCameraBuffers(size_t playerId) const { return mPlayerCameraUbos[playerId]; }

    void AddCamera(Camera *camera) { mCameras.push_back(camera); }

    void UpdateCameraTransforms();

    void UploadCameras(VkCommandBuffer cmdBuff);

#ifndef NDEBUG
    void CheckPlayerCount(size_t activePlayerCount) const {
        if (activePlayerCount < 1 || activePlayerCount > GlobalConfig::maxPlayers) {
            SPDLOG_ERROR("Invalid active player count ({}). Must be >= 1 and <= max players ({}).",
                         activePlayerCount,
                         GlobalConfig::maxPlayers);
            exit(EXIT_FAILURE);
        }
    }

    void CheckActivePlayerCountOverride(size_t activePlayerCount) {
        if (activePlayerCount < 1 || activePlayerCount > mActivePlayerCount) {
            SPDLOG_ERROR("Invalid active player count override ({}). Must be >= 1 and <= active players ({}).",
                         activePlayerCount,
                         mActivePlayerCount);
            // exit(EXIT_FAILURE);
        }
    }
#endif // NDEBUG

    size_t GetActivePlayerCount() const {
        if (mActivePlayerCountOverride.override == Override::ACTIVE) {
            return mActivePlayerCountOverride.playerCount;
        } else {
            return mActivePlayerCount;
        }
    }

    size_t GetPlayerCount() const { return mPlayerCount; }

    size_t PostIncrementPlayerCount() {
        size_t playerCount = mPlayerCount;

#ifndef NDEBUG
        CheckPlayerCount(mPlayerCount + 1);
#endif // NDEBUG

        ++mPlayerCount;

        return playerCount;
    }

    void SetActivePlayerCount(size_t activePlayerCount) {
#ifndef NDEBUG
        CheckPlayerCount(activePlayerCount);
#endif // NDEBUG

        mActivePlayerCount = activePlayerCount;
    }

    void SetActivePlayerCountOverrideInactive() {
        mActivePlayerCountOverride.override = Override::INACTIVE;
    }

    void SetActivePlayerCountOverride(size_t activePlayerCount) {
#ifndef NDEBUG
        // Check against global config max players
        CheckPlayerCount(activePlayerCount);
        // Check against current scene active players
        CheckActivePlayerCountOverride(activePlayerCount);
#endif // NDEBUG

        mActivePlayerCountOverride.override = Override::ACTIVE;
        mActivePlayerCountOverride.playerCount = activePlayerCount;
    }
        // CSM requires player camera transforms to compute splits
    const std::array<CameraTransform, GlobalConfig::maxPlayers> &GetPlayerCameraTransforms() const { return mPlayerCameraTransforms; }

    NetworkEntitiesManager *GetNetworkEntitiesManager() const {
        assert(mNetworkEntitiesManager);
        return mNetworkEntitiesManager;
    }

#ifdef JPH_DEBUG_RENDERER
    DebugRendererSimple *GetDebugRenderer() const { return mDebugRenderer; }

    void SetDebugRenderer(DebugRendererSimple *debugRenderer) { mDebugRenderer = debugRenderer; }
#endif // JPH_DEBUG_RENDERER

    uint32_t PostIncrementEntityCount() { return kEntityCount++; }

  public:
    SignalSystem mSignalSystem;

private:
    Scene *mCurrentScene = nullptr;

    Context *mContext = nullptr;
    MaterialManager *mMaterialManager = nullptr;
    MeshManager *mMeshManager = nullptr;
    TextureManager *mTextureManager = nullptr;

    std::vector<size_t> m_FrontMeshes;
    std::vector<size_t> m_BackMeshes;
    std::vector<Entity *> m_Entities;

    std::unordered_map<Entity *, std::vector<Entity *>> mCharacterEntities;

    std::vector<Animation> m_Animations;
    std::vector<Skin> m_Skins;

    // Actual player count
    size_t mPlayerCount = 0;

    // Active player count is how many players we want to have a camera
    size_t mActivePlayerCount = 0;
    // Override the active player count as a debug tool
    ActivePlayerCountOverride mActivePlayerCountOverride = {Override::INACTIVE, 1};

    std::vector<Camera *> mCameras;
    std::array<CameraTransform, GlobalConfig::maxPlayers> mPlayerCameraTransforms;
    std::array<std::vector<Buffer>, GlobalConfig::maxPlayers> mPlayerCameraUbos;

    gui::Settings::ActivePlayerCountOverride mGuiActivePlayerCountOverride = {};

    std::filesystem::path mSceneFilename;

    NetworkEntitiesManager *mNetworkEntitiesManager;

#ifdef JPH_DEBUG_RENDERER
    DebugRendererSimple *mDebugRenderer = nullptr;
#endif // JPH_DEBUG_RENDERER

    std::atomic<uint32_t> kEntityCount = 0;
};

