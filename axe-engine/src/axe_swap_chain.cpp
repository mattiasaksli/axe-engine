#include "axe_swap_chain.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace Axe
{
	AxeSwapChain::AxeSwapChain( AxeDevice& deviceRef, VkExtent2D extent )
		: device{ deviceRef },
		  windowExtent{ extent }
	{
		Init();
	}

	AxeSwapChain::AxeSwapChain( AxeDevice& deviceRef, VkExtent2D extent, std::shared_ptr<AxeSwapChain> previousSwapChain )
		: device{ deviceRef },
		  windowExtent{ extent },
		  oldSwapChain{ std::move( previousSwapChain ) }
	{
		Init();

		oldSwapChain = nullptr;
	}

	void AxeSwapChain::Init()
	{
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDepthResources();
		CreateFramebuffers();
		CreateSyncObjects();
	}

	AxeSwapChain::~AxeSwapChain()
	{
		for ( const auto imageView : swapChainImageViews )
		{
			vkDestroyImageView( device.Device(), imageView, nullptr );
		}
		swapChainImageViews.clear();

		if ( swapChain != nullptr )
		{
			vkDestroySwapchainKHR( device.Device(), swapChain, nullptr );
			swapChain = nullptr;
		}

		for ( size_t i = 0; i < depthImages.size(); i++ )
		{
			vkDestroyImageView( device.Device(), depthImageViews[ i ], nullptr );
			vkDestroyImage( device.Device(), depthImages[ i ], nullptr );
			vkFreeMemory( device.Device(), depthImageMemoryHandles[ i ], nullptr );
		}

		for ( const auto framebuffer : swapChainFramebuffers )
		{
			vkDestroyFramebuffer( device.Device(), framebuffer, nullptr );
		}

		vkDestroyRenderPass( device.Device(), renderPass, nullptr );

		// Cleanup synchronization objects
		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			vkDestroySemaphore( device.Device(), renderFinishedSemaphores[ i ], nullptr );
			vkDestroySemaphore( device.Device(), imageAvailableForRenderingSemaphores[ i ], nullptr );
			vkDestroyFence( device.Device(), inFlightFences[ i ], nullptr );
		}
	}

	VkResult AxeSwapChain::AcquireNextImage( uint32_t* imageIndex ) const
	{
		// Block until previous frame has finished rendering

		vkWaitForFences(
			device.Device(),
			1,
			&inFlightFences[ currentFrame ],
			VK_TRUE,
			std::numeric_limits<uint64_t>::max()
		);

		// Get the next image used for rendering

		const VkResult result = vkAcquireNextImageKHR(
			device.Device(),
			swapChain,
			std::numeric_limits<uint64_t>::max(),
			imageAvailableForRenderingSemaphores[ currentFrame ],  // Must be a non-signaled semaphore
			VK_NULL_HANDLE,
			imageIndex
		);

		return result;
	}

	VkResult AxeSwapChain::SubmitCommandBuffers( const VkCommandBuffer* buffers, const uint32_t* imageIndex )
	{
		// Wait until previous buffers have been submitted

		if ( imagesInFlight[ *imageIndex ] != VK_NULL_HANDLE )
		{
			vkWaitForFences( device.Device(), 1, &imagesInFlight[ *imageIndex ], VK_TRUE, UINT64_MAX );
		}
		imagesInFlight[ *imageIndex ] = inFlightFences[ currentFrame ];

		// Submit current command buffers

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[ ] = { imageAvailableForRenderingSemaphores[ currentFrame ] };
		constexpr VkPipelineStageFlags waitStages[ ] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = buffers;

		const VkSemaphore signalSemaphores[ ] = { renderFinishedSemaphores[ currentFrame ] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences( device.Device(), 1, &inFlightFences[ currentFrame ] );

		if ( vkQueueSubmit( device.GraphicsQueue(), 1, &submitInfo, inFlightFences[ currentFrame ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to submit draw command buffer" );
		}

		// Present the result of the command buffers to the screen

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swapChains[ ] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = imageIndex;

		const auto result = vkQueuePresentKHR( device.PresentQueue(), &presentInfo );

		currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;

		return result;
	}

	void AxeSwapChain::CreateSwapChain()
	{
		const SwapChainSupportDetails swapChainSupport = device.GetSwapChainSupport();

		const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
		const VkPresentModeKHR presentMode = ChooseSwapPresentMode( swapChainSupport.presentModes );
		const VkExtent2D extent = ChooseSwapExtent( swapChainSupport.capabilities );

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if ( swapChainSupport.capabilities.maxImageCount > 0 &&
		     imageCount > swapChainSupport.capabilities.maxImageCount
		)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = device.Surface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const QueueFamilyIndices indices = device.FindPhysicalQueueFamilies();
		const uint32_t queueFamilyIndices[ ] = { indices.graphicsFamily, indices.presentFamily };

		if ( indices.graphicsFamily != indices.presentFamily )
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;      // Optional
			createInfo.pQueueFamilyIndices = nullptr;  // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;

		if ( vkCreateSwapchainKHR( device.Device(), &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create swap chain" );
		}

		// We only specified a minimum number of images in the swap chain, so the implementation is
		// allowed to create a swap chain with more. That's why we'll first query the final number of
		// images with vkGetSwapchainImagesKHR, then resize the container, and finally call it again to
		// retrieve the handles.
		vkGetSwapchainImagesKHR( device.Device(), swapChain, &imageCount, nullptr );
		swapChainImages.resize( imageCount );
		vkGetSwapchainImagesKHR( device.Device(), swapChain, &imageCount, swapChainImages.data() );

		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void AxeSwapChain::CreateImageViews()
	{
		swapChainImageViews.resize( swapChainImages.size() );

		for ( size_t i = 0; i < swapChainImages.size(); i++ )
		{
			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = swapChainImages[ i ];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = swapChainImageFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if ( vkCreateImageView( device.Device(), &viewInfo, nullptr, &swapChainImageViews[ i ] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to create texture image view" );
			}
		}
	}

	void AxeSwapChain::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = GetSwapChainImageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef;
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment = {};
		depthAttachment.format = FindDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef;
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.srcStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.dstSubpass = 0;
		subpassDependency.dstStageMask =
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		subpassDependency.dstAccessMask =
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		const std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &subpassDependency;

		if ( vkCreateRenderPass( device.Device(), &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create render pass" );
		}
	}

	void AxeSwapChain::CreateFramebuffers()
	{
		swapChainFramebuffers.resize( ImageCount() );

		for ( size_t i = 0; i < ImageCount(); i++ )
		{
			std::array<VkImageView, 2> attachments = { swapChainImageViews[ i ], depthImageViews[ i ] };

			const VkExtent2D swapChainImageExtent = GetSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainImageExtent.width;
			framebufferInfo.height = swapChainImageExtent.height;
			framebufferInfo.layers = 1;

			if ( vkCreateFramebuffer(
				     device.Device(),
				     &framebufferInfo,
				     nullptr,
				     &swapChainFramebuffers[ i ] ) != VK_SUCCESS
			)
			{
				throw std::runtime_error( "Failed to create framebuffer" );
			}
		}
	}

	void AxeSwapChain::CreateDepthResources()
	{
		const VkFormat depthFormat = FindDepthFormat();
		swapChainDepthFormat = depthFormat;
		const VkExtent2D swapChainImageExtent = GetSwapChainExtent();

		depthImages.resize( ImageCount() );
		depthImageMemoryHandles.resize( ImageCount() );
		depthImageViews.resize( ImageCount() );

		for ( size_t i = 0; i < depthImages.size(); i++ )
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = swapChainImageExtent.width;
			imageInfo.extent.height = swapChainImageExtent.height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = depthFormat;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.flags = 0;

			device.CreateImageWithInfo(
				imageInfo,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImages[ i ],
				depthImageMemoryHandles[ i ] );

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = depthImages[ i ];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = depthFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if ( vkCreateImageView( device.Device(), &viewInfo, nullptr, &depthImageViews[ i ] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to create depth texture image view" );
			}
		}
	}

	void AxeSwapChain::CreateSyncObjects()
	{
		// Creates a set of sync objects for each frame in flight

		imageAvailableForRenderingSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
		renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
		inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );
		imagesInFlight.resize( ImageCount(), VK_NULL_HANDLE );

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			if ( vkCreateSemaphore( device.Device(), &semaphoreInfo, nullptr, &imageAvailableForRenderingSemaphores[ i ] ) !=
			     VK_SUCCESS ||
			     vkCreateSemaphore( device.Device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[ i ] ) !=
			     VK_SUCCESS ||
			     vkCreateFence( device.Device(), &fenceInfo, nullptr, &inFlightFences[ i ] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to create synchronization objects for a frame" );
			}
		}
	}

	VkSurfaceFormatKHR AxeSwapChain::ChooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats )
	{
		for ( const auto& availableFormat : availableFormats )
		{
			if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			     availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
			{
				return availableFormat;
			}
		}

		return availableFormats[ 0 ];
	}

	VkPresentModeKHR AxeSwapChain::ChooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes )
	{
		for ( const auto& availablePresentMode : availablePresentModes )
		{
			if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				std::cout << "Present mode: Mailbox" << std::endl;

				return availablePresentMode;
			}
		}

		// for ( const auto& availablePresentMode : availablePresentModes )
		// {
		// 	if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
		// 	{
		// 		std::cout << "Present mode: Immediate" << std::endl;
		// 		return availablePresentMode;
		// 	}
		// }

		std::cout << "Present mode: V-Sync" << std::endl;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D AxeSwapChain::ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) const
	{
		if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
		{
			return capabilities.currentExtent;
		}

		// If we're using a high DPI display

		VkExtent2D actualExtent = windowExtent;
		actualExtent.width = std::clamp<uint32_t>( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
		actualExtent.height = std::clamp<uint32_t>( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

		return actualExtent;
	}

	VkFormat AxeSwapChain::FindDepthFormat() const
	{
		return device.FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
}
