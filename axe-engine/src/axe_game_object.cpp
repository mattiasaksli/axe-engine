#include "axe_game_object.h"

namespace Axe
{
	// Returns an affine transformation matrix with the transformations being
	// Translate * Ry * Rx * Rz * Scale (in the right to left order)
	glm::mat4 TransformComponent::Mat4() const
	{
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// Reading from left to right - the rotations are extrinsic (in world space)
		// Reading from right to left - the rotations are intrinsic (in local space)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix

		const float c3 = glm::cos( rotation.z );
		const float s3 = glm::sin( rotation.z );
		const float c2 = glm::cos( rotation.x );
		const float s2 = glm::sin( rotation.x );
		const float c1 = glm::cos( rotation.y );
		const float s1 = glm::sin( rotation.y );

		return glm::mat4{
			{
				scale.x * ( c1 * c3 + s1 * s2 * s3 ),
				scale.x * ( c2 * s3 ),
				scale.x * ( c1 * s2 * s3 - c3 * s1 ),
				0.0f,
			},
			{
				scale.y * ( c3 * s1 * s2 - c1 * s3 ),
				scale.y * ( c2 * c3 ),
				scale.y * ( c1 * c3 * s2 + s1 * s3 ),
				0.0f,
			},
			{
				scale.z * ( c2 * s1 ),
				scale.z * ( -s2 ),
				scale.z * ( c1 * c2 ),
				0.0f,
			},
			{ translation.x, translation.y, translation.z, 1.0f }
		};
	}

	glm::mat3 TransformComponent::NormalMatrix() const
	{
		const float c3 = glm::cos( rotation.z );
		const float s3 = glm::sin( rotation.z );
		const float c2 = glm::cos( rotation.x );
		const float s2 = glm::sin( rotation.x );
		const float c1 = glm::cos( rotation.y );
		const float s1 = glm::sin( rotation.y );
		const glm::vec3 inverseScale = 1.0f / scale;

		return glm::mat3{
			{
				inverseScale.x * ( c1 * c3 + s1 * s2 * s3 ),
				inverseScale.x * ( c2 * s3 ),
				inverseScale.x * ( c1 * s2 * s3 - c3 * s1 ),
			},
			{
				inverseScale.y * ( c3 * s1 * s2 - c1 * s3 ),
				inverseScale.y * ( c2 * c3 ),
				inverseScale.y * ( c1 * c3 * s2 + s1 * s3 ),
			},
			{
				inverseScale.z * ( c2 * s1 ),
				inverseScale.z * ( -s2 ),
				inverseScale.z * ( c1 * c2 ),
			}
		};
	}

	AxeGameObject AxeGameObject::MakePointLight( float intensity, float radius, glm::vec3 color )
	{
		AxeGameObject gameObject = AxeGameObject::CreateGameObject();
		gameObject.color = color;
		gameObject.transform.scale.x = radius;

		gameObject.pointLight = std::make_unique<PointLightComponent>(  );
		gameObject.pointLight->lightIntensity = intensity;

		return gameObject;
	}
}
