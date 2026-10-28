#include "Catch.hpp"

#include <cstring>

#include "../OpenClaw/Engine/UserInterface/GamepadMapper.h"

namespace
{
    SDL_Event ButtonEvent(Uint8 button, bool pressed)
    {
        SDL_Event evt;
        memset(&evt, 0, sizeof(evt));
        evt.type = pressed ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
        evt.cbutton.button = button;
        evt.cbutton.state = pressed ? SDL_PRESSED : SDL_RELEASED;
        evt.cbutton.timestamp = 1234;
        return evt;
    }

    SDL_Event AxisEvent(Uint8 axis, Sint16 value)
    {
        SDL_Event evt;
        memset(&evt, 0, sizeof(evt));
        evt.type = SDL_CONTROLLERAXISMOTION;
        evt.caxis.axis = axis;
        evt.caxis.value = value;
        return evt;
    }

    std::vector<SDL_Event> Translate(GamepadMapper& mapper, const SDL_Event& evt)
    {
        std::vector<SDL_Event> keyEvents;
        mapper.TranslateEvent(evt, keyEvents);
        return keyEvents;
    }

    void RequireKeyEvent(const SDL_Event& evt, Uint32 type, SDL_Keycode keyCode, SDL_Scancode scanCode)
    {
        REQUIRE(evt.type == type);
        REQUIRE(evt.key.keysym.sym == keyCode);
        REQUIRE(evt.key.keysym.scancode == scanCode);
        REQUIRE(evt.key.repeat == 0);
        REQUIRE(evt.key.state == (type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED));
    }
}

TEST_CASE("----- GAMEPAD MAPPER -----")
{
    GamepadMapper mapper;

    SECTION("Face buttons map to the keys the game listens to")
    {
        struct { Uint8 button; SDL_Keycode keyCode; SDL_Scancode scanCode; } bindings[] =
        {
            { SDL_CONTROLLER_BUTTON_A, SDLK_SPACE, SDL_SCANCODE_SPACE },
            { SDL_CONTROLLER_BUTTON_X, SDLK_LCTRL, SDL_SCANCODE_LCTRL },
            { SDL_CONTROLLER_BUTTON_B, SDLK_LALT, SDL_SCANCODE_LALT },
            { SDL_CONTROLLER_BUTTON_Y, SDLK_LSHIFT, SDL_SCANCODE_LSHIFT },
            { SDL_CONTROLLER_BUTTON_START, SDLK_ESCAPE, SDL_SCANCODE_ESCAPE },
            { SDL_CONTROLLER_BUTTON_DPAD_UP, SDLK_UP, SDL_SCANCODE_UP },
            { SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDLK_DOWN, SDL_SCANCODE_DOWN },
            { SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDLK_LEFT, SDL_SCANCODE_LEFT },
            { SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDLK_RIGHT, SDL_SCANCODE_RIGHT },
        };

        for (const auto& binding : bindings)
        {
            std::vector<SDL_Event> down = Translate(mapper, ButtonEvent(binding.button, true));
            REQUIRE(down.size() == 1);
            RequireKeyEvent(down[0], SDL_KEYDOWN, binding.keyCode, binding.scanCode);
            REQUIRE(down[0].key.timestamp == 1234);

            std::vector<SDL_Event> up = Translate(mapper, ButtonEvent(binding.button, false));
            REQUIRE(up.size() == 1);
            RequireKeyEvent(up[0], SDL_KEYUP, binding.keyCode, binding.scanCode);
        }
    }

    SECTION("Unmapped buttons are consumed without producing keys")
    {
        std::vector<SDL_Event> keyEvents;
        REQUIRE(mapper.TranslateEvent(ButtonEvent(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, true), keyEvents));
        REQUIRE(keyEvents.empty());
    }

    SECTION("Non-controller events are not translated")
    {
        SDL_Event evt;
        memset(&evt, 0, sizeof(evt));
        evt.type = SDL_KEYDOWN;

        std::vector<SDL_Event> keyEvents;
        REQUIRE_FALSE(mapper.TranslateEvent(evt, keyEvents));
        REQUIRE(keyEvents.empty());
    }

    SECTION("Left stick presses a direction past the threshold and releases it near the centre")
    {
        REQUIRE(Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, 10000)).empty());

        std::vector<SDL_Event> down = Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, 20000));
        REQUIRE(down.size() == 1);
        RequireKeyEvent(down[0], SDL_KEYDOWN, SDLK_RIGHT, SDL_SCANCODE_RIGHT);

        // Between the release and press thresholds the key stays down
        REQUIRE(Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, 14000)).empty());

        std::vector<SDL_Event> up = Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, 5000));
        REQUIRE(up.size() == 1);
        RequireKeyEvent(up[0], SDL_KEYUP, SDLK_RIGHT, SDL_SCANCODE_RIGHT);
    }

    SECTION("Left stick Y axis maps up and down")
    {
        std::vector<SDL_Event> up = Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTY, -32768));
        REQUIRE(up.size() == 1);
        RequireKeyEvent(up[0], SDL_KEYDOWN, SDLK_UP, SDL_SCANCODE_UP);

        // Flicking straight to the other side releases one direction and presses the other
        std::vector<SDL_Event> flick = Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTY, 32767));
        REQUIRE(flick.size() == 2);
        RequireKeyEvent(flick[0], SDL_KEYUP, SDLK_UP, SDL_SCANCODE_UP);
        RequireKeyEvent(flick[1], SDL_KEYDOWN, SDLK_DOWN, SDL_SCANCODE_DOWN);
    }

    SECTION("Other axes are ignored")
    {
        REQUIRE(Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_RIGHTX, 32767)).empty());
        REQUIRE(Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_TRIGGERLEFT, 32767)).empty());
    }

    SECTION("A key held by the D-pad and the stick is released only when both let go")
    {
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_DPAD_LEFT, true)).size() == 1);
        REQUIRE(Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, -20000)).empty());

        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_DPAD_LEFT, false)).empty());

        std::vector<SDL_Event> up = Translate(mapper, AxisEvent(SDL_CONTROLLER_AXIS_LEFTX, 0));
        REQUIRE(up.size() == 1);
        RequireKeyEvent(up[0], SDL_KEYUP, SDLK_LEFT, SDL_SCANCODE_LEFT);
    }

    SECTION("Start and Select share Escape")
    {
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_START, true)).size() == 1);
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_BACK, true)).empty());
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_START, false)).empty());
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_BACK, false)).size() == 1);
    }
}

TEST_CASE("----- GAMEPAD MAPPER BINDINGS -----")
{
    GamepadMapper mapper;

    SECTION("Defaults match the original mapping")
    {
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Jump) == "Cross");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Attack) == "Square");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Fire) == "Circle");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_SwitchAmmo) == "Triangle");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Menu) == "Start,Select");
    }

    SECTION("Rebinding moves the key to the new button")
    {
        REQUIRE(mapper.SetActionButtons(GamepadMapper::Action_Jump, "R"));

        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_A, true)).empty());
        std::vector<SDL_Event> down = Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, true));
        REQUIRE(down.size() == 1);
        RequireKeyEvent(down[0], SDL_KEYDOWN, SDLK_SPACE, SDL_SCANCODE_SPACE);
    }

    SECTION("A button taken by another action leaves its old action")
    {
        REQUIRE(mapper.SetActionButtons(GamepadMapper::Action_Attack, "Cross, L"));
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Attack) == "Cross,L");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Jump) == "");

        std::vector<SDL_Event> down = Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_A, true));
        REQUIRE(down.size() == 1);
        RequireKeyEvent(down[0], SDL_KEYDOWN, SDLK_LCTRL, SDL_SCANCODE_LCTRL);
    }

    SECTION("Invalid lists change nothing")
    {
        REQUIRE_FALSE(mapper.SetActionButtons(GamepadMapper::Action_Jump, ""));
        REQUIRE_FALSE(mapper.SetActionButtons(GamepadMapper::Action_Jump, "Cross,Nope"));
        REQUIRE_FALSE(mapper.SetActionButtons(GamepadMapper::Action_Jump, "DPadUp"));
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Jump) == "Cross");
    }

    SECTION("The D-pad always moves")
    {
        REQUIRE(mapper.SetActionButtons(GamepadMapper::Action_Menu, "Select"));
        std::vector<SDL_Event> down = Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_DPAD_LEFT, true));
        REQUIRE(down.size() == 1);
        RequireKeyEvent(down[0], SDL_KEYDOWN, SDLK_LEFT, SDL_SCANCODE_LEFT);
        REQUIRE(Translate(mapper, ButtonEvent(SDL_CONTROLLER_BUTTON_START, true)).empty());
    }

    SECTION("Reset restores the defaults")
    {
        REQUIRE(mapper.SetActionButtons(GamepadMapper::Action_Jump, "R"));
        mapper.ResetToDefaults();
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Jump) == "Cross");
        REQUIRE(mapper.GetActionButtons(GamepadMapper::Action_Menu) == "Start,Select");
    }
}
