#include "renderer/PresentationViewport.hpp"

#include <algorithm>

namespace OpenLRR::Renderer
{
    PresentationViewport CalculatePresentationViewport(
        int framebufferWidth,
        int framebufferHeight,
        int renderWidth,
        int renderHeight
    )
    {
        PresentationViewport viewport{};

        if (framebufferWidth <= 0 ||
            framebufferHeight <= 0 ||
            renderWidth <= 0 ||
            renderHeight <= 0)
        {
            return viewport;
        }

        const float scaleX =
            static_cast<float>(framebufferWidth) /
            static_cast<float>(renderWidth);

        const float scaleY =
            static_cast<float>(framebufferHeight) /
            static_cast<float>(renderHeight);

        viewport.scale = std::min(scaleX, scaleY);

        viewport.width =
            static_cast<int>(
                static_cast<float>(renderWidth) * viewport.scale
            );

        viewport.height =
            static_cast<int>(
                static_cast<float>(renderHeight) * viewport.scale
            );

        viewport.x =
            (framebufferWidth - viewport.width) / 2;

        viewport.y =
            (framebufferHeight - viewport.height) / 2;

        return viewport;
    }

    bool PresentationToRenderCoordinates(
        const PresentationViewport& viewport,
        double framebufferX,
        double framebufferY,
        int renderWidth,
        int renderHeight,
        double* renderX,
        double* renderY
    )
    {
        if (viewport.width <= 0 ||
            viewport.height <= 0 ||
            viewport.scale <= 0.0f ||
            renderWidth <= 0 ||
            renderHeight <= 0 ||
            renderX == nullptr ||
            renderY == nullptr)
        {
            return false;
        }

        const double localX =
            framebufferX -
            static_cast<double>(viewport.x);

        const double localY =
            framebufferY -
            static_cast<double>(viewport.y);

        if (localX < 0.0 ||
            localY < 0.0 ||
            localX >= static_cast<double>(viewport.width) ||
            localY >= static_cast<double>(viewport.height))
        {
            return false;
        }

        *renderX =
            localX /
            static_cast<double>(viewport.scale);

        *renderY =
            localY /
            static_cast<double>(viewport.scale);

        if (*renderX < 0.0 ||
            *renderY < 0.0 ||
            *renderX >= static_cast<double>(renderWidth) ||
            *renderY >= static_cast<double>(renderHeight))
        {
            return false;
        }

        return true;
    }

}
