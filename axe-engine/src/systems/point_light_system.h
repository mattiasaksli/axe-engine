#pragma once

#include "axe_device.h"
#include "axe_pipeline.h"
#include "axe_frame_info.h"

#include <memory>

namespace Axe
{
	class PointLightSystem
	{
	public:
		PointLightSystem( AxeDevice& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout );
		~PointLightSystem();

		PointLightSystem( const PointLightSystem& ) = delete;
		PointLightSystem& operator=( const PointLightSystem& ) = delete;
		PointLightSystem( const PointLightSystem&& ) = delete;
		PointLightSystem& operator=( const PointLightSystem&& ) = delete;

		void Update( const FrameInfo& frameInfo, GlobalUBO& ubo ) const;
		void Render( const FrameInfo& frameInfo ) const;

	private:
		AxeDevice& axeDevice;

		VkPipelineLayout pipelineLayout = {};
		std::unique_ptr<AxePipeline> axePipeline;

		void CreatePipelineLayout( VkDescriptorSetLayout globalSetLayout );
		void CreatePipeline( VkRenderPass renderPass );
	};
}
