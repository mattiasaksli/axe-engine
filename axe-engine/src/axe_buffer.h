#pragma once

#include "axe_device.h"

namespace Axe
{
	class AxeBuffer
	{
	public:
		AxeBuffer(
			AxeDevice& device,
			VkDeviceSize instanceSize,
			uint32_t instanceCount,
			VkBufferUsageFlags usageFlags,
			VkMemoryPropertyFlags memoryPropertyFlags,
			VkDeviceSize minOffsetAlignment = 1 );
		~AxeBuffer();

		AxeBuffer( const AxeBuffer& ) = delete;
		AxeBuffer& operator=( const AxeBuffer& ) = delete;
		AxeBuffer( const AxeBuffer&& ) = delete;
		AxeBuffer& operator=( const AxeBuffer&& ) = delete;

		VkResult Map( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 );
		void Unmap();

		void WriteToBuffer( const void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;
		[[nodiscard]] VkResult Flush( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;
		[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;
		[[nodiscard]] VkResult Invalidate( VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0 ) const;

		// When multiple instances are in a single buffer
		void WriteToIndex( const void* data, int index ) const;
		[[nodiscard]] VkResult FlushIndex( int index ) const;
		[[nodiscard]] VkDescriptorBufferInfo DescriptorInfoForIndex( int index ) const;
		[[nodiscard]] VkResult InvalidateIndex( int index ) const;

		[[nodiscard]] VkBuffer GetBufferHandle() const { return buffer; }
		[[nodiscard]] void* GetMappedMemory() const { return mapped; }
		[[nodiscard]] uint32_t GetInstanceCount() const { return instanceCount; }
		[[nodiscard]] VkDeviceSize GetInstanceSize() const { return instanceSize; }
		[[nodiscard]] VkDeviceSize GetAlignmentSize() const { return instanceSize; }
		[[nodiscard]] VkBufferUsageFlags GetUsageFlags() const { return usageFlags; }
		[[nodiscard]] VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return memoryPropertyFlags; }
		[[nodiscard]] VkDeviceSize GetBufferSize() const { return bufferSize; }

	private:
		AxeDevice& axeDevice;
		void* mapped = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;

		VkDeviceSize bufferSize;
		uint32_t instanceCount;
		VkDeviceSize instanceSize;
		VkDeviceSize alignmentSize;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryPropertyFlags;

		static VkDeviceSize GetAlignment( VkDeviceSize instanceSize, VkDeviceSize minOffsetAlignment );
	};
}
