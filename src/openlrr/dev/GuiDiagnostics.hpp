#pragma once

namespace OpenLRR::Dev::GuiDiagnostics
{
    void Update();

    void Render();

    bool BlocksSceneInput(
        double renderX,
        double renderY
    );
}
