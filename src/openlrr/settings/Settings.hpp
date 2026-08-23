#pragma once

namespace OpenLRR::Settings
{
    enum class DisplayMode
	{
		Windowed,
		BorderlessFullscreen,
		Fullscreen
	};

    struct Resolution
    {
        int width = 1280;
        int height = 800;

        constexpr bool operator==(const Resolution&) const = default;
    };

    struct DisplaySettings
	{
		DisplayMode mode = DisplayMode::Windowed;
		Resolution renderResolution{};
		int refreshRate = 60;
	};

    struct AppSettings
    {
        DisplaySettings display{};
    };
}
