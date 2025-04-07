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
#include "CharacterEntity.hpp"
#include "Image.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"
#include "LightManager.hpp"


#include "ImGuiRenderer.hpp"

#include "Config.hpp"

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

    void LoadGLTF(const std::filesystem::path &aFilepath);

    void DrawOpaque(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void DrawAlphaMasked(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

    void DrawShadowMap(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t caseCadeIndex = 0);
    void DrawSkinned(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout, uint32_t caseCadeIndex = 0);
    void AddLightSource(Light& LightSource);
    void Update(double aDeltaTime);
    void UpdateUi(double aDeltaTime);

    void Awake();

    void Load(const std::filesystem::path &filePath);
    void Unload();

    void StartUp(Context *context, MaterialManager *materialManager,
                 MeshManager *meshManager, TextureManager *textureManager);
    void ShutDown();

    TextureManager *GetTextureManager() const { return mTextureManager; }


    std::vector<Entity *>& GetEntities() { return m_Entities; }

    const std::filesystem::path &GetSceneFilename() { return mSceneFilename; }

    void SetHasCharacter(bool hasCharacter) { mHasCharacter = hasCharacter; }
    [[nodiscard]] bool HasCharacter() const { return mHasCharacter; }

    CharacterEntity &GetCharacter() { return *mCharacter; }

    const std::vector<Camera *> &GetCameras() const { return mCameras; }

    const std::vector<Buffer> &GetCameraBuffers(size_t playerId) const { return mPlayerCameraUbos[playerId]; }

    void AddCamera(Camera *camera) { mCameras.push_back(camera); }

    void SetMainCharacter(Entity *entity) {
        mCharacter = static_cast<CharacterEntity*>(entity);
        mHasCharacter = true;
    }

    void UpdateCameraTransforms();

    void UploadCameras(VkCommandBuffer cmdBuff);


    Camera *GetActiveCamera();

    void SwitchCamera();

    size_t GetPlayerCount() const { return mPlayerCount; }

    // CSM requires player camera transforms to compute splits
    std::array<CameraTransform, GlobalConfig::maxPlayers> GetPlayerCameraTransforms() const { return mPlayerCameraTransforms; }

private:
    Scene *mCurrentScene = nullptr;

    Context *mContext = nullptr;
    MaterialManager *mMaterialManager = nullptr;
    MeshManager *mMeshManager = nullptr;
    TextureManager *mTextureManager = nullptr;

    std::vector<size_t> m_FrontMeshes;
    std::vector<size_t> m_BackMeshes;
    std::vector<Entity *> m_Entities;
    std::vector<Animation> m_Animations;
    std::vector<Skin> m_Skins;

    bool mHasCharacter = false;
    CharacterEntity *mCharacter;

    size_t mPlayerCount = 1;
    std::vector<Camera *> mCameras;
    std::array<CameraTransform, GlobalConfig::maxPlayers> mPlayerCameraTransforms;
    std::array<std::vector<Buffer>, GlobalConfig::maxPlayers> mPlayerCameraUbos;

    gui::TimerData mGuiTimerData{};

    std::filesystem::path mSceneFilename;
};

