#pragma once

#include "platform/Platform.hpp"

#include <string>

namespace OpenLRR::Input
{

    struct KeyBinding
    {
        Platform::Key key = Platform::Key::Unknown;
        unsigned modifiers = 0;
    };

    struct MouseBinding
    {
        Platform::MouseButton button = Platform::MouseButton::Left;
        unsigned modifiers = 0;
    };

    KeyBinding MakeKeyBinding(Platform::Key key);
    MouseBinding MakeMouseBinding(Platform::MouseButton button);

    std::string ToString(const KeyBinding& binding);
    std::string ToString(const MouseBinding& binding);
}
