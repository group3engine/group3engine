#include "Scene.hpp"

#include <unordered_map>

#include <tracy/Tracy.hpp>

#include "ResourceManager.hpp"

#include "ImGuiRenderer.hpp"

Scene* Scene::sActiveScene = nullptr;

void Scene::AddLightSource(Light &LightSource) {
    m_Lights.push_back(std::move(LightSource));
}

void Scene::Update(double aDeltaTime) {
    ZoneScopedN("Scene::Update");

    // update the entities
    for(auto &entity : m_Entities) {
        entity->BaseUpdate(aDeltaTime);
        entity->Update(aDeltaTime);
    }
    // late update the entities
    for(auto &entity : m_Entities) {
        entity->LateUpdate(aDeltaTime);
    }

    for (auto& light : m_Lights)
    {
        glm::mat4 ortho = glm::ortho(-light.view, light.view, -light.view, light.view, light.near, light.far);
        glm::mat4 view = glm::lookAt(glm::vec3(light.position), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
        light.LightSpaceMatrix = ortho * view;
    }

    // Fill GPU Data with data defined for the scene
    for (size_t i = 0; i < m_Lights.size(); i++) {
        m_LightBuffer.lights[i].type = static_cast<int>(m_Lights[i].Type);
        m_LightBuffer.lights[i].LightPosition = m_Lights[i].position;
        m_LightBuffer.lights[i].LightColour = m_Lights[i].colour;
        m_LightBuffer.lights[i].LightSpaceMatrix = m_Lights[i].LightSpaceMatrix;
    }

    // Pass the light data to the GPU to update all light properties
    m_LightUBO[vkutil::currentFrame].Update(context, &m_LightBuffer, sizeof(vkutil::LightBuffer));
}

void Scene::UpdateUi(double aDeltaTime) {
    ZoneScopedN("Scene::UpdateUi");

    // New timer window
    mGuiTimerData.time += aDeltaTime;
    ImGuiRenderer::NewTimer(mGuiTimerData);

    for (auto &entity : m_Entities) {
        entity->UpdateUi(aDeltaTime);
    }
}

void Scene::Destroy()
{
	for (auto& buffer : m_LightUBO)
	{
		buffer.Destroy();
	}
        // delete the mesh manager, material manager and texture manager
        delete mMeshManager;
        delete mMaterialManager;
        delete mTextureManager;

        // delete the entities
        for (auto &entity : m_Entities) {
            delete entity;
        }
        m_Entities.clear();
}

void Scene::Load(const std::filesystem::path &aFilepath) {
    // Load the GLTF file
    LoadGLTF(aFilepath, *mMeshManager, *mMaterialManager, *mTextureManager,
             m_Entities, false, m_Animations, m_Skins);

}

void Scene::Awake()
{
    // call the awake function on all entities
    for (auto &entity : m_Entities) {
        entity->Awake();
    }
}

Scene::Scene(Context &context)
    : context(context) {
    m_LightUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    // Light uniform buffers
    for (auto &buffer : m_LightUBO) {
        buffer = CreateBuffer(
            "LightUBO", context, sizeof(vkutil::LightBuffer),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }

    // create the mesh manager, material manager and texture manager
    mMeshManager = new MeshManager(context);
    mMaterialManager = new MaterialManager(context);
    mTextureManager = new TextureManager(context);
    SetActiveScene(this);
}

void Scene::DrawOpaque(VkCommandBuffer cmd,
                       VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawOpaque(cmd, pipelineLayout);
    }
}

void Scene::DrawAlphaMasked(VkCommandBuffer cmd,
                            VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawCutout(cmd, pipelineLayout);
    }
}
void Scene::DrawShadowMap(VkCommandBuffer cmd,
                          VkPipelineLayout pipelineLayout) {
    for (auto& entity : m_Entities) {
        entity->RecordDrawShadow(cmd, pipelineLayout);
    }
}

void Scene::DrawSkinned(VkCommandBuffer cmd,
                        VkPipelineLayout pipelineLayout) {
    for (auto &entity : m_Entities) {
        entity->RecordDrawSkinned(cmd, pipelineLayout);
    }
}
