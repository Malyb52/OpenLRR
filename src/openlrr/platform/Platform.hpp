#pragma once
#include <vulkan/vulkan.h>

namespace OpenLRR::Platform
{
    enum class CursorVisibility
    {
        Hidden,
        Visible
    };

    enum class CursorClipping
    {
        Off,
        GameArea
    };
enum class Modifier : unsigned
{
    None  = 0,
    Shift = 1 << 0,
    Ctrl  = 1 << 1,
    Alt   = 1 << 2,
    Super = 1 << 3
};
    enum class Key
    {
        Unknown,

        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,

        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,

        Semicolon,
        Equal,

        A, B, C, D, E, F, G, H, I, J,
        K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,

        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,

        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,

        Right,
        Left,
        Down,
        Up,

        PageUp,
        PageDown,
        Home,
        End,

        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,

        F1, F2, F3, F4, F5, F6,
        F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18,
        F19, F20, F21, F22, F23, F24, F25,

        Keypad0,
        Keypad1,
        Keypad2,
        Keypad3,
        Keypad4,
        Keypad5,
        Keypad6,
        Keypad7,
        Keypad8,
        Keypad9,

        KeypadDecimal,
        KeypadDivide,
        KeypadMultiply,
        KeypadSubtract,
        KeypadAdd,
        KeypadEnter,
        KeypadEqual,

        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,

        RightShift,
        RightControl,
        RightAlt,
        RightSuper,

        Menu,

        Count
    };

    enum class MouseButton
    {
        Left,
        Right,
        Middle,
        Button4,
        Button5,
        Button6,
        Button7,
        Button8,

        Count
    };

    bool Initialise();
    void Shutdown();

    bool CreateWindow(int width, int height, const char* title);

    const char** GetRequiredVulkanExtensions(unsigned* count);

    bool CreateVulkanSurface(
        VkInstance instance,
        VkSurfaceKHR* surface
    );

    void PollEvents();
    void BeginInputFrame();

    bool ShouldClose();
    bool IsActive();

    int GetWindowWidth();
    int GetWindowHeight();

    bool IsKeyDown(Key key);
    bool WasKeyPressed(Key key);
    bool WasKeyReleased(Key key);

    bool IsMouseButtonDown(MouseButton button);
    bool WasMouseButtonPressed(MouseButton button);
    bool WasMouseButtonReleased(MouseButton button);

    double GetCursorX();
    double GetCursorY();

    unsigned GetScrollModifiers();
    double GetScrollX();

    unsigned GetScrollModifiers();
    double GetScrollY();

    const char* GetKeyName(Key key);
    const char* GetMouseButtonName(MouseButton button);

    void SetWindowTitle(const char* title);

    void SetCursorVisibility(CursorVisibility visibility);
    void SetCursorClipping(CursorClipping clipping);
}
