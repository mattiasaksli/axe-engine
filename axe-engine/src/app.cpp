#include "app.h"

#include "axe_buffer.h"
#include "axe_camera.h"
#include "keyboard_movement_controller.h"
#include "systems/simple_render_system.h"
#include "systems/point_light_system.h"

#include <chrono>

namespace Axe
{
	App::App()
	{
		globalPool = AxeDescriptorPool::Builder( axeDevice )
		             .SetMaxSets( AxeSwapChain::MAX_FRAMES_IN_FLIGHT )
		             .AddPoolSize( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, AxeSwapChain::MAX_FRAMES_IN_FLIGHT )
		             .Build();
		LoadGameObjects();
	}

	App::~App() {}

	void App::Run()
	{
		// Uniform buffer object
		std::vector<std::unique_ptr<AxeBuffer>> globalUBObuffers( AxeSwapChain::MAX_FRAMES_IN_FLIGHT );
		for ( auto& uboBuffer : globalUBObuffers )
		{
			uboBuffer = std::make_unique<AxeBuffer>(
				axeDevice,
				sizeof( GlobalUBO ),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
				axeDevice.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment
			);
			uboBuffer->Map();
		}

		// Global descriptor sets for UBOs
		auto globalSetLayout = AxeDescriptorSetLayout::Builder( axeDevice )
		                       .AddBinding( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS )
		                       .Build();
		std::vector<VkDescriptorSet> globalDescriptorSets( AxeSwapChain::MAX_FRAMES_IN_FLIGHT );
		for ( size_t i = 0; i < globalDescriptorSets.size(); ++i )
		{
			auto bufferInfo = globalUBObuffers[ i ]->DescriptorInfo();
			AxeDescriptorWriter( *globalSetLayout, *globalPool )
				.WriteBuffer( 0, &bufferInfo )
				.Build( globalDescriptorSets[ i ] );
		}

		// Render systems
		const SimpleRenderSystem simpleRenderSystem{ axeDevice, axeRenderer.GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };
		const PointLightSystem pointLightSystem{ axeDevice, axeRenderer.GetSwapChainRenderPass(), globalSetLayout->GetDescriptorSetLayout() };

		// Camera
		AxeCamera camera = {};
		auto cameraGameObject = AxeGameObject::CreateGameObject();
		cameraGameObject.transform.translation.z = -2.5f;

		// Keyboard controller
		constexpr KeyboardMovementController cameraController = {};

		// Frame start time
		auto startTime = std::chrono::high_resolution_clock::now();

		while ( !axeWindow.ShouldClose() )
		{
			glfwPollEvents();

			// Game loop timing
			auto currentTime = std::chrono::high_resolution_clock::now();
			const float frameTime = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();
			startTime = currentTime;

			// Camera movement
			cameraController.moveInPlaneXZ( axeWindow.GetGLFWwindow(), frameTime, cameraGameObject );
			camera.SetViewYXZ( cameraGameObject.transform.translation, cameraGameObject.transform.rotation );

			// Camera view matrix
			const float aspectRatio = axeRenderer.GetAspectRatio();
			camera.SetPerspectiveProjection( glm::radians( 90.0f ), aspectRatio, 0.1f, 100.0f );

			if ( const auto commandBuffer = axeRenderer.BeginFrame() )	// BeginFrame() returns a nullptr if the swap chain needs to be recreated
			{
				int frameIndex = axeRenderer.GetFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[ frameIndex ],
					gameObjects
				};

				// Update
				GlobalUBO ubo = {};
				ubo.projectionMatrix = camera.GetProjection();
				ubo.viewMatrix = camera.GetView();
				ubo.inverseViewMatrix = camera.GetInverseView();

				pointLightSystem.Update( frameInfo, ubo );

				globalUBObuffers[ frameIndex ]->WriteToBuffer( &ubo );
				if ( globalUBObuffers[ frameIndex ]->Flush() != VK_SUCCESS )
				{
					throw std::runtime_error( "Error flushing global uniform buffer object buffer to GPU" );
				}

				// Render
				axeRenderer.BeginSwapChainRenderPass( commandBuffer );

				simpleRenderSystem.RenderGameObjects( frameInfo );
				pointLightSystem.Render( frameInfo );

				axeRenderer.EndSwapChainRenderPass( commandBuffer );
				axeRenderer.EndFrame();
			}
		}

		vkDeviceWaitIdle( axeDevice.Device() );
	}

	// void SillySierpinskiTriangle( std::vector<AxeModel::Vertex>& vertices, const size_t depth, const glm::vec2& v0, const glm::vec2& v1, const glm::vec2& v2 )
	// {
	// 	if ( depth == 0 )
	// 	{
	// 		vertices.push_back( { v0, {1.0f, 0.0f, 0.0f} } );
	// 		vertices.push_back( { v1, {0.0f, 1.0f, 0.0f} } );
	// 		vertices.push_back( { v2, {0.0f, 0.0f, 1.0f} } );
	// 		return;
	// 	}
	//
	// 	const glm::vec2 v0_v1_midpoint = ( v0 + v1 ) * 0.5f;
	// 	const glm::vec2 v1_v2_midpoint = ( v1 + v2 ) * 0.5f;
	// 	const glm::vec2 v2_v0_midpoint = ( v2 + v0 ) * 0.5f;
	//
	// 	SillySierpinskiTriangle( vertices, depth - 1, v0, v0_v1_midpoint, v2_v0_midpoint );	// Top triangle
	// 	SillySierpinskiTriangle( vertices, depth - 1, v0_v1_midpoint, v1, v1_v2_midpoint );	// Right triangle
	// 	SillySierpinskiTriangle( vertices, depth - 1, v2_v0_midpoint, v1_v2_midpoint, v2 );	// Left triangle
	// }

	// void App::LoadGameObjectsTriangle()
	// {
	// 	std::vector<AxeModel::Vertex> vertices = {
	// 		{ { 0.0f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
	// 		{ { 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } },
	// 		{ { -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } }
	// 	};
	//
	// 	// constexpr size_t depth = 6;
	// 	// vertices.reserve( static_cast<size_t>(std::pow( 3, depth + 1 )) );
	// 	// SillySierpinskiTriangle( vertices, depth, { 0.0f, -0.9f }, { 0.9f, 0.9f }, { -0.9f, 0.9f } );
	//
	// 	const auto axeModel = std::make_shared<AxeModel>( axeDevice, vertices );
	//
	// 	auto triangle = AxeGameObject::CreateGameObject();
	// 	triangle.model = axeModel;
	// 	triangle.color = { 0.25f, 0.75f, 0.1f };
	// 	triangle.transform2D.translation.x = 0.2f;
	// 	triangle.transform2D.scale = { 2.0f, 0.5f };
	// 	triangle.transform2D.rotation = 0.25f * glm::two_pi<float>();
	//
	// 	gameObjects.push_back( std::move( triangle ) );
	// }

	void App::LoadGameObjects()
	{
		std::shared_ptr<AxeModel> axeModel = AxeModel::CreateModelFromFile( axeDevice, "models/flat_vase.obj" );
		{
			auto flatVase = AxeGameObject::CreateGameObject();
			flatVase.model = axeModel;
			flatVase.transform.translation = { -0.5f, 0.5f, 0.0f };
			flatVase.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };
			gameObjects.emplace( flatVase.GetId(), std::move( flatVase ) );
		}

		{
			axeModel = AxeModel::CreateModelFromFile( axeDevice, "models/smooth_vase.obj" );
			auto smoothVase = AxeGameObject::CreateGameObject();
			smoothVase.model = axeModel;
			smoothVase.transform.translation = { 0.5f, 0.5f, 0.0f };
			smoothVase.transform.scale = glm::vec3{ 3.0f, 1.5f, 3.0f };
			gameObjects.emplace( smoothVase.GetId(), std::move( smoothVase ) );
		}

		{
			axeModel = AxeModel::CreateModelFromFile( axeDevice, "models/quad.obj" );
			auto floor = AxeGameObject::CreateGameObject();
			floor.model = axeModel;
			floor.transform.translation = { 0.0f, 0.5f, 0.0f };
			floor.transform.scale = glm::vec3{ 3.0f, 1.0f, 3.0f };
			gameObjects.emplace( floor.GetId(), std::move( floor ) );
		}

		{
			std::vector<glm::vec3> lightColors{
				{ 1.f, .1f, .1f },
				{ .1f, .1f, 1.f },
				{ .1f, 1.f, .1f },
				{ 1.f, 1.f, .1f },
				{ .1f, 1.f, 1.f },
				{ 1.f, 1.f, 1.f }
			};

			for ( int i = 0; i < lightColors.size(); ++i )
			{
				auto pointLight = AxeGameObject::MakePointLight( 0.5f );
				pointLight.color = lightColors[ i ];
				auto rotateLight = glm::rotate(
					glm::mat4{ 1.0f },
					i * glm::two_pi<float>() / lightColors.size(),
					glm::vec3{ 0.0f, -1.0f, 0.0f }
				);
				pointLight.transform.translation = glm::vec3{ rotateLight * glm::vec4{ -1.0f, -1.0f, -1.0f, 1.0f } };
				gameObjects.emplace( pointLight.GetId(), std::move( pointLight ) );
			}
		}
	}
}
