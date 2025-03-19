#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

#include "Buffer.hpp"
#include "Context.hpp"
#include "Entity.hpp"
#include "Image.hpp"
#include "Light.hpp"
#include "MaterialManager.hpp"
#include "MeshManager.hpp"
#include "TextureManager.hpp"
#include "Utils.hpp"

#include "CharacterVirtualTest.h"
#include "CharacterEntity.hpp"

#include "ImGuiRenderer.hpp"

class Scene {
public:
    static Scene* GetActiveScene() { return sActiveScene; }
private:
    static Scene* sActiveScene;
    static void SetActiveScene(Scene* scene) { sActiveScene = scene; }
  public:
    explicit Scene(Context &context);
    void Load(const std::filesystem::path &aFilepath);

    void DrawOpaque(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void DrawAlphaMasked(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void DrawShadowMap(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void DrawSkinned(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
    void AddLightSource(Light& LightSource);
    void Update(double aDeltaTime);
    void UpdateUi(double aDeltaTime);
    void Awake();

    void Destroy();

    TextureManager *GetTextureManager() const { return mTextureManager; }

    std::vector<Light> &GetLights() { return m_Lights; }

    std::vector<Buffer> &GetLightsUBO() { return m_LightUBO; }

    std::vector<Entity *>& GetEntities() { return m_Entities; }

    void SetHasCharacter(bool hasCharacter) { mHasCharacter = hasCharacter; }
    [[nodiscard]] bool HasCharacter() const { return mHasCharacter; }

    Entity &GetCharacter() { return *mCharacter; }

    void SetMainCharacter(CharacterEntity *entity) {
        mCharacter = entity;
        mHasCharacter = true;
    }

  private:
    Context &context;
    MeshManager *mMeshManager;
    MaterialManager *mMaterialManager;
    TextureManager *mTextureManager;

    std::vector<size_t> m_FrontMeshes;
    std::vector<size_t> m_BackMeshes;
    std::vector<Light>  m_Lights;
    vkutil::LightBuffer m_LightBuffer;
    std::vector<Buffer> m_LightUBO;
    std::vector<Entity *> m_Entities;
    std::vector<Animation> m_Animations;
    std::vector<Skin> m_Skins;

    bool mHasCharacter = false;
    Entity *mCharacter;

    gui::TimerData mGuiTimerData{};
};

