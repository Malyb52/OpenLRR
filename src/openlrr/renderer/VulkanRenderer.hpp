#pragma once

#include "settings/Settings.hpp"
#include "renderer/PresentationViewport.hpp"

namespace OpenLRR::Renderer
{
    bool InitialiseVulkan(
		const OpenLRR::Settings::Resolution& renderResolution
	);

    void SetClearColor(
        float red,
        float green,
        float blue,
        float alpha
    );

    bool BeginFrame(
		const OpenLRR::Settings::Resolution& renderResolution
	);

	bool EndFrame();
	
	bool IsFrameInProgress();
	
	int GetRenderWidth();
    int GetRenderHeight();

	PresentationViewport GetPresentationViewport();

    void DrawFilledRect(
        int x,
        int y,
        int width,
        int height,
        float red,
        float green,
        float blue,
        float alpha
    );

    void ShutdownVulkan();
}
