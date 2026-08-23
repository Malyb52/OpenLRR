#pragma once

namespace OpenLRR::Input
{
    struct RenderCursorPosition
    {
        double x = 0.0;
        double y = 0.0;
    };

    bool GetRenderCursorPosition(
        RenderCursorPosition* position
    );
}
