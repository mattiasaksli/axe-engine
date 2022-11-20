/*
 * Encapsulates a vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

#include "axe_buffer.h"

#include <cassert>
#include <cstring>

namespace Axe
{
	/**
 * Returns the minimum instance size required to be compatible with devices minOffsetAlignment
 *
 * @param instanceSize The size of an instance
 * @param minOffsetAlignment The minimum required alignment, in bytes, for the offset member (eg
 * minUniformBufferOffsetAlignment)
 *
 * @return VkResult of the buffer mapping call
 */
	VkDeviceSize AxeBuffer::GetAlignment( const VkDeviceSize instanceSize, const VkDeviceSize minOffsetAlignment )
	{
		if ( minOffsetAlignment > 0 )
		{
			return ( instanceSize + minOffsetAlignment - 1 ) & ~( minOffsetAlignment - 1 );
		}
		return instanceSize;
	}

	AxeBuffer::AxeBuffer(
		AxeDevice& device,
		const VkDeviceSize instanceSize,
		const uint32_t instanceCount,
		const VkBufferUsageFlags usageFlags,
		const VkMemoryPropertyFlags memoryPropertyFlags,
		const VkDeviceSize minOffsetAlignment )
		: axeDevice{ device },
		  instanceCount{ instanceCount },
		  instanceSize{ instanceSize },
		  usageFlags{ usageFlags },
		  memoryPropertyFlags{ memoryPropertyFlags }
	{
		alignmentSize = GetAlignment( instanceSize, minOffsetAlignment );
		bufferSize = alignmentSize * instanceCount;
		device.CreateBuffer( bufferSize, usageFlags, memoryPropertyFlags, buffer, memory );
	}

	AxeBuffer::~AxeBuffer()
	{
		Unmap();
		vkDestroyBuffer( axeDevice.Device(), buffer, nullptr );
		vkFreeMemory( axeDevice.Device(), memory, nullptr );
	}

	/**
	 * Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
	 *
	 * @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete
	 * buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the buffer mapping call
	 */
	VkResult AxeBuffer::Map( const VkDeviceSize size, const VkDeviceSize offset )
	{
		assert( buffer && memory && "Buffer needs to be created before it can be mapped" );
		return vkMapMemory( axeDevice.Device(), memory, offset, size, 0, &mapped );
	}

	/**
	 * Unmap a mapped memory range
	 *
	 * @note Does not return a result as vkUnmapMemory can't fail
	 */
	void AxeBuffer::Unmap()
	{
		if ( mapped )
		{
			vkUnmapMemory( axeDevice.Device(), memory );
			mapped = nullptr;
		}
	}

	/**
	 * Copies the specified data to the mapped buffer. Default value writes whole buffer range
	 *
	 * @param data Pointer to the data to copy
	 * @param size (Optional) Size of the data to copy. Pass VK_WHOLE_SIZE to flush the complete buffer
	 * range.
	 * @param offset (Optional) Byte offset from beginning of mapped region
	 *
	 */
	void AxeBuffer::WriteToBuffer( const void* data, const VkDeviceSize size, const VkDeviceSize offset ) const
	{
		assert( mapped && "Cannot copy to unmapped buffer" );

		if ( size == VK_WHOLE_SIZE )
		{
			memcpy( mapped, data, bufferSize );
		}
		else
		{
			auto memOffset = static_cast<char *>(mapped);
			memOffset += offset;
			memcpy( memOffset, data, size );
		}
	}

	/**
	 * Flush a memory range of the buffer to make it visible to the device
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the
	 * complete buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the flush call
	 */
	VkResult AxeBuffer::Flush( const VkDeviceSize size, const VkDeviceSize offset ) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges( axeDevice.Device(), 1, &mappedRange );
	}

	/**
	 * Invalidate a memory range of the buffer to make it visible to the host
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate
	 * the complete buffer range.
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkResult of the invalidate call
	 */
	VkResult AxeBuffer::Invalidate( const VkDeviceSize size, const VkDeviceSize offset ) const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges( axeDevice.Device(), 1, &mappedRange );
	}

	/**
	 * Create a buffer info descriptor
	 *
	 * @param size (Optional) Size of the memory range of the descriptor
	 * @param offset (Optional) Byte offset from beginning
	 *
	 * @return VkDescriptorBufferInfo of specified offset and range
	 */
	VkDescriptorBufferInfo AxeBuffer::DescriptorInfo( const VkDeviceSize size, const VkDeviceSize offset ) const
	{
		return VkDescriptorBufferInfo{
			buffer,
			offset,
			size,
		};
	}

	/**
	 * Copies "instanceSize" bytes of data to the mapped buffer at an offset of index * alignmentSize
	 *
	 * @param data Pointer to the data to copy
	 * @param index Used in offset calculation
	 *
	 */
	void AxeBuffer::WriteToIndex( const void* data, const int index ) const
	{
		WriteToBuffer( data, instanceSize, index * alignmentSize );
	}

	/**
	 *  Flush the memory range at index * alignmentSize of the buffer to make it visible to the device
	 *
	 * @param index Used in offset calculation
	 *
	 */
	VkResult AxeBuffer::FlushIndex( const int index ) const
	{
		return Flush( alignmentSize, index * alignmentSize );
	}

	/**
	 * Create a buffer info descriptor
	 *
	 * @param index Specifies the region given by index * alignmentSize
	 *
	 * @return VkDescriptorBufferInfo for instance at index
	 */
	VkDescriptorBufferInfo AxeBuffer::DescriptorInfoForIndex( const int index ) const
	{
		return DescriptorInfo( alignmentSize, index * alignmentSize );
	}

	/**
	 * Invalidate a memory range of the buffer to make it visible to the host
	 *
	 * @note Only required for non-coherent memory
	 *
	 * @param index Specifies the region to invalidate: index * alignmentSize
	 *
	 * @return VkResult of the invalidate call
	 */
	VkResult AxeBuffer::InvalidateIndex( const int index ) const
	{
		return Invalidate( alignmentSize, index * alignmentSize );
	}
}
