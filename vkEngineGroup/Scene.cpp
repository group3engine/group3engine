#include "Scene.hpp"
#include <unordered_map>

vk::Scene::Scene(Context& context, MaterialManager& materialManager) : context(context), materialManager{ materialManager } 
{
	m_LightUBO.resize(MAX_FRAMES_IN_FLIGHT);
	// Light uniform buffers
	for (auto& buffer : m_LightUBO)
		buffer = CreateBuffer("LightUBO", context, sizeof(LightBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

}

void vk::Scene::AddModel(GLTFModel& GLTF, MaterialManager& materialManager)
{
	// Load textures from disk
	// Want to load only unique materials 
	std::unordered_map<int, int> map;
	for (auto& mesh : GLTF.meshes)
	{
		if (map[mesh.materialIndex] > 0)
		{
			continue;
		}
		materialManager.materials[mesh.materialIndex].textures.resize(mesh.textures.size());
		for (size_t i = 0; i < mesh.textures.size(); i++) {
			VkFormat FORMAT = i == 0 ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; // index 0 is albedo, the rest should use UNORM 
			materialManager.materials[mesh.materialIndex].textures[i] = (std::move(LoadTextureFromDisk(mesh.textures[i], context, FORMAT)));
		}
		map[mesh.materialIndex] = 1;
	}

	for (auto& mesh : GLTF.meshes)
	{
		VkDeviceSize vertexSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
		CreateAndUploadBuffer(context, mesh.vertices.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, mesh.vertexBuffer);

		VkDeviceSize indexSize = sizeof(mesh.indices[0]) * mesh.indices.size();
		CreateAndUploadBuffer(context, mesh.indices.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indexBuffer);
	}

	gltfModels.push_back(std::move(GLTF));
}

// This should really be called RenderMeshes which renders meshes in the scene
void vk::Scene::DrawGLTF(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
{
	for (auto& model : gltfModels)
	{
		for (auto& mesh : model.meshes)
		{
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &materialManager.materialDescriptorSets[mesh.materialIndex], 0, nullptr);

			MeshPushConstants pc = {};
			pc.ModelMatrix = glm::mat4(1.0f);
			pc.ModelMatrix = glm::scale(pc.ModelMatrix, glm::vec3(0.005, 0.005, 0.005));
			vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants), &pc);
			// Set up push constants
			VkDeviceSize offset[] = { 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer.buffer, offset);
			vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
		}
	}
}


// TODO: Sort and implement these 
void vk::Scene::RenderFrontMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
{

}
void vk::Scene::RenderBackMeshes(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout)
{

}

void vk::Scene::AddLightSource(Light& LightSource)
{
	m_Lights.push_back(std::move(LightSource));
}

void vk::Scene::Update(GLFWwindow* window)
{
	float speed = 1000.0f;
	float adjustmentSpeed = 1000.0f; // Speed of adjustments
	static float near = 0.1f; // Starting near value
	static float far = 13790.0; // Starting far value
	static float view = 14.0f;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		m_Lights[0].position.y += speed * deltaTime; // Move up
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		m_Lights[0].position.y -= speed * deltaTime; // Move down
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		m_Lights[0].position.x -= speed * deltaTime; // Move left
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		m_Lights[0].position.x += speed * deltaTime; // Move right
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
		m_Lights[0].position.z += speed * deltaTime; // Move forward
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
		m_Lights[0].position.z -= speed * deltaTime; // Move backward

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		near = std::max(0.01, near - adjustmentSpeed * deltaTime); // Decrease near plane
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		near += adjustmentSpeed * deltaTime; // Increase near plane

	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
		far = std::max(near + 1.0, far - adjustmentSpeed * deltaTime); // Decrease far plane
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)
		far += adjustmentSpeed * deltaTime; // Increase far plane

	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		view = view - adjustmentSpeed * deltaTime; // Decrease far plane
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		view = view + adjustmentSpeed * deltaTime;

	//std::cout << m_Lights[0].position.x << ", " << m_Lights[0].position.y << ", " << m_Lights[0].position.z << std::endl;
	//std::cout << "Near: " << near << ", Far: " << far << " view: " << view << std::endl;

	for (auto& light : m_Lights)
	{
		glm::mat4 ortho = glm::ortho(-view, view, -view, view, near, far);
		glm::mat4 view = glm::lookAt(glm::vec3(light.position), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0));
		light.LightSpaceMatrix = ortho * view;
	}

	// Fill GPU Data with data defined for the scene 
	for (size_t i = 0; i < m_Lights.size(); i++)
	{
		m_LightBuffer.lights[i].type = static_cast<int>(m_Lights[i].Type);
		m_LightBuffer.lights[i].LightPosition = m_Lights[i].position;
		m_LightBuffer.lights[i].LightColour = m_Lights[i].colour;
		m_LightBuffer.lights[i].LightSpaceMatrix = m_Lights[i].LightSpaceMatrix;
	}

	// Pass the light data to the GPU to update all light properties 
	m_LightUBO[currentFrame].WriteToBuffer(m_LightBuffer, sizeof(LightBuffer));
}

void vk::Scene::Destroy()
{
	// Destroy model resources for the GLTF resources loaded in 
	if (!gltfModels.empty())
	{
		for (auto& model : gltfModels)
		{
			model.Destroy();
		}
	}

	for (auto& buffer : m_LightUBO)
	{
		buffer.Destroy(context.device);
	}
}
