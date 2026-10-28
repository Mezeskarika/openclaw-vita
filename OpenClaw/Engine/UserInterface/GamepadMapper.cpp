#include "GamepadMapper.h"

#include <cstring>
#include <iterator>

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

    // SDL's names for the Vita's buttons: SDL maps Cross to A, Circle to B, Square to X, Triangle to Y
    struct ButtonName
    {
        const char* name;
        Uint8 button;
    };
    const ButtonName BUTTON_NAMES[] =
    {
        { "Cross", SDL_CONTROLLER_BUTTON_A },
        { "Circle", SDL_CONTROLLER_BUTTON_B },
        { "Square", SDL_CONTROLLER_BUTTON_X },
        { "Triangle", SDL_CONTROLLER_BUTTON_Y },
        { "L", SDL_CONTROLLER_BUTTON_LEFTSHOULDER },
        { "R", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER },
        { "Start", SDL_CONTROLLER_BUTTON_START },
        { "Select", SDL_CONTROLLER_BUTTON_BACK },
    };

    const char* ACTION_NAMES[GamepadMapper::Action_Count] = { "Jump", "Attack", "Fire", "SwitchAmmo", "Menu" };
}

GamepadMapper::GamepadMapper()
{
    ResetToDefaults();
}

void GamepadMapper::ResetToDefaults()
{
    m_ButtonActions.clear();
    m_ButtonActions[SDL_CONTROLLER_BUTTON_A] = Action_Jump;
    m_ButtonActions[SDL_CONTROLLER_BUTTON_X] = Action_Attack;
    m_ButtonActions[SDL_CONTROLLER_BUTTON_B] = Action_Fire;
    m_ButtonActions[SDL_CONTROLLER_BUTTON_Y] = Action_SwitchAmmo;
    m_ButtonActions[SDL_CONTROLLER_BUTTON_START] = Action_Menu;
    m_ButtonActions[SDL_CONTROLLER_BUTTON_BACK] = Action_Menu;
}

const char* GamepadMapper::GetActionName(Action action)
{
    return (action >= 0 && action < Action_Count) ? ACTION_NAMES[action] : "";
}

bool GamepadMapper::ButtonFromName(const std::string& name, Uint8& outButton)
{
    for (const ButtonName& entry : BUTTON_NAMES)
    {
        if (name == entry.name)
        {
            outButton = entry.button;
            return true;
        }
    }
    return false;
}

bool GamepadMapper::SetActionButtons(Action action, const std::string& buttonList)
{
    if (action < 0 || action >= Action_Count)
    {
        return false;
    }

    std::vector<Uint8> buttons;
    size_t start = 0;
    while (start <= buttonList.size())
    {
        size_t end = buttonList.find(',', start);
        if (end == std::string::npos)
        {
            end = buttonList.size();
        }

        std::string name = buttonList.substr(start, end - start);
        size_t first = name.find_first_not_of(" \t");
        size_t last = name.find_last_not_of(" \t");
        name = (first == std::string::npos) ? "" : name.substr(first, last - first + 1);

        Uint8 button;
        if (!ButtonFromName(name, button))
        {
            return false;
        }
        buttons.push_back(button);
        start = end + 1;
    }

    if (buttons.empty())
    {
        return false;
    }

    // A button does one thing: taking it for this action removes it from any other
    for (auto it = m_ButtonActions.begin(); it != m_ButtonActions.end();)
    {
        it = (it->second == action) ? m_ButtonActions.erase(it) : std::next(it);
    }
    for (Uint8 button : buttons)
    {
        m_ButtonActions[button] = action;
    }
    return true;
}

std::string GamepadMapper::GetActionButtons(Action action) const
{
    std::string list;
    for (const ButtonName& entry : BUTTON_NAMES)
    {
        auto findIt = m_ButtonActions.find(entry.button);
        if (findIt != m_ButtonActions.end() && findIt->second == action)
        {
            list += (list.empty() ? "" : ",") + std::string(entry.name);
        }
    }
    return list;
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
GamepadMapper::KeyBinding GamepadMapper::GetActionKey(Action action)
{
    switch (action)
    {
        case Action_Jump: return { SDLK_SPACE, SDL_SCANCODE_SPACE };
        case Action_Attack: return { SDLK_LCTRL, SDL_SCANCODE_LCTRL };
        case Action_Fire: return { SDLK_LALT, SDL_SCANCODE_LALT };
        case Action_SwitchAmmo: return { SDLK_LSHIFT, SDL_SCANCODE_LSHIFT };
        default: return { SDLK_ESCAPE, SDL_SCANCODE_ESCAPE };
    }
}

bool GamepadMapper::GetButtonBinding(Uint8 button, KeyBinding& outBinding) const
{
    auto findIt = m_ButtonActions.find(button);
    if (findIt != m_ButtonActions.end())
    {
        outBinding = GetActionKey(findIt->second);
        return true;
    }

    switch (button)
    {
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
