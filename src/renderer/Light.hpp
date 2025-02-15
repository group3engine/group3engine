#pragma once

#include "Image.hpp"
#include "Utils.hpp"
#include "glm/glm.hpp"
#include "vulkan utilities/Buffer.hpp"

namespace vk
{
	enum class LightType
	{
		Directional,
		Spot
	};

	struct Light
	{
		LightType Type = LightType::Directional;
		glm::vec4 position;
		glm::vec4 colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glm::mat4 LightSpaceMatrix = glm::mat4(1.0f);
	};
}