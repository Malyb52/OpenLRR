#pragma once

namespace OpenLRR::Platform
{
    bool Initialise();
    void Shutdown();

    bool CreateWindow(int width, int height, const char* title);
    void PollEvents();
    bool ShouldClose();
}
