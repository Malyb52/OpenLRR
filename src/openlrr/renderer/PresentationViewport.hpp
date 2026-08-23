#pragma once

namespace OpenLRR::Renderer
{
    struct PresentationViewport
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        float scale = 1.0f;
    };

    PresentationViewport CalculatePresentationViewport(
        int framebufferWidth,
        int framebufferHeight,
        int renderWidth,
        int renderHeight
    );

    bool PresentationToRenderCoordinates(
        const PresentationViewport& viewport,
        double framebufferX,
        double framebufferY,
        int renderWidth,
        int renderHeight,
        double* renderX,
        double* renderY
    );

}
