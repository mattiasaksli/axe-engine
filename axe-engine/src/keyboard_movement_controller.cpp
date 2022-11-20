#include "keyboard_movement_controller.h"

namespace Axe
{
	void KeyboardMovementController::moveInPlaneXZ( GLFWwindow* window, const float deltaTime, AxeGameObject& gameObject ) const
	{
		// Look direction

		glm::vec3 rotation{ 0 };
		if ( glfwGetKey( window, keys.lookRight ) == GLFW_PRESS ) rotation.y += 1.0f;
		if ( glfwGetKey( window, keys.lookLeft ) == GLFW_PRESS ) rotation.y -= 1.0f;
		if ( glfwGetKey( window, keys.lookUp ) == GLFW_PRESS ) rotation.x += 1.0f;
		if ( glfwGetKey( window, keys.lookDown ) == GLFW_PRESS ) rotation.x -= 1.0f;

		if ( glm::length( rotation ) > glm::epsilon<float>() )
		{
			gameObject.transform.rotation += lookSpeed * deltaTime * glm::normalize( rotation );
		}

		// Limit pitch values between about +/- 85 degrees
		gameObject.transform.rotation.x = glm::clamp( gameObject.transform.rotation.x, -1.5f, 1.5f );

		gameObject.transform.rotation.y = glm::mod( gameObject.transform.rotation.y, glm::two_pi<float>() );

		// Move direction

		const float yaw = gameObject.transform.rotation.y;
		const glm::vec3 forwardDir{ sin( yaw ), 0.0f, cos( yaw ) };
		const glm::vec3 rightDir{ forwardDir.z, 0.0f, -forwardDir.x };
		constexpr glm::vec3 upDir{ 0.0f, -1.0f, 0.0f };

		glm::vec3 moveDir{0.0f};
		if ( glfwGetKey( window, keys.moveForward ) == GLFW_PRESS ) moveDir += forwardDir;
		if ( glfwGetKey( window, keys.moveBackward ) == GLFW_PRESS ) moveDir -= forwardDir;
		if ( glfwGetKey( window, keys.moveRight ) == GLFW_PRESS ) moveDir += rightDir;
		if ( glfwGetKey( window, keys.moveLeft ) == GLFW_PRESS ) moveDir -= rightDir;
		if ( glfwGetKey( window, keys.moveUp ) == GLFW_PRESS ) moveDir += upDir;
		if ( glfwGetKey( window, keys.moveDown ) == GLFW_PRESS ) moveDir -= upDir;

		if ( glm::length( moveDir ) > glm::epsilon<float>() )
		{
			gameObject.transform.translation += moveSpeed * deltaTime * glm::normalize( moveDir );
		}
	}
}
