#pragma once

#include <glm/glm.hpp>

#include "Image.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

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
        float view = -10.0f;//-13.0f; // TODO: this is temp and needs to be removed in the future
		float near = 0.1f;
        float far = 55.0f;
	};
}