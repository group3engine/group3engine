#pragma once

// Creates command pool, command buffers
#include <memory>
#include <vector>

#include "Camera.hpp"
#include "PresentPass.hpp"
#include "Scene.hpp"
#include "ShadowMap.hpp"
#include "Volk.hpp"
#include "Bloom.hpp"
#include "Composite.hpp"
#include "DepthPrepass.hpp"
#include "ForwardPass.hpp"

namespace vk
{
	class Context;
	class Renderer
	{
	public:
		Renderer() = default;
		Renderer(Context& context);

		void Destroy();

		void Render();
		void Update(double deltaTime);
        std::shared_ptr<Scene> m_scene;
		
	private:
		void CreateResources();
		void CreateFences();
		void CreateSemaphores();
		void CreateCommandPool();
		void AllocateCommandBuffers();

		void Submit();
		void Present(uint32_t imageIndex);

	private:
		Context& context;
		std::vector<VkFence> m_Fences;
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkCommandPool> m_commandPool;

		

		std::unique_ptr<DepthPrepass>     m_DepthPrepass;
		std::unique_ptr<ForwardPass>	  m_ForwardPass;
		std::unique_ptr<ShadowMap>		  m_ShadowMap;
		std::unique_ptr<Bloom>			  m_BloomPass;
		std::unique_ptr<Composite>        m_CompositePass;
		std::unique_ptr<PresentPass>	  m_PresentPass;

		std::shared_ptr<Camera> m_camera;

	};
}