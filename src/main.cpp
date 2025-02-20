#include <tuple>
#include <chrono>
#include <limits>
#include <vector>
#include <stdexcept>

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Volk.hpp"

#include <iostream>

// My includes
#include "Utils.hpp"
#include "Context.hpp"
#include "Engine.hpp"
#include <string>

int main() try {
    Engine engine;

    if (!engine.Initialize()) {
        std::cout << "Failed to initialize engine. " << std::endl;
        return 0;
    }

    engine.Run();

    return 0;

} catch (std::exception const &eErr) {

    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "Error: %s\n", eErr.what());
    return 1;
}

// EOF vim:syntax=cpp:foldmethod=marker:ts=4:noexpandtab:
