#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace Axe
{
	class AxeCamera
	{
	public:
		void SetOrthographicProjection( float left, float right, float top, float bottom, float near, float far );

		void SetPerspectiveProjection( float fovY, float aspectRatio, float near, float far );

		[[nodiscard]] const glm::mat4& GetProjection() const { return projectionMatrix; }

		// Camera
		[[nodiscard]] const glm::mat4& GetView() const { return viewMatrix; }
		[[nodiscard]] const glm::mat4& GetInverseView() const { return inverseViewMatrix; }
		[[nodiscard]] const glm::vec3 GetWorldSpacePosition() const { return glm::vec3{ inverseViewMatrix[3] }; }

		void SetViewDirection( glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f } );
		void SetViewTarget( glm::vec3 position, glm::vec3 target, glm::vec3 up = glm::vec3{ 0.0f, -1.0f, 0.0f } );
		void SetViewYXZ( glm::vec3 position, glm::vec3 rotation );

	private:
		glm::mat4 projectionMatrix{ 1.0f };
		glm::mat4 viewMatrix{ 1.0f };
		glm::mat4 inverseViewMatrix{ 1.0f };
	};
}
