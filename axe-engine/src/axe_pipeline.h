#pragma once

#include "axe_device.h"

#include <string>
#include <vector>

namespace Axe
{
	struct PipelineConfigInfo
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions = {};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};

		VkPipelineViewportStateCreateInfo viewportInfo = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
		VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};

		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};

		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;

		PipelineConfigInfo() = default;
		~PipelineConfigInfo() = default;
		
		PipelineConfigInfo( const PipelineConfigInfo& ) = delete;
		PipelineConfigInfo& operator=( const PipelineConfigInfo& ) = delete;
		PipelineConfigInfo( const PipelineConfigInfo&& ) = delete;
		PipelineConfigInfo& operator=( const PipelineConfigInfo&& ) = delete;
	};

	class AxePipeline
	{
	public:
		AxePipeline(
			AxeDevice& device,
			const PipelineConfigInfo& pipelineConfig,
			const std::string& vertFilePath,
			const std::string& fragFilePath
		);
		~AxePipeline();

		AxePipeline( const AxePipeline& ) = delete;
		AxePipeline& operator=( const AxePipeline& ) = delete;
		AxePipeline( const AxePipeline&& ) = delete;
		AxePipeline& operator=( const AxePipeline&& ) = delete;

		void Bind( VkCommandBuffer commandBuffer ) const;

		static void DefaultPipelineConfigInfo( PipelineConfigInfo& pipelineConfig );
		static void EnableAlphaBlending( PipelineConfigInfo& pipelineConfig );

	private:
		AxeDevice& axeDevice;	// This will outlive any instance of AxePipeline, so it won't turn into a dangling pointer
		VkPipeline graphicsPipeline = {};
		VkShaderModule vertShaderModule = {};
		VkShaderModule fragShaderModule = {};

		static std::vector<char> ReadFile( const std::string& filepath );

		void CreateGraphicsPipeline(
			const std::string& vertFilePath,
			const std::string& fragFilePath,
			const PipelineConfigInfo& pipelineConfig
		);

		void CreateShaderModule( const std::vector<char>& shaderCode, VkShaderModule* shaderModule ) const;
	};
}
