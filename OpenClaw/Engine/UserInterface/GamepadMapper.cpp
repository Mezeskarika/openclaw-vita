#include "GamepadMapper.h"

#include <cstring>
#include <iterator>

namespace
{
    // Left stick deflection that presses a direction, and the lower one that releases it again,
    // so a stick resting near the threshold does not flicker the key
    const int AXIS_PRESS_THRESHOLD = 16000;
    const int AXIS_RELEASE_THRESHOLD = 12000;

    // The stick is four directions, not two independent axes: up and down only within 30 degrees of
    // vertical (35 once held), left and right anywhere else. Read as two axes, pushing right and a
    // little up pressed Up too, which kept Claw climbing at the top of a ladder and made him ignore
    // Jump on it (PS Vita, 0.5.0). Claw needs no diagonals: picking up is Up plus a button
    const double VERTICAL_PRESS_RATIO = 1.73;   // tan(60 degrees)
    const double VERTICAL_HOLD_RATIO = 1.43;    // tan(55 degrees)

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
    if (axis == SDL_CONTROLLER_AXIS_LEFTX)
    {
        m_StickX = value;
    }
    else if (axis == SDL_CONTROLLER_AXIS_LEFTY)
    {
        m_StickY = value;
    }
    else
    {
        return;
    }
    UpdateStick(timestamp, outKeyEvents);
}

void GamepadMapper::UpdateStick(Uint32 timestamp, std::vector<SDL_Event>& outKeyEvents)
{
    struct Direction
    {
        int inputId;
        KeyBinding key;
        bool isVertical;
        int sign;
        bool pressed;
    };
    Direction directions[] =
    {
        { GetAxisInputId(SDL_CONTROLLER_AXIS_LEFTY, false), { SDLK_UP, SDL_SCANCODE_UP }, true, -1, false },
        { GetAxisInputId(SDL_CONTROLLER_AXIS_LEFTY, true), { SDLK_DOWN, SDL_SCANCODE_DOWN }, true, 1, false },
        { GetAxisInputId(SDL_CONTROLLER_AXIS_LEFTX, false), { SDLK_LEFT, SDL_SCANCODE_LEFT }, false, -1, false },
        { GetAxisInputId(SDL_CONTROLLER_AXIS_LEFTX, true), { SDLK_RIGHT, SDL_SCANCODE_RIGHT }, false, 1, false },
    };

    const bool wasVertical = IsInputHeld(directions[0].inputId, directions[0].key) ||
        IsInputHeld(directions[1].inputId, directions[1].key);
    const double absX = m_StickX < 0 ? -(double)m_StickX : (double)m_StickX;
    const double absY = m_StickY < 0 ? -(double)m_StickY : (double)m_StickY;
    const bool isVertical = absY > absX * (wasVertical ? VERTICAL_HOLD_RATIO : VERTICAL_PRESS_RATIO);

    for (Direction& direction : directions)
    {
        const int value = (direction.isVertical ? m_StickY : m_StickX) * direction.sign;
        const int threshold = IsInputHeld(direction.inputId, direction.key) ? AXIS_RELEASE_THRESHOLD : AXIS_PRESS_THRESHOLD;
        direction.pressed = (direction.isVertical == isVertical) && value > threshold;
    }

    // Releases first, so a direction change reads as one key up, then the other down
    for (const Direction& direction : directions)
    {
        if (!direction.pressed)
        {
            SetInputState(direction.inputId, direction.key, false, timestamp, outKeyEvents);
        }
    }
    for (const Direction& direction : directions)
    {
        if (direction.pressed)
        {
            SetInputState(direction.inputId, direction.key, true, timestamp, outKeyEvents);
        }
    }
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
