# CLAUDE.md

PS Vita port of OpenClaw (C++11 reimplementation of Captain Claw, 1997), based on
`pjasicek/OpenClaw` at upstream commit `5ee5740` (July 2022), imported as this repo's first commit.
Version history: `CHANGELOG.md`.

**Scope is the Vita only.** The Windows, Android and Emscripten builds and their code were removed
(roadmap item 9). The one other build kept is the Linux host build, a development tool for
profiling and reproducing bugs (see "Debugging on the host"); it is not a supported platform.

Read first: `vita/ROADMAP.md` (what's left, numbered items), `vita/PLAN.md` (stage history and
decisions), `vita/PERFORMANCE.md` (measurements and why the renderer is set up as it is).

## Build and test

| Command | What it does |
|---|---|
| `vita/build.sh` | Builds `build-vita/openclaw.vpk` in the pinned `vitasdk/vitasdk@sha256:…` Docker image, then checks the VPK has every required file. `vita/build.sh clean` starts fresh. No local VitaSDK needed. |
| `tests/run.sh` | Host unit tests (Catch 1.5, `tests/Catch.hpp`) into `build-tests/`. Compiles only against the bundled SDL headers, no SDL library. |
| CI | `.github/workflows/build.yml` runs both of the above on every push and pull request and keeps the VPK and ELF as an artifact. |

- The host has no SDL dev packages. Anything that needs real SDL (desktop build, profiling) runs in
  an `ubuntu:22.04` container with `libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev
  libsdl2-ttf-dev libsdl2-gfx-dev`.
- The first Vita build downloads FreePats from deb.debian.org (SHA-256 pinned in
  `vita/Freepats.cmake`) into `build-vita/`.
- `build-vita/openclaw` (unstripped ELF) is needed to symbolize Vita crash dumps
  (`.psp2dmp` → `vita-parse-core`). Keep it alongside any VPK that gets tested.
- Docker containers run as root write root-owned files into mounted dirs; pass `-u 1000:1000` or
  `chown` at the end, or later cleanup fails.

## Layout

| Path | Contents |
|---|---|
| `OpenClaw/` | Game (`Claw*.cpp`, `ActorController.cpp`) and engine (`Engine/`: Actor components, Audio, Events, GameApp, Physics, Resource, Scene, UserInterface, Util) |
| `libwap/` | Readers for the original formats: REZ archive, WWD levels, PID images, PAL, ANI, XMI→MIDI |
| `Box2D/` | Bundled Box2D 2.3.2. Use this one; the SDK's box2d is a different version |
| `ThirdParty/` | TinyXML, FastDelegate, and **old SDL 2.0.4 headers in `ThirdParty/SDL2/`**: the Vita build must not see them (see gotchas); the host unit tests compile against them |
| `Build_Release/` | Runtime files. `ASSETS/` (menus `MENU.xml`, actor prototypes, level metadata) is zipped into `ASSETS.ZIP` by the Vita build (`vita/Assets.cmake` → `vita/tools/make_assets_zip.py`), so edit `ASSETS/`, not the zip. The game's zip reader needs sizes in the local headers: don't switch to `cmake -E tar` or other streaming zip writers; `SAVES.XML` template, `clacon.ttf`, `console02.tga` |
| `vita/` | `Vita.cmake` (VPK packaging), `Freepats.cmake`, `config.xml` (Vita defaults), `timidity.cfg`, `sce_sys/` LiveArea, `tools/`, docs |
| `tests/` | Host unit tests (`GamepadMapperTests.cpp`) |

Every new source file must be added to its directory's `CMakeLists.txt` (`target_sources`).

## Code conventions

- Match the surrounding code: 4-space indent, braces on their own line, `m_` members, `m_p`
  pointers, `m_b` bools, `g_` globals, `V` prefix on virtuals (`VOnUpdate`), `LOG` / `LOG_WARNING` /
  `LOG_ERROR` macros, `SAFE_DELETE`. Systems talk through events
  (`IEventMgr::Get()->VTriggerEvent` / `VQueueEvent`).
- Platform code goes behind `#ifdef __vita__` (the compiler's macro).
- **Line endings:** about two thirds of `OpenClaw/` and `libwap/` sources are CRLF, the rest LF;
  preserve each file's style. After editing, check
  that a CRLF file has no LF-only lines and that `git diff --stat` shows only the lines you meant;
  a whole-file diff means the endings got converted.

## Vita runtime facts (each cost a debugging round to learn)

- **Files:** `app0:` (the installed VPK) is read-only and is the working directory, so relative
  paths resolve there. Writable data lives in `ux0:/data/openclaw/`. On first start
  `MainLoop.cpp` copies `config.xml` and `SAVES.XML` there from `app0:`; later starts use the
  `ux0:` copies. **Changing `vita/config.xml` only reaches existing installs if the user deletes
  `ux0:data/openclaw/config.xml`** — say so whenever defaults change.
- `CLAW.REZ` (original game data, user-supplied) is at `ux0:/data/openclaw/CLAW.REZ`;
  `<RezArchive>` may be an absolute path, other assets resolve against `<AssetsFolder>app0:/`.
- **Logging:** `SDL_Log` on Vita appends to `ux0:data/SDL_Log.txt`, opening and closing the file
  per line. Delete it before a test run. Avoid per-frame logging.
- **Rendering:** `InitializeDisplay` forces SDL's `VITA gxm` driver (otherwise SDL picks GLES2 on
  vitaGL, which needs `libshacccg.suprx` on the device). Naming a driver makes SDL turn draw
  batching **off**, so `SDL_HINT_RENDER_BATCHING=1` is set explicitly. Without it the game ran at
  7–24 fps instead of 60.
- **Audio:** mixer at 22,050 Hz mono (the game's sounds are 22 kHz; 44.1 kHz made SDL resample
  every sound on first use, causing stalls). Music: XMI tracks → MIDI → SDL_mixer's Timidity with
  FreePats from `app0:/freepats/timidity.cfg` (`<TimidityCfg>`). `vita/timidity.cfg` holds only
  the instruments Claw's music uses; regenerate with `vita/tools/make_timidity_cfg.py`. Music
  loads on a thread per track; `Audio.cpp` makes only the latest request play and frees the
  replaced track.
- **Shutdown:** on Vita `Terminate()` returns early and `main()` ends the process with
  `sceKernelExitProcess` (the full teardown hung). Quit goes through `QuitGameDelegate`, which only
  stops the main loop.
- **Memory/clocks:** `main.cpp` sets `_newlib_heap_size_user` (128 MB) and
  `sceUserMainThreadStackSize` (1 MB), and raises CPU to 444 MHz / GPU to 222 MHz.
- **Input:** `GamepadMapper` turns controller buttons and the left stick into the key events the
  game already handles (`BaseGameApp::OnEvent`). Gameplay reacts to key *events*; nothing in
  gameplay polls `SDL_GetKeyboardState`.
- **Linking** is static with an explicit library list in the root `CMakeLists.txt`: the SDK's CMake
  configs and pkg-config files leave out zlib, libpng, `SceShaccCgExt` and pthread.
- **`vita-elf-create: … segment 1 overlaps`** means the code ended just short of a 64 KiB boundary,
  leaving no room for the module data `vita-elf-create` appends. Fixed for good by
  `-Wl,--defsym=__sce_headroom=0x2000` on the Vita link; don't remove it.

## Known gaps in the engine (not Vita bugs)

- Options → Edit Players, Difficulty, Controls, Display, plus Multiplayer and Load Custom Level,
  point at menu pages that don't exist (`SwitchPageDelegate` logs "Probably not implemented yet").
  No difficulty logic exists anywhere.
- Settings saving: `BaseGameApp::SaveGameOptions` writes only the options the menus change (today:
  `Audio` volumes and on/off) into the loaded `config.xml` document, through `config.xml.tmp` and
  remove-then-rename (the Vita can't rename over an existing file). Call it right after a change,
  not on exit: the Vita build skips `Terminate()`. New menu settings must be added there.
  TinyXML reformats the file and trims trailing spaces in values when it writes it.
- Startup log noise that is expected: `[MEMORY_PROFILER] … not supported`, `Unknown ammo type`,
  `OnEnemyLeftAgroRange … no such actor`.

## Debugging and measuring

- **Vita3K** (emulator) is the first test target; real hardware is the final word. Its data folder
  on Linux is `~/.local/share/Vita3K/Vita3K/` by default (`ux0/app/OCLW00001` = installed game,
  `ux0/data/openclaw/` = CLAW.REZ, config, saves). Vita3K timing is not hardware timing.
- **`<LogFrameStats>`** (on in the Vita config) logs every 2 s:
  `fps | frame (worst) | update | physics | render | present | draws`. `update` = game logic incl.
  physics; `render` = walking the scene and queueing draws; `present` = SDL submitting draws plus
  waiting for vsync. High `render` with tiny `present` meant batching was off.
- `<FrontPlaneMode>` normal/skip/tiny: experiment switch for the foreground tile plane.
- Level 1 actor and tile counts, sound formats and the instruments the music uses were found by
  small programs linking `libwap` against `CLAW.REZ`; that's the quickest way to answer
  "what's in the game data" questions.

### Debugging on the host

Build the desktop target in the Ubuntu container (`cmake -S . -B build && make`; it outputs to
`../Build_Release` relative to the build dir) and run it with:
- `xvfb-run -a` (SDL's `dummy` video driver has no accelerated renderer, and the game asks for one)
  and `SDL_AUDIODRIVER=dummy`;
- `ASSETS.ZIP` from `build-vita/ASSETS.ZIP` (or `python3 -I vita/tools/make_assets_zip.py
  Build_Release/ASSETS <out>`), plus `SAVES.XML`, `clacon.ttf` and `console02.tga` from
  `Build_Release/`;
- a `config.xml` derived from `vita/config.xml` with `AssetsFolder` empty, `RezArchive` pointing at
  `CLAW.REZ`, `TimidityCfg` removed (or pointing at a FreePats folder), vsync off, and
  `<SkipMenu>true</SkipMenu>` to go straight into level 1;
- screenshots for checking menus: run under a plain `Xvfb :N -screen 0 960x544x24`, send keys with
  `xdotool key Down`, capture with ImageMagick `import -window root shot.png`;
- `startup_commands.txt` in the working directory for console commands at level start, e.g.
  `teleport 689 5400` (the foreground-column area of level 1);
- callgrind with `--instr-atstart=no` and `callgrind_control -i on` after loading, to profile
  gameplay only; gdb for hangs. A SIGTERM reaches the game as `SDL_QUIT`.

## Repo, releases, licence

- Remotes: `github` = `Mezeskarika/openclaw-vita` (branch `main`, where work goes), `origin` =
  upstream `pjasicek/OpenClaw` (reference only, don't push).
- History starts with one import commit holding upstream's tree at `5ee5740`; upstream's own
  history isn't in this repo. To compare with upstream, `git fetch origin` and diff against
  `5ee5740`. Don't fetch upstream's tags into this repo (`v0.0`, `v0.2`, `v0.3` would clash with
  ours).
- **Commit freely, but never push without explicit approval.** That covers every write to GitHub:
  `git push` (branches, tags, deletions, force-pushes) and `gh release create/edit/delete`.
  Commit locally, then say what is ready to push and ask; approval for one push doesn't carry
  over to the next. The maintainer may also push by hand.
- Commits use the GitHub no-reply address configured in the repo's local git config. Never
  commit or tag with a work email; check `git log --format='%ae %ce'` before pushing.
- Licence GPL-3.0 (`LICENSE.txt`). FreePats: GPL-2.0-or-later with an exception; its `COPYING`
  and `README` ship in the VPK.
- **Never add Captain Claw data** to the repo or VPK: `CLAW.REZ`, anything extracted or rendered
  from it (music renders, sprites), Monolith logos/art. That includes LiveArea artwork.

## Versioning and releases

- Semantic versioning, tags `vX.Y.Z` (annotated; message = version, then what was verified and
  where). `0.x` while untested on real hardware; `1.0.0` once it plays through on a Vita.
  A new feature or behaviour change bumps the minor version, a fix-only release the patch version.
- Only tag builds that were tested (Vita3K or hardware), and record where in the tag message.
- `CHANGELOG.md` (Keep a Changelog): add every user-visible change under `[Unreleased]` in the same
  commit as the change. At release, rename `[Unreleased]` to the version and date, update the
  compare links at the bottom, and keep the "Differences from upstream OpenClaw" / "Differences
  from the original game" sections current.
- The VPK's own version (`VITA_VERSION` in `vita/Vita.cmake`, format `MM.mm`, shown by the Vita)
  is `01.00` in 0.1.0–0.3.0. From the next release on, set it in the release commit as
  major/minor/patch → `0M.mp`, e.g. 0.4.0 → `00.40`, 1.2.3 → `01.23`.
- Release procedure:
  1. Changelog and `VITA_VERSION` updated and committed; annotated tag created.
  2. Build from a clean `git archive <tag>` export with that export's `vita/build.sh`, and compare
     the VPK contents with the build that was tested.
  3. After approval (see above): push `main` and the tag **by name** (never `git push --tags`).
  4. After approval: copy the VPK to `OpenClaw-Vita-X.Y.Z.vpk` and
     `gh release create <tag> OpenClaw-Vita-X.Y.Z.vpk --prerelease --title "OpenClaw for PS Vita X.Y.Z"
     --notes-file …` (pre-release until tested on hardware). The VPK is the only asset to attach.
     Notes start with a bold download link to the VPK
     (`…/releases/download/<tag>/OpenClaw-Vita-X.Y.Z.vpk`) and a sentence saying the *Source code*
     archives are added by GitHub automatically and aren't needed to play (GitHub always adds them;
     they can't be removed). Then only what is specific to this version: a pre-release note, what's
     new (from the changelog), and issues that a later version fixes. End with one line linking to
     the README for requirements, installation, controls, known limitations and licence. Don't
     repeat those in release notes; they live in `README.md`. The release page orders by creation,
     so create releases in version order.
  5. The installed `gh` (2.4) has no `release edit`/`delete-asset`: use
     `gh api -X PATCH repos/<owner>/<repo>/releases/<id> -F body=@notes.md` and
     `gh api -X DELETE repos/<owner>/<repo>/releases/assets/<id>`.
