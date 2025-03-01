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

namespace vk
{
	class Scene
	{
	public:

		explicit Scene(Context& context);
		void Load(const std::filesystem::path& aFilepath);

		void DrawOpaque(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
		void DrawAlphaMasked(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
		void DrawShadowMap(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
                void DrawSkinned(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
		void AddLightSource(Light& LightSource);
		void Update();

		void Destroy();

		std::vector<Light>&							   GetLights() { return m_Lights; }
		std::vector<Buffer>&						   GetLightsUBO() { return m_LightUBO; }

		std::vector<Entity>& GetEntities() { return m_Entities; }
	private:
		Context& context;
                MeshManager *mMeshManager;
                MaterialManager *mMaterialManager;
                TextureManager *mTextureManager;

		std::vector<size_t> m_FrontMeshes;
		std::vector<size_t> m_BackMeshes;
		std::vector<Light>  m_Lights;
		LightBuffer m_LightBuffer;
		std::vector<Buffer> m_LightUBO;
		std::vector<Entity> m_Entities;
                std::vector<Animation> m_Animations;
                std::vector<Skin> m_Skins;
	};
}
