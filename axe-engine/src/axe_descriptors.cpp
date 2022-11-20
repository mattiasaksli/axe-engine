#include "axe_descriptors.h"

#include <cassert>
#include <ranges>
#include <stdexcept>

namespace Axe
{
	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||   Descriptor Set Layout   ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	// *************** Descriptor Set Layout Builder *********************

	AxeDescriptorSetLayout::Builder& AxeDescriptorSetLayout::Builder::AddBinding(
		const uint32_t binding,
		const VkDescriptorType descriptorType,
		const VkShaderStageFlags stageFlags,
		const uint32_t count )
	{
		assert( !bindings.contains( binding ) && "Binding already in use" );
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count;
		layoutBinding.stageFlags = stageFlags;
		bindings[ binding ] = layoutBinding;
		return *this;
	}

	std::unique_ptr<AxeDescriptorSetLayout> AxeDescriptorSetLayout::Builder::Build() const
	{
		return std::make_unique<AxeDescriptorSetLayout>( axeDevice, bindings );
	}

	// *************** Descriptor Set Layout *********************

	AxeDescriptorSetLayout::AxeDescriptorSetLayout( AxeDevice& axeDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings )
		: axeDevice{ axeDevice },
		  bindings{ bindings }
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for ( const auto& binding : bindings | std::views::values )
		{
			setLayoutBindings.push_back( binding );
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if ( vkCreateDescriptorSetLayout(
			     axeDevice.Device(),
			     &descriptorSetLayoutInfo,
			     nullptr,
			     &descriptorSetLayout ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create descriptor set layout" );
		}
	}

	AxeDescriptorSetLayout::~AxeDescriptorSetLayout()
	{
		vkDestroyDescriptorSetLayout( axeDevice.Device(), descriptorSetLayout, nullptr );
	}

	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||      Descriptor Pool      ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	// *************** Descriptor Pool Builder *********************

	AxeDescriptorPool::Builder& AxeDescriptorPool::Builder::AddPoolSize( const VkDescriptorType descriptorType, const uint32_t count )
	{
		poolSizes.push_back( { descriptorType, count } );
		return *this;
	}

	AxeDescriptorPool::Builder& AxeDescriptorPool::Builder::SetPoolFlags( const VkDescriptorPoolCreateFlags flags )
	{
		poolFlags = flags;
		return *this;
	}

	AxeDescriptorPool::Builder& AxeDescriptorPool::Builder::SetMaxSets( const uint32_t count )
	{
		maxSets = count;
		return *this;
	}

	std::unique_ptr<AxeDescriptorPool> AxeDescriptorPool::Builder::Build() const
	{
		return std::make_unique<AxeDescriptorPool>( axeDevice, maxSets, poolFlags, poolSizes );
	}

	// *************** Descriptor Pool *********************

	AxeDescriptorPool::AxeDescriptorPool(
		AxeDevice& axeDevice,
		const uint32_t maxSets,
		const VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize>& poolSizes )
		: axeDevice{ axeDevice }
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if ( vkCreateDescriptorPool( axeDevice.Device(), &descriptorPoolInfo, nullptr, &descriptorPool ) !=
		     VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create descriptor pool" );
		}
	}

	AxeDescriptorPool::~AxeDescriptorPool()
	{
		vkDestroyDescriptorPool( axeDevice.Device(), descriptorPool, nullptr );
	}

	bool AxeDescriptorPool::AllocateDescriptorSet( const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor ) const
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		if ( vkAllocateDescriptorSets( axeDevice.Device(), &allocInfo, &descriptor ) != VK_SUCCESS )
		{
			return false;
		}
		return true;
	}

	void AxeDescriptorPool::FreeDescriptors( const std::vector<VkDescriptorSet>& descriptors ) const
	{
		vkFreeDescriptorSets(
			axeDevice.Device(),
			descriptorPool,
			static_cast<uint32_t>(descriptors.size()),
			descriptors.data() );
	}

	void AxeDescriptorPool::ResetPool() const
	{
		vkResetDescriptorPool( axeDevice.Device(), descriptorPool, 0 );
	}

	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||     Descriptor Writer     ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	AxeDescriptorWriter::AxeDescriptorWriter( AxeDescriptorSetLayout& setLayout, AxeDescriptorPool& pool )
		: setLayout{ setLayout },
		  pool{ pool } { }

	AxeDescriptorWriter& AxeDescriptorWriter::WriteBuffer( const uint32_t binding, const VkDescriptorBufferInfo* bufferInfo )
	{
		assert( setLayout.bindings.contains( binding ) && "Layout does not contain specified binding" );

		const auto& bindingDescription = setLayout.bindings[ binding ];

		assert( bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple" );

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		writes.push_back( write );
		return *this;
	}

	AxeDescriptorWriter& AxeDescriptorWriter::WriteImage( const uint32_t binding, const VkDescriptorImageInfo* imageInfo )
	{
		assert( setLayout.bindings.contains( binding ) && "Layout does not contain specified binding" );

		const auto& bindingDescription = setLayout.bindings[ binding ];

		assert( bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple" );

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;

		writes.push_back( write );
		return *this;
	}

	bool AxeDescriptorWriter::Build( VkDescriptorSet& set )
	{
		const bool success = pool.AllocateDescriptorSet( setLayout.GetDescriptorSetLayout(), set );
		if ( !success )
		{
			return false;
		}
		Overwrite( set );
		return true;
	}

	void AxeDescriptorWriter::Overwrite( const VkDescriptorSet& set )
	{
		for ( auto& write : writes )
		{
			write.dstSet = set;
		}
		vkUpdateDescriptorSets( pool.axeDevice.Device(), writes.size(), writes.data(), 0, nullptr );
	}
}
