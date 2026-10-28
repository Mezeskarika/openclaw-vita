# OpenClaw for PS Vita

A PlayStation Vita port of [OpenClaw](https://github.com/pjasicek/OpenClaw), the open-source
reimplementation of Captain Claw (1997).

> **Status:** pre-release. Runs in the Vita3K emulator (level 1 at 60 fps, with music);
> not yet tested on a real Vita.

**Download:** the `.vpk` from the latest [release](https://github.com/Mezeskarika/openclaw-vita/releases).
What changed in each version: [`CHANGELOG.md`](CHANGELOG.md).

## You need

- A PS Vita that runs homebrew (HENkaku/Ensō) with VitaShell, or the
  [Vita3K](https://vita3k.org) emulator.
- **Your own copy of `CLAW.REZ`** from the original Captain Claw. It's the game's data file and
  is **not included**.

## Install

1. Install the `.vpk` with VitaShell (or open it with Vita3K).
2. Copy `CLAW.REZ` to `ux0:data/openclaw/`.
   - In Vita3K that folder is `ux0/data/openclaw/` inside Vita3K's data folder (on Linux,
     `~/.local/share/Vita3K/Vita3K/` by default).
3. Start OpenClaw.

On first start, the game creates `config.xml` and `SAVES.XML` in `ux0:data/openclaw/`.

**Updating from an older version:** install the new `.vpk` over the old one, then delete
`ux0:data/openclaw/config.xml` so the new version's defaults are used. Keep `SAVES.XML`, it holds
your progress.

The game log is written to `ux0:data/SDL_Log.txt`.

## Controls

| Button | Action |
|---|---|
| D-pad / left stick | Move, climb, menu navigation |
| Cross | Jump, confirm |
| Square | Attack |
| Circle | Secondary weapon |
| Triangle | Switch ammo type |
| Start / Select | In-game menu, back |

## Known limitations

- Options → Edit Players, Difficulty, Controls and Display (also Multiplayer and Load Custom
  Level) are greyed out: they were never implemented in OpenClaw. (In 0.3.0 and earlier they're
  selectable but do nothing.)
- Settings changed in the menus aren't saved between sessions.
- Placeholder LiveArea artwork.

## Development

- **What's next:** [`vita/ROADMAP.md`](vita/ROADMAP.md)
- **How the port was done:** [`vita/PLAN.md`](vita/PLAN.md),
  [`vita/PERFORMANCE.md`](vita/PERFORMANCE.md)
- **Building:** `vita/build.sh` builds `build-vita/openclaw.vpk` in Docker; `tests/run.sh` runs
  the unit tests
- **Original project:** [pjasicek/OpenClaw](https://github.com/pjasicek/OpenClaw) (desktop builds,
  project background)

## Licence

GPL-3.0, like OpenClaw (see [`LICENSE.txt`](LICENSE.txt)). The bundled FreePats instruments are
GPL-2.0-or-later with an exception for compositions; their licence files are inside the VPK.
Captain Claw and its data are © Monolith Productions; this project contains no game data.
