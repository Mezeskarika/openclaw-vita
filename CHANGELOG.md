# Changelog

All notable changes to the PS Vita port. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/) (see "Versioning" in `CLAUDE.md`).

Versions before 1.0.0 have only been tested in the Vita3K emulator, not on real hardware.
The base is OpenClaw at [pjasicek/OpenClaw@5ee5740](https://github.com/pjasicek/OpenClaw/tree/5ee5740b3c8da31ea04fda90ea1d45ed12f4c7c5)
(July 2022), imported as this repository's first commit.

## [Unreleased]

### Added
- `vita/ROADMAP.md`: the remaining work, numbered.
- `CLAUDE.md`: build commands, conventions and the Vita findings so far.
- This changelog and version numbers (`v0.1.0`–`v0.3.0` replace the earlier `vita-vita3k-*` tags).

### Changed
- The README describes the Vita port and links to the original OpenClaw project instead of
  carrying its README. It now holds the requirements, installation, updating, controls and known
  limitations, which the release notes link to instead of repeating.
- Release files are named `OpenClaw-Vita-X.Y.Z.vpk`.
- Repository history: upstream OpenClaw's 573 commits are replaced by a single import commit;
  the upstream history stays available at pjasicek/OpenClaw.
- Menu items that lead nowhere are shown greyed out and can't be selected: Multiplayer,
  Load Custom Level, and Options → Edit Players, Difficulty, Controls, Display.

- GitHub Actions workflow that builds the VPK and runs the unit tests on every push.
- Menus can be used by tapping the front touchscreen.
- Button mapping can be changed in `config.xml` (`<ControlOptions><Gamepad>`), including the
  L and R buttons.
- Sound and music volume and on/off, changed on the Audio page, are saved to `config.xml`
  immediately and kept across restarts.
- On Vita, an unreadable `config.xml` is replaced with the Vita defaults from the VPK instead of
  desktop defaults.

### Removed
- Everything the Vita build doesn't use: Windows, Android and Emscripten builds and code paths,
  Visual Studio and Eclipse projects, old CI and Linux packaging, the Windows launcher and MIDI
  server, prebuilt Windows binaries, unused libraries. A Linux build remains as a development tool.

### Fixed
- The Vita build could fail in `vita-elf-create` ("segment 1 overlaps") depending on code size.

## [0.3.0] - 2026-10-28

Music. Checked in Vita3K: plays in levels, no stutter, still 60 fps.

### Added
- Music: the game's 50 MIDI tracks (from `CLAW.REZ`) played by SDL_mixer's Timidity with the
  FreePats instruments, bundled in the VPK under `app0:/freepats/` with their licence files. Only
  the 49 instruments the music uses are shipped (15 MB).
- `<TimidityCfg>` config option.

### Changed
- Music on by default, volume 15 of 20.

### Fixed
- Each music track change leaked the previous track, including every instrument loaded for it.
- Two tracks loading at the same time could finish in the wrong order, and a track still loading
  could start playing after the game had stopped the music.

## [0.2.0] - 2026-10-28

Speed and stability. Level 1 runs at 60 fps in Vita3K throughout, foreground columns included
(was 7–24 fps).

### Added
- `<LogFrameStats>`: fps and a per-frame time breakdown (update, physics, render, present, draws)
  in the log every 2 seconds.
- `<FrontPlaneMode>` debug switch for the foreground tile plane.
- The log records the rate the audio device actually opened at.

### Changed
- Sound mixed at 22,050 Hz, the rate of nearly all of the game's sounds: removes a stall each time
  a sound played for the first time.
- CPU at 444 MHz and GPU at 222 MHz (Vita default for apps: 333 MHz CPU).

### Fixed
- SDL draw batching was off (a side effect of choosing the renderer by name), so every tile and
  sprite went to the GPU on its own. This was the cause of the low frame rate.
- Quitting from the menu froze the game.
- The game aborted on every start once a checkpoint had been saved while Claw was dying (his health
  was saved as -1 and couldn't be read back). Saves are now read leniently and a dying Claw's
  health is no longer saved.

## [0.1.0] - 2026-10-28

First build that runs: boots to the menu and plays level 1 in Vita3K, at 7–24 fps, without music.

### Added
- PS Vita build: `-DVita=ON` CMake option, `vita/build.sh` (builds the VPK in a pinned VitaSDK
  Docker image), placeholder LiveArea images.
- Vita file layout: bundled files read from `app0:`, `CLAW.REZ` and the writable `config.xml` and
  `SAVES.XML` in `ux0:data/openclaw/`, the latter two copied there on first start.
- Gamepad controls: the Vita's buttons and left stick act as the keys the game already uses
  (Cross jump, Square attack, Circle secondary weapon, Triangle switch ammo, Start/Select menu).
- Host unit tests for the gamepad mapping (`tests/`).
- `vita/PLAN.md`: the staged plan for the port.

### Known issues
- Quitting from the menu freezes (fixed in 0.2.0).
- 7–24 fps (fixed in 0.2.0).

---

## Differences from upstream OpenClaw

Changes in this port compared with pjasicek/OpenClaw@5ee5740, besides the Vita build itself:

- **Bug fixes in OpenClaw's own code:** quitting from the menu called `exit()` from inside an
  event handler; checkpoint saves could store a dying Claw's health and then crash on load; music
  tracks leaked and could play in the wrong order.
- **Additions:** gamepad support, `<LogFrameStats>`, `<TimidityCfg>`, `<FrontPlaneMode>`, absolute
  paths for `<RezArchive>`.
- **Vita specifics:** SDL's native GXM renderer with batching forced on, 22,050 Hz mixer, raised
  CPU/GPU clocks, `app0:`/`ux0:` file layout, bundled FreePats instruments, the engine's teardown
  skipped on exit.
- **Scope:** only the Vita build is maintained. The Windows, Android and WebAssembly builds and
  their code were removed; a Linux build is kept for development only.

## Differences from the original game

How the port plays differently from Captain Claw (1997):

- **Music** uses the FreePats instruments instead of a 1997 sound card or Windows' MIDI synth.
  The tracks themselves are the original ones.
- **Options menu:** Edit Players, Difficulty, Controls and Display (and Multiplayer, Load Custom
  Level) are greyed out. They were never implemented in OpenClaw.

[Unreleased]: https://github.com/Mezeskarika/openclaw-vita/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/Mezeskarika/openclaw-vita/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/Mezeskarika/openclaw-vita/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Mezeskarika/openclaw-vita/releases/tag/v0.1.0
