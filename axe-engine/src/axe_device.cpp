#include "axe_device.h"

// std headers
#include <cstring>
#include <iostream>
#include <set>
#include <unordered_set>

namespace Axe
{
	// ##########################################################################################################
	// ######################################   Local callback functions   ######################################
	// ##########################################################################################################

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData )
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n" << std::endl;

		return VK_FALSE;
	}

	VkResult CreateDebugUtilsMessengerEXT(
		const VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger )
	{
		const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
			instance,
			"vkCreateDebugUtilsMessengerEXT"
		));

		if ( func != nullptr )
		{
			return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(
		const VkInstance instance,
		const VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator )
	{
		const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
			instance,
			"vkDestroyDebugUtilsMessengerEXT" ));
		if ( func != nullptr )
		{
			func( instance, debugMessenger, pAllocator );
		}
	}

	// ########################################################################################################
	// ######################################   Class member functions   ######################################
	// ########################################################################################################

	AxeDevice::AxeDevice( AxeWindow& window ) : window{ window }
	{
		CreateInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateCommandPool();
	}

	AxeDevice::~AxeDevice()
	{
		vkDestroyCommandPool( logicalDevice, commandPool, nullptr );
		// Command buffers are destroyed when the command pool they are allocated from is destroyed

		vkDestroyDevice( logicalDevice, nullptr );

		if ( enableValidationLayers )
		{
			DestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );
		}

		vkDestroySurfaceKHR( instance, surface, nullptr );

		vkDestroyInstance( instance, nullptr );
	}

	void AxeDevice::CreateInstance()
	{
		if ( enableValidationLayers && !CheckValidationLayerSupport() )
		{
			throw std::runtime_error( "Validation layers requested, but not available" );
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Axe Engine App";
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "Axe Engine";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		const auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if ( enableValidationLayers )
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			PopulateDebugMessengerCreateInfo( debugCreateInfo );
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if ( vkCreateInstance( &createInfo, nullptr, &instance ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create Vulkan instance" );
		}

		HasGlfwRequiredInstanceExtensions();
	}

	void AxeDevice::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );

		if ( deviceCount == 0 )
		{
			throw std::runtime_error( "Failed to find GPUs with Vulkan support" );
		}

		std::cout << "Device count: " << deviceCount << std::endl;
		std::vector<VkPhysicalDevice> devices( deviceCount );
		vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );

		for ( const auto& device : devices )
		{
			if ( IsDeviceSuitable( device ) )
			{
				physicalDevice = device;
				break;
			}
		}

		if ( physicalDevice == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "Failed to find a suitable GPU" );
		}

		vkGetPhysicalDeviceProperties( physicalDevice, &physicalDeviceProperties );
		std::cout << "Physical device: " << physicalDeviceProperties.deviceName << "\n" << std::endl;
	}

	void AxeDevice::CreateLogicalDevice()
	{
		// ####################   Setup queue info   ####################

		QueueFamilyIndices indices = FindQueueFamilies( physicalDevice );

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

		float queuePriority = 1.0f;
		for ( uint32_t queueFamily : uniqueQueueFamilies )
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back( queueCreateInfo );
		}

		// ####################   Setup physical device features   ####################

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		// ####################   Create logical device   ####################

		VkDeviceCreateInfo logicalDeviceInfo = {};
		logicalDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		logicalDeviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		logicalDeviceInfo.pQueueCreateInfos = queueCreateInfos.data();

		logicalDeviceInfo.pEnabledFeatures = &deviceFeatures;
		logicalDeviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		logicalDeviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

		// Device specific validation layers are deprecated, but we'll enable them anyway for compatibility
		if ( enableValidationLayers )
		{
			logicalDeviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			logicalDeviceInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			logicalDeviceInfo.enabledLayerCount = 0;
		}

		if ( vkCreateDevice( physicalDevice, &logicalDeviceInfo, nullptr, &logicalDevice ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create logical device" );
		}

		// ####################   Get device queue handles   ####################

		vkGetDeviceQueue( logicalDevice, indices.graphicsFamily, 0, &graphicsQueue );
		vkGetDeviceQueue( logicalDevice, indices.presentFamily, 0, &presentQueue );
	}

	void AxeDevice::CreateCommandPool()
	{
		const QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags =
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		if ( vkCreateCommandPool( logicalDevice, &poolInfo, nullptr, &commandPool ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create command pool" );
		}
	}

	void AxeDevice::CreateSurface()
	{
		window.CreateWindowSurface( instance, &surface );
	}

	bool AxeDevice::IsDeviceSuitable( VkPhysicalDevice device )
	{
		QueueFamilyIndices indices = FindQueueFamilies( device );

		bool extensionsSupported = CheckDeviceExtensionSupport( device );

		bool swapChainAdequate = false;
		if ( extensionsSupported )
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( device );
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures( device, &supportedFeatures );

		return indices.IsComplete() &&
		       extensionsSupported &&
		       swapChainAdequate &&
		       supportedFeatures.samplerAnisotropy;
	}

	void AxeDevice::PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
		createInfo.pUserData = nullptr;
	}

	void AxeDevice::SetupDebugMessenger()
	{
		if ( !enableValidationLayers ) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo( createInfo );
		if ( CreateDebugUtilsMessengerEXT( instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to set up debug messenger" );
		}
	}

	bool AxeDevice::CheckValidationLayerSupport() const
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

		std::vector<VkLayerProperties> availableLayers( layerCount );
		vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

		for ( const char* layerName : validationLayers )
		{
			bool layerFound = false;

			for ( const auto& layerProperties : availableLayers )
			{
				if ( strcmp( layerName, layerProperties.layerName ) == 0 )
				{
					layerFound = true;
					break;
				}
			}

			if ( !layerFound )
			{
				return false;
			}
		}

		return true;
	}

	std::vector<const char *> AxeDevice::GetRequiredExtensions() const
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

		std::vector extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

		if ( enableValidationLayers )
		{
			extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}

		return extensions;
	}

	void AxeDevice::HasGlfwRequiredInstanceExtensions() const
	{
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
		std::vector<VkExtensionProperties> extensions( extensionCount );
		vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );

		std::cout << "Available extensions:" << std::endl;

		std::unordered_set<std::string> available;
		for ( const auto& extension : extensions )
		{
			std::cout << "\t" << extension.extensionName << std::endl;
			available.insert( extension.extensionName );
		}

		std::cout << "Required extensions:" << std::endl;

		auto requiredExtensions = GetRequiredExtensions();
		for ( const auto& required : requiredExtensions )
		{
			std::cout << "\t" << required << std::endl;
			if ( !available.contains( required ) )
			{
				throw std::runtime_error( "Missing required GLFW extension" );
			}
		}

		std::cout << "\n";
	}

	bool AxeDevice::CheckDeviceExtensionSupport( const VkPhysicalDevice device ) const
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

		std::vector<VkExtensionProperties> availableExtensions( extensionCount );
		vkEnumerateDeviceExtensionProperties(
			device,
			nullptr,
			&extensionCount,
			availableExtensions.data() );

		std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

		for ( const auto& extension : availableExtensions )
		{
			requiredExtensions.erase( extension.extensionName );
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices AxeDevice::FindQueueFamilies( VkPhysicalDevice device ) const
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

		std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

		int i = 0;
		for ( const auto& queueFamily : queueFamilies )
		{
			if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				indices.graphicsFamily = i;
				indices.graphicsFamilyHasValue = true;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

			if ( queueFamily.queueCount > 0 && presentSupport )
			{
				indices.presentFamily = i;
				indices.presentFamilyHasValue = true;
			}

			if ( indices.IsComplete() )
			{
				break;
			}

			i++;
		}

		return indices;
	}

	SwapChainSupportDetails AxeDevice::QuerySwapChainSupport( VkPhysicalDevice device ) const
	{
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

		if ( formatCount != 0 )
		{
			details.formats.resize( formatCount );
			vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

		if ( presentModeCount != 0 )
		{
			details.presentModes.resize( presentModeCount );
			vkGetPhysicalDeviceSurfacePresentModesKHR(
				device,
				surface,
				&presentModeCount,
				details.presentModes.data() );
		}
		return details;
	}

	VkFormat AxeDevice::FindSupportedFormat(
		const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const VkFormatFeatureFlags features ) const
	{
		for ( const VkFormat format : candidates )
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &props );

			if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
			{
				return format;
			}
			if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
			{
				return format;
			}
		}
		throw std::runtime_error( "Failed to find supported format" );
	}

	uint32_t AxeDevice::FindMemoryType( const uint32_t typeFilter, const VkMemoryPropertyFlags memoryProperties ) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties( physicalDevice, &memProperties );
		for ( uint32_t i = 0; i < memProperties.memoryTypeCount; i++ )
		{
			if ( ( typeFilter & ( 1 << i ) ) &&
			     ( memProperties.memoryTypes[ i ].propertyFlags & memoryProperties ) == memoryProperties )
			{
				return i;
			}
		}

		throw std::runtime_error( "Failed to find suitable memory type" );
	}

	void AxeDevice::CreateBuffer(
		const VkDeviceSize size,
		const VkBufferUsageFlags usage,
		const VkMemoryPropertyFlags memoryProperties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory ) const
	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if ( vkCreateBuffer( logicalDevice, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create vertex buffer" );
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements( logicalDevice, buffer, &memRequirements );

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, memoryProperties );

		if ( vkAllocateMemory( logicalDevice, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to allocate vertex buffer memory" );
		}

		vkBindBufferMemory( logicalDevice, buffer, bufferMemory, 0 );
	}

	VkCommandBuffer AxeDevice::BeginSingleTimeCommands() const
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers( logicalDevice, &allocInfo, &commandBuffer );

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer( commandBuffer, &beginInfo );
		return commandBuffer;
	}

	void AxeDevice::EndSingleTimeCommands( VkCommandBuffer commandBuffer ) const
	{
		vkEndCommandBuffer( commandBuffer );

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
		vkQueueWaitIdle( graphicsQueue );

		vkFreeCommandBuffers( logicalDevice, commandPool, 1, &commandBuffer );
	}

	void AxeDevice::CopyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size ) const
	{
		const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

		EndSingleTimeCommands( commandBuffer );
	}

	void AxeDevice::CopyBufferToImage(
		const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height, const uint32_t layerCount ) const
	{
		const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferImageCopy region;
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = layerCount;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
		EndSingleTimeCommands( commandBuffer );
	}

	void AxeDevice::CreateImageWithInfo(
		const VkImageCreateInfo& imageInfo,
		const VkMemoryPropertyFlags memoryProperties,
		VkImage& image,
		VkDeviceMemory& imageMemory ) const
	{
		if ( vkCreateImage( logicalDevice, &imageInfo, nullptr, &image ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create image" );
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements( logicalDevice, image, &memRequirements );

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, memoryProperties );

		if ( vkAllocateMemory( logicalDevice, &allocInfo, nullptr, &imageMemory ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to allocate image memory" );
		}

		if ( vkBindImageMemory( logicalDevice, image, imageMemory, 0 ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to bind image memory" );
		}
	}
}
