#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>

#include "Context.hpp"
#include "Image.hpp"
#include "Utils.hpp"
#include "Light.hpp"
#include "GLTF.hpp"
#include "Buffer.hpp"

namespace vk
{
	class Scene
	{
	public:

		Scene(Context& context, MaterialManager& materialManager);
		void AddModel(GLTFModel& GLTF, MaterialManager& materialManager);

		// TODO: Sort and implement these 
		void RenderFrontMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);
		void RenderBackMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout);

		void DrawGLTF(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout); // Does it make sense for this to take VkPipeline? 
		void AddLightSource(Light& LightSource);
		void Update(GLFWwindow* window);

		void Destroy();

		std::vector<Light>&							   GetLights() { return m_Lights; }
		std::vector<Buffer>&						   GetLightsUBO() { return m_LightUBO; }

	private:
		Context& context;
		MaterialManager& materialManager;

		std::vector<size_t> m_FrontMeshes;
		std::vector<size_t> m_BackMeshes;
		std::vector<Light>  m_Lights;
		LightBuffer m_LightBuffer;
		std::vector<Buffer> m_LightUBO;
		std::vector<GLTFModel> gltfModels;
	};
}