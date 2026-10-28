# OpenClaw Vita port: roadmap

What is still to do, as of 2026-10-28. `PLAN.md` is the stage-by-stage history of the port so far;
this file is the work list from here on. Each item says what "done" means and where in the code it
starts.

Status: **[todo]** not started, **[research]** needs findings before any code, **[blocked]** waits on
another item.

Current state (v0.3.0): the game boots in Vita3K, runs at 60 fps, plays music,
saves and quits cleanly. Nothing has run on real hardware yet.

**Scope: the PS Vita only.** Changes don't need to keep the Windows, Linux, macOS, Android or
WebAssembly builds working; item 9 removes what the Vita doesn't need. The one exception worth
deciding on deliberately is the Linux host build used for profiling and smoke tests (see 9).

## Overview

| # | Item | Kind | Depends on |
|---|---|---|---|
| 1 | Navigate menus with touch | code, **needs Vita3K check** | — |
| 2 | Disable unimplemented menu items | menu data, **done** | — |
| 3 | Save settings to `config.xml` | code, **done** | — |
| 4 | Remappable buttons | code + menu | 3, 8 (page layout) |
| 5 | Real LiveArea artwork | art | — |
| 6 | README: installation and config options | docs, **in progress** | — |
| 7 | CHANGELOG per tagged version, incl. divergences | docs, **done** | — |
| 8 | Research the original game's Options submenus | research, **done** | — |
| 9 | Strip everything the Vita build doesn't need | cleanup, **done** | — |
| 10 | CI job that builds the VPK | infra, **written, not yet run** | — |
| 11 | Research FOV / resolution settings for the Display menu | research | 8 |
| 12 | Plan difficulty settings (easy to hard options) | research, **planned** | 8 |
| 13 | Implement the disabled Options submenus | code + menu | 3, 4, 8, 11, 12 |

Suggested order: 2, 6 and 9 first (small, visible, and 9 makes everything after it simpler), then
3 and 8 (they unblock 4, 11, 12, 13), then 1, 4, 11, 12, 13. 5 and 10 can happen any time.

---

## 1. Navigate menus with touch **[implemented; needs a Vita3K/hardware check]**

SDL 2.32 on Vita synthesizes mouse clicks from the touch panels only once
`SDL_HINT_VITA_TOUCH_MOUSE_DEVICE` is set (unset, its internal device id stays 0 and nothing is
generated, despite the docs calling the front panel the default). `InitializeDisplay` now sets it to
the front panel before `SDL_Init`, so taps reach the menus' existing click handling
(`ScreenElementMenuPage::VOnEvent`, scaled by `g_MenuScale`). In gameplay, touch-generated mouse
events (`which == SDL_TOUCH_MOUSEID`) are dropped unless the touch controls are enabled
(`HumanView::IsTouchOutsideMenus`), so a tap can't end firing.

Not verified: the host build can't emulate the Vita touch panel, and mouse clicks in the headless
Xvfb test setup (no window manager) make the window disappear even on an empty spot, so that setup
can't test clicks either. Check in Vita3K (a mouse click there is a front-panel touch): tap items
on the main menu, Options, Audio (also the volume knobs) and the in-game menu; tapping during
gameplay should do nothing.

## 2. Disable unimplemented menu items **[done]**

Multiplayer, Single Player → Load Custom Level, and Options → Edit Players, Difficulty, Controls,
Display are `<State>Disabled</State>` in `Build_Release/ASSETS/MENU.xml`. The game data has no
greyed-out images for them (only Replay Movies, Audio and Upload Scores have one), so
`ScreenElementMenuItem::VOnRender` draws a disabled item without a `<DisabledImage>` as its inactive
image, darkened. Navigation and clicks already skip disabled items. Checked on the host build:
greyed out, skipped by up/down, no `Could not switch to page` in the log. 13 re-enables the ones
that get implemented.

## 3. Save settings to `config.xml` **[done]**

`BaseGameApp` keeps the loaded config document; `SaveGameOptions` writes the options the menus can
change into it and saves through `config.xml.tmp` plus remove-then-rename (the Vita can't rename
over an existing file); `LoadGameOptions` recovers the `.tmp` file if a save was interrupted. On
Vita an unreadable config is replaced with `app0:/config.xml` instead of desktop defaults. The Audio
page saves on every volume and on/off change (`HumanView::SaveAudioOptions`), which also keeps the
in-memory `musicVolume` current, so the boss fight no longer resets the music volume to the old
config value.

Checked on the host build: changing sound volume and turning sound off through the menu updates
`config.xml` immediately; every other option and all comments are kept (TinyXML re-indents the file
and trims the trailing space in `<CommandPrompt>`). Not yet checked on Vita: that `remove` +
`rename` works on `ux0:`.

New settings pages (4, 13) add their values to `SaveGameOptions`.

## 4. Remappable buttons **[todo]** (needs 3; UI blocked on 8)

- The Vita mapping is one table: `GamepadMapper::GetButtonBinding`
  (`OpenClaw/Engine/UserInterface/GamepadMapper.cpp`). Step 1: read it from `config.xml`
  (e.g. `<ControlOptions><Gamepad><Jump>A</Jump>…`), keeping today's mapping as the default.
  The host unit tests (`tests/GamepadMapperTests.cpp`) cover the table and need extending.
- Step 2: an in-game remapping page (Options → Controls): list the actions, select one, press the
  new button, reject conflicts, "reset to defaults", save through 3. This needs the menu engine
  to wait for a button press, which it can't do today.
- Check: remap jump, restart, the new button jumps; defaults restore the original mapping.

## 5. Real LiveArea artwork **[todo]**

- Replace the placeholders from `vita/make_livearea.py` in `vita/sce_sys/`: `icon0.png` 128×128,
  `livearea/contents/bg.png` 840×500, `livearea/contents/startup.png` 280×158. All must be 8-bit
  indexed PNGs, or the Vita refuses to install the VPK.
- Licence: the VPK must not contain Monolith's artwork (logo, Claw sprites from CLAW.REZ). Use
  original art, or art whose licence allows redistribution, and record the source and licence next
  to the files.
- Optional: `pic0.png` (960×544) for the home screen.
- Check: install on Vita3K and hardware, icon and LiveArea display correctly.

## 6. README: installation and config options **[in progress]**

Done: `README.md` covers requirements, install (VitaShell and Vita3K), updating (delete
`config.xml`, keep `SAVES.XML`), the log file, controls, known limitations, development links and
licences (OpenClaw GPL-3.0, FreePats GPL-2.0-or-later with exception). Release notes link to it
instead of repeating those sections.

Still to do:
- Config options that matter on Vita: `Size`/`Scale`, `UseVerticalSync`, `Frequency`,
  `SoundVolume`, `MusicVolume`, `MusicOn`, `TimidityCfg`, `ResourceCacheSize`, `ShowFps`,
  `LogFrameStats`, `FrontPlaneMode`, `SkipMenu`: what each does and sensible values.
- Where `CLAW.REZ` comes from (which editions of the game include it).
- Keep the controls table in sync with `GamepadMapper` (and with 4 once buttons are remappable).

## 7. CHANGELOG **[done]**

`CHANGELOG.md` at the repo root: one section per version (`v0.1.0`–`v0.3.0`), plus "Differences from
upstream OpenClaw" and "Differences from the original game". Keep it current with every change
(see "Versioning and releases" in `CLAUDE.md`), including the removals from 9, difficulty (12) and
anything from 13.

## 8. Research the original game's Options submenus **[done]**

Findings in `vita/OPTIONS_RESEARCH.md`, from CLAW.REZ (unused menu images, the original menu
definition `CLAW.IFC`) and Monolith's Claw Design Bible. In short: the original had no Difficulty
settings page (difficulty, Normal or Easy, was chosen when creating a player); Controls was a
keyboard/joypad binding dialog; Display had color depth, detail, front layer, a letterbox
"game play area" slider and movie mode; Audio also had Voice and Ambient toggles.


What did Captain Claw (1997) put behind Edit Players, Difficulty, Controls and Display (and
Multiplayer, Load Custom Level)?

- Sources: the original manual, the game itself or recordings of it, CLAW.REZ (the menu images
  under `/STATES/MENU/IMAGES/MAIN/OPTIONS/` and any submenu images that OpenClaw never used,
  e.g. difficulty or detail-level graphics), community wikis.
- Deliverable: a section per submenu here, listing its options, what they did in the original, and
  whether each makes sense on Vita. This feeds 4, 11, 12 and 13.

## 9. Strip everything the Vita build doesn't need **[done]**

Removed: Visual Studio and Eclipse projects, Travis/AppVeyor CI, deb/rpm packaging, Windows
prebuilt binaries and SDL libraries, the C# launcher, the Windows MIDI RPC server and client code,
the Android and Emscripten code paths and CMake branches, MSVC debug-heap hooks, libsigc++
(unused), an unused header-only Box2D copy in `ThirdParty/`, Box2D's and TinyXML's examples, docs
and generated files, desktop configs, the committed `ASSETS.ZIP` (now generated by the build) and
the VS-only libwap tests (`Catch.hpp` moved to `tests/`). Tracked files went from about 73 MB to
8 MB.

Kept on purpose:
- **The Linux host build** (root `CMakeLists.txt`, non-Vita branch), as a development tool: the
  profiling, the save-crash reproduction, the music render tests and the menu screenshots all ran
  on it. Removing it later is one CMake branch plus the `#ifndef __vita__` code in `MainLoop.cpp`.
- **`ThirdParty/SDL2/` headers**: `tests/run.sh` compiles against them on a host without SDL dev
  packages. Their Windows libraries are gone.
- Third-party library sources as they are (`Miniz` still has `_WIN32` branches; it's vendored
  code).

Each step was checked with the Vita build, the host unit tests and the Linux build (level 1 loads).

## 10. CI job that builds the VPK **[written; runs once pushed]**

`.github/workflows/build.yml` (GitHub Actions): on every push, pull request or manual run it calls
`vita/build.sh` and `tests/run.sh` on `ubuntu-24.04` and uploads `openclaw.vpk` and the unstripped
`openclaw` ELF as an artifact named after the commit. The FreePats download is cached. Checked
with actionlint; it hasn't run on GitHub yet, because nothing has been pushed.

Left out on purpose: building the Linux host build in CI (a development tool, not a product), and
release automation (releases stay manual, see `CLAUDE.md`). Either can be added later.

## 11. Research FOV and resolution for the Display menu **[research]**

Could a Display page offer settings that actually change anything on a Vita?

- The Vita screen is fixed at 960×544. The game renders at window size × `<Scale>`
  (`SDL_RenderSetScale` in `BaseGameApp::InitializeDisplay`; `BaseGameApp::SetWindowSize` and
  `SetScale` already exist and are used by the console).
- "FOV" here means how much of the level is visible: at `Scale` 1 the camera shows 960×544 world
  pixels, at 2 it shows 480×272, zoomed in. The original game was designed for 640×480, so a
  scale of about 1.13 (544/480) shows the original's vertical view. Questions to answer:
  - Do non-integer scales look acceptable (filtering, tile seams)? `SDL_HINT_RENDER_SCALE_QUALITY`
    and `SDL_RenderSetLogicalSize` are the levers.
  - Does changing it at runtime work (camera, HUD, menu positions via `g_MenuScale`)?
  - Do the HUD and menus need to follow the scale or stay fixed?
  - Does a wider view change the game (enemies noticing Claw from off-screen, `VIsVisible`
    culling), or cost frame rate?
- Deliverable: which Display options are worth having (e.g. "View: Original / Wide / Close"),
  what each does on Vita, and the implementation sketch for 13.

## 12. Plan difficulty settings **[planned; needs a decision]**

Plan and recommendation in `vita/OPTIONS_RESEARCH.md` ("Difficulty"): the original's Easy mode
raised Claw's melee damage (5 → 20); OpenClaw hard-codes it in `ClawControllableComponent`.
Recommended: Normal/Easy with a melee multiplier, optionally plus reduced damage taken. Needs the
maintainer's choice before 13.


The engine has no notion of difficulty today; "Difficulty" appears only as a menu page name.
Lay out options from cheapest to most involved, then pick one:

1. **Damage scaling (easiest).** One multiplier on damage dealt to Claw (and/or by Claw), e.g.
   easy: enemies deal 20% less, hard: 20% more. Single point of change: `HealthComponent::AddHealth`
   (`OpenClaw/Engine/Actor/Components/ControllerComponents/HealthComponent.cpp`), checking whose
   health it is and the damage type, and leaving death tiles (`DamageType_DeathTile`) alone.
2. **Resources.** Start with more or fewer lives/ammo, bigger or smaller health pickups
   (`PickupComponent`, the checkpoint save values).
3. **Enemy health.** Scale enemy max health when actors are created (actor prototypes in
   `ASSETS/ACTOR_PROTOTYPES`, or a multiplier at creation).
4. **Enemy behaviour.** Attack frequency, reaction time and projectile speed in the enemy AI
   components (`Engine/Actor/Components/EnemyAI/`). Most work, most risk to balance.

For each: what changes, which files, how to test it, and whether it affects saves (should a save
remember its difficulty?). Recommendation and the Difficulty page layout go into 13.

## 13. Implement the disabled Options submenus **[blocked on 3, 4, 8, 11, 12]**

Using the findings from 8, 11 and 12, build the pages that make sense on Vita and re-enable their
buttons (undoing 2 for those):

- **Controls:** the remapping page from 4.
- **Difficulty:** the option chosen in 12.
- **Display:** the options chosen in 11.
- **Edit Players:** only if 8 shows it is worth it (player profiles = separate save files).

Every setting is saved through 3. The menus also need pages that change a value in place
(left/right on a setting), as the Audio page already does for volume. Each page needs a Vita3K
check, and the CHANGELOG (7) entry for it.
