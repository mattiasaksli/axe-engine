﻿#include "axe_camera.h"

#include <cassert>
#include <glm/ext/scalar_constants.hpp>

namespace Axe
{
	void AxeCamera::SetOrthographicProjection(
		const float left, const float right, const float top, const float bottom, const float near, const float far )
	{
		projectionMatrix = glm::mat4{ 1.0f };
		projectionMatrix[ 0 ][ 0 ] = 2.0f / ( right - left );
		projectionMatrix[ 1 ][ 1 ] = 2.0f / ( bottom - top );
		projectionMatrix[ 2 ][ 2 ] = 1.0f / ( far - near );
		projectionMatrix[ 3 ][ 0 ] = -( right + left ) / ( right - left );
		projectionMatrix[ 3 ][ 1 ] = -( bottom + top ) / ( bottom - top );
		projectionMatrix[ 3 ][ 2 ] = -near / ( far - near );
	}

	void AxeCamera::SetPerspectiveProjection( const float fovY, const float aspectRatio, const float near, const float far )
	{
		assert( glm::abs(aspectRatio - glm::epsilon<float>()) > 0.0f && "Perspective projection aspect ration cannot be 0" );

		const float tanHalfFovy = tan( fovY / 2.0f );
		projectionMatrix = glm::mat4{ 0.0f };
		projectionMatrix[ 0 ][ 0 ] = 1.0f / ( aspectRatio * tanHalfFovy );
		projectionMatrix[ 1 ][ 1 ] = 1.0f / ( tanHalfFovy );
		projectionMatrix[ 2 ][ 2 ] = far / ( far - near );
		projectionMatrix[ 2 ][ 3 ] = 1.0f;
		projectionMatrix[ 3 ][ 2 ] = -( far * near ) / ( far - near );
	}

	void AxeCamera::SetViewDirection( const glm::vec3 position, const glm::vec3 direction, const glm::vec3 up )
	{
		assert( glm::abs(glm::length( direction ) - glm::epsilon<float>()) > 0.0f && "Camera view direction vector length cannot be 0" );

		// Orthonormal basis vectors
		const glm::vec3 w{ glm::normalize( direction ) };
		const glm::vec3 u{ glm::normalize( glm::cross( w, up ) ) };
		const glm::vec3 v{ glm::cross( w, u ) };

		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[ 0 ][ 0 ] = u.x;
		viewMatrix[ 1 ][ 0 ] = u.y;
		viewMatrix[ 2 ][ 0 ] = u.z;
		viewMatrix[ 0 ][ 1 ] = v.x;
		viewMatrix[ 1 ][ 1 ] = v.y;
		viewMatrix[ 2 ][ 1 ] = v.z;
		viewMatrix[ 0 ][ 2 ] = w.x;
		viewMatrix[ 1 ][ 2 ] = w.y;
		viewMatrix[ 2 ][ 2 ] = w.z;
		viewMatrix[ 3 ][ 0 ] = -glm::dot( u, position );
		viewMatrix[ 3 ][ 1 ] = -glm::dot( v, position );
		viewMatrix[ 3 ][ 2 ] = -glm::dot( w, position );

		inverseViewMatrix = glm::mat4{1.f};
		inverseViewMatrix[0][0] = u.x;
		inverseViewMatrix[0][1] = u.y;
		inverseViewMatrix[0][2] = u.z;
		inverseViewMatrix[1][0] = v.x;
		inverseViewMatrix[1][1] = v.y;
		inverseViewMatrix[1][2] = v.z;
		inverseViewMatrix[2][0] = w.x;
		inverseViewMatrix[2][1] = w.y;
		inverseViewMatrix[2][2] = w.z;
		inverseViewMatrix[3][0] = position.x;
		inverseViewMatrix[3][1] = position.y;
		inverseViewMatrix[3][2] = position.z;
	}

	void AxeCamera::SetViewTarget( const glm::vec3 position, const glm::vec3 target, const glm::vec3 up )
	{
		SetViewDirection( position, target - position, up );
	}

	void AxeCamera::SetViewYXZ( const glm::vec3 position, const glm::vec3 rotation )
	{
		// Inverse (transpose) of the rotation matrix
		const float c3 = glm::cos( rotation.z );
		const float s3 = glm::sin( rotation.z );
		const float c2 = glm::cos( rotation.x );
		const float s2 = glm::sin( rotation.x );
		const float c1 = glm::cos( rotation.y );
		const float s1 = glm::sin( rotation.y );
		const glm::vec3 u{ ( c1 * c3 + s1 * s2 * s3 ), ( c2 * s3 ), ( c1 * s2 * s3 - c3 * s1 ) };
		const glm::vec3 v{ ( c3 * s1 * s2 - c1 * s3 ), ( c2 * c3 ), ( c1 * c3 * s2 + s1 * s3 ) };
		const glm::vec3 w{ ( c2 * s1 ), ( -s2 ), ( c1 * c2 ) };
		viewMatrix = glm::mat4{ 1.f };
		viewMatrix[ 0 ][ 0 ] = u.x;
		viewMatrix[ 1 ][ 0 ] = u.y;
		viewMatrix[ 2 ][ 0 ] = u.z;
		viewMatrix[ 0 ][ 1 ] = v.x;
		viewMatrix[ 1 ][ 1 ] = v.y;
		viewMatrix[ 2 ][ 1 ] = v.z;
		viewMatrix[ 0 ][ 2 ] = w.x;
		viewMatrix[ 1 ][ 2 ] = w.y;
		viewMatrix[ 2 ][ 2 ] = w.z;
		viewMatrix[ 3 ][ 0 ] = -glm::dot( u, position );
		viewMatrix[ 3 ][ 1 ] = -glm::dot( v, position );
		viewMatrix[ 3 ][ 2 ] = -glm::dot( w, position );

		inverseViewMatrix = glm::mat4{1.f};
		inverseViewMatrix[0][0] = u.x;
		inverseViewMatrix[0][1] = u.y;
		inverseViewMatrix[0][2] = u.z;
		inverseViewMatrix[1][0] = v.x;
		inverseViewMatrix[1][1] = v.y;
		inverseViewMatrix[1][2] = v.z;
		inverseViewMatrix[2][0] = w.x;
		inverseViewMatrix[2][1] = w.y;
		inverseViewMatrix[2][2] = w.z;
		inverseViewMatrix[3][0] = position.x;
		inverseViewMatrix[3][1] = position.y;
		inverseViewMatrix[3][2] = position.z;
	}
}
