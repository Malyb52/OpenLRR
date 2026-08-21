#include "input/InputActions.hpp"

#include <array>
#include <cstddef>

namespace
{
    using OpenLRR::Input::Action;
    using OpenLRR::Input::KeyBinding;
    using OpenLRR::Input::MouseBinding;

    constexpr std::size_t ActionCount =
        static_cast<std::size_t>(Action::Count);

    std::array<KeyBinding, ActionCount> gKeyBindings{};
    std::array<MouseBinding, ActionCount> gMouseBindings{};

    std::array<bool, ActionCount> gHasKeyBinding{};
    std::array<bool, ActionCount> gHasMouseBinding{};

    std::size_t ActionIndex(Action action)
    {
        return static_cast<std::size_t>(action);
    }

    bool ModifiersMatch(unsigned expected)
    {
        unsigned current = 0;

        using OpenLRR::Platform::Key;
        using OpenLRR::Platform::Modifier;

        if (OpenLRR::Platform::IsKeyDown(Key::LeftShift) ||
            OpenLRR::Platform::IsKeyDown(Key::RightShift))
        {
            current |= static_cast<unsigned>(Modifier::Shift);
        }

        if (OpenLRR::Platform::IsKeyDown(Key::LeftControl) ||
            OpenLRR::Platform::IsKeyDown(Key::RightControl))
        {
            current |= static_cast<unsigned>(Modifier::Ctrl);
        }

        if (OpenLRR::Platform::IsKeyDown(Key::LeftAlt) ||
            OpenLRR::Platform::IsKeyDown(Key::RightAlt))
        {
            current |= static_cast<unsigned>(Modifier::Alt);
        }

        if (OpenLRR::Platform::IsKeyDown(Key::LeftSuper) ||
            OpenLRR::Platform::IsKeyDown(Key::RightSuper))
        {
            current |= static_cast<unsigned>(Modifier::Super);
        }

        return current == expected;
    }
}

namespace OpenLRR::Input
{
    void SetKeyBinding(Action action, const KeyBinding& binding)
    {
        const std::size_t index = ActionIndex(action);

        gKeyBindings[index] = binding;
        gHasKeyBinding[index] = true;
    }

    void SetMouseBinding(Action action, const MouseBinding& binding)
    {
        const std::size_t index = ActionIndex(action);

        gMouseBindings[index] = binding;
        gHasMouseBinding[index] = true;
    }

    bool IsActionPressed(Action action)
    {
        const std::size_t index = ActionIndex(action);

        if (gHasKeyBinding[index]) {
            const KeyBinding& binding = gKeyBindings[index];

            if (Platform::WasKeyPressed(binding.key) &&
                ModifiersMatch(binding.modifiers))
            {
                return true;
            }
        }

        if (gHasMouseBinding[index]) {
            const MouseBinding& binding = gMouseBindings[index];

            if (Platform::WasMouseButtonPressed(binding.button) &&
                ModifiersMatch(binding.modifiers))
            {
                return true;
            }
        }

        return false;
    }

    const char* GetActionName(Action action)
    {
        switch (action) {
        case Action::TestAction1:
            return "Test Action 1";

        case Action::TestAction2:
            return "Test Action 2";

        case Action::TestAction3:
            return "Test Action 3";

        default:
            return "Unknown Action";
        }
    }
}
