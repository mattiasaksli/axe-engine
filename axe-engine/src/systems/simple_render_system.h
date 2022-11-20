#pragma once

#include "axe_device.h"
#include "axe_pipeline.h"
#include "axe_frame_info.h"

#include <memory>

namespace Axe
{
	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem( AxeDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout );
		~SimpleRenderSystem();

		SimpleRenderSystem( const SimpleRenderSystem& ) = delete;
		SimpleRenderSystem& operator=( const SimpleRenderSystem& ) = delete;
		SimpleRenderSystem( const SimpleRenderSystem&& ) = delete;
		SimpleRenderSystem& operator=( const SimpleRenderSystem&& ) = delete;


		void RenderGameObjects( const FrameInfo& frameInfo ) const;

	private:
		AxeDevice& axeDevice;

		VkPipelineLayout pipelineLayout = {};
		std::unique_ptr<AxePipeline> axePipeline;

		void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void CreatePipeline( VkRenderPass renderPass );
	};
}
