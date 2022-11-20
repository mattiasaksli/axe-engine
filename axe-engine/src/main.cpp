#include "app.h"

#include <cstdlib>
#include <iostream>

int main()
{
	std::ios::sync_with_stdio(false);

	try
	{
		Axe::App app{};
		app.Run();
	}
	catch ( const std::exception& e )
	{
		std::cerr << "\nError: " << e.what() << "\n";

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
