# OpenClaw Vita port: roadmap

What is still to do, as of 2026-10-28. `PLAN.md` is the stage-by-stage history of the port so far;
this file is the work list from here on. Each item says what "done" means and where in the code it
starts.

Status: **[todo]** not started, **[research]** needs findings before any code, **[blocked]** waits on
another item.

Current state (tag `vita-vita3k-music`): the game boots in Vita3K, runs at 60 fps, plays music,
saves and quits cleanly. Nothing has run on real hardware yet.

## Overview

| # | Item | Kind | Depends on |
|---|---|---|---|
| 1 | Navigate menus with touch | code | — |
| 2 | Disable unimplemented menu items | menu data | — |
| 3 | Remappable buttons | code + menu | 7 (page layout) |
| 4 | Real LiveArea artwork | art | — |
| 5 | README: installation and config options | docs | — |
| 6 | CHANGELOG per tagged version, incl. divergences | docs | — |
| 7 | Research the original game's Options submenus | research | — |
| 8 | CI job that builds the VPK | infra | — |
| 9 | Research FOV / resolution settings for the Display menu | research | 7 |
| 10 | Plan difficulty settings (easy to hard options) | research | 7 |
| 11 | Implement the disabled Options submenus | code + menu | 3, 7, 9, 10 |

Suggested order: 2 and 5 first (small, visible), then 7 (it feeds 3, 9, 10, 11), then 1, 3, 9, 10, 11.
4, 6 and 8 can happen any time.

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
- Make sure up/down navigation skips disabled items.
- Applies to desktop too, since `MENU.xml` is shared.
- Check: the items look disabled, can't be selected, and the log has no `Could not switch to page`.

## 3. Remappable buttons **[todo]** (UI blocked on 7)

- The Vita mapping is one table: `GamepadMapper::GetButtonBinding`
  (`OpenClaw/Engine/UserInterface/GamepadMapper.cpp`). Step 1: read it from `config.xml`
  (e.g. `<ControlOptions><Gamepad><Jump>A</Jump>…`), keeping today's mapping as the default.
  The host unit tests (`tests/GamepadMapperTests.cpp`) cover the table and need extending.
- Step 2: an in-game remapping page (Options → Controls): list the actions, select one, press the
  new button, reject conflicts, "reset to defaults". This needs the menu engine to wait for a
  button press, which it can't do today.
- The game doesn't write `config.xml` yet (`BaseGameApp::SaveGameOptions` logs "Not implemented
  yet!"), so saving settings has to be built first. That also serves 11.
- Check: remap jump, restart, the new button jumps; defaults restore the original mapping.

## 4. Real LiveArea artwork **[todo]**

- Replace the placeholders from `vita/make_livearea.py` in `vita/sce_sys/`: `icon0.png` 128×128,
  `livearea/contents/bg.png` 840×500, `livearea/contents/startup.png` 280×158. All must be 8-bit
  indexed PNGs, or the Vita refuses to install the VPK.
- Licence: the VPK must not contain Monolith's artwork (logo, Claw sprites from CLAW.REZ). Use
  original art, or art whose licence allows redistribution, and record the source and licence next
  to the files.
- Optional: `pic0.png` (960×544) for the home screen.
- Check: install on Vita3K and hardware, icon and LiveArea display correctly.

## 5. README: installation and config options **[todo]**

A user-facing `vita/README.md`:

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

## 6. CHANGELOG **[todo]**

`vita/CHANGELOG.md`, one section per tag, from the commit history:

| Tag | Commit | Headline |
|---|---|---|
| `vita-vita3k-level1-runs` | `d7ef718` | First build that runs in Vita3K |
| `vita-vita3k-60fps` | `d805a42` | 60 fps, quit fixed, save crash fixed |
| `vita-vita3k-music` | `7e353e4` | Music via Timidity + FreePats |

Plus a "Differences from upstream OpenClaw" section (base: upstream `5ee5740`, July 2022), split
into Vita-only changes and changes that also affect desktop builds, e.g.:
- desktop too: gamepad support (`SDL_INIT_GAMECONTROLLER`), lenient save loading and no longer
  saving a dying Claw's health, menu Quit no longer calls `exit()` from inside an event handler,
  music tracks freed and ordered correctly, `<LogFrameStats>`, `<TimidityCfg>`, `<FrontPlaneMode>`,
  absolute `RezArchive` paths;
- Vita only: forced GXM renderer with batching, 22,050 Hz mixer, CPU/GPU clocks, `app0:`/`ux0:`
  layout, bundled FreePats.

And a "Differences from the original game" section for anything that plays differently from
Captain Claw (1997): today the instrument sound (FreePats instead of a 1997 sound card) and the
missing menus; later difficulty (10) and anything from 11.

## 7. Research the original game's Options submenus **[research]**

What did Captain Claw (1997) put behind Edit Players, Difficulty, Controls and Display (and
Multiplayer, Load Custom Level)?

- Sources: the original manual, the game itself or recordings of it, CLAW.REZ (the menu images
  under `/STATES/MENU/IMAGES/MAIN/OPTIONS/` and any submenu images that OpenClaw never used,
  e.g. difficulty or detail-level graphics), community wikis.
- Deliverable: a section per submenu here, listing its options, what they did in the original, and
  whether each makes sense on Vita. This feeds 3, 9, 10 and 11.

## 8. CI job that builds the VPK **[todo]**

- `vita/build.sh` already builds in a pinned Docker image (`vitasdk/vitasdk@sha256:8227da1b…`) and
  checks the VPK contents; `tests/run.sh` runs the host unit tests. CI only has to call both and
  keep `build-vita/openclaw.vpk` (plus the unstripped `build-vita/openclaw` ELF for crash dumps) as
  artifacts.
- Also compile the desktop Linux build (Ubuntu with the `libsdl2-*-dev` packages), since the Vita
  changes touch shared code.
- Open question: where does this repository live from now on? `origin` is still upstream
  `pjasicek/OpenClaw`, so the CI provider (GitHub Actions, GitLab CI, …) depends on where the fork
  gets pushed. Decide before writing the job.
- Network: the build downloads FreePats from deb.debian.org (SHA-256 pinned). Cache it in CI.

## 9. Research FOV and resolution for the Display menu **[research]**

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
  what each does on Vita, and the implementation sketch for 11.

## 10. Plan difficulty settings **[research]**

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
remember its difficulty?). Recommendation and the Difficulty page layout go into 11.

## 11. Implement the disabled Options submenus **[blocked on 3, 7, 9, 10]**

Using the findings from 7, 9 and 10, build the pages that make sense on Vita and re-enable their
buttons (undoing 2 for those):

- **Controls:** the remapping page from 3.
- **Difficulty:** the option chosen in 10.
- **Display:** the options chosen in 9.
- **Edit Players:** only if 7 shows it is worth it (player profiles = separate save files).

Shared prerequisites: saving settings to `ux0:data/openclaw/config.xml` (see 3), and menu pages
that change a value in place (left/right on a setting), as the Audio page already does for volume.
Each page needs a Vita3K check, and the CHANGELOG (6) entry for it.
