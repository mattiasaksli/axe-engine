#include "point_light_system.h"

#include <stdexcept>
#include <ranges>
#include <map>

namespace Axe
{
	struct PointLightPushConstants
	{
		glm::vec4 position{};
		glm::vec4 color{};
		float radius = 0;
	};

	PointLightSystem::PointLightSystem( AxeDevice& device, const VkRenderPass renderPass, const VkDescriptorSetLayout globalSetLayout )
		: axeDevice{ device }
	{
		CreatePipelineLayout( globalSetLayout );
		CreatePipeline( renderPass );
	}

	PointLightSystem::~PointLightSystem()
	{
		vkDestroyPipelineLayout( axeDevice.Device(), pipelineLayout, nullptr );
	}

	void PointLightSystem::CreatePipelineLayout( const VkDescriptorSetLayout globalSetLayout )
	{
		// Used for specifying uniform variables
		VkPushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof( PointLightPushConstants );

		const std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { globalSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if ( vkCreatePipelineLayout( axeDevice.Device(), &pipelineLayoutInfo, nullptr, &pipelineLayout ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create pipeline layout" );
		}
	}

	void PointLightSystem::CreatePipeline( const VkRenderPass renderPass )
	{
		assert( pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout" );

		PipelineConfigInfo pipelineConfig = {};
		AxePipeline::DefaultPipelineConfigInfo( pipelineConfig );
		AxePipeline::EnableAlphaBlending( pipelineConfig );
		pipelineConfig.bindingDescriptions.clear();
		pipelineConfig.attributeDescriptions.clear();
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;

		axePipeline = std::make_unique<AxePipeline>(
			axeDevice,
			pipelineConfig,
			"shaders/point_light.vert.spv",
			"shaders/point_light.frag.spv"
		);
	}

	void PointLightSystem::Update( const FrameInfo& frameInfo, GlobalUBO& ubo ) const
	{
		const auto rotateLight = glm::rotate(
			glm::mat4{ 1.0f },
			frameInfo.frameTime,
			glm::vec3{ 0.0f, -1.0f, 0.0f }
		);

		int lightIndex = 0;
		for ( auto& gameObject : frameInfo.gameObjects | std::views::values )
		{
			if ( gameObject.pointLight == nullptr )
			{
				continue;
			}

			assert( lightIndex < MAX_LIGHTS && "Point lights amount exceeds MAX_LIGHTS" );

			// Update light position
			gameObject.transform.translation = glm::vec3{ rotateLight * glm::vec4( gameObject.transform.translation, 1.0f ) };

			// Copy light to ubo
			ubo.pointLights[ lightIndex ].position = glm::vec4{ gameObject.transform.translation, 1.0f };
			ubo.pointLights[ lightIndex ].color = glm::vec4{ gameObject.color, gameObject.pointLight->lightIntensity };

			lightIndex++;
		}

		ubo.numActiveLights = lightIndex;
	}

	void PointLightSystem::Render( const FrameInfo& frameInfo ) const
	{
		// Sort lights
		std::map<float, AxeGameObject::UID> sortedLights;
		for ( const auto& gameObject : frameInfo.gameObjects | std::views::values )
		{
			if ( gameObject.pointLight == nullptr )
			{
				continue;
			}

			auto offset = frameInfo.camera.GetWorldSpacePosition() - gameObject.transform.translation;
			float distanceSquared = glm::dot( offset, offset );
			sortedLights[ distanceSquared ] = gameObject.GetId();
		}

		axePipeline->Bind( frameInfo.commandBuffer );

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr
		);

		// Iterate through sorted lights in reverse order
		for ( auto it = sortedLights.rbegin(); it != sortedLights.rend(); ++it )
		{
			auto& gameObject = frameInfo.gameObjects.at( it->second );

			PointLightPushConstants pushConstants = {};
			pushConstants.position = glm::vec4( gameObject.transform.translation, 1.0f );
			pushConstants.color = glm::vec4( gameObject.color, gameObject.pointLight->lightIntensity );
			pushConstants.radius = gameObject.transform.scale.x;

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof( PointLightPushConstants ),
				&pushConstants
			);
			vkCmdDraw( frameInfo.commandBuffer, 6, 1, 0, 0 );
		}
	}
}
