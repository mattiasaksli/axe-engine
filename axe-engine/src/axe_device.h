#pragma once

#include "axe_window.h"

#include <vector>

namespace Axe
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily = 0;
		uint32_t presentFamily = 0;
		bool graphicsFamilyHasValue = false;
		bool presentFamilyHasValue = false;

		[[nodiscard]] bool IsComplete() const { return graphicsFamilyHasValue && presentFamilyHasValue; }
	};

	class AxeDevice
	{
	public:
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		VkPhysicalDeviceProperties physicalDeviceProperties = {};

		explicit AxeDevice( AxeWindow& window );
		~AxeDevice();

		// Can't be copied or moved
		AxeDevice( const AxeDevice& ) = delete;
		AxeDevice& operator=( const AxeDevice& ) = delete;
		AxeDevice( AxeDevice&& ) = delete;
		AxeDevice& operator=( AxeDevice&& ) = delete;

		[[nodiscard]] VkCommandPool GetCommandPool() const { return commandPool; }
		[[nodiscard]] VkDevice Device() const { return logicalDevice; }
		[[nodiscard]] VkSurfaceKHR Surface() const { return surface; }
		[[nodiscard]] VkQueue GraphicsQueue() const { return graphicsQueue; }
		[[nodiscard]] VkQueue PresentQueue() const { return presentQueue; }

		[[nodiscard]] SwapChainSupportDetails GetSwapChainSupport() const { return QuerySwapChainSupport( physicalDevice ); }
		[[nodiscard]] QueueFamilyIndices FindPhysicalQueueFamilies() const { return FindQueueFamilies( physicalDevice ); }
		[[nodiscard]] uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties ) const;
		[[nodiscard]] VkFormat FindSupportedFormat( const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features ) const;

		// Buffer helper functions
		void CreateBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags memoryProperties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory
		) const;
		[[nodiscard]] VkCommandBuffer BeginSingleTimeCommands() const;
		void EndSingleTimeCommands( VkCommandBuffer commandBuffer ) const;
		void CopyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) const;
		void CopyBufferToImage(
			VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount
		) const;

		void CreateImageWithInfo(
			const VkImageCreateInfo& imageInfo,
			VkMemoryPropertyFlags memoryProperties,
			VkImage& image,
			VkDeviceMemory& imageMemory
		) const;

	private:
		VkInstance instance = {};
		VkDebugUtilsMessengerEXT debugMessenger = {};
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		AxeWindow& window;
		VkCommandPool commandPool = {};

		VkDevice logicalDevice = {};
		VkSurfaceKHR surface = {};
		VkQueue graphicsQueue = {};
		VkQueue presentQueue = {};

		const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

		// Helper functions
		bool IsDeviceSuitable( VkPhysicalDevice device );
		[[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;
		[[nodiscard]] bool CheckValidationLayerSupport() const;
		QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device ) const;
		void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );
		void HasGlfwRequiredInstanceExtensions() const;
		bool CheckDeviceExtensionSupport( VkPhysicalDevice device ) const;
		SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device ) const;
	};
}
