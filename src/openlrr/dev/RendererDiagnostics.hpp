#pragma once

namespace OpenLRR::Dev::RendererDiagnostics
{
    void Update();
    void RenderCheckerboard();
	
	void ToggleInvertColors();
	void TogglePurpleOverride();
	void ToggleTileSize();
	void Reset();

	bool IsInvertColorsEnabled();
	bool IsPurpleOverrideEnabled();
	bool IsAlternateTileSizeEnabled();
}
