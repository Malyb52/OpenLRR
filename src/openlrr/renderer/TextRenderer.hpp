#pragma once

namespace OpenLRR::Renderer
{
    void DrawText(
        const char* text,
        int x,
        int y,
        int pixelSize,
        float red,
        float green,
        float blue,
        float alpha
    );

    int MeasureTextWidth(
        const char* text,
        int pixelSize
    );

    int GetTextHeight(
        int pixelSize
    );
}
