#include "axe_pipeline.h"

#include "axe_model.h"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cassert>

namespace Axe
{
	AxePipeline::AxePipeline(
		AxeDevice& device,
		const PipelineConfigInfo& pipelineConfig,
		const std::string& vertFilePath,
		const std::string& fragFilePath
	) : axeDevice{ device }
	{
		CreateGraphicsPipeline( vertFilePath, fragFilePath, pipelineConfig );
	}

	AxePipeline::~AxePipeline()
	{
		vkDestroyShaderModule( axeDevice.Device(), vertShaderModule, nullptr );
		vkDestroyShaderModule( axeDevice.Device(), fragShaderModule, nullptr );
		vkDestroyPipeline( axeDevice.Device(), graphicsPipeline, nullptr );
	}


	std::vector<char> AxePipeline::ReadFile( const std::string& filepath )
	{
		std::ifstream file{ filepath, std::ios::ate | std::ios::binary }; // ate goes to the end of the file

		if ( !file.is_open() )
		{
			const std::string faultyFile = std::filesystem::absolute( std::filesystem::path{ filepath } ).string();
			throw std::runtime_error( "Failed to open file: " + faultyFile );
		}

		const int64_t fileSize = file.tellg();
		// Since we're at the end of the file, tellg() gets the last position of the file, which is the file size

		std::vector<char> buffer( fileSize );

		file.seekg( 0 );
		file.read( buffer.data(), fileSize );

		file.close();

		return buffer;
	}

	void AxePipeline::CreateGraphicsPipeline(
		const std::string& vertFilePath,
		const std::string& fragFilePath,
		const PipelineConfigInfo& pipelineConfig
	)
	{
		assert( pipelineConfig.pipelineLayout != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: no pipelineLayout provided in pipelineConfig"
		);
		assert( pipelineConfig.renderPass != VK_NULL_HANDLE &&
			"Cannot create graphics pipeline: no renderPass provided in pipelineConfig"
		);

		// ####################   Setup shader modules   ####################

		const auto vertCode = ReadFile( vertFilePath );
		const auto fragCode = ReadFile( fragFilePath );

		CreateShaderModule( vertCode, &vertShaderModule );
		CreateShaderModule( fragCode, &fragShaderModule );

		// ####################   Setup shader stages from shader modules   ####################

		VkPipelineShaderStageCreateInfo shaderStages[ 2 ] = {};

		shaderStages[ 0 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[ 0 ].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[ 0 ].module = vertShaderModule;
		shaderStages[ 0 ].pName = "main";
		shaderStages[ 0 ].flags = 0;
		shaderStages[ 0 ].pNext = nullptr;
		shaderStages[ 0 ].pSpecializationInfo = nullptr;

		shaderStages[ 1 ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[ 1 ].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[ 1 ].module = fragShaderModule;
		shaderStages[ 1 ].pName = "main";
		shaderStages[ 1 ].flags = 0;
		shaderStages[ 1 ].pNext = nullptr;
		shaderStages[ 1 ].pSpecializationInfo = nullptr;

		// ####################   Setup vertex input   ####################

		const auto& bindingDescriptions = pipelineConfig.bindingDescriptions;
		const auto& attributeDescriptions = pipelineConfig.attributeDescriptions;

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// ####################   Setup graphics pipeline   ####################

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &pipelineConfig.inputAssemblyInfo;
		pipelineInfo.pViewportState = &pipelineConfig.viewportInfo;
		pipelineInfo.pRasterizationState = &pipelineConfig.rasterizationInfo;
		pipelineInfo.pMultisampleState = &pipelineConfig.multisampleInfo;
		pipelineInfo.pColorBlendState = &pipelineConfig.colorBlendInfo;
		pipelineInfo.pDepthStencilState = &pipelineConfig.depthStencilInfo;
		pipelineInfo.pDynamicState = &pipelineConfig.dynamicStateInfo;

		pipelineInfo.layout = pipelineConfig.pipelineLayout;
		pipelineInfo.renderPass = pipelineConfig.renderPass;
		pipelineInfo.subpass = pipelineConfig.subpass;

		// The GPU can create a pipeline faster during runtime if it can derive the new pipeline from an existing one (but we don't need to use this yet)
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = nullptr;

		if ( vkCreateGraphicsPipelines( axeDevice.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create graphics pipeline" );
		}
	}

	void AxePipeline::CreateShaderModule( const std::vector<char>& shaderCode, VkShaderModule* shaderModule ) const
	{
		VkShaderModuleCreateInfo createShaderInfo = {};
		createShaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createShaderInfo.codeSize = shaderCode.size();
		createShaderInfo.pCode = reinterpret_cast<const uint32_t *>(shaderCode.data());

		if ( vkCreateShaderModule( axeDevice.Device(), &createShaderInfo, nullptr, shaderModule ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create shader module" );
		}
	}

	void AxePipeline::Bind( const VkCommandBuffer commandBuffer ) const
	{
		vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );
	}

	void AxePipeline::DefaultPipelineConfigInfo( PipelineConfigInfo& pipelineConfig )
	{
		// ######################################################################################################################
		// #################################################   Input Assembly   #################################################
		// ######################################################################################################################

		pipelineConfig.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineConfig.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineConfig.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		// ######################################################################################################################
		// ##############################################   Viewport and Scissor   ##############################################
		// ######################################################################################################################

		pipelineConfig.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		pipelineConfig.viewportInfo.viewportCount = 1;
		pipelineConfig.viewportInfo.pViewports = nullptr;
		pipelineConfig.viewportInfo.scissorCount = 1;
		pipelineConfig.viewportInfo.pScissors = nullptr;

		// #####################################################################################################################
		// #################################################   Rasterization   #################################################
		// #####################################################################################################################

		pipelineConfig.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		pipelineConfig.rasterizationInfo.depthClampEnable = VK_FALSE;
		pipelineConfig.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		pipelineConfig.rasterizationInfo.lineWidth = 1.0f;
		pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		pipelineConfig.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		pipelineConfig.rasterizationInfo.depthBiasEnable = VK_FALSE;
		pipelineConfig.rasterizationInfo.depthBiasConstantFactor = 0.0f;  // Optional
		pipelineConfig.rasterizationInfo.depthBiasClamp = 0.0f;           // Optional
		pipelineConfig.rasterizationInfo.depthBiasSlopeFactor = 0.0f;     // Optional

		// #####################################################################################################################
		// #################################################   Multisampling   #################################################
		// #####################################################################################################################

		pipelineConfig.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		pipelineConfig.multisampleInfo.sampleShadingEnable = VK_FALSE;
		pipelineConfig.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		pipelineConfig.multisampleInfo.minSampleShading = 1.0f;           // Optional
		pipelineConfig.multisampleInfo.pSampleMask = nullptr;             // Optional
		pipelineConfig.multisampleInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
		pipelineConfig.multisampleInfo.alphaToOneEnable = VK_FALSE;       // Optional

		// ######################################################################################################################
		// #################################################   Color Blending   #################################################
		// ######################################################################################################################

		pipelineConfig.colorBlendAttachment.blendEnable = VK_FALSE;
		pipelineConfig.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
		pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
		pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional

		pipelineConfig.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		pipelineConfig.colorBlendInfo.logicOpEnable = VK_FALSE;
		pipelineConfig.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  // Optional
		pipelineConfig.colorBlendInfo.attachmentCount = 1;
		pipelineConfig.colorBlendInfo.pAttachments = &pipelineConfig.colorBlendAttachment;
		pipelineConfig.colorBlendInfo.blendConstants[ 0 ] = 0.0f;  // Optional
		pipelineConfig.colorBlendInfo.blendConstants[ 1 ] = 0.0f;  // Optional
		pipelineConfig.colorBlendInfo.blendConstants[ 2 ] = 0.0f;  // Optional
		pipelineConfig.colorBlendInfo.blendConstants[ 3 ] = 0.0f;  // Optional

		// #######################################################################################################################
		// ##############################################   Depth/Stencil Testing   ##############################################
		// #######################################################################################################################

		pipelineConfig.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
		pipelineConfig.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		pipelineConfig.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.minDepthBounds = 0.0f;  // Optional
		pipelineConfig.depthStencilInfo.maxDepthBounds = 1.0f;  // Optional
		pipelineConfig.depthStencilInfo.stencilTestEnable = VK_FALSE;
		pipelineConfig.depthStencilInfo.front = {};  // Optional
		pipelineConfig.depthStencilInfo.back = {};   // Optional

		// #######################################################################################################################
		// ##################################################   Dynamic State   ##################################################
		// #######################################################################################################################

		pipelineConfig.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		pipelineConfig.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		pipelineConfig.dynamicStateInfo.pDynamicStates = pipelineConfig.dynamicStateEnables.data();
		pipelineConfig.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(pipelineConfig.dynamicStateEnables.size());
		pipelineConfig.dynamicStateInfo.flags = 0;

		pipelineConfig.bindingDescriptions = AxeModel::Vertex::GetBindingDescriptions();
		pipelineConfig.attributeDescriptions = AxeModel::Vertex::GetAttributeDescriptions();
	}

	void AxePipeline::EnableAlphaBlending( PipelineConfigInfo& pipelineConfig )
	{
		pipelineConfig.colorBlendAttachment.blendEnable = VK_TRUE;

		pipelineConfig.colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		pipelineConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}
}
