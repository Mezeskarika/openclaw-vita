#include "GamepadMapper.h"

#include <cstring>

namespace
{
    // Left stick deflection that presses a direction, and the lower one that releases it again,
    // so a stick resting near the threshold does not flicker the key
    const int AXIS_PRESS_THRESHOLD = 16000;
    const int AXIS_RELEASE_THRESHOLD = 12000;

    // Stick directions get input ids after the button ids: two per axis (negative, positive)
    int GetAxisInputId(Uint8 axis, bool isPositive)
    {
        return SDL_CONTROLLER_BUTTON_MAX + axis * 2 + (isPositive ? 1 : 0);
    }
}

bool GamepadMapper::TranslateEvent(const SDL_Event& evt, std::vector<SDL_Event>& outKeyEvents)
{
    switch (evt.type)
    {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
        {
            KeyBinding key;
            if (GetButtonBinding(evt.cbutton.button, key))
            {
                SetInputState(evt.cbutton.button, key, evt.type == SDL_CONTROLLERBUTTONDOWN,
                              evt.cbutton.timestamp, outKeyEvents);
            }
            return true;
        }
        case SDL_CONTROLLERAXISMOTION:
        {
            TranslateAxis(evt.caxis.axis, evt.caxis.value, evt.caxis.timestamp, outKeyEvents);
            return true;
        }
        default:
            return false;
    }
}

// The keys are the ones ActorController, the menus and the score screen listen to
bool GamepadMapper::GetButtonBinding(Uint8 button, KeyBinding& outBinding)
{
    switch (button)
    {
        // Cross: jump, confirm in menus
        case SDL_CONTROLLER_BUTTON_A: outBinding = { SDLK_SPACE, SDL_SCANCODE_SPACE }; return true;
        // Square: attack
        case SDL_CONTROLLER_BUTTON_X: outBinding = { SDLK_LCTRL, SDL_SCANCODE_LCTRL }; return true;
        // Circle: fire secondary weapon
        case SDL_CONTROLLER_BUTTON_B: outBinding = { SDLK_LALT, SDL_SCANCODE_LALT }; return true;
        // Triangle: switch ammo type
        case SDL_CONTROLLER_BUTTON_Y: outBinding = { SDLK_LSHIFT, SDL_SCANCODE_LSHIFT }; return true;
        // Start / Select: in-game menu, back
        case SDL_CONTROLLER_BUTTON_START: outBinding = { SDLK_ESCAPE, SDL_SCANCODE_ESCAPE }; return true;
        case SDL_CONTROLLER_BUTTON_BACK: outBinding = { SDLK_ESCAPE, SDL_SCANCODE_ESCAPE }; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: outBinding = { SDLK_UP, SDL_SCANCODE_UP }; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: outBinding = { SDLK_DOWN, SDL_SCANCODE_DOWN }; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: outBinding = { SDLK_LEFT, SDL_SCANCODE_LEFT }; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: outBinding = { SDLK_RIGHT, SDL_SCANCODE_RIGHT }; return true;
        default:
            return false;
    }
}

void GamepadMapper::TranslateAxis(Uint8 axis, Sint16 value, Uint32 timestamp, std::vector<SDL_Event>& outKeyEvents)
{
    KeyBinding negativeKey;
    KeyBinding positiveKey;
    if (axis == SDL_CONTROLLER_AXIS_LEFTX)
    {
        negativeKey = { SDLK_LEFT, SDL_SCANCODE_LEFT };
        positiveKey = { SDLK_RIGHT, SDL_SCANCODE_RIGHT };
    }
    else if (axis == SDL_CONTROLLER_AXIS_LEFTY)
    {
        negativeKey = { SDLK_UP, SDL_SCANCODE_UP };
        positiveKey = { SDLK_DOWN, SDL_SCANCODE_DOWN };
    }
    else
    {
        return;
    }

    int negativeId = GetAxisInputId(axis, false);
    int positiveId = GetAxisInputId(axis, true);

    int negativeThreshold = IsInputHeld(negativeId, negativeKey) ? AXIS_RELEASE_THRESHOLD : AXIS_PRESS_THRESHOLD;
    int positiveThreshold = IsInputHeld(positiveId, positiveKey) ? AXIS_RELEASE_THRESHOLD : AXIS_PRESS_THRESHOLD;

    SetInputState(negativeId, negativeKey, value < -negativeThreshold, timestamp, outKeyEvents);
    SetInputState(positiveId, positiveKey, value > positiveThreshold, timestamp, outKeyEvents);
}

void GamepadMapper::SetInputState(int inputId, const KeyBinding& key, bool pressed, Uint32 timestamp,
                                  std::vector<SDL_Event>& outKeyEvents)
{
    std::set<int>& holders = m_HeldKeys[key.keyCode];
    bool wasHeld = !holders.empty();

    if (pressed)
    {
        holders.insert(inputId);
    }
    else
    {
        holders.erase(inputId);
    }

    bool isHeld = !holders.empty();
    if (isHeld != wasHeld)
    {
        outKeyEvents.push_back(CreateKeyEvent(key, isHeld, timestamp));
    }
}

bool GamepadMapper::IsInputHeld(int inputId, const KeyBinding& key) const
{
    auto findIt = m_HeldKeys.find(key.keyCode);
    return findIt != m_HeldKeys.end() && findIt->second.count(inputId) != 0;
}

SDL_Event GamepadMapper::CreateKeyEvent(const KeyBinding& key, bool pressed, Uint32 timestamp)
{
    SDL_Event keyEvent;
    memset(&keyEvent, 0, sizeof(keyEvent));

    keyEvent.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    keyEvent.key.timestamp = timestamp;
    keyEvent.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    keyEvent.key.repeat = 0;
    keyEvent.key.keysym.sym = key.keyCode;
    keyEvent.key.keysym.scancode = key.scanCode;
    keyEvent.key.keysym.mod = KMOD_NONE;

    return keyEvent;
}
