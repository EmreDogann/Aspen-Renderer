#include "Aspen/Core/application.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <stdlib.h>

int main() {
	Aspen::FirstApp app{};

	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}