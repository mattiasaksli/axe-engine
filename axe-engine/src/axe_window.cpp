#include "axe_window.h"

#include <stdexcept>

namespace Axe
{
	AxeWindow::AxeWindow( const int w, const int h, std::string name )
		: width{ w }, height{ h }, windowName{ std::move( name ) }
	{
		InitWindow();
	}

	AxeWindow::~AxeWindow()
	{
		glfwDestroyWindow( window );
		glfwTerminate();
	}

	void AxeWindow::FramebufferResizedCallback( GLFWwindow* window, int width, int height )
	{
		const auto axeWindow = static_cast<AxeWindow *>(glfwGetWindowUserPointer( window ));

		axeWindow->framebufferResized = true;
		axeWindow->width = width;
		axeWindow->height = height;
	}

	void AxeWindow::InitWindow()
	{
		glfwInit();
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

		window = glfwCreateWindow( width, height, windowName.c_str(), nullptr, nullptr );
		glfwSetWindowUserPointer( window, this );
		glfwSetFramebufferSizeCallback( window, FramebufferResizedCallback );

		// Center window on the primary monitor
		const GLFWvidmode* videoMode = glfwGetVideoMode( glfwGetPrimaryMonitor() );
		const int windowX = (videoMode->width - width) / 2;
		const int windowY = (videoMode->height - height) / 2;
		glfwSetWindowPos( window, windowX, windowY );
	}

	void AxeWindow::CreateWindowSurface( VkInstance instance, VkSurfaceKHR* surface ) const
	{
		if ( glfwCreateWindowSurface( instance, window, nullptr, surface ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create window surface" );
		}
	}
}
