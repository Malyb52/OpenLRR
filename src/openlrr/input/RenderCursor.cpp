#include "input/RenderCursor.hpp"

#include "platform/Platform.hpp"
#include "renderer/PresentationViewport.hpp"
#include "renderer/VulkanRenderer.hpp"

namespace OpenLRR::Input
{
    bool GetRenderCursorPosition(
        RenderCursorPosition* position
    )
    {
        if (position == nullptr) {
            return false;
        }

        const int windowWidth =
            OpenLRR::Platform::GetWindowWidth();

        const int windowHeight =
            OpenLRR::Platform::GetWindowHeight();

        int framebufferWidth = 0;
        int framebufferHeight = 0;

        OpenLRR::Platform::GetFramebufferSize(
            &framebufferWidth,
            &framebufferHeight
        );

        if (windowWidth <= 0 ||
            windowHeight <= 0 ||
            framebufferWidth <= 0 ||
            framebufferHeight <= 0)
        {
            return false;
        }

        const double framebufferX =
            OpenLRR::Platform::GetCursorX() *
            static_cast<double>(framebufferWidth) /
            static_cast<double>(windowWidth);

        const double framebufferY =
            OpenLRR::Platform::GetCursorY() *
            static_cast<double>(framebufferHeight) /
            static_cast<double>(windowHeight);

        double renderX = 0.0;
        double renderY = 0.0;

        if (!OpenLRR::Renderer::PresentationToRenderCoordinates(
                OpenLRR::Renderer::GetPresentationViewport(),
                framebufferX,
                framebufferY,
                OpenLRR::Renderer::GetRenderWidth(),
                OpenLRR::Renderer::GetRenderHeight(),
                &renderX,
                &renderY))
        {
            return false;
        }

        position->x =
            renderX;

        position->y =
            renderY;

        return true;
    }
}
