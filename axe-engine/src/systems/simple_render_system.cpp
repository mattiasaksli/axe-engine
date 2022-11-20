#include "simple_render_system.h"

#include <glm/glm.hpp>

#include <stdexcept>
#include <ranges>

namespace Axe
{
	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{ 1.0f };
		glm::mat4 normalMatrix{ 1.0f };
	};

	SimpleRenderSystem::SimpleRenderSystem( AxeDevice& device, const VkRenderPass renderPass, const VkDescriptorSetLayout globalSetLayout )
		: axeDevice{ device }
	{
		CreatePipelineLayout( globalSetLayout );
		CreatePipeline( renderPass );
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout( axeDevice.Device(), pipelineLayout, nullptr );
	}

	void SimpleRenderSystem::CreatePipelineLayout( const VkDescriptorSetLayout globalSetLayout )
	{
		// Used for specifying uniform variables
		VkPushConstantRange pushConstantRange;
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof( SimplePushConstantData );

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

	void SimpleRenderSystem::CreatePipeline( const VkRenderPass renderPass )
	{
		assert( pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout" );

		PipelineConfigInfo pipelineConfig = {};
		AxePipeline::DefaultPipelineConfigInfo( pipelineConfig );
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;

		axePipeline = std::make_unique<AxePipeline>(
			axeDevice,
			pipelineConfig,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv"
		);
	}

	void SimpleRenderSystem::RenderGameObjects( const FrameInfo& frameInfo ) const
	{
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

		for ( auto& gameObject : frameInfo.gameObjects | std::views::values )
		{
			// Skip the gameObject if there's no model to render			TODO: implement ECS instead
			if ( gameObject.model == nullptr )
			{
				continue;
			}

			SimplePushConstantData push = {};
			push.modelMatrix = gameObject.transform.Mat4();
			push.normalMatrix = gameObject.transform.NormalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof( SimplePushConstantData ),
				&push
			);

			gameObject.model->Bind( frameInfo.commandBuffer );
			gameObject.model->Draw( frameInfo.commandBuffer );
		}
	}
}
