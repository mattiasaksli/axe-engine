#include "axe_renderer.h"

#include <stdexcept>
#include <array>

namespace Axe
{
	AxeRenderer::AxeRenderer( AxeWindow& window, AxeDevice& device )
		: axeWindow{ window }, axeDevice{ device }
	{
		RecreateSwapChain();
		CreateCommandBuffers();
	}

	AxeRenderer::~AxeRenderer()
	{
		FreeCommandBuffers();
	}

	VkCommandBuffer AxeRenderer::BeginFrame()
	{
		assert( !isFrameStarted && "Cannot call BeginFrame() while frame is already in progress" );

		// Gets a handle to the next image to render to
		const auto result = axeSwapChain->AcquireNextImage( &currentImageIndex );

		// Check if the surface properties have changed and are no longer compatible with the swap chain
		if ( result == VK_ERROR_OUT_OF_DATE_KHR )
		{
			RecreateSwapChain();

			return nullptr;
		}

		if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
		{
			throw std::runtime_error( "Failed to aquire next swap chain image" );
		}

		isFrameStarted = true;

		const auto commandBuffer = GetCurrentCommandBuffer();


		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if ( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to begin recording command buffer" );
		}

		return commandBuffer;
	}

	void AxeRenderer::EndFrame()
	{
		assert( isFrameStarted && "Cannot call EndFrame() while frame is not in progress" );

		const auto commandBuffer = GetCurrentCommandBuffer();


		if ( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to end recording command buffer" );
		}

		// Submit the command buffer to have it render to that image and draw to the screen
		const auto result = axeSwapChain->SubmitCommandBuffers( &commandBuffer, &currentImageIndex );

		// Check if the surface properties have changed (even if the swap chain could be used to present to the surface)
		if ( result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || axeWindow.WasWindowResized() )
		{
			axeWindow.ResetWindowResizedFlag();
			RecreateSwapChain();
		}
		else if ( result != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to present swap chain image" );
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % AxeSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void AxeRenderer::BeginSwapChainRenderPass( const VkCommandBuffer commandBuffer ) const
	{
		assert( isFrameStarted && "Cannot call BeginSwapChainRenderPass() while frame is not in progress" );
		assert( commandBuffer == GetCurrentCommandBuffer() && "Cannot begin render pass on a command buffer from a different frame" );

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = axeSwapChain->GetRenderPass();
		renderPassInfo.framebuffer = axeSwapChain->GetFrameBuffer( currentImageIndex );

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = axeSwapChain->GetSwapChainExtent();

		// Values to clear frame buffer to
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[ 0 ].color = { { 0.005f, 0.005f, 0.005f, 1.0f } }; // Used by the color attachment, since we specified VK_ATTACHMENT_LOAD_OP_CLEAR
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

		// Set the dynamic viewport and scissor values
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(axeSwapChain->GetSwapChainExtent().width);
		viewport.height = static_cast<float>(axeSwapChain->GetSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		const VkRect2D scissor = { { 0, 0 }, axeSwapChain->GetSwapChainExtent() };

		vkCmdSetViewport( commandBuffer, 0, 1, &viewport );
		vkCmdSetScissor( commandBuffer, 0, 1, &scissor );
	}

	void AxeRenderer::EndSwapChainRenderPass( const VkCommandBuffer commandBuffer ) const
	{
		assert( isFrameStarted && "Cannot call EndSwapChainRenderPass() while frame is not in progress" );
		assert( commandBuffer == GetCurrentCommandBuffer() && "Cannot end render pass on a command buffer from a different frame" );

		vkCmdEndRenderPass( commandBuffer );
	}

	void AxeRenderer::RecreateSwapChain()
	{
		// Wait until the current swap chain is no longer being used

		auto extent = axeWindow.GetExtent();
		while ( extent.width == 0 || extent.height == 0 )
		{
			extent = axeWindow.GetExtent();
			glfwWaitEvents();
		}
		vkDeviceWaitIdle( axeDevice.Device() );

		// Create the new swap chain and pipeline (since the pipeline depends on the swap chain for now)

		if ( axeSwapChain == nullptr )
		{
			axeSwapChain = std::make_unique<AxeSwapChain>( axeDevice, extent );
		}
		else
		{
			std::shared_ptr<AxeSwapChain> oldSwapChain = std::move( axeSwapChain );
			axeSwapChain = std::make_unique<AxeSwapChain>( axeDevice, extent, oldSwapChain );

			if (!oldSwapChain->AreSwapChainFormatsEqual( *axeSwapChain ))
			{
				throw std::runtime_error("Swap chain image (or depth) format has changed");
			}
		}
	}

	void AxeRenderer::CreateCommandBuffers()
	{
		commandBuffers.resize( AxeSwapChain::MAX_FRAMES_IN_FLIGHT );

		VkCommandBufferAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = axeDevice.GetCommandPool();
		allocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if ( vkAllocateCommandBuffers( axeDevice.Device(), &allocateInfo, commandBuffers.data() ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to allocate command buffers" );
		}
	}

	void AxeRenderer::FreeCommandBuffers()
	{
		vkFreeCommandBuffers(
			axeDevice.Device(),
			axeDevice.GetCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data()
		);
		commandBuffers.clear();
	}
}
