#ifndef __GAMEPADMAPPER_H__
#define __GAMEPADMAPPER_H__

#include <SDL2/SDL.h>
#include <map>
#include <set>
#include <string>
#include <vector>

// Turns game controller input (e.g. the PS Vita's built-in controls) into the keyboard events
// the game already handles, so menus, gameplay and score screens need no gamepad code of their own.
// Makes no SDL calls, so it can be unit tested without an SDL runtime.
class GamepadMapper
{
public:
    // What a button can be bound to. The D-pad and left stick always move.
    enum Action
    {
        Action_Jump,        // Also confirms in menus
        Action_Attack,
        Action_Fire,        // Secondary weapon
        Action_SwitchAmmo,
        Action_Menu,        // In-game menu, back in menus
        Action_Count
    };

    GamepadMapper();

    // Appends the SDL_KEYDOWN / SDL_KEYUP events synthesized from a controller button or axis event.
    // Returns false if the event is not a controller button or axis event.
    bool TranslateEvent(const SDL_Event& evt, std::vector<SDL_Event>& outKeyEvents);

    // Binds an action to a comma-separated list of Vita button names (see ButtonFromName), replacing
    // the buttons it had; a button taken from another action leaves that action. Returns false,
    // changing nothing, if the list is empty or has an unknown name.
    bool SetActionButtons(Action action, const std::string& buttonList);
    // The comma-separated button names bound to an action, e.g. "Start,Select"
    std::string GetActionButtons(Action action) const;
    void ResetToDefaults();

    // Config element name of an action ("Jump", "Attack", ...)
    static const char* GetActionName(Action action);
    // Vita button names: Cross, Circle, Square, Triangle, L, R, Start, Select
    static bool ButtonFromName(const std::string& name, Uint8& outButton);

private:
    struct KeyBinding
    {
        SDL_Keycode keyCode;
        SDL_Scancode scanCode;
    };

    bool GetButtonBinding(Uint8 button, KeyBinding& outBinding) const;
    static KeyBinding GetActionKey(Action action);
    static SDL_Event CreateKeyEvent(const KeyBinding& key, bool pressed, Uint32 timestamp);

    void TranslateAxis(Uint8 axis, Sint16 value, Uint32 timestamp, std::vector<SDL_Event>& outKeyEvents);
    void UpdateStick(Uint32 timestamp, std::vector<SDL_Event>& outKeyEvents);
    void SetInputState(int inputId, const KeyBinding& key, bool pressed, Uint32 timestamp,
                       std::vector<SDL_Event>& outKeyEvents);
    bool IsInputHeld(int inputId, const KeyBinding& key) const;

    // Remappable buttons (everything except the D-pad) and the action each one triggers
    std::map<Uint8, Action> m_ButtonActions;

    // Several inputs can hold the same key (D-pad left and stick left).
    // A key goes down with the first input and up when the last one lets go.
    std::map<SDL_Keycode, std::set<int>> m_HeldKeys;

    // Last position of the left stick
    Sint16 m_StickX = 0;
    Sint16 m_StickY = 0;
};

#endif
