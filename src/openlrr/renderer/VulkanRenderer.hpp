#pragma once

namespace OpenLRR::Renderer
{
    bool InitialiseVulkan();

    void SetClearColor(
        float red,
        float green,
        float blue,
        float alpha
    );

    bool RenderFrame();

    void ShutdownVulkan();
}
