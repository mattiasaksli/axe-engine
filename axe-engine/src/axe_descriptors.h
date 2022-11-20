#pragma once

#include "axe_device.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Axe
{
	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||   Descriptor Set Layout   ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	class AxeDescriptorSetLayout
	{
	public:
		class Builder
		{
		public:
			explicit Builder( AxeDevice& axeDevice ) : axeDevice{ axeDevice } {}

			Builder& AddBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1
			);
			[[nodiscard]] std::unique_ptr<AxeDescriptorSetLayout> Build() const;

		private:
			AxeDevice& axeDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		AxeDescriptorSetLayout(AxeDevice& axeDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings );
		~AxeDescriptorSetLayout();

		AxeDescriptorSetLayout( const AxeDescriptorSetLayout& ) = delete;
		AxeDescriptorSetLayout& operator=( const AxeDescriptorSetLayout& ) = delete;
		AxeDescriptorSetLayout( const AxeDescriptorSetLayout&& ) = delete;
		AxeDescriptorSetLayout& operator=( const AxeDescriptorSetLayout&& ) = delete;

		[[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		AxeDevice& axeDevice;
		VkDescriptorSetLayout descriptorSetLayout = {};
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

		friend class AxeDescriptorWriter;
	};

	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||      Descriptor Pool      ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	class AxeDescriptorPool
	{
	public:
		class Builder
		{
		public:
			explicit Builder( AxeDevice& axeDevice ) : axeDevice{ axeDevice } {}

			Builder& AddPoolSize( VkDescriptorType descriptorType, uint32_t count );
			Builder& SetPoolFlags( VkDescriptorPoolCreateFlags flags );
			Builder& SetMaxSets( uint32_t count );
			[[nodiscard]] std::unique_ptr<AxeDescriptorPool> Build() const;

		private:
			AxeDevice& axeDevice;
			std::vector<VkDescriptorPoolSize> poolSizes = {};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		AxeDescriptorPool(
			AxeDevice& axeDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize>& poolSizes 
		);
		~AxeDescriptorPool();

		AxeDescriptorPool( const AxeDescriptorPool& ) = delete;
		AxeDescriptorPool& operator=( const AxeDescriptorPool& ) = delete;
		AxeDescriptorPool( const AxeDescriptorPool&& ) = delete;
		AxeDescriptorPool& operator=( const AxeDescriptorPool&& ) = delete;

		bool AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor ) const;

		void FreeDescriptors( const std::vector<VkDescriptorSet>& descriptors ) const;

		void ResetPool() const;

	private:
		AxeDevice& axeDevice;
		VkDescriptorPool descriptorPool = {};

		friend class AxeDescriptorWriter;
	};

	// ||                                           ||---------------------------||                                           ||
	// ||-------------------------------------------||     Descriptor Writer     ||-------------------------------------------||
	// ||                                           ||---------------------------||                                           ||

	class AxeDescriptorWriter
	{
	public:
		AxeDescriptorWriter( AxeDescriptorSetLayout& setLayout, AxeDescriptorPool& pool );

		AxeDescriptorWriter& WriteBuffer( uint32_t binding, const VkDescriptorBufferInfo* bufferInfo );
		AxeDescriptorWriter& WriteImage( uint32_t binding, const VkDescriptorImageInfo* imageInfo );

		bool Build( VkDescriptorSet& set );
		void Overwrite( const VkDescriptorSet& set );

	private:
		AxeDescriptorSetLayout& setLayout;
		AxeDescriptorPool& pool;
		std::vector<VkWriteDescriptorSet> writes;
	};
}
