#pragma once

#include "input/InputBinding.hpp"

namespace OpenLRR::Input
{
    enum class Action
    {
        TestAction1,
        TestAction2,
        TestAction3,

        Count
    };

    void SetKeyBinding(Action action, const KeyBinding& binding);
    void SetMouseBinding(Action action, const MouseBinding& binding);

    bool IsActionPressed(Action action);

    const char* GetActionName(Action action);
}
