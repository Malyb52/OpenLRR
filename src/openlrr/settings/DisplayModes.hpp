
#pragma once

#include "settings/Settings.hpp"

#include <array>
#include <string_view>

namespace OpenLRR::Settings
{
    struct ResolutionPreset
    {
        Resolution resolution;
        std::string_view label;
    };

    inline constexpr std::array ResolutionPresets{
        ResolutionPreset{ { 640, 480 },   "640x480 (4:3)" },
        ResolutionPreset{ { 800, 600 },   "800x600 (4:3)" },
        ResolutionPreset{ { 1024, 768 },  "1024x768 (4:3)" },
        ResolutionPreset{ { 1280, 960 },  "1280x960 (4:3)" },

        ResolutionPreset{ { 1280, 720 },  "1280x720 (16:9)" },
        ResolutionPreset{ { 1600, 900 },  "1600x900 (16:9)" },
        ResolutionPreset{ { 1920, 1080 }, "1920x1080 (16:9)" },
        ResolutionPreset{ { 2560, 1440 }, "2560x1440 (16:9)" },

        ResolutionPreset{ { 1280, 800 },  "1280x800 (16:10)" },
        ResolutionPreset{ { 1440, 900 },  "1440x900 (16:10)" },
        ResolutionPreset{ { 1680, 1050 }, "1680x1050 (16:10)" },
        ResolutionPreset{ { 1920, 1200 }, "1920x1200 (16:10)" },
    };
}
