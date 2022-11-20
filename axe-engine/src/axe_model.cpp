#include "axe_model.h"

#include "axe_utils.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <cassert>
#include <unordered_map>

namespace std
{
	template <>
	struct hash<Axe::AxeModel::Vertex>
	{
		size_t operator()( const Axe::AxeModel::Vertex& vertex ) const noexcept
		{
			size_t seed = 0;
			Axe::HashCombine( seed, vertex.position, vertex.color, vertex.normal, vertex.uv );

			return seed;
		}
	};
}

namespace Axe
{
	AxeModel::AxeModel( AxeDevice& device, const Data& data )
		: axeDevice{ device }
	{
		CreateVertexBuffers( data.vertices );
		CreateIndexBuffers( data.indices );
	}

	AxeModel::~AxeModel() {}

	std::unique_ptr<AxeModel> AxeModel::CreateModelFromFile( AxeDevice& device, const std::string& filePath )
	{
		Data data{};
		data.LoadModel( filePath );

		std::cout << "Loaded model '" << filePath << "' with " << data.vertices.size() << " unique vertices\n";

		return std::make_unique<AxeModel>( device, data );
	}

	void AxeModel::Bind( VkCommandBuffer commandBuffer ) const
	{
		const VkBuffer buffers[ ] = { vertexBuffer->GetBufferHandle() };
		constexpr VkDeviceSize offsets[ ] = { 0 };

		vkCmdBindVertexBuffers( commandBuffer, 0, 1, buffers, offsets );

		if ( hasIndexBuffer )
		{
			vkCmdBindIndexBuffer( commandBuffer, indexBuffer->GetBufferHandle(), 0, VK_INDEX_TYPE_UINT32 );
		}
	}

	void AxeModel::Draw( VkCommandBuffer commandBuffer ) const
	{
		if ( hasIndexBuffer )
		{
			vkCmdDrawIndexed( commandBuffer, indexCount, 1, 0, 0, 0 );
		}
		else
		{
			vkCmdDraw( commandBuffer, vertexCount, 1, 0, 0 );
		}
	}

	std::vector<VkVertexInputBindingDescription> AxeModel::Vertex::GetBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions( 1 );
		bindingDescriptions[ 0 ].binding = 0;
		bindingDescriptions[ 0 ].stride = sizeof( Vertex );
		bindingDescriptions[ 0 ].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> AxeModel::Vertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};

		attributeDescriptions.push_back( { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, position ) } );
		attributeDescriptions.push_back( { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, color ) } );
		attributeDescriptions.push_back( { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( Vertex, normal ) } );
		attributeDescriptions.push_back( { 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof( Vertex, uv ) } );

		return attributeDescriptions;
	}

	void AxeModel::CreateVertexBuffers( const std::vector<Vertex>& vertices )
	{
		vertexCount = static_cast<uint32_t>(vertices.size());

		assert( vertexCount >= 3 && "Vertex count must be at least 3" );

		const VkDeviceSize bufferSize = sizeof( vertices[ 0 ] ) * vertexCount;
		constexpr uint32_t vertexSize = sizeof( vertices[ 0 ] );

		AxeBuffer stagingBuffer(
			axeDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	// Syncs the CPU mapped memory with the actual GPU memory
		);

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer( vertices.data() );

		vertexBuffer = std::make_unique<AxeBuffer>(
			axeDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		// Copy data from the (GPU) staging buffer into the (GPU) vertex buffer
		axeDevice.CopyBuffer( stagingBuffer.GetBufferHandle(), vertexBuffer->GetBufferHandle(), bufferSize );
	}

	void AxeModel::CreateIndexBuffers( const std::vector<uint32_t>& indices )
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;

		if ( !hasIndexBuffer )
		{
			return;
		}

		const VkDeviceSize bufferSize = sizeof( indices[ 0 ] ) * indexCount;
		constexpr uint32_t indexSize = sizeof( indices[ 0 ] );

		AxeBuffer stagingBuffer(
			axeDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	// Syncs the CPU mapped memory with the actual GPU memory
		);

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer( indices.data() );

		indexBuffer = std::make_unique<AxeBuffer>(
			axeDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		// Copy data from the (GPU) staging buffer into the (GPU) index buffer
		axeDevice.CopyBuffer( stagingBuffer.GetBufferHandle(), indexBuffer->GetBufferHandle(), bufferSize );
	}

	void AxeModel::Data::LoadModel( const std::string& filePath )
	{
		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warnings;
		std::string errors;

		if ( !tinyobj::LoadObj( &attributes, &shapes, &materials, &warnings, &errors, filePath.c_str() ) )
		{
			throw std::runtime_error( warnings + errors );
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

		for ( const auto& shape : shapes )
		{
			for ( const auto& index : shape.mesh.indices )
			{
				Vertex vertex = {};

				if ( index.vertex_index >= 0 )
				{
					vertex.position = {
						attributes.vertices[ 3 * index.vertex_index + 0 ],
						attributes.vertices[ 3 * index.vertex_index + 1 ],
						attributes.vertices[ 3 * index.vertex_index + 2 ],
					};

					vertex.color = {
						attributes.colors[ 3 * index.vertex_index + 0 ],
						attributes.colors[ 3 * index.vertex_index + 1 ],
						attributes.colors[ 3 * index.vertex_index + 2 ],
					};
				}

				if ( index.normal_index >= 0 )
				{
					vertex.normal = {
						attributes.normals[ 3 * index.normal_index + 0 ],
						attributes.normals[ 3 * index.normal_index + 1 ],
						attributes.normals[ 3 * index.normal_index + 2 ],
					};
				}

				if ( index.texcoord_index >= 0 )
				{
					vertex.uv = {
						attributes.texcoords[ 2 * index.texcoord_index + 0 ],
						attributes.texcoords[ 2 * index.texcoord_index + 1 ],
					};
				}

				// Only add to the vertex buffer if this is a new unique vertex
				if ( !uniqueVertices.contains( vertex ) )
				{
					uniqueVertices[ vertex ] = static_cast<uint32_t>(vertices.size());
					vertices.push_back( vertex );
				}
				indices.push_back( uniqueVertices[ vertex ] );
			}
		}
	}
}
