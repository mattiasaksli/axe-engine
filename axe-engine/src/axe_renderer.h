#pragma once

#include "axe_window.h"
#include "axe_device.h"
#include "axe_swap_chain.h"

#include <memory>
#include <cassert>

namespace Axe
{
	class AxeRenderer
	{
	public:
		AxeRenderer( AxeWindow& window, AxeDevice& device );
		~AxeRenderer();

		AxeRenderer( const AxeRenderer& ) = delete;
		AxeRenderer& operator=( const AxeRenderer& ) = delete;
		AxeRenderer( const AxeRenderer&& ) = delete;
		AxeRenderer& operator=( const AxeRenderer&& ) = delete;

		[[nodiscard]] VkRenderPass GetSwapChainRenderPass() const { return axeSwapChain->GetRenderPass(); }
		[[nodiscard]] float GetAspectRatio() const { return axeSwapChain->ExtentAspectRatio(); }
		[[nodiscard]] bool IsFrameInProgress() const { return isFrameStarted; }

		[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const
		{
			assert( isFrameStarted && "Cannot get command buffer when frame is not in progress" );
			return commandBuffers[ currentFrameIndex ];
		}

		[[nodiscard]] int GetFrameIndex() const
		{
			assert( isFrameStarted && "Cannot get frame index when frame is not in progress" );
			return currentFrameIndex;
		}

		VkCommandBuffer BeginFrame();
		void EndFrame();

		void BeginSwapChainRenderPass( VkCommandBuffer commandBuffer ) const;
		void EndSwapChainRenderPass( VkCommandBuffer commandBuffer ) const;

	private:
		AxeWindow& axeWindow;
		AxeDevice& axeDevice;
		std::unique_ptr<AxeSwapChain> axeSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex = 0;
		int currentFrameIndex = 0;
		bool isFrameStarted = false;

		void RecreateSwapChain();
		void CreateCommandBuffers();
		void FreeCommandBuffers();
	};
}
