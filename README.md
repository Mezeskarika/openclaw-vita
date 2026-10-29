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
- Optional, for the cutscenes: **`MOVIES/CLAW.FEC`** from the original game CD (about 350 MB, also
  not included).

## Install

1. Install the `.vpk` with VitaShell (or open it with Vita3K).
2. Copy `CLAW.REZ` to `ux0:data/openclaw/`. For the cutscenes, copy `CLAW.FEC` from the CD's
   `MOVIES` folder there too.
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

Menus can also be used by tapping the touchscreen. Any button, or a tap, skips a cutscene.

The buttons can be changed in `ux0:data/openclaw/config.xml`, under `<ControlOptions><Gamepad>`
(button names: Cross, Circle, Square, Triangle, L, R, Start, Select). An older `config.xml`
without that section uses the defaults above.

## Known limitations

- Options → Edit Players and Controls (also Multiplayer and Load Custom Level) are greyed out:
  they were never implemented in OpenClaw. (In 0.3.0 and earlier they're selectable but do
  nothing.)
- Settings changed in the menus (Audio, Difficulty, Display) are saved in `config.xml`.
- Replay Movies is greyed out without `CLAW.FEC`.
- Placeholder LiveArea artwork.

## Settings (`config.xml`)

`ux0:data/openclaw/config.xml` is created from the defaults on first start. The Audio,
Difficulty and Display pages write their settings there; everything else is edited by hand (copy it to a PC with VitaShell's FTP or
USB mode, edit, copy back). Delete the file to get the defaults back. Options worth knowing:

| Option | Default | What it does |
|---|---|---|
| `Display/Size`, `Display/Scale` | 960×544, 1 | Render size and zoom. At scale 1 you see 960×544 world pixels; 2 shows half as much, zoomed in (the Display page's Game play area slider). Other values work but draw uneven pixel rows. Keep the size at 960×544. |
| `Display/FrontLayer` | true | The foreground layer (e.g. the columns in level 1). Set by the Display page. |
| `Display/Movies` | Interlaced | Cutscenes at the original's 640×480 with every other line black, as the original showed them, or `Stretched` to fill the screen. Set by the Display page. |
| `Display/UseVerticalSync` | true | Lock to the screen's 60 Hz. Turning it off gives no benefit on the Vita. |
| `Audio/Frequency` | 22050 | Mixer rate. The game's sounds are 22,050 Hz; 44100 makes each sound get resampled the first time it plays (stutter). |
| `Audio/SoundVolume`, `Audio/MusicVolume` | 10, 15 | Volumes, sound 0–100 in steps of 10, music 0–20. Set by the Audio page. |
| `Audio/SoundOn`, `Audio/MusicOn` | true, true | Set by the Audio page. |
| `Gameplay/PlayIntroMovies` | true | Monolith logo and introduction at start-up (needs `CLAW.FEC`). |
| `Gameplay/Difficulty` | Normal | `Easy`: Claw's sword, punches and kicks hit harder, as in the original's Easy mode. Set by the Difficulty page. |
| `Audio/TimidityCfg` | `app0:/freepats/timidity.cfg` | Instrument set for the music. Point it at another Timidity config on `ux0:` to use other instruments. |
| `Assets/RezArchive` | `ux0:/data/openclaw/CLAW.REZ` | Where the original game data is. |
| `Assets/MoviesArchive` | `ux0:/data/openclaw/CLAW.FEC` | Where the cutscenes are. |
| `Assets/ResourceCacheSize` | 50 | Memory (MB) for loaded game data. |
| `ControlOptions/Gamepad` | see Controls | Button mapping. |
| `GlobalOptions/ShowFps` | true | FPS counter in the corner. |
| `GlobalOptions/LogFrameStats` | true | Frame-time breakdown in `ux0:data/SDL_Log.txt` every 2 seconds. |
| `DebugOptions/SkipMenu`, `SkipMenuToLevel` | false, 1 | Start straight in the given level. |
| `DebugOptions/FrontPlaneMode` | normal | `skip` hides the foreground layer (e.g. the columns in level 1). |

The `GlobalOptions` gameplay values (jump height, speeds) are OpenClaw's tuning; changing them
changes how the game plays.

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
GPL-2.0-or-later with an exception for compositions; their licence files are inside the VPK. The
menu font, [Cinzel](https://github.com/NDISCOVER/Cinzel), is under the SIL Open Font Licence
(`Build_Release/fonts/OFL.txt`, also in the VPK). The cutscenes are decoded with
[libsmacker](https://github.com/greg-kennedy/libsmacker) (LGPL-2.1, `ThirdParty/libsmacker/COPYING`);
the movie player is based on the one in
[captain-claw-s60](https://github.com/Dante-Leoncini/captain-claw-s60) (GPL-3.0).
Captain Claw and its data are © Monolith Productions; this project contains no game data.
