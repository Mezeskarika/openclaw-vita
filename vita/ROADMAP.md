# OpenClaw Vita port: roadmap

What is still to do, as of 2026-10-28. `PLAN.md` is the stage-by-stage history of the port so far;
this file is the work list from here on. Each item says what "done" means and where in the code it
starts.

Status: **[todo]** not started, **[research]** needs findings before any code, **[blocked]** waits on
another item.

Current state (tag `vita-vita3k-music`): the game boots in Vita3K, runs at 60 fps, plays music,
saves and quits cleanly. Nothing has run on real hardware yet.

**Scope: the PS Vita only.** Changes don't need to keep the Windows, Linux, macOS, Android or
WebAssembly builds working; item 9 removes what the Vita doesn't need. The one exception worth
deciding on deliberately is the Linux host build used for profiling and smoke tests (see 9).

## Overview

| # | Item | Kind | Depends on |
|---|---|---|---|
| 1 | Navigate menus with touch | code | — |
| 2 | Disable unimplemented menu items | menu data | — |
| 3 | Save settings to `config.xml` | code | — |
| 4 | Remappable buttons | code + menu | 3, 8 (page layout) |
| 5 | Real LiveArea artwork | art | — |
| 6 | README: installation and config options | docs | — |
| 7 | CHANGELOG per tagged version, incl. divergences | docs | — |
| 8 | Research the original game's Options submenus | research | — |
| 9 | Strip everything the Vita build doesn't need | cleanup | decision on host build |
| 10 | CI job that builds the VPK | infra | 9 (fewer things to build) |
| 11 | Research FOV / resolution settings for the Display menu | research | 8 |
| 12 | Plan difficulty settings (easy to hard options) | research | 8 |
| 13 | Implement the disabled Options submenus | code + menu | 3, 4, 8, 11, 12 |

Suggested order: 2, 6 and 9 first (small, visible, and 9 makes everything after it simpler), then
3 and 8 (they unblock 4, 11, 12, 13), then 1, 4, 11, 12, 13. 5, 7 and 10 can happen any time.

---

## 1. Navigate menus with touch **[todo]**

Tap a menu item on the Vita's front touchscreen to select it.

- Menus already handle mouse clicks: `ScreenElementMenu::VOnEvent`
  (`OpenClaw/Engine/UserInterface/UserInterface.cpp`, the `SDL_MOUSEBUTTONDOWN` branch) hit-tests the
  click against each item's rectangle, scaled by `g_MenuScale`.
- On Vita, SDL reports the touchscreen as `SDL_FINGER*` events. SDL can also turn touches into
  synthetic mouse events (`SDL_HINT_TOUCH_MOUSE_EVENTS`); check whether the Vita backend does, and
  if not, translate `SDL_FINGERDOWN` (normalised 0–1 coordinates) into a left click in
  `BaseGameApp::OnEvent`.
- Keep touch out of gameplay unless the Android touch controls are enabled (`<TouchScreen><Enable>`
  in `config.xml`, currently false on Vita), so a tap in a level doesn't count as a click.
- Check: tap every item on the main menu, Options, Audio and the in-game menu; buttons still work.

## 2. Disable unimplemented menu items **[todo]**

Options → Edit Players, Difficulty, Controls, Display, plus Main → Multiplayer and
Single Player → Load Custom Level, point at menu pages that don't exist. Selecting them does nothing
and logs `Could not switch to page: …, Probably not implemented yet`.

- Pure data change in `Build_Release/ASSETS/MENU.xml` (bundled via `ASSETS.ZIP`): set
  `<State>Disabled</State>` and give each a `<DisabledImage>`. The engine already supports this
  (`StringToMenuItemState` in `UserInterface.cpp`; the Audio button has a disabled image, and 66
  items in `MENU.xml` use one). Check the CLAW.REZ menu images for greyed-out variants
  (e.g. `009_AUDIO.PCX` is Audio's) and use the matching one for each button.
- `INGAME_MENU.XML` only links the Audio page, so it needs no change.
- Make sure up/down navigation skips disabled items.
- Check: the items look disabled, can't be selected, and the log has no `Could not switch to page`.

## 3. Save settings to `config.xml` **[todo]**

Prerequisite for every setting the player can change in a menu (4 and 13). Today nothing is saved:
`BaseGameApp::SaveGameOptions` (`OpenClaw/Engine/GameApp/BaseGameApp.cpp`) only logs
"Not implemented yet!", and is called once, from `Terminate()`, which the Vita build skips.
Even the Audio page's volume and music on/off are lost on restart.

- Write `ux0:data/openclaw/config.xml` when a setting changes (not on exit: the Vita can kill the
  app from the home screen without the game noticing).
- Update the loaded document rather than regenerating it, so options the menus don't touch
  (`TimidityCfg`, `LogFrameStats`, debug options, comments) survive.
- Write to a temporary file and rename it over `config.xml`, so a crash mid-write can't leave a
  broken config. `LoadGameOptions` treats an unreadable config like a missing one: it writes the
  built-in desktop defaults over it, which would silently lose the Vita settings.
- First user: the Audio page (sound/music volume and on/off).
- Check: change the volume, restart, it's kept; `config.xml` still has every other option.

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

## 6. README: installation and config options **[todo]**

The top-level `README.md` is a short Vita placeholder (links to releases, roadmap, build, and the
upstream README in `OPENCLAW_README.md`). Expand it, or add a user-facing `vita/README.md` linked
from it, with:

- Requirements: a Vita on HENkaku/Ensō with VitaShell, or Vita3K; your own copy of `CLAW.REZ`
  (where it comes from, that it isn't included).
- Install: VPK via VitaShell, copy `CLAW.REZ` to `ux0:data/openclaw/`. Vita3K: the equivalent
  folder under its pref path.
- Controls (the `GamepadMapper` table).
- Files in `ux0:data/openclaw/`: `config.xml` (copied from the VPK on first start; delete it to get
  the defaults of a newer version back), `SAVES.XML`, and the log `ux0:data/SDL_Log.txt`.
- Config options that matter on Vita: `Size`/`Scale`, `UseVerticalSync`, `Frequency`,
  `SoundVolume`, `MusicVolume`, `MusicOn`, `TimidityCfg`, `ResourceCacheSize`, `ShowFps`,
  `LogFrameStats`, `FrontPlaneMode`, `SkipMenu`.
- Known limitations (unimplemented menus, no hardware testing yet), credits and licences (OpenClaw
  GPL-3.0, FreePats GPL-2.0-or-later with exception).
- Building it yourself: `vita/build.sh`, `tests/run.sh`.

## 7. CHANGELOG **[todo]**

`vita/CHANGELOG.md`, one section per tag, from the commit history:

| Tag | Commit | Headline |
|---|---|---|
| `vita-vita3k-level1-runs` | `d7ef718` | First build that runs in Vita3K |
| `vita-vita3k-60fps` | `d805a42` | 60 fps, quit fixed, save crash fixed |
| `vita-vita3k-music` | `7e353e4` | Music via Timidity + FreePats |

Plus a "Differences from upstream OpenClaw" section (base: upstream `5ee5740`, July 2022):
- engine fixes that came from the original code: lenient save loading and no longer saving a dying
  Claw's health, menu Quit no longer calling `exit()` from inside an event handler, music tracks
  freed and ordered correctly;
- additions: gamepad support, `<LogFrameStats>`, `<TimidityCfg>`, `<FrontPlaneMode>`, absolute
  `RezArchive` paths;
- Vita specifics: forced GXM renderer with batching, 22,050 Hz mixer, CPU/GPU clocks, `app0:`/`ux0:`
  layout, bundled FreePats;
- removals from 9.

And a "Differences from the original game" section for anything that plays differently from
Captain Claw (1997): today the instrument sound (FreePats instead of a 1997 sound card) and the
missing menus; later difficulty (12) and anything from 13.

## 8. Research the original game's Options submenus **[research]**

What did Captain Claw (1997) put behind Edit Players, Difficulty, Controls and Display (and
Multiplayer, Load Custom Level)?

- Sources: the original manual, the game itself or recordings of it, CLAW.REZ (the menu images
  under `/STATES/MENU/IMAGES/MAIN/OPTIONS/` and any submenu images that OpenClaw never used,
  e.g. difficulty or detail-level graphics), community wikis.
- Deliverable: a section per submenu here, listing its options, what they did in the original, and
  whether each makes sense on Vita. This feeds 4, 11, 12 and 13.

## 9. Strip everything the Vita build doesn't need **[todo]**

Remove other platforms' code, binaries and tooling, so the repo, the build and the code paths only
contain what the Vita uses.

**Decide first: keep the Linux host build or not.** It is what made the profiling, the save-crash
reproduction and the music render tests possible (desktop build in a container, callgrind, gdb).
Recommendation: keep a minimal Linux target for development only, and drop the rest.

Candidates (sizes from the current tree):

| What | Where | Notes |
|---|---|---|
| Windows prebuilt binaries | `Build_Release/*.dll`, `*.lib`, `*.exe`, `msvcr120d.dll`, `Release/` (1.4 MB) | |
| Windows SDL 2.0.4 headers and libs | `ThirdParty/SDL2/` (7.3 MB) | The Vita build already avoids them; the host unit tests use the headers, so they'd switch to the system SDL headers |
| Windows MIDI server | `MidiProc/`, the `_WIN32` music code in `OpenClaw/Engine/Audio/Audio.cpp` | Most of `Audio.cpp`'s `#ifdef`s |
| Launcher | `ClawLauncher/` (6.1 MB, C# WinForms), `Build_Release/clawlauncher`, `ClawLauncher.exe*` | |
| Visual Studio projects | `OpenClaw.sln`, 37 `*.vcxproj*` files | Every new source file currently has to be added to them too |
| Old CI and packaging | `appveyor.yml`, `travis.sh`, `Scripts/` (deb/rpm), `build_and_run.sh` | Replaced by 10 |
| Android | `Android.cmake`, the `ANDROID` branches (`main.cpp`, `MainLoop.cpp`, `BaseGameLogic.cpp`) | Keep the touch code itself if 1 uses it |
| WebAssembly | `__EMSCRIPTEN__` branches (12 files), `Build_Release/openclaw.html` | |
| Desktop configs | `Build_Release/config.xml`, `config_linux*.xml`, the duplicate `Build_Release/MENU.xml` (differs from `ASSETS/MENU.xml`) | Check which one the Vita uses before deleting |
| Unused libraries | `libsigc++3/`, `ThirdParty/sigc++-3.0/` (no `#include` anywhere) | |
| libwap tests | `libwap_tests/` (VS project, Catch) | Keep `Catch.hpp`, used by `tests/` |

- Keep the Vita path identical: after stripping, `vita/build.sh` and `tests/run.sh` must still pass
  and the VPK must contain the same files.
- Do it in small commits by area, so a removal that turns out to matter is easy to revert.
- The upstream README now lives in `OPENCLAW_README.md` (it describes Windows, Linux, Android and
  Emscripten builds). Trim it to project background and credits, or fold that into the README (6).

## 10. CI job that builds the VPK **[todo]**

- `vita/build.sh` already builds in a pinned Docker image (`vitasdk/vitasdk@sha256:8227da1b…`) and
  checks the VPK contents; `tests/run.sh` runs the host unit tests. CI only has to call both and
  keep `build-vita/openclaw.vpk` (plus the unstripped `build-vita/openclaw` ELF for crash dumps) as
  artifacts.
- If 9 keeps the Linux host build, compile it too.
- Open question: where does this repository live from now on? `origin` is still upstream
  `pjasicek/OpenClaw`, so the CI provider (GitHub Actions, GitLab CI, …) depends on where the fork
  gets pushed. Decide before writing the job.
- Network: the build downloads FreePats from deb.debian.org (SHA-256 pinned). Cache it in CI.

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

## 12. Plan difficulty settings **[research]**

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
