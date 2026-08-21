#include "input/InputBinding.hpp"
#include <string>

namespace OpenLRR::Input
{
    namespace
    {
        unsigned GetCurrentModifiers()
        {
            unsigned modifiers = 0;

            if (Platform::IsKeyDown(Platform::Key::LeftShift) ||
                Platform::IsKeyDown(Platform::Key::RightShift))
            {
                modifiers |= static_cast<unsigned>(Platform::Modifier::Shift);
            }

            if (Platform::IsKeyDown(Platform::Key::LeftControl) ||
                Platform::IsKeyDown(Platform::Key::RightControl))
            {
                modifiers |= static_cast<unsigned>(Platform::Modifier::Ctrl);
            }

            if (Platform::IsKeyDown(Platform::Key::LeftAlt) ||
                Platform::IsKeyDown(Platform::Key::RightAlt))
            {
                modifiers |= static_cast<unsigned>(Platform::Modifier::Alt);
            }

            if (Platform::IsKeyDown(Platform::Key::LeftSuper) ||
                Platform::IsKeyDown(Platform::Key::RightSuper))
            {
                modifiers |= static_cast<unsigned>(Platform::Modifier::Super);
            }

            return modifiers;
        }
    }

KeyBinding MakeKeyBinding(Platform::Key key)
{
    KeyBinding binding;
    binding.key = key;
    binding.modifiers = GetCurrentModifiers();

    switch (key) {
    case Platform::Key::LeftShift:
    case Platform::Key::RightShift:
        binding.modifiers &=
            ~static_cast<unsigned>(Platform::Modifier::Shift);
        break;

    case Platform::Key::LeftControl:
    case Platform::Key::RightControl:
        binding.modifiers &=
            ~static_cast<unsigned>(Platform::Modifier::Ctrl);
        break;

    case Platform::Key::LeftAlt:
    case Platform::Key::RightAlt:
        binding.modifiers &=
            ~static_cast<unsigned>(Platform::Modifier::Alt);
        break;

    case Platform::Key::LeftSuper:
    case Platform::Key::RightSuper:
        binding.modifiers &=
            ~static_cast<unsigned>(Platform::Modifier::Super);
        break;

    default:
        break;
    }

    return binding;
}

MouseBinding MakeMouseBinding(Platform::MouseButton button)
{
    MouseBinding binding;
    binding.button = button;
    binding.modifiers = GetCurrentModifiers();
    return binding;
}
std::string ToString(const KeyBinding& binding)
{
    std::string result;

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Ctrl)) {
        result += "Ctrl+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Shift)) {
        result += "Shift+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Alt)) {
        result += "Alt+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Super)) {
        result += "Super+";
    }

    result += Platform::GetKeyName(binding.key);

    return result;
}

std::string ToString(const MouseBinding& binding)
{
    std::string result;

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Ctrl)) {
        result += "Ctrl+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Shift)) {
        result += "Shift+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Alt)) {
        result += "Alt+";
    }

    if (binding.modifiers & static_cast<unsigned>(Platform::Modifier::Super)) {
        result += "Super+";
    }

    result += "Mouse";
    result += Platform::GetMouseButtonName(binding.button);

    return result;
}
}
