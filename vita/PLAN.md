# OpenClaw → PS Vita: staged plan

The goal is a build that can be installed and checked on a Vita (or in an emulator) as early as possible.
Each stage ends in something testable. Music comes last.

Status legend: **[branch]** = implemented on branch `worktree-vita-port-build-setup`. The VPK builds and the host unit tests pass, but nothing has run on Vita3K or hardware yet.
**[todo]** = not started.

---

## Resources at a glance

| Resource | What it is / where to get it | Needed from stage |
|---|---|---|
| Docker | Runs the VitaSDK toolchain without installing it on the host | 0 |
| VitaSDK image | `vitasdk/vitasdk@sha256:8227da1b…` (pinned in `vita/build.sh`). Ships GCC 15.2, SDL 2.32.8, SDL2_image/mixer/ttf/gfx, so nothing has to be built from source | 0 |
| `CLAW.REZ` | From your own copy of the original Captain Claw (1997). Not in the repo, must never go into the VPK | 1 |
| A Vita that runs homebrew | HENkaku/Ensō + VitaShell (to install VPKs and copy files over FTP/USB) | 1 (or Vita3K first) |
| Vita3K (optional, before hardware) | Emulator, Linux AppImage. Needs the official firmware file `PSVUPDAT.PUP` from Sony's update site (plus optionally the font PUP) | 1 |
| vitacompanion (optional) | Vita plugin: FTP on :1337 and a command port on :1338 (`launch`, `destroy`). Makes the device loop scriptable | 1 |
| Host SDL2 dev libs | `libsdl2-dev`, `libsdl2-image-dev`, `libsdl2-mixer-dev`, `libsdl2-ttf-dev`, `libsdl2-gfx-dev` (or a Docker Ubuntu image). Only for the host smoke test | testing |
| `libshacccg.suprx` | Vita runtime shader compiler. **Not needed** as long as SDL uses its native `VITA gxm` renderer (forced in code). Only needed if we ever fall back to SDL's GLES2/vitaGL renderer | — |
| `vita-parse-core` | Turns a Vita crash dump (`.psp2dmp`) into a symbolized backtrace. Python tool from the VitaSDK project | 1 |
| Timidity patches (e.g. freepats, ~30 MB) **or** offline MIDI→OGG converter (timidity/fluidsynth on PC) | For music | 6 |

---

## Stage 0: Toolchain and build skeleton **[branch]**

**Goal:** `vita/build.sh` produces `openclaw.vpk` in a clean checkout, with no Vita and no game data.

Changes:
- `CMakeLists.txt`: `-DVita=ON` option. It picks up the VitaSDK toolchain, links the SDK's static SDL2 libraries, and packages `eboot.bin` + LiveArea + bundled files into a VPK (`vita/Vita.cmake`).
- The static link list is spelled out by hand. The SDK's SDL CMake configs and pkg-config files both leave out zlib, libpng, `SceShaccCgExt` and pthread, and the link fails without them.
- Header fix: `ThirdParty/SDL2/` contains Windows SDL **2.0.4** headers. With `include_directories(./ThirdParty/)` they shadow the SDK's SDL 2.32 headers and the Vita build fails with 113 errors in `SDL_stdinc.h`. The Vita build stages only the `Tinyxml/` and `FastDelegate/` headers into the build tree and uses that instead. Other platforms are untouched.
- SDL2_gfx stays: the SDK ships it, so `PhysicsDebugDrawer.cpp` compiles unchanged. (This corrects `assessment.md`, which suggested compiling it out.)
- `vita/sce_sys/`: placeholder icon and LiveArea images (8-bit indexed PNGs, which the Vita requires). Title ID `OCLW00001`.
- `vita/build.sh`: Docker wrapper. Output in `build-vita/` (git-ignored).

Resources: Docker, VitaSDK image.
Verification (automated): the build succeeds and `unzip -l build-vita/openclaw.vpk` lists `eboot.bin`, `sce_sys/param.sfo`, the LiveArea files and the bundled assets.

## Stage 1: Boot to the main menu **[branch]**

**Goal:** the app starts, shows the menu, and the menu can be navigated with the Vita's buttons. Music is off.

Changes:
- File layout on the Vita:
  - `app0:` (read-only, inside the VPK): `config.xml` (Vita defaults), `SAVES.XML` (empty template), `ASSETS.ZIP`, `clacon.ttf`, `console02.tga`.
  - `ux0:data/openclaw/` (writable): `CLAW.REZ` (copied there by the user), plus `config.xml` and `SAVES.XML`, which are copied from `app0:` on first start. A missing `SAVES.XML` is fatal in `BaseGameLogic::Initialize`, so the copy is required.
- `MainLoop.cpp`: `__vita__` branch for the user directory. It skips the "config.xml in working directory" hack, because the working directory on Vita is the read-only `app0:`.
- `BaseGameApp.cpp`: `RezArchive` may now be an absolute path. On Vita the config uses `AssetsFolder=app0:/` and `RezArchive=ux0:/data/openclaw/CLAW.REZ`.
- Renderer: forces SDL's `VITA gxm` driver. The SDK's SDL would otherwise pick GLES2 through vitaGL, which needs `libshacccg.suprx` on the device. The available drivers and the chosen one are logged.
- `main.cpp`: explicit newlib heap (128 MB; read by newlib's `sbrk`) and main-thread stack (1 MB; read by `vita-elf-create`).
- `vita/config.xml`: 960×544, vsync on, resource cache 50 MB, sound effects on, **music off**.

Resources: `CLAW.REZ`; Vita3K + firmware PUP and/or a Vita with VitaShell.
Verification:
- Delete `ux0:data/SDL_Log.txt` before each run; SDL only ever appends to it.
- The log contains `Renderer: VITA gxm`, `Resource cache successfully initialized` and `Display successfully initialized`, and no `ERROR` lines except one expected `Mix_LoadMUS_RW` error per music track. With `MusicOn=false` the engine still loads each track (paused, so music can be switched on in the menu), and without a Timidity config that load fails. Stage 6 removes it.
- The menu renders at full screen and responds to the D-pad and Cross.
- On a crash the Vita writes a `.psp2dmp` file to `ux0:data/`. `vita-parse-core` symbolizes it against the unstripped `build-vita/openclaw` ELF, so keep that ELF next to every VPK you test.

Not yet confirmed on a device: that the `app0:/` and `ux0:/data/...` path forms open correctly, that `VITA gxm` is the exact driver name, and that SDL has a controller mapping for the built-in controls (the log says so either way).

Open questions to settle here: does 960×544 at `Scale` 1 look right, or is 480×272 at `Scale` 2 closer to the original's framing? (The game was designed for 640×480.)

## Stage 2: Playable level 1 **[branch: input mapping]**

**Goal:** level 1 can be completed with the buttons, and a save is written to `ux0:`.

Changes:
- `SDL_INIT_GAMECONTROLLER` is now part of `SDL_Init` on all platforms, so the host test can use the same path. Side effect: on desktop, a connected gamepad now also drives the game. The log lists every joystick and whether SDL has a controller mapping for it; without a mapping no input arrives.
- `Engine/UserInterface/GamepadMapper.{h,cpp}`: turns controller events into the key events the game already handles, so gameplay, menus, the in-game menu and the end-of-level screen all work without touching them. Gameplay reads key *events* (`ActorController::VOnKeyDown`), so synthesized events are enough. Nothing in gameplay polls `SDL_GetKeyboardState` (only the debug free camera does).
- The developer console opens on the grave key (`~`), which no button produces, so it is effectively disabled on Vita.

Default mapping:

| Vita | Key sent | Action |
|---|---|---|
| D-pad / left stick | Arrow keys | Move, climb, menu navigation |
| Cross | Space | Jump, confirm in menus |
| Square | Left Ctrl | Attack |
| Circle | Left Alt | Fire secondary weapon |
| Triangle | Left Shift | Switch ammo type |
| Start | Escape | In-game menu / back |
| Select | Escape | Back |

Resources: `CLAW.REZ`, hardware strongly preferred (Vita3K input is via host keyboard).
Verification: play level 1 to the end; pause/resume; check that `ux0:data/openclaw/SAVES.XML` changes after a checkpoint save.

## Stage 3: Performance and memory **[in progress]**

**Goal:** a steady frame rate on level 1 and the busiest later levels. At 60 Hz with vsync the even rates are 60/30/20 fps, so the realistic target is a locked 30, with 20 as the floor.

Stages 1 and 2 are confirmed in Vita3K (tag `vita-vita3k-level1-runs`): the game boots and level 1 is playable, at 7–24 fps.

Done:
- Quit freeze (not yet re-tested on Vita): the quit event now only stops the main loop, instead of tearing everything down and calling `exit()` from inside an event callback. On Vita the teardown is skipped and the process ends with `sceKernelExitProcess`. On desktop the teardown stops audio before freeing sounds and shuts SDL down; checked to exit cleanly on the host.
- `<FrontPlaneMode>` debug switch (normal / skip / tiny) to find out why the foreground columns are so slow in Vita3K.
- The log shows the rate the audio device actually opened at.
- Startup crash after a checkpoint save: Claw can touch a checkpoint while dying, which saved health -1 as `4294967295`; `std::stoi` threw on it and the game aborted on every later start (all platforms). Saves are now parsed leniently, a non-positive health loads as full health, and it is no longer saved.
- SDL draw batching re-enabled. Naming the `VITA gxm` driver had silently turned it off, so every draw went to the GPU separately (see `vita/PERFORMANCE.md`).

Result in Vita3K: a locked 60 fps throughout level 1, columns included (was 7–24 fps). Remaining for this stage: the same measurement on real hardware.
- `<LogFrameStats>`: fps plus update/physics/render/present times and draw count, logged every 2 s.
- Mixer at 22,050 Hz (the game's sounds are 22 kHz; resampling them stalled the game).
- CPU 444 MHz / GPU 222 MHz on hardware.

Measurements, analysis and the ranked list of next steps: `vita/PERFORMANCE.md`.

Still open from the original list:
- Tune `ResourceCacheSize`, the newlib heap size and, if needed, the `ATTRIBUTE2=12` param.sfo flag for extended memory mode.
- Compiler flags: `-O2 -g` today, Cortex-A9/NEON/Thumb-2 confirmed as the toolchain defaults. Try `-O3`/LTO.

Resources: hardware (Vita3K timing is not representative).

## Stage 4: Display and UX polish **[todo]**

- Final choice of resolution/scale; check HUD and menu layout at 960×544.
- Front touchscreen: enable the existing Android touch controls (`Engine/UserInterface/Touch/`, `<TouchScreen>` in config) as an option.
- Remappable buttons in `config.xml`.
- Suspend/resume behaviour (the main loop already ignores frames >1 s after a pause).

## Stage 5: Sound effects **[todo]**

Sound effects (WAV via `Mix_Chunk`) are on from stage 1 because the engine requires audio to initialize. This stage verifies them properly: channel count, latency (`ChunkSize`), volume, crackling. If they cause trouble earlier, set `<SoundOn>false</SoundOn>` in `ux0:data/openclaw/config.xml`.

## Stage 6: Music **[todo]**

The game's music is XMI, converted to MIDI at runtime and played with `Mix_LoadMUS_RW` on a separate thread (`Audio.cpp:157-185`).

- **Option A: Timidity on the device.** The SDK's SDL2_mixer is built with `SDL2MIXER_MIDI_TIMIDITY=ON` (checked in the vitasdk `sdl2_mixer` VITABUILD), so this is code-free apart from shipping a `timidity.cfg` + patches to `ux0:data/openclaw/` and calling `Mix_SetTimidityCfg`. Risk: CPU cost on the A9.
- **Option B: pre-rendered OGG.** Convert all XMI tracks to OGG on a PC and load those on Vita instead. More work up front, cheap at runtime.

Recommendation: try A first, measure CPU, fall back to B.
Resources: freepats (A) or timidity/fluidsynth + a soundfont on the PC (B).

## Stage 7: Release packaging **[todo]**

Real LiveArea art, version number, a short user README (where to put `CLAW.REZ`), and a CI job that builds the VPK.

---

## Automated testing options

Ordered from cheapest to most realistic. Layers A and B are set up on this branch.

### A. Build gate: no game data, no device **[branch]**
`vita/build.sh` compiles in the pinned VitaSDK container and checks the VPK contents. This catches compiler/linker breakage on every change and can run in CI as-is (GitHub Actions supports the same image).
The desktop Linux build should be gated the same way, since the Vita changes touch shared engine code. It was checked by hand in an `ubuntu:22.04` container with the `libsdl2-*-dev` packages; a script for it is a candidate for CI.

### B. Host unit tests: no game data, no SDL runtime **[branch]**
`tests/` builds with the host compiler and the bundled SDL headers only (no SDL library, because the code under test makes no SDL calls). It uses the Catch framework already in `libwap_tests/`.
Covered so far: `GamepadMapper` (button → key, stick thresholds, D-pad/stick overlap, release ordering), 167 assertions.
Candidates: path resolution for `RezArchive`, config defaults.
Run: `tests/run.sh`.

### C. Host smoke test: needs `CLAW.REZ` and host SDL libs **[todo]**
Run the normal Linux build with the Vita config values under `xvfb-run` and `SDL_AUDIODRIVER=dummy`.
- SDL's `dummy` video driver has no accelerated renderer, and the code asks for one, so a virtual X server is needed.
- Needs a small debug option, e.g. `<QuitAfterMs>`, plus `SkipMenu=true` to load level 1 and exit by itself. Pass if exit code is 0 and the log has no `ERROR` lines.
- Input can be scripted through SDL's virtual joystick API (`SDL_JoystickAttachVirtual`, SDL ≥ 2.0.14), which goes through the same `GamepadMapper` path as on Vita.
- `startup_commands.txt` (console commands at start, e.g. `teleport x y`, `Invincible on`) can place Claw at specific spots for repeatable checks.

### D. Emulator smoke test: Vita3K, needs `CLAW.REZ` + firmware PUP **[todo]**
- One-time: `Vita3K --firmware PSVUPDAT.PUP`.
- Per run: `Vita3K openclaw.vpk` installs and starts it; later runs use `Vita3K --frontend sdl -r OCLW00001`. Put `CLAW.REZ` into Vita3K's `ux0/data/openclaw/`.
- Under `xvfb-run` with a timeout, then grep `ux0/data/SDL_Log.txt` as in stage 1.
- Input could be driven by `xdotool` key presses (Vita3K maps the keyboard to buttons), but that is brittle. Use it for boot/level-load checks, not gameplay.
- Caveats: Vita3K's compatibility with SDL homebrew is not guaranteed; its timing says nothing about real performance.

### E. On-device loop: semi-automated **[todo]**
With vitacompanion: upload `eboot.bin` over FTP, `launch OCLW00001` on port 1338, wait, fetch `ux0:data/SDL_Log.txt` over FTP, grep it. Gameplay, input feel and performance stay manual, using a short checklist per stage.

---

## How to build (stage 0)

```sh
vita/build.sh            # builds build-vita/openclaw.vpk inside Docker
tests/run.sh             # host unit tests
```

Install `build-vita/openclaw.vpk` with VitaShell, copy `CLAW.REZ` to `ux0:data/openclaw/`, start the app. Logs: `ux0:data/SDL_Log.txt`.
