#include "platform/Platform.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>

namespace
{
    using OpenLRR::Platform::Key;
    using OpenLRR::Platform::MouseButton;

    constexpr std::size_t KeyCount =
        static_cast<std::size_t>(Key::Count);

    constexpr std::size_t MouseButtonCount =
        static_cast<std::size_t>(MouseButton::Count);

    GLFWwindow* gWindow = nullptr;

    bool gActive = false;

    int gWindowWidth = 0;
    int gWindowHeight = 0;

    std::array<bool, KeyCount> gKeyDown{};
    std::array<bool, KeyCount> gKeyPressed{};
    std::array<bool, KeyCount> gKeyReleased{};

    std::array<bool, MouseButtonCount> gMouseDown{};
    std::array<bool, MouseButtonCount> gMousePressed{};
    std::array<bool, MouseButtonCount> gMouseReleased{};

    double gCursorX = 0.0;
    double gCursorY = 0.0;

    double gScrollX = 0.0;
    double gScrollY = 0.0;

    OpenLRR::Platform::CursorClipping gCursorClipping =
        OpenLRR::Platform::CursorClipping::Off;

    std::size_t KeyIndex(Key key)
    {
        return static_cast<std::size_t>(key);
    }

    std::size_t MouseIndex(MouseButton button)
    {
        return static_cast<std::size_t>(button);
    }

    Key TranslateKey(int key)
    {
        switch (key) {
        case GLFW_KEY_SPACE:         return Key::Space;
        case GLFW_KEY_APOSTROPHE:    return Key::Apostrophe;
        case GLFW_KEY_COMMA:         return Key::Comma;
        case GLFW_KEY_MINUS:         return Key::Minus;
        case GLFW_KEY_PERIOD:        return Key::Period;
        case GLFW_KEY_SLASH:         return Key::Slash;

        case GLFW_KEY_0: return Key::Num0;
        case GLFW_KEY_1: return Key::Num1;
        case GLFW_KEY_2: return Key::Num2;
        case GLFW_KEY_3: return Key::Num3;
        case GLFW_KEY_4: return Key::Num4;
        case GLFW_KEY_5: return Key::Num5;
        case GLFW_KEY_6: return Key::Num6;
        case GLFW_KEY_7: return Key::Num7;
        case GLFW_KEY_8: return Key::Num8;
        case GLFW_KEY_9: return Key::Num9;

        case GLFW_KEY_SEMICOLON: return Key::Semicolon;
        case GLFW_KEY_EQUAL:     return Key::Equal;

        case GLFW_KEY_A: return Key::A;
        case GLFW_KEY_B: return Key::B;
        case GLFW_KEY_C: return Key::C;
        case GLFW_KEY_D: return Key::D;
        case GLFW_KEY_E: return Key::E;
        case GLFW_KEY_F: return Key::F;
        case GLFW_KEY_G: return Key::G;
        case GLFW_KEY_H: return Key::H;
        case GLFW_KEY_I: return Key::I;
        case GLFW_KEY_J: return Key::J;
        case GLFW_KEY_K: return Key::K;
        case GLFW_KEY_L: return Key::L;
        case GLFW_KEY_M: return Key::M;
        case GLFW_KEY_N: return Key::N;
        case GLFW_KEY_O: return Key::O;
        case GLFW_KEY_P: return Key::P;
        case GLFW_KEY_Q: return Key::Q;
        case GLFW_KEY_R: return Key::R;
        case GLFW_KEY_S: return Key::S;
        case GLFW_KEY_T: return Key::T;
        case GLFW_KEY_U: return Key::U;
        case GLFW_KEY_V: return Key::V;
        case GLFW_KEY_W: return Key::W;
        case GLFW_KEY_X: return Key::X;
        case GLFW_KEY_Y: return Key::Y;
        case GLFW_KEY_Z: return Key::Z;

        case GLFW_KEY_LEFT_BRACKET:  return Key::LeftBracket;
        case GLFW_KEY_BACKSLASH:     return Key::Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:  return Key::GraveAccent;

        case GLFW_KEY_ESCAPE:    return Key::Escape;
        case GLFW_KEY_ENTER:     return Key::Enter;
        case GLFW_KEY_TAB:       return Key::Tab;
        case GLFW_KEY_BACKSPACE: return Key::Backspace;
        case GLFW_KEY_INSERT:    return Key::Insert;
        case GLFW_KEY_DELETE:    return Key::Delete;

        case GLFW_KEY_RIGHT: return Key::Right;
        case GLFW_KEY_LEFT:  return Key::Left;
        case GLFW_KEY_DOWN:  return Key::Down;
        case GLFW_KEY_UP:    return Key::Up;

        case GLFW_KEY_PAGE_UP:   return Key::PageUp;
        case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
        case GLFW_KEY_HOME:      return Key::Home;
        case GLFW_KEY_END:       return Key::End;

        case GLFW_KEY_CAPS_LOCK:    return Key::CapsLock;
        case GLFW_KEY_SCROLL_LOCK:  return Key::ScrollLock;
        case GLFW_KEY_NUM_LOCK:     return Key::NumLock;
        case GLFW_KEY_PRINT_SCREEN: return Key::PrintScreen;
        case GLFW_KEY_PAUSE:        return Key::Pause;

        case GLFW_KEY_F1:  return Key::F1;
        case GLFW_KEY_F2:  return Key::F2;
        case GLFW_KEY_F3:  return Key::F3;
        case GLFW_KEY_F4:  return Key::F4;
        case GLFW_KEY_F5:  return Key::F5;
        case GLFW_KEY_F6:  return Key::F6;
        case GLFW_KEY_F7:  return Key::F7;
        case GLFW_KEY_F8:  return Key::F8;
        case GLFW_KEY_F9:  return Key::F9;
        case GLFW_KEY_F10: return Key::F10;
        case GLFW_KEY_F11: return Key::F11;
        case GLFW_KEY_F12: return Key::F12;
        case GLFW_KEY_F13: return Key::F13;
        case GLFW_KEY_F14: return Key::F14;
        case GLFW_KEY_F15: return Key::F15;
        case GLFW_KEY_F16: return Key::F16;
        case GLFW_KEY_F17: return Key::F17;
        case GLFW_KEY_F18: return Key::F18;
        case GLFW_KEY_F19: return Key::F19;
        case GLFW_KEY_F20: return Key::F20;
        case GLFW_KEY_F21: return Key::F21;
        case GLFW_KEY_F22: return Key::F22;
        case GLFW_KEY_F23: return Key::F23;
        case GLFW_KEY_F24: return Key::F24;
        case GLFW_KEY_F25: return Key::F25;

        case GLFW_KEY_KP_0: return Key::Keypad0;
        case GLFW_KEY_KP_1: return Key::Keypad1;
        case GLFW_KEY_KP_2: return Key::Keypad2;
        case GLFW_KEY_KP_3: return Key::Keypad3;
        case GLFW_KEY_KP_4: return Key::Keypad4;
        case GLFW_KEY_KP_5: return Key::Keypad5;
        case GLFW_KEY_KP_6: return Key::Keypad6;
        case GLFW_KEY_KP_7: return Key::Keypad7;
        case GLFW_KEY_KP_8: return Key::Keypad8;
        case GLFW_KEY_KP_9: return Key::Keypad9;

        case GLFW_KEY_KP_DECIMAL:  return Key::KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE:   return Key::KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return Key::KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return Key::KeypadSubtract;
        case GLFW_KEY_KP_ADD:      return Key::KeypadAdd;
        case GLFW_KEY_KP_ENTER:    return Key::KeypadEnter;
        case GLFW_KEY_KP_EQUAL:    return Key::KeypadEqual;

        case GLFW_KEY_LEFT_SHIFT:    return Key::LeftShift;
        case GLFW_KEY_LEFT_CONTROL:  return Key::LeftControl;
        case GLFW_KEY_LEFT_ALT:      return Key::LeftAlt;
        case GLFW_KEY_LEFT_SUPER:    return Key::LeftSuper;

        case GLFW_KEY_RIGHT_SHIFT:   return Key::RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
        case GLFW_KEY_RIGHT_ALT:     return Key::RightAlt;
        case GLFW_KEY_RIGHT_SUPER:   return Key::RightSuper;

        case GLFW_KEY_MENU: return Key::Menu;

        default:
            return Key::Unknown;
        }
    }

    MouseButton TranslateMouseButton(int button)
    {
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:   return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:  return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_4:      return MouseButton::Button4;
        case GLFW_MOUSE_BUTTON_5:      return MouseButton::Button5;
        case GLFW_MOUSE_BUTTON_6:      return MouseButton::Button6;
        case GLFW_MOUSE_BUTTON_7:      return MouseButton::Button7;
        case GLFW_MOUSE_BUTTON_8:      return MouseButton::Button8;

        default:
            return MouseButton::Count;
        }
    }

    void FocusCallback(GLFWwindow*, int focused)
    {
        gActive = (focused == GLFW_TRUE);

        if (!gActive) {
            gKeyDown.fill(false);
            gMouseDown.fill(false);
        }
    }

    void WindowSizeCallback(GLFWwindow*, int width, int height)
    {
        gWindowWidth = width;
        gWindowHeight = height;
    }

    void KeyCallback(GLFWwindow*, int key, int, int action, int)
    {
        const Key translated = TranslateKey(key);

        if (translated == Key::Unknown) {
            return;
        }

        const std::size_t index = KeyIndex(translated);

        if (action == GLFW_PRESS) {
            gKeyDown[index] = true;
            gKeyPressed[index] = true;
        }
        else if (action == GLFW_RELEASE) {
            gKeyDown[index] = false;
            gKeyReleased[index] = true;
        }
    }

    void MouseButtonCallback(GLFWwindow*, int button, int action, int)
    {
        const MouseButton translated = TranslateMouseButton(button);

        if (translated == MouseButton::Count) {
            return;
        }

        const std::size_t index = MouseIndex(translated);

        if (action == GLFW_PRESS) {
            gMouseDown[index] = true;
            gMousePressed[index] = true;
        }
        else if (action == GLFW_RELEASE) {
            gMouseDown[index] = false;
            gMouseReleased[index] = true;
        }
    }

    void CursorPositionCallback(GLFWwindow*, double x, double y)
    {
        gCursorX = x;
        gCursorY = y;
    }

    void ScrollCallback(GLFWwindow*, double xOffset, double yOffset)
    {
        gScrollX += xOffset;
        gScrollY += yOffset;
    }
}

namespace OpenLRR::Platform
{
    bool Initialise()
    {
        return glfwInit() == GLFW_TRUE;
    }

    void Shutdown()
    {
        if (gWindow != nullptr) {
            glfwDestroyWindow(gWindow);
            gWindow = nullptr;
        }

        glfwTerminate();
    }

    bool CreateWindow(int width, int height, const char* title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        gWindow = glfwCreateWindow(
            width,
            height,
            title,
            nullptr,
            nullptr
        );

        if (gWindow == nullptr) {
            return false;
        }

        gWindowWidth = width;
        gWindowHeight = height;
        gActive =
            glfwGetWindowAttrib(gWindow, GLFW_FOCUSED) == GLFW_TRUE;

        glfwSetWindowFocusCallback(gWindow, FocusCallback);
        glfwSetWindowSizeCallback(gWindow, WindowSizeCallback);
        glfwSetKeyCallback(gWindow, KeyCallback);
        glfwSetMouseButtonCallback(gWindow, MouseButtonCallback);
        glfwSetCursorPosCallback(gWindow, CursorPositionCallback);
        glfwSetScrollCallback(gWindow, ScrollCallback);

        return true;
    }

    void PollEvents()
    {
        glfwPollEvents();
    }

    void BeginInputFrame()
    {
        gKeyPressed.fill(false);
        gKeyReleased.fill(false);

        gMousePressed.fill(false);
        gMouseReleased.fill(false);

        gScrollX = 0.0;
        gScrollY = 0.0;
    }

    bool ShouldClose()
    {
        return gWindow == nullptr ||
               glfwWindowShouldClose(gWindow);
    }

    bool IsActive()
    {
        return gActive;
    }

    int GetWindowWidth()
    {
        return gWindowWidth;
    }

    int GetWindowHeight()
    {
        return gWindowHeight;
    }

    bool IsKeyDown(Key key)
    {
        if (key == Key::Unknown || key == Key::Count) {
            return false;
        }

        return gKeyDown[KeyIndex(key)];
    }

    bool WasKeyPressed(Key key)
    {
        if (key == Key::Unknown || key == Key::Count) {
            return false;
        }

        return gKeyPressed[KeyIndex(key)];
    }

    bool WasKeyReleased(Key key)
    {
        if (key == Key::Unknown || key == Key::Count) {
            return false;
        }

        return gKeyReleased[KeyIndex(key)];
    }

    bool IsMouseButtonDown(MouseButton button)
    {
        if (button == MouseButton::Count) {
            return false;
        }

        return gMouseDown[MouseIndex(button)];
    }

    bool WasMouseButtonPressed(MouseButton button)
    {
        if (button == MouseButton::Count) {
            return false;
        }

        return gMousePressed[MouseIndex(button)];
    }

    bool WasMouseButtonReleased(MouseButton button)
    {
        if (button == MouseButton::Count) {
            return false;
        }

        return gMouseReleased[MouseIndex(button)];
    }

    double GetCursorX()
    {
        return gCursorX;
    }

    double GetCursorY()
    {
        return gCursorY;
    }

    double GetScrollX()
    {
        return gScrollX;
    }

    double GetScrollY()
    {
        return gScrollY;
    }

    const char* GetKeyName(Key key)
    {
        switch (key) {
        case Key::Space: return "Space";
        case Key::Apostrophe: return "Apostrophe";
        case Key::Comma: return "Comma";
        case Key::Minus: return "Minus";
        case Key::Period: return "Period";
        case Key::Slash: return "Slash";

        case Key::Num0: return "0";
        case Key::Num1: return "1";
        case Key::Num2: return "2";
        case Key::Num3: return "3";
        case Key::Num4: return "4";
        case Key::Num5: return "5";
        case Key::Num6: return "6";
        case Key::Num7: return "7";
        case Key::Num8: return "8";
        case Key::Num9: return "9";

        case Key::Semicolon: return "Semicolon";
        case Key::Equal: return "Equal";

        case Key::A: return "A";
        case Key::B: return "B";
        case Key::C: return "C";
        case Key::D: return "D";
        case Key::E: return "E";
        case Key::F: return "F";
        case Key::G: return "G";
        case Key::H: return "H";
        case Key::I: return "I";
        case Key::J: return "J";
        case Key::K: return "K";
        case Key::L: return "L";
        case Key::M: return "M";
        case Key::N: return "N";
        case Key::O: return "O";
        case Key::P: return "P";
        case Key::Q: return "Q";
        case Key::R: return "R";
        case Key::S: return "S";
        case Key::T: return "T";
        case Key::U: return "U";
        case Key::V: return "V";
        case Key::W: return "W";
        case Key::X: return "X";
        case Key::Y: return "Y";
        case Key::Z: return "Z";

        case Key::LeftBracket: return "Left Bracket";
        case Key::Backslash: return "Backslash";
        case Key::RightBracket: return "Right Bracket";
        case Key::GraveAccent: return "Grave Accent";

        case Key::Escape: return "Escape";
        case Key::Enter: return "Enter";
        case Key::Tab: return "Tab";
        case Key::Backspace: return "Backspace";
        case Key::Insert: return "Insert";
        case Key::Delete: return "Delete";

        case Key::Right: return "Right";
        case Key::Left: return "Left";
        case Key::Down: return "Down";
        case Key::Up: return "Up";

        case Key::PageUp: return "Page Up";
        case Key::PageDown: return "Page Down";
        case Key::Home: return "Home";
        case Key::End: return "End";

        case Key::CapsLock: return "Caps Lock";
        case Key::ScrollLock: return "Scroll Lock";
        case Key::NumLock: return "Num Lock";
        case Key::PrintScreen: return "Print Screen";
        case Key::Pause: return "Pause";

        case Key::F1: return "F1";
        case Key::F2: return "F2";
        case Key::F3: return "F3";
        case Key::F4: return "F4";
        case Key::F5: return "F5";
        case Key::F6: return "F6";
        case Key::F7: return "F7";
        case Key::F8: return "F8";
        case Key::F9: return "F9";
        case Key::F10: return "F10";
        case Key::F11: return "F11";
        case Key::F12: return "F12";
        case Key::F13: return "F13";
        case Key::F14: return "F14";
        case Key::F15: return "F15";
        case Key::F16: return "F16";
        case Key::F17: return "F17";
        case Key::F18: return "F18";
        case Key::F19: return "F19";
        case Key::F20: return "F20";
        case Key::F21: return "F21";
        case Key::F22: return "F22";
        case Key::F23: return "F23";
        case Key::F24: return "F24";
        case Key::F25: return "F25";

        case Key::Keypad0: return "Keypad 0";
        case Key::Keypad1: return "Keypad 1";
        case Key::Keypad2: return "Keypad 2";
        case Key::Keypad3: return "Keypad 3";
        case Key::Keypad4: return "Keypad 4";
        case Key::Keypad5: return "Keypad 5";
        case Key::Keypad6: return "Keypad 6";
        case Key::Keypad7: return "Keypad 7";
        case Key::Keypad8: return "Keypad 8";
        case Key::Keypad9: return "Keypad 9";

        case Key::KeypadDecimal: return "Keypad Decimal";
        case Key::KeypadDivide: return "Keypad Divide";
        case Key::KeypadMultiply: return "Keypad Multiply";
        case Key::KeypadSubtract: return "Keypad Subtract";
        case Key::KeypadAdd: return "Keypad Add";
        case Key::KeypadEnter: return "Keypad Enter";
        case Key::KeypadEqual: return "Keypad Equal";

        case Key::LeftShift: return "Left Shift";
        case Key::LeftControl: return "Left Control";
        case Key::LeftAlt: return "Left Alt";
        case Key::LeftSuper: return "Left Super";

        case Key::RightShift: return "Right Shift";
        case Key::RightControl: return "Right Control";
        case Key::RightAlt: return "Right Alt";
        case Key::RightSuper: return "Right Super";

        case Key::Menu: return "Menu";

        default:
            return "Unknown";
        }
    }

    const char* GetMouseButtonName(MouseButton button)
    {
        switch (button) {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::Button4: return "Button 4";
        case MouseButton::Button5: return "Button 5";
        case MouseButton::Button6: return "Button 6";
        case MouseButton::Button7: return "Button 7";
        case MouseButton::Button8: return "Button 8";

        default:
            return "Unknown";
        }
    }

    void SetWindowTitle(const char* title)
    {
        if (gWindow != nullptr) {
            glfwSetWindowTitle(gWindow, title);
        }
    }

    void SetCursorVisibility(CursorVisibility visibility)
    {
        if (gWindow == nullptr) {
            return;
        }

        switch (visibility) {
        case CursorVisibility::Hidden:
            glfwSetInputMode(
                gWindow,
                GLFW_CURSOR,
                GLFW_CURSOR_HIDDEN
            );
            break;

        case CursorVisibility::Visible:
            glfwSetInputMode(
                gWindow,
                GLFW_CURSOR,
                GLFW_CURSOR_NORMAL
            );
            break;
        }
    }

    void SetCursorClipping(CursorClipping clipping)
    {
        gCursorClipping = clipping;

        if (gWindow == nullptr) {
            return;
        }

        switch (clipping) {
        case CursorClipping::Off:
            glfwSetInputMode(
                gWindow,
                GLFW_CURSOR,
                GLFW_CURSOR_NORMAL
            );
            break;

        case CursorClipping::GameArea:
            glfwSetInputMode(
                gWindow,
                GLFW_CURSOR,
                GLFW_CURSOR_DISABLED
            );
            break;
        }
    }
}
