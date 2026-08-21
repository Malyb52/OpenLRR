#include "platform/Platform.hpp"

#include <iostream>

int main()
{
    if (!OpenLRR::Platform::Initialise()) {
        std::cerr << "Failed to initialise platform\n";
        return 1;
    }

    if (!OpenLRR::Platform::CreateWindow(800, 600, "OpenLRR")) {
        std::cerr << "Failed to create window\n";
        OpenLRR::Platform::Shutdown();
        return 1;
    }

    while (!OpenLRR::Platform::ShouldClose()) {
        OpenLRR::Platform::PollEvents();
    }

    OpenLRR::Platform::Shutdown();

    return 0;
}
