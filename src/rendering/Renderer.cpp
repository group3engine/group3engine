#include "Renderer.hpp"

#include <filesystem>

#include "Context.hpp"
#include "Light.hpp"
#include "Utils.hpp"
#include "SampleGLTFFilePaths.hpp"

namespace
{
	// This should be placed elsewhere. Put here for simplicity while testing
	// Don't really need to define these, can pass the pos, dir, up directly to camera constructor
	// Camera default values
	constexpr glm::vec3 cameraPos = glm::vec3(1.0f, 1.0f, 1.0f); //1.0f, 2.0f, -24.0f
	constexpr glm::vec3 cameraDir = glm::vec3(1.0f, 1.0f, -1.0f);
	constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0);
}

vk::Renderer::Renderer(Context& context) : context{context}
{
	std::printf("Launching Renderer\n");
	vk::renderType = RenderType::FORWARD;

	CreateResources();

	m_materialManager.materials.reserve(25);
	for (int i = 0; i < 25; ++i) {
		m_materialManager.materials.emplace_back(context);
	}

	m_materialManager.Setup(context);

	// Current path is the current working directory, i.e., where the root CMakeLists.txt is
	std::filesystem::path basePath = std::filesystem::current_path() / "assets";
	std::filesystem::path gltfPath = basePath / Sample::Sponza;
	auto gltf = vk::LoadGLTF(context, gltfPath);

	// Samplers
	repeatSamplerAniso	 	  = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_TRUE,  VK_COMPARE_OP_LESS_OR_EQUAL);
	repeatSampler			  = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
	clampToEdgeSamplerAniso   = CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FALSE, VK_COMPARE_OP_GREATER);
	
	// Camera
	m_camera = std::make_shared<Camera>(context, cameraPos, glm::normalize(cameraPos + cameraDir), up, context.extent.width / (float)context.extent.height);
	
	// GLFW callbacks
	glfwSetWindowUserPointer(context.mWindow, m_camera.get());

	// Define Light sources
	Light directionalLight;
	directionalLight.Type = LightType::Directional;
	directionalLight.position = glm::vec4(-8.161, 23.6f, 4.0f, 1.0f); // -0.2972
	directionalLight.colour   = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	std::vector<glm::vec4> spotLightPositions;

	// Random spot light positions put side by side each other 
	for (size_t i = 0; i < 25; i++) {
		spotLightPositions.push_back(glm::vec4(1.0, 10.0f, 1.0f, 1.0f)); // Modify as needed
	}

	// Create the scene which will store models and lights
	// Add GLTF to the scene 
	// Add a directional light source defined earlier 
	m_scene = std::make_shared<Scene>(context, m_materialManager);
	m_scene->AddModel(gltf, m_materialManager);
	m_scene->AddLightSource(directionalLight);

	// Loop through the positions and instantiate a light 
	// and pass to the scene to add the lights to the scene
	for (const auto& position : spotLightPositions)
	{
		Light spotLight = {};
		spotLight.Type = LightType::Spot;
		spotLight.position = position;
		spotLight.colour = glm::vec4(0.3f, 0.0f, 1.0f, 1.0f);
		m_scene->AddLightSource(spotLight);
	}

	// Models should not all be loaded 
	// We have the data to build materials 
	m_materialManager.BuildMaterials(context);

	std::cout << "Number of Lights: " << m_scene->GetLights().size() << std::endl;

	// Renderer passes
	m_ShadowMap	    = std::make_unique<ShadowMap>(context, m_scene);
	m_DepthPrepass  = std::make_unique<DepthPrepass>(context, m_scene, m_camera);
	m_ForwardPass   = std::make_unique<ForwardPass>(context, m_ShadowMap->GetRenderTarget(), m_DepthPrepass->GetRenderTarget(), m_scene, m_camera);
	m_BloomPass		= std::make_unique<Bloom>(context, m_ForwardPass->GetBrightnessTarget());
	m_CompositePass = std::make_unique<Composite>(context, m_ForwardPass->GetRenderTarget(), m_BloomPass->GetRenderTarget());
	m_PresentPass   = std::make_unique<PresentPass>(context, m_CompositePass->GetRenderTarget()); 
}

void vk::Renderer::Destroy()
{
	vkDeviceWaitIdle(context.device);

	m_DepthPrepass.reset();
	m_ForwardPass.reset();
	m_ShadowMap.reset();
	m_BloomPass.reset();
	m_CompositePass.reset();
	m_PresentPass.reset();
	m_camera.reset();
	m_scene->Destroy();

	vkDestroySampler(context.device, repeatSamplerAniso, nullptr);
	vkDestroySampler(context.device, repeatSampler, nullptr);
	vkDestroySampler(context.device, clampToEdgeSamplerAniso, nullptr);

	m_materialManager.Destroy(context);

	for (auto& fence : m_Fences)
	{
		vkDestroyFence(context.device, fence, nullptr);
	}

	for (auto& semaphore : m_imageAvailableSemaphores)
	{
		vkDestroySemaphore(context.device, semaphore, nullptr);
	}

	for (auto& semaphore : m_renderFinishedSemaphores)
	{
		vkDestroySemaphore(context.device, semaphore, nullptr);
	}

	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkFreeCommandBuffers(context.device, m_commandPool[i], 1, &m_commandBuffers[i]);
	}

	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyCommandPool(context.device, m_commandPool[i], nullptr);
	}
}

void vk::Renderer::CreateResources()
{
	CreateFences();
	CreateSemaphores();
	CreateCommandPool();
	AllocateCommandBuffers();
}

void vk::Renderer::CreateFences()
{
	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++)
	{
		// Fence
		VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT
		};

		VkFence fence = VK_NULL_HANDLE;
		VK_CHECK(vkCreateFence(context.device, &fenceInfo, nullptr, &fence), "Failedd to create Fence.");
		m_Fences.push_back(std::move(fence));
	}
}

void vk::Renderer::CreateSemaphores()
{
	// Image available semaphore
	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++) {
		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkSemaphore semaphore = VK_NULL_HANDLE;
		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &semaphore), "Failed to create image available semaphore");
		m_imageAvailableSemaphores.push_back(std::move(semaphore));
	}

	// Render finished sempahore
	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++) {
		VkSemaphoreCreateInfo semaphoreInfo = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
		};

		VkSemaphore semaphore = VK_NULL_HANDLE;
		VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &semaphore), "Failed to create render finished semaphore");
		m_renderFinishedSemaphores.push_back(std::move(semaphore));
	}
}

void vk::Renderer::CreateCommandPool()
{
	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkCommandPoolCreateInfo cmdPool{};
		cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cmdPool.queueFamilyIndex = context.graphicsFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VK_CHECK(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool");
		m_commandPool.push_back(std::move(commandPool));
	}
}

void vk::Renderer::AllocateCommandBuffers()
{
	for (size_t i = 0; i < (size_t)vk::MAX_FRAMES_IN_FLIGHT; i++)
	{
		// Allocate command buffers from command pool
		VkCommandBufferAllocateInfo cmdAlloc{};
		cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAlloc.commandPool = m_commandPool[i];
		cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdAlloc.commandBufferCount = 1;

		VkCommandBuffer cmd = VK_NULL_HANDLE;
		VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffer");
		m_commandBuffers.push_back(cmd);
	}
}

void vk::Renderer::Render()
{
	vkWaitForFences(context.device, 1, &m_Fences[vk::currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t index;
	VkResult getImageIndex = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, m_imageAvailableSemaphores[vk::currentFrame], VK_NULL_HANDLE, &index);

	if (getImageIndex == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// Recreate swapchain
		context.RecreateSwapchain();
		m_DepthPrepass->Resize();
		m_ShadowMap->Resize();
		m_ForwardPass->Resize();
		m_BloomPass->Resize();
		m_CompositePass->Resize();
		m_PresentPass->Resize();
	}
	else if (getImageIndex != VK_SUCCESS && getImageIndex != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to aquire swapchain image");
	}

	vkResetFences(context.device, 1, &m_Fences[vk::currentFrame]);
	vkResetCommandBuffer(m_commandBuffers[vk::currentFrame], 0);

	VkCommandBuffer& cmd = m_commandBuffers[vk::currentFrame];

	{
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
		};

		VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer");

		m_ShadowMap->Execute(cmd);
		m_DepthPrepass->Execute(cmd);
		m_ForwardPass->Execute(cmd);
		m_BloomPass->Execute(cmd);
		m_CompositePass->Execute(cmd);
		m_PresentPass->Execute(cmd, index);

		vkEndCommandBuffer(cmd);
	}

	Submit();
	Present(index);

	vk::currentFrame = (vk::currentFrame + 1) % vk::MAX_FRAMES_IN_FLIGHT;
}

void vk::Renderer::Submit()
{
	VkPipelineStageFlags waitStage = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo subtmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_imageAvailableSemaphores[vk::currentFrame],
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &m_commandBuffers[vk::currentFrame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &m_renderFinishedSemaphores[vk::currentFrame]
	};

	VkResult result = vkQueueSubmit(context.graphicsQueue, 1, &subtmitInfo, m_Fences[vk::currentFrame]);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit command buffers");
	}

}

void vk::Renderer::Present(uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &m_renderFinishedSemaphores[vk::currentFrame],
		.swapchainCount = 1,
		.pSwapchains = &context.swapchain,
		.pImageIndices = &imageIndex,
	};


	VkResult result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// Recreate the swapchain
		context.RecreateSwapchain();
		m_DepthPrepass->Resize();
		m_ShadowMap->Resize();
		m_ForwardPass->Resize();
		m_BloomPass->Resize();
		m_CompositePass->Resize();
		m_PresentPass->Resize();
	}
}

void vk::Renderer::Update(double deltaTime)
{
	m_camera->Update(context.extent.width, context.extent.height, deltaTime);
	m_scene->Update();

	// Update passes
	m_ShadowMap->Update();
	m_ForwardPass->Update();
	m_PresentPass->Update();
}
