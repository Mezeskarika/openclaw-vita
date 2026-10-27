#ifndef __GAMEPADMAPPER_H__
#define __GAMEPADMAPPER_H__

#include <SDL2/SDL.h>
#include <map>
#include <set>
#include <vector>

// Turns game controller input (e.g. the PS Vita's built-in controls) into the keyboard events
// the game already handles, so menus, gameplay and score screens need no gamepad code of their own.
// Makes no SDL calls, so it can be unit tested without an SDL runtime.
class GamepadMapper
{
public:
    // Appends the SDL_KEYDOWN / SDL_KEYUP events synthesized from a controller button or axis event.
    // Returns false if the event is not a controller button or axis event.
    bool TranslateEvent(const SDL_Event& evt, std::vector<SDL_Event>& outKeyEvents);

private:
    struct KeyBinding
    {
        SDL_Keycode keyCode;
        SDL_Scancode scanCode;
    };

    static bool GetButtonBinding(Uint8 button, KeyBinding& outBinding);
    static SDL_Event CreateKeyEvent(const KeyBinding& key, bool pressed, Uint32 timestamp);

    void TranslateAxis(Uint8 axis, Sint16 value, Uint32 timestamp, std::vector<SDL_Event>& outKeyEvents);
    void SetInputState(int inputId, const KeyBinding& key, bool pressed, Uint32 timestamp,
                       std::vector<SDL_Event>& outKeyEvents);
    bool IsInputHeld(int inputId, const KeyBinding& key) const;

    // Several inputs can hold the same key (D-pad left and stick left).
    // A key goes down with the first input and up when the last one lets go.
    std::map<SDL_Keycode, std::set<int>> m_HeldKeys;
};

#endif
