# Vita performance: measurements and options

Goal: a consistent frame rate. Starting point (Vita3K v0.2.1, Vulkan, Intel iGPU, level 1):

| Area | Vita3K fps |
|---|---|
| Plain areas | < 24 |
| Moving platforms / enemies | ~11 |
| Foreground columns | ~7 |

## Read this first: Vita3K is not the target

Vita3K runs the Vita's ARM code through a JIT and translates every GPU draw call into Vulkan.
Its costs are not the hardware's costs, and they can point in opposite directions.
The measurements below suggest exactly that:

- **In Vita3K the frame rate tracks the number of draw calls.** The column area adds 112 draws
  (see below) and takes the frame from ~42 ms to ~143 ms, roughly 0.9 ms per extra draw.
  The game's own CPU work barely changes there.
- **On real hardware the game's CPU work is the likely limit.** GXM handles a few hundred
  small draws per frame easily. The Cortex-A9 running the game logic is the tight part.

So: optimise for both, but judge results on hardware. Running on a real Vita is safe
(see `PLAN.md`, stage 1), and nothing replaces a hardware number.

## Measurements

### Frame breakdown on the host

Same code and config, desktop Linux build in a container (`<LogFrameStats>`), level 1:

| Spot | update (ms) | of which physics | render (ms) | draws/frame |
|---|---|---|---|---|
| Spawn point | 1.2–1.5 | 0.4–0.7 | 0.1 | 365 |
| Column band (teleport 689 5400) | 1.3–1.5 | 0.7–0.9 | 0.1 | 477 |

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
44,100 Hz, so SDL resampled each sound the first time it played. One such load cost 166 M
instructions on the host (97% of it in `SDL_ConvertAudio`). The loader also decodes each WAV
twice (`WavLoader.cpp`, `VGetLoadedResourceSize`). On the Vita that is a visible hitch
every time a new sound plays.

## Options, in recommended order

| # | Change | Helps Vita3K | Helps hardware | Effort | Risk | Status |
|---|---|---|---|---|---|---|
| 1 | Mixer at 22,050 Hz (removes resampling stalls) | stalls | stalls, mixing CPU | config | none audible | **done** |
| 2 | CPU 444 MHz / GPU 222 MHz (apps start at 333 MHz) | no | +33% CPU | tiny | none | **done** |
| 3 | Measure on hardware with `<LogFrameStats>` | — | — | none | none | next |
| 4 | Tile atlases: one texture per plane, so SDL merges a plane into ~1 draw | **large** | small–medium | 1–2 days | low | proposed |
| 5 | Don't update off-screen decorations and pickups (animation, logic) | small | **medium** | 1–2 days | medium | proposed |
| 6 | Physics: Box2D iterations 10/8 → 8/3, review continuous collision | small | medium | hours | gameplay feel | proposed |
| 7 | Frame pacing: lock to 30 fps (or 20) with vsync, and clamp the update step | consistency | consistency | hours | low | proposed |
| 8 | Draw opaque tiles (whole background) without blending | small | fill rate | hours | low | proposed |
| 9 | Decode each WAV once; preload a level's sounds at load time | stalls | stalls | hours | low | proposed |
| 10 | `-O3` / LTO | small | small | config | low | proposed |

Notes:

- **4. Atlases.** SDL's Vita renderer already merges consecutive draws that share a texture
  and blend mode into one `sceGxmDraw` (`SDL_render_vita_gxm.c`, `VITA_GXM_RunCommandQueue`).
  Each tile is its own texture today, so nothing merges. With one atlas per plane
  (a 2048×2048 texture holds 1,024 tiles of 64×64), the tile loop stays as it is, apart from
  passing a source rectangle. Built once per level in `TilePlaneRenderComponent::VInit`
  (`RenderComponent.cpp`). Expected: ~365–477 draws → roughly 3 for tiles plus one per
  visible actor.
- **5. Off-screen actors.** Most of the 1,479 actors are decorations and pickups whose
  update only advances an animation nobody sees. Needs care so that gameplay actors
  (enemies, elevators, pegs) still update.
- **6. Physics.** 8/3 are Box2D's recommended defaults. Fewer iterations change how stacked
  or pushing bodies settle, so this needs a playtest of jumps, elevators and crates.
- **7. Pacing.** The display runs at 60 Hz, so with vsync the possible steady rates are
  60, 30, 20 and 15 fps. 24 fps cannot be shown evenly; a frame rate that alternates between
  30 and 20 looks worse than a locked 20. Proposal: present every 2nd vblank (30 fps) when the
  frame fits in 33 ms, otherwise every 3rd (20 fps). Separately, the game passes the real frame
  time to physics, so at 7 fps each physics step covers ~140 ms, which risks falling through
  platforms. Clamping the step (e.g. to 50 ms) makes slow frames slow down the game instead.
- **Vita3K settings worth a quick try** (emulator, not game): `hashless-texture-cache: true`
  in `~/.config/Vita3K/config.yml`. It skips re-hashing textures on use. It only affects
  the emulator, so don't judge the game's speed by it.

## How to measure

1. Delete `ux0:data/openclaw/config.xml` (in Vita3K:
   `~/.local/share/Vita3K/Vita3K/ux0/data/openclaw/config.xml`) so the new defaults
   (`LogFrameStats`, 22,050 Hz) are copied from the VPK on the next start.
2. Delete `ux0:data/SDL_Log.txt`.
3. Play: stand at the spawn point for ~10 s, walk to an area with enemies/platforms, then the columns.
4. Every 2 s the log gets a line like
   `[FrameStats] 23.9 fps | frame 41.8 ms (worst 60.2) | update 9.1 | physics 3.9 | render 2.0 | present 30.1 | draws 365`.
   - `update` / `physics` / `render`: the game's CPU work.
   - `present`: SDL executing the queued draws plus waiting for the GPU and vsync. In Vita3K
     this is where the emulator's per-draw cost shows up.
   - `draws`: tile + actor draws (HUD not counted).
