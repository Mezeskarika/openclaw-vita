# Vita performance: measurements and options

> **Status (2026-10-28):** In Vita3K, level 1 now runs at a locked 60 fps everywhere,
> including the foreground columns, after turning SDL draw batching back on (v0.2.0).
> Next: one run on real hardware. The rest of this document is the history of how we got here
> and the options kept in reserve in case hardware turns out slower.

Goal: a consistent frame rate. Starting point (Vita3K v0.2.1, Vulkan, Intel iGPU, level 1):

| Area | Vita3K fps |
|---|---|
| Plain areas | < 24 |
| Moving platforms / enemies | ~11 |
| Foreground columns | ~7 |

## Read this first: Vita3K is not the target

Vita3K runs the Vita's ARM code through a JIT and translates every GPU draw call into Vulkan.
Its costs are not the hardware's costs, and they can point in opposite directions.

- **In Vita3K the foreground columns are disproportionately expensive.** In the plain area the
  frame is ~42 ms for ~365 draws, about 0.11 ms per draw on average. The column area adds the
  Front plane's ~170 draws and the frame grows to ~143 ms, about 0.6 ms per extra draw, 5–8×
  the average. So it is not simply the number of draws. These draws are large translucent
  overlays covering most of the screen, which points at blending/fill cost in the emulator,
  but that is not proven yet. The game's own CPU work barely changes there (host measurement).
- **On real hardware the game's CPU work is the likely limit.** GXM handles a few hundred
  small draws per frame easily. The Cortex-A9 running the game logic is the tight part.

`<FrontPlaneMode>` (see "How to measure") tells the two Vita3K explanations apart in one run.

So: optimise for both, but judge results on hardware. Running on a real Vita is safe
(see `PLAN.md`, stage 1), and nothing replaces a hardware number.

## Vita3K run with frame stats (2026-10-28)

`FrameStats` lines from the user's run (mixer at 22,050 Hz, `FrontPlaneMode` normal):

| Area | fps | update (ms) | render (ms) | present (ms) | draws |
|---|---|---|---|---|---|
| Plain | 20–24 | 2.5–3 | 40–47 | 0.1–0.2 | 300–380 |
| Enemies / platforms | 10–16 | 3–4 | 60–100 | 0.2–0.3 | 330–430 |
| Columns | 6.8–8 | 3–5 | 120–143 | 0.3 | 459 |
| Same, game paused (menu open) | 7 | 2 | 135–138 | 0.3 | 460 |

**Finding: SDL draw batching was off.** `render` (walking the scene and calling
`SDL_RenderCopy`) took 40–140 ms while `present` took almost nothing, so the draws were being
executed inside the `SDL_RenderCopy` calls, one at a time. Cause: SDL 2.32 turns batching off
whenever the app names a render driver (`SDL_render.c`, `SDL_CreateRenderer`), unless
`SDL_HINT_RENDER_BATCHING` is set. Forcing `VITA gxm` (stage 1) therefore made every tile and
sprite a separate GPU submission with its own state setup. Fixed by setting the batching hint.
This affects real hardware too, not only the emulator.

The game's own CPU work (`update`, physics included) is only 2–5 ms per frame in Vita3K.

**Result after the fix** (Vita3K, same route, `FrontPlaneMode` normal):

| Area | fps | worst frame (ms) | update (ms) | of which physics | render (ms) | present (ms) | draws |
|---|---|---|---|---|---|---|---|
| Plain | 60.0 | 16.9–17.0 | 4.3–4.9 | 1.8–2.3 | 0.8–1.0 | 10.6–11.2 | 300–380 |
| Enemies / platforms | 60.0 | 16.9–17.0 | 4.0–4.6 | 1.6–2.0 | 0.8–1.0 | 11.0–11.6 | 230–350 |
| Columns | 60.0 | 16.9–17.0 | 4.4–4.6 | 1.8–1.9 | 0.8–0.9 | 11.0–11.3 | 485–492 |

The game is now vsync-bound: the real work (update + render + SDL submitting the draws) is
roughly 6–8 ms of the 16.7 ms frame, and `present` is mostly waiting for the next vblank.
The columns cost nothing noticeable any more, so the `FrontPlaneMode` experiment is no longer
needed. A handful of single frames hit 20–36 ms (likely first-time loads); worth watching,
not a problem yet.

The 4.3 s worst frame right after loading a level is the level load itself, not gameplay.

## Earlier host measurements

### Frame breakdown on the host

Same code and config, desktop Linux build in a container (`<LogFrameStats>`), level 1:

| Spot | update (ms) | of which physics | render (ms) | draws/frame |
|---|---|---|---|---|
| Spawn point | 1.2–1.5 | 0.4–0.7 | 0.1 | 365 |
| Column band (teleport 689 5400) | 1.3–1.5 | 0.7–0.9 | 0.1 | 477 (Front plane: 170) |

`present` is left out: the container has no GPU, so it measures software OpenGL.

### Where the update time goes (callgrind, 156 frames at the spawn point)

Per frame, in instructions (host x86; the Vita's ARM count is in the same ballpark):

| Part | Instructions / frame | Share of update |
|---|---|---|
| Whole update (`BaseGameLogic::VOnUpdate`) | 3.5 M | 100% |
| Physics (`b2World::Step`, 10 velocity / 8 position iterations) | 1.4 M | 41% |
| – of which continuous collision (`SolveTOI`) | 0.5 M | 15% |
| – of which finding new contacts (broad phase) | 0.4 M | 12% |
| Actor updates (`Actor::Update`, all 1,479 actors) | 0.8 M (steady state) | 23% |
| – of which animations | 0.5 M | 15% |
| A sound loaded for the first time (spike, not every frame) | 166 M once | — |
| Queueing draws (`Scene::OnRender`) | 0.36 M | (render, not update) |

Rough hardware estimate: the A9 at 444 MHz manages on the order of 150–250 M instructions/s
on this kind of pointer-heavy C++, so the steady update alone is about **15–25 ms per frame**,
before SDL's renderer backend runs. That is in reach of 30 fps but not of 60. Treat it as
an estimate until there is a hardware number.

### Why the columns hurt

Level 1's `Front` plane repeats horizontally and has tiles in the band at world y ≈ 4,860–6,140
(just below the spawn point). Anywhere in that band, every on-screen foreground tile is drawn:
up to ~170 extra 64×64 translucent draws per frame on top of the background (~170, all
opaque) and the main plane.

### Level 1 actor count

1,479 actors, including 485 animated decorations (`BehindAniCandy`), 299 + 138 pickups,
64 crumbling pegs, 56 toggle pegs and 26 elevators. All of them are updated every frame,
on screen or not.

### The sound stall

1,068 of the game's 1,147 sounds are 22,050 Hz 8-bit mono. The mixer was opened at
44,100 Hz, so SDL resampled each sound the first time it played (97% of the load cost was in
`SDL_ConvertAudio`). The loader also decodes each WAV twice (`WavLoader.cpp`,
`VGetLoadedResourceSize`). Benchmark of loading all 1,101 22 kHz sounds on the host:

| Mixer rate | Average per load | Worst load |
|---|---|---|
| 44,100 Hz | 1.95 ms | 36.5 ms |
| 22,050 Hz | 0.02 ms | 1.2 ms |

On the Vita's much slower CPU, the 44.1 kHz figures turn into visible hitches every time a new
sound plays, twice over because of the double decode. The Vita log now prints the rate the
audio device actually opened at (`Audio device: ... Hz`); if it isn't 22050, SDL_mixer still
resamples and this fix does nothing there.

## Options, in recommended order

Options 5–11 were ranked before batching was fixed. With Vita3K at 60 fps, none of them is
needed for the emulator; whether any is needed on hardware depends on step 4.

| # | Change | Helps Vita3K | Helps hardware | Effort | Risk | Status |
|---|---|---|---|---|---|---|
| 0 | Turn SDL draw batching back on (`SDL_HINT_RENDER_BATCHING`) | **large** | **large** | tiny | none | **done** |
| 1 | Mixer at 22,050 Hz (removes resampling stalls) | stalls | stalls, mixing CPU | config | none audible | **done** |
| 2 | CPU 444 MHz / GPU 222 MHz (apps start at 333 MHz) | no | +33% CPU | tiny | none | **done** |
| 3 | `<FrontPlaneMode>` experiment | — | — | none | none | not needed any more |
| 4 | Measure on hardware with `<LogFrameStats>` | — | decides 5–11 | none | none | **next** |
| 5 | Tile atlases: one texture per plane, so SDL merges a plane into ~1 draw | no longer needed | if `present` is high on hardware | 1–2 days | low | reserve |
| 6 | Don't update off-screen decorations and pickups (animation, logic) | — | if `update` is high on hardware | 1–2 days | medium | reserve |
| 7 | Physics: Box2D iterations 10/8 → 8/3, review continuous collision | — | if `physics` is high on hardware | hours | gameplay feel | reserve |
| 8 | Frame pacing: fall back to a locked 30 fps when 60 can't be held; clamp the update step | — | consistency | hours | low | reserve |
| 9 | Less blending/fill: opaque tiles without blending | — | if GPU-bound on hardware | hours | low | reserve |
| 10 | Decode each WAV once; preload a level's sounds at load time | occasional spikes | spikes | hours | low | proposed |
| 11 | `-O3` / LTO | — | small | config | low | reserve |

Notes:

- **5. Atlases.** SDL's Vita renderer already merges consecutive draws that share a texture
  and blend mode into one `sceGxmDraw` (`SDL_render_vita_gxm.c`, `VITA_GXM_RunCommandQueue`).
  Each tile is its own texture today, so nothing merges. With one atlas per plane
  (a 2048×2048 texture holds 1,024 tiles of 64×64), the tile loop stays as it is, apart from
  passing a source rectangle. Built once per level in `TilePlaneRenderComponent::VInit`
  (`RenderComponent.cpp`). Expected: ~365–477 draws → roughly 3 for tiles plus one per
  visible actor. This only removes per-draw overhead; it does nothing for blending/fill cost,
  so only start it if experiment 3 points at per-draw cost.
- **6. Off-screen actors.** Most of the 1,479 actors are decorations and pickups whose
  update only advances an animation nobody sees. Needs care so that gameplay actors
  (enemies, elevators, pegs) still update.
- **7. Physics.** 8/3 are Box2D's recommended defaults. Fewer iterations change how stacked
  or pushing bodies settle, so this needs a playtest of jumps, elevators and crates.
- **8. Pacing.** The display runs at 60 Hz, so with vsync the possible steady rates are
  60, 30, 20 and 15 fps. 24 fps cannot be shown evenly; a frame rate that alternates between
  30 and 20 looks worse than a locked 20. Proposal: present every 2nd vblank (30 fps) when the
  frame fits in 33 ms, otherwise every 3rd (20 fps). Separately, the game passes the real frame
  time to physics, so at 7 fps each physics step covers ~140 ms, which risks falling through
  platforms. Clamping the step (e.g. to 50 ms) makes slow frames slow down the game instead.
- **9. Blending.** Every tile texture has an alpha channel, so every tile is drawn with
  blending, including the fully opaque background. Palette index 0, the "transparent" colour,
  has alpha 1 rather than 0 (`libwap/PalFile.cpp`), so transparent pixels are still blended
  rather than discarded.
- **Vita3K settings worth a quick try** (emulator, not game): `hashless-texture-cache: true`
  in `~/.config/Vita3K/config.yml`. It skips re-hashing textures on use. It only affects
  the emulator, so don't judge the game's speed by it.

## How to measure

1. Install the new VPK. Then delete `ux0:data/openclaw/config.xml` (in Vita3K:
   `~/.local/share/Vita3K/Vita3K/ux0/data/openclaw/config.xml`) so the new defaults
   (`LogFrameStats`, 22,050 Hz, `FrontPlaneMode`) are copied from the VPK on the next start.
   Keep `SAVES.XML`.
2. Delete `ux0:data/SDL_Log.txt`.
3. Play: stand at the spawn point for ~10 s, walk to an area with enemies/platforms, then the columns.
4. Every 2 s the log gets a line like
   `[FrameStats] 23.9 fps | frame 41.8 ms (worst 60.2) | update 9.1 | physics 3.9 | render 2.0 | present 30.1 | draws 365`.
   - `update` / `physics` / `render`: the game's CPU work.
   - `present`: SDL executing the queued draws plus waiting for the GPU and vsync. In Vita3K
     this is where the emulator's per-draw cost shows up.
   - `draws`: tile + actor draws (HUD not counted).
5. Front plane experiment (Vita3K, at the columns). Edit `<FrontPlaneMode>` in
   `ux0:data/openclaw/config.xml`, restart, stand among the columns for ~10 s, note the
   FrameStats line. Do it for `normal`, `skip` and `tiny`:
   - `skip` back to plain-area speed → the Front plane is the whole cost.
   - `tiny` about as fast as `skip` → the cost is filling/blending pixels, not the number of
     draws. Atlases (5) won't help; reducing blending (9) might.
   - `tiny` about as slow as `normal` → the cost is per draw. Atlases (5) are the fix.
