#pragma once

#include "axe_device.h"
#include "axe_buffer.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>

namespace Axe
{
	class AxeModel
	{
	public:
		struct Vertex
		{
			glm::vec3 position = {};
			glm::vec3 color = {};
			glm::vec3 normal = {};
			glm::vec2 uv = {};

			static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

			bool operator==( const Vertex& other ) const
			{
				return position == other.position
				       && color == other.color
				       && normal == other.normal
				       && uv == other.uv;
			}
		};

		struct Data
		{
			std::vector<Vertex> vertices = {};
			std::vector<uint32_t> indices = {};

			void LoadModel( const std::string& filePath );
		};

		static std::unique_ptr<AxeModel> CreateModelFromFile( AxeDevice& device, const std::string& filePath );

		AxeModel( AxeDevice& device, const AxeModel::Data& data );
		~AxeModel();

		AxeModel( const AxeModel& ) = delete;
		AxeModel& operator=( const AxeModel& ) = delete;
		AxeModel( const AxeModel&& ) = delete;
		AxeModel& operator=( const AxeModel&& ) = delete;

		void Bind( VkCommandBuffer commandBuffer ) const;
		void Draw( VkCommandBuffer commandBuffer ) const;

	private:
		AxeDevice& axeDevice;

		std::unique_ptr<AxeBuffer> vertexBuffer;
		uint32_t vertexCount = 0;

		bool hasIndexBuffer = false;
		std::unique_ptr<AxeBuffer> indexBuffer;
		uint32_t indexCount = 0;

		void CreateVertexBuffers( const std::vector<Vertex>& vertices );
		void CreateIndexBuffers( const std::vector<uint32_t>& indices );
	};
}
