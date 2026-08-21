#include "platform/Platform.hpp"
#include "input/InputBinding.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
    using namespace OpenLRR::Platform;
    using Clock = std::chrono::steady_clock;

    if (!Initialise()) {
        std::cerr << "Failed to initialise platform\n";
        return 1;
    }

    if (!CreateWindow(800, 600, "OpenLRR")) {
        std::cerr << "Failed to create window\n";
        Shutdown();
        return 1;
    }

    auto temporaryTitleUntil = Clock::time_point{};
    bool wasActive = IsActive();

    while (!ShouldClose()) {
        BeginInputFrame();
        PollEvents();

        const bool active = IsActive();
        const auto now = Clock::now();

        if (active != wasActive) {
            temporaryTitleUntil = Clock::time_point{};

            if (active) {
                SetWindowTitle("OpenLRR");
            }
            else {
                SetWindowTitle("OpenLRR - Focus lost");
            }
        }

        if (active) {
            std::string diagnostic;

            for (int value = 0;
                 value < static_cast<int>(MouseButton::Count);
                 ++value)
            {
                const auto button =
                    static_cast<MouseButton>(value);

                if (WasMouseButtonPressed(button)) {
                    if (!diagnostic.empty()) {
                        diagnostic += " | ";
                    }

                    const auto binding =
                        OpenLRR::Input::MakeMouseBinding(button);

                    diagnostic += "Binding: ";
                    diagnostic += OpenLRR::Input::ToString(binding);
                }
            }

            for (int value =
                     static_cast<int>(Key::Unknown) + 1;
                 value < static_cast<int>(Key::Count);
                 ++value)
            {
                const auto key =
                    static_cast<Key>(value);

                if (WasKeyPressed(key)) {
                    if (!diagnostic.empty()) {
                        diagnostic += " | ";
                    }

                    const auto binding =
                        OpenLRR::Input::MakeKeyBinding(key);

                    diagnostic += "Binding: ";
                    diagnostic += OpenLRR::Input::ToString(binding);
                }
            }

            if (GetScrollX() != 0.0 ||
    GetScrollY() != 0.0)
{
    if (!diagnostic.empty()) {
        diagnostic += " | ";
    }

    diagnostic += "Scroll: ";

    const unsigned modifiers = GetScrollModifiers();

    if (modifiers &
        static_cast<unsigned>(Modifier::Ctrl))
    {
        diagnostic += "Ctrl+";
    }

    if (modifiers &
        static_cast<unsigned>(Modifier::Shift))
    {
        diagnostic += "Shift+";
    }

    if (modifiers &
        static_cast<unsigned>(Modifier::Alt))
    {
        diagnostic += "Alt+";
    }

    if (modifiers &
        static_cast<unsigned>(Modifier::Super))
    {
        diagnostic += "Super+";
    }

    if (GetScrollY() > 0.0) {
        diagnostic += "Up";
    }
    else if (GetScrollY() < 0.0) {
        diagnostic += "Down";
    }
    else if (GetScrollX() > 0.0) {
        diagnostic += "Right";
    }
    else {
        diagnostic += "Left";
    }
}

            if (!diagnostic.empty()) {
                const std::string title =
                    std::string("OpenLRR - ") + diagnostic;

                SetWindowTitle(title.c_str());

                temporaryTitleUntil =
                    now + std::chrono::milliseconds(700);
            }
            else if (temporaryTitleUntil !=
                         Clock::time_point{} &&
                     now >= temporaryTitleUntil)
            {
                SetWindowTitle("OpenLRR");
                temporaryTitleUntil = Clock::time_point{};
            }
        }

        wasActive = active;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    Shutdown();
    return 0;
}
