# The original game's Options menu, and what to build on Vita

Research for roadmap items 8 (what the original Options submenus contained) and 12 (difficulty),
with findings for 4 (remappable buttons) and 11 (view size / Display menu).

Sources:
- **CLAW.REZ**: the menu images under `/STATES/MENU/IMAGES/MAIN/OPTIONS/` (including the ones
  OpenClaw never used), the original menu definition `/STATES/MENU/CLAW.IFC`, and the dialog
  images under `/STATES/DIALOGS/`.
- **Monolith's Claw Design Bible** (design document, about 100 pages, dated 10/20/2006 in the
  PDF): <https://www.gamedevs.org/uploads/monolith-claw.pdf>, sections 6.1.1.2.4 (Options),
  6.1.1.3 (in-game menu), 4.1.1 (help screen) and 7.1 (attack damage). It is a design document,
  not the retail manual, so the shipped game may differ in details.

## Summary

| Submenu | Original | Worth having on Vita | Effort |
|---|---|---|---|
| Edit Players | Player profiles: New, Info, Change name, Remove. Saves and **difficulty** belong to the player | Only as the home of difficulty; otherwise low value | Medium |
| Difficulty | Not a settings page: a message, "Difficulty level is based on the current player. To change the difficulty, create a new player." | Yes, as a direct Normal/Easy choice | Small |
| Controls | Two columns, keyboard and joypad; each action picks a key/button from a drop-down; "Revert to Defaults" | Yes (roadmap 4) | Medium |
| Display | Color depth 8/16 bit, Detail high/low, Front layer on/off, Game play area slider, Movies stretched/interlaced | Partly: Front layer, Detail, Game play area | Small to medium |
| Multiplayer, Load Custom Level | Network play; user-made levels | No | — |

## Edit Players

From the design bible (6.1.1.2.4.1) and the unused images (`OPTIONS/EDIT/*`: NEW, INFO, CHANGE,
REMOVE incl. a greyed-out REMOVE, BACK, title "EDIT PLAYERS"):

- All saved games and high scores are stored per player name. The first name is entered at install
  time; if none, "Player".
- **New**: adds a player (the "new player dialog"); its **Easy** check box sets the difficulty.
- **Info**: shows the selected player's information. The dialog images in
  `/STATES/DIALOGS/IMAGES/EDITPLAYERS/` (treasure chest, health heart, lives head, pistol, magic,
  dynamite animations) suggest it showed that player's score, health, lives and ammo.
- **Change**: renames the player. **Remove**: deletes the player and all of their saves.
- The dialogs themselves were Windows dialog resources in the original executable; CLAW.REZ only
  has their tiled background textures, so there is no layout to copy.

## Difficulty

- In the final menu definition (`CLAW.IFC`) the Options page has **no Difficulty entry at all**:
  Edit Players, Controls, Audio, Display, Back. The design bible still describes a Difficulty page
  that only shows the message above, and CLAW.REZ has its title, BACK button and the message image
  (`OPTIONS/DIFFICULTY/*`). OpenClaw's MENU.xml put a Difficulty button on the Options page
  anyway.
- **Only two levels: Normal (default) and Easy.** Easy was a check box when creating a player.
- **What Easy changed** (design bible 7.1, "Captain Claw Attacks and Damage"): Claw's melee attacks
  hit much harder; ranged attacks were unchanged.

  | Claw's attack | Normal | Easy |
  |---|---|---|
  | Punch, kick, sword swipe, jump sword swipe | 5 | 20 |
  | Throw | 4 | 20 |
  | Ducking sword swipe | 2 | 10 |
  | Claw swipe (catnip) | 10 | 20 |
  | Pistol / magic claw / dynamite / power-up swords | 8 / 25 / 15 / 20 | unchanged |

  (The bible also notes that power-up swords, magic claw and claw swipe do less damage to bosses.)
- The retail v1.2 patch added a cheat `MPEASYMODE` ("Easy mode"), according to
  [cheatbook.de](https://www.cheatbook.de/files/claw.htm).

### In OpenClaw today

- Claw's melee damage is hard-coded in `ClawControllableComponent`
  (`OpenClaw/Engine/Actor/Components/ControllableComponent.cpp`, the attack-frame handler): **10**,
  **5** while ducking, **100** with catnip. OpenClaw's numbers are already OpenClaw's own scale, not
  the bible's (enemy health in `ASSETS/ACTOR_PROTOTYPES` is on the same scale).
- Projectile damage is in `ActorTemplates.cpp` (pistol 20, magic 50) and the projectile prototypes.
- Damage taken by Claw goes through `HealthComponent::AddHealth`.

### Options, cheapest first (roadmap 12)

1. **Original Easy: Claw's melee damage ×2–×4.** Faithful to the original (5 → 20 is ×4; on
   OpenClaw's scale ×2 already makes most early enemies die in one or two hits). One multiplier at
   the melee attack site. Small, low risk, and it's what the original did.
2. **Damage taken by Claw scaled** (e.g. Easy −20%/−50%, Hard +20%), in `HealthComponent::AddHealth`
   for Claw only, excluding death tiles (`DamageType_DeathTile`). Small. Also gives a "Hard" level,
   which the original didn't have.
3. **Resources**: more lives/health at the start and from pickups (`PickupComponent`, checkpoint
   values). Small to medium; touches saves.
4. **Enemy health or behaviour**: scale enemy max health at creation, or attack rates in
   `Engine/Actor/Components/EnemyAI/`. Most work, most balancing risk.

**Recommendation:** Normal and Easy, where Easy = option 1 (Claw's melee ×4 on the original's
scale, i.e. ×2–×4 here; pick by playtesting level 1–2) plus optionally option 2 at −25%. Skip Hard
unless wanted. Store it as a game option (`<Difficulty>` in config.xml, saved through
`SaveGameOptions`) rather than per player, unless Edit Players is built; then store it in the
player profile as the original did.

## Controls

Design bible 6.1.1.2.4.2 and the menu outline: a page with two columns (keyboard, joypad); each
action has a drop-down of keys or buttons 1–8; "Revert to Defaults" per device. CLAW.REZ has
`/STATES/DIALOGS/IMAGES/CONFIGCONTROLS/` (background, KEYBOARD and JOYSTICK labels): it was a dialog.

Bindable actions and defaults:

| Action | Keyboard | Joypad |
|---|---|---|
| Jump | Space | 1 |
| Hand-to-hand attack | Ctrl | 2 |
| Use projectile | Alt | 3 |
| Toggle projectile | Shift | 4 |
| Throw | Z (also Ctrl+Up) | 5 |
| Pistol | 1 | 6 |
| Magic | 3 | 7 |
| Dynamite | 2 | 8 |
| Special weapon/magic | 4 | — |

On Vita that maps directly onto roadmap 4: the same action list, one column (Vita buttons), a
"Revert to Defaults" item. Today's `GamepadMapper` covers jump, attack, use projectile, toggle
projectile and the menu; throw and direct weapon selection (pistol/magic/dynamite) have no button
yet. L and R are free.

## Display

Unused images `OPTIONS/DISPLAY/*` and design bible 6.1.1.2.4.3. The final menu definition lists
Detail, Front, Area and Movies (no Color Depth).

| Option | Original | On Vita |
|---|---|---|
| Color depth 8/16 bit | Chose the 8-bit or 16-bit REZ file (16-bit was on the CD) | Drop: one REZ, rendering is 32-bit |
| Detail high/low | Low hid "eye-candy" sprites flagged by the level designers, lighting effects and sparkles around treasure | Possible if those flags survive in the level data (needs checking in libwap's WWD object flags); low value now that the game runs at 60 fps |
| Front layer on/off | Hide the foreground plane (default on) | Easy: the `<FrontPlaneMode>skip` debug option already does it; needs a menu toggle and saving |
| Game play area | Slider; shrinks the view **letterbox-style** (less height, same width); also `+`/`-` keys in game | See below |
| Movies stretched/interlaced | Smacker movie playback mode | Drop: OpenClaw has no movies (Replay Movies is disabled too) |

### Game play area on Vita (input for roadmap 11)

The original only ever made the view *smaller* (black bars top and bottom), for slow PCs. On Vita
the interesting direction is the other one: how much of the level is visible at 960×544.
OpenClaw renders at window size × `<Scale>`; at scale 1 the Vita shows 960×544 world pixels, about
1.5× the original's 640×480 width. Candidate options: "Wide (960×544)", "Original-like (scale
≈1.13, shows 480 world pixels of height)", "Close (scale 1.5)". Roadmap 11 still needs to check
how non-integer scales look and whether the HUD/menus cope; the original's letterbox shrink is not
worth reproducing.

## Audio (already in OpenClaw)

The original also had **Voice** and **Ambient** on/off (images `OPTIONS/AUDIO/005–012`, 032–035;
both in `CLAW.IFC`). OpenClaw's Audio page has only Sound and Music. Voice (Claw's quotes) and
Ambient (level-wide wind, water, chains) toggles would be cheap additions if wanted.

## Suggested order for roadmap 13

1. Difficulty (Normal/Easy as above) as a direct page, re-enabling the Difficulty button.
2. Controls (roadmap 4), with the original action list.
3. Display with Front layer and View size (after 11).
4. Edit Players only if per-player saves are wanted.
