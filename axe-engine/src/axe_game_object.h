#pragma once

#include "axe_model.h"

#include <glm/gtc/matrix_transform.hpp>

#include <unordered_map>

namespace Axe
{
	struct TransformComponent
	{
		glm::vec3 translation = {};
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
		glm::vec3 rotation = {};

		[[nodiscard]] glm::mat4 Mat4() const;
		[[nodiscard]] glm::mat3 NormalMatrix() const;
	};

	struct PointLightComponent
	{
		float lightIntensity = 1.0f;
	};

	class AxeGameObject
	{
	public:
		using UID = unsigned int;
		using Map = std::unordered_map<UID, AxeGameObject>;

		static AxeGameObject CreateGameObject()
		{
			static UID currentId = 0;
			return AxeGameObject{ currentId++ };
		}

		static AxeGameObject MakePointLight( float intensity = 5.0f, float radius = 0.1f, glm::vec3 color = glm::vec3{ 1.0f } );

		glm::vec3 color = {};
		TransformComponent transform = {};

		// Optional pointer components
		std::shared_ptr<AxeModel> model;
		std::unique_ptr<PointLightComponent> pointLight = nullptr;

		AxeGameObject( const AxeGameObject& ) = delete;
		AxeGameObject& operator=( const AxeGameObject& ) = delete;
		AxeGameObject( AxeGameObject&& ) = default;
		AxeGameObject& operator=( AxeGameObject&& ) = default;

		[[nodiscard]] UID GetId() const { return id; }

	private:
		UID id;

		explicit AxeGameObject( const UID objectId ) : id{ objectId } {}
	};
}
