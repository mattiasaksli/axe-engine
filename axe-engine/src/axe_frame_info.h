#pragma once

#include "axe_camera.h"
#include "axe_game_object.h"

#include <vulkan/vulkan.h>

namespace Axe
{
	constexpr int MAX_LIGHTS = 10;

	struct PointLight
	{
		glm::vec4 position{}; // ignore w
		glm::vec4 color{}; // w is intensity
	};

	struct GlobalUBO
	{
		glm::mat4 projectionMatrix{ 1.0f };
		glm::mat4 viewMatrix{ 1.0f };
		glm::mat4 inverseViewMatrix{1.0f};
		glm::vec4 ambientColor{ 1.0f, 1.0f, 1.0f, 0.02f }; // w is intensity
		PointLight pointLights[ MAX_LIGHTS ];
		int numActiveLights = 0;
	};

	struct FrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		AxeCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		AxeGameObject::Map& gameObjects;
	};
}
