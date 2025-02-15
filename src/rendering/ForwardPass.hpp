#pragma once
#include "Volk.hpp"
#include <memory>
#include <unordered_map>
#include "Camera.hpp"

#define OPAQUE_FRAGMENT_SHADER "shaders/pbr.frag.spv"
#define OPAQUE_VERTEX_SHADER "shaders/pbr.vert.spv"
#define ALPHA_MASK_FRAGMENT_SHADER "shaders/pbra.frag.spv"
#define ALPHA_MASK_VERTEX_SHADER "shaders/pbr.vert.spv"

namespace vk
{
	class Context;
	class Scene;
	class Buffer;

	class ForwardPass
	{
	public:

		ForwardPass(Context& context, Image& shadowMap, Image& depthPrepass, std::shared_ptr<Scene>& scene, std::shared_ptr<Camera>& camera);
		~ForwardPass();
		void Execute(VkCommandBuffer cmd);
		void Update();

		void Resize();
		Image& GetRenderTarget() { return m_RenderTarget; }
		Image& GetBrightnessTarget() { return m_BrightnessTexture; }

	private:
		void CreatePipeline();
		void CreateRenderPass();
		void CreateFramebuffer();
		void BuildDescriptors();

		Image m_RenderTarget;
		Image m_DepthTarget;
		Image m_BrightnessTexture;

		VkRenderPass m_renderPass;
		VkFramebuffer m_framebuffer;
		VkDescriptorSetLayout meshDescriptorSetLayout;

		Context& context;
		Image& shadowMap;
		Image& depthPrepass;
		std::shared_ptr<Scene> scene;
		std::shared_ptr<Camera> camera;
		std::vector<VkDescriptorSet> m_descriptorSets;
		std::unordered_map<int, std::pair<VkPipeline, VkPipelineLayout>> m_pipelines;
	};
}