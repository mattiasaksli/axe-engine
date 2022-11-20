#pragma once

#include "axe_device.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace Axe
{
	class AxeSwapChain
	{
	public:
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

		AxeSwapChain( AxeDevice& deviceRef, VkExtent2D extent );
		AxeSwapChain( AxeDevice& deviceRef, VkExtent2D extent, std::shared_ptr<AxeSwapChain> previousSwapChain );
		~AxeSwapChain();

		
		AxeSwapChain( const AxeSwapChain& ) = delete;
		AxeSwapChain& operator=( const AxeSwapChain& ) = delete;
		AxeSwapChain( const AxeSwapChain&& ) = delete;
		AxeSwapChain& operator=( const AxeSwapChain&& ) = delete;

		[[nodiscard]] VkFramebuffer GetFrameBuffer( const size_t index ) const { return swapChainFramebuffers[ index ]; }
		[[nodiscard]] VkRenderPass GetRenderPass() const { return renderPass; }
		[[nodiscard]] VkImageView GetImageView( const size_t index ) const { return swapChainImageViews[ index ]; }
		[[nodiscard]] size_t ImageCount() const { return swapChainImages.size(); }
		[[nodiscard]] VkFormat GetSwapChainImageFormat() const { return swapChainImageFormat; }
		[[nodiscard]] VkExtent2D GetSwapChainExtent() const { return swapChainExtent; }
		[[nodiscard]] uint32_t Width() const { return swapChainExtent.width; }
		[[nodiscard]] uint32_t Height() const { return swapChainExtent.height; }

		[[nodiscard]] float ExtentAspectRatio() const
		{
			return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
		}

		[[nodiscard]] VkFormat FindDepthFormat() const;

		VkResult AcquireNextImage( uint32_t* imageIndex ) const;
		VkResult SubmitCommandBuffers( const VkCommandBuffer* buffers, const uint32_t* imageIndex );

		[[nodiscard]] bool AreSwapChainFormatsEqual( const AxeSwapChain& otherSwapChain ) const
		{
			return otherSwapChain.swapChainImageFormat == swapChainImageFormat &&
			       otherSwapChain.swapChainDepthFormat == swapChainDepthFormat;
		}

	private:
		VkFormat swapChainImageFormat = {};
		VkFormat swapChainDepthFormat = {};
		VkExtent2D swapChainExtent = {};

		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkRenderPass renderPass = {};

		std::vector<VkImage> depthImages;
		std::vector<VkDeviceMemory> depthImageMemoryHandles;
		std::vector<VkImageView> depthImageViews;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;

		AxeDevice& device;
		VkExtent2D windowExtent;

		VkSwapchainKHR swapChain = {};
		std::shared_ptr<AxeSwapChain> oldSwapChain;

		std::vector<VkSemaphore> imageAvailableForRenderingSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		std::vector<VkFence> imagesInFlight;
		size_t currentFrame = 0;	// Which frame in flight we are currently on out of MAX_FRAMES_IN_FLIGHT

		void Init();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateSyncObjects();

		// Helper functions
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR>& availableFormats );
		VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes );
		[[nodiscard]] VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) const;
	};
}
