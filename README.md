# OpenClaw for PS Vita

A PlayStation Vita port of [OpenClaw](https://github.com/pjasicek/OpenClaw), the open-source
reimplementation of Captain Claw (1997).

> **Status:** pre-release. Runs in the Vita3K emulator (level 1 at 60 fps, with music);
> not yet tested on a real Vita.

You need your own copy of `CLAW.REZ`, the original game's data file. It is not included.

- **Download:** [Releases](https://github.com/Mezeskarika/openclaw-vita/releases), with install
  steps and controls in the release notes
- **What's next:** [`vita/ROADMAP.md`](vita/ROADMAP.md)
- **How the port was done:** [`vita/PLAN.md`](vita/PLAN.md),
  [`vita/PERFORMANCE.md`](vita/PERFORMANCE.md)
- **Building:** `vita/build.sh` builds `build-vita/openclaw.vpk` in Docker; `tests/run.sh` runs
  the unit tests
- **Original project:** [pjasicek/OpenClaw](https://github.com/pjasicek/OpenClaw) (desktop builds,
  project background)

## Licence

GPL-3.0, like OpenClaw (see [`LICENSE.txt`](LICENSE.txt)). Captain Claw and its data are
© Monolith Productions; this project contains no game data.
