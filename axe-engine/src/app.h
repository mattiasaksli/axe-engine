#pragma once

#include "axe_window.h"
#include "axe_device.h"
#include "axe_renderer.h"
#include "axe_game_object.h"
#include "axe_descriptors.h"

#include <memory>

namespace Axe
{
	class App
	{
	public:
		static constexpr int WIDTH = 1200;
		static constexpr int HEIGHT = 900;

		App();
		~App();

		App( const App& ) = delete;
		App& operator=( const App& ) = delete;
		App( const App&& ) = delete;
		App& operator=( const App&& ) = delete;

		void Run();

	private:
		AxeWindow axeWindow{ WIDTH, HEIGHT, "Hey Paul!" };
		AxeDevice axeDevice{ axeWindow };
		AxeRenderer axeRenderer{ axeWindow, axeDevice };

		std::unique_ptr<AxeDescriptorPool> globalPool = {};

		AxeGameObject::Map gameObjects;

		void LoadGameObjects();
	};
}
