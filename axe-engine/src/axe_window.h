#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Axe
{
	class AxeWindow
	{
	public:
		AxeWindow( int w, int h, std::string name );
		~AxeWindow();
		
		AxeWindow( const AxeWindow& ) = delete;
		AxeWindow& operator=( const AxeWindow& ) = delete;
		AxeWindow( AxeWindow&& ) = delete;
		AxeWindow& operator=( AxeWindow&& ) = delete;

		[[nodiscard]] bool ShouldClose() const { return glfwWindowShouldClose( window ); }
		[[nodiscard]] VkExtent2D GetExtent() const { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		[[nodiscard]] bool WasWindowResized() const { return framebufferResized; }
		void ResetWindowResizedFlag() { framebufferResized = false; }
		[[nodiscard]] GLFWwindow* GetGLFWwindow() const { return window; }

		void CreateWindowSurface( VkInstance instance, VkSurfaceKHR* surface ) const;

	private:
		int width;
		int height;
		bool framebufferResized = false;

		std::string windowName;
		GLFWwindow* window = {};

		static void FramebufferResizedCallback( GLFWwindow* window, int width, int height );

		void InitWindow();
	};
}
