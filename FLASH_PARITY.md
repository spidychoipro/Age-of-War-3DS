# Flash parity work

Reference: `spidychoipro/Age-of-War-Flash-Decompiled` at
`db4edee40bb054b659f7c634302c67068a8aedc8`.
Port baseline: `e5e3fe3a9e21dec1585e90e7956fb3de38798700`.

## Rules corrected

- Read the actual SWF header: **40 fps**, not the previous assumption of 50.
- All 16 unit EN/ES rows and all 15 TU/TS turret rows now match the decompilation.
  One simulation unit represents 50 Flash pixels. Both armies use ES; difficulty
  multiplies enemy troop HP/damage by 1.3 or 2, without multiplying turret damage.
- Train one unit at a time, with four waiting slots and original build durations.
- Start with 175 gold, two 500-HP bases, and no free turrets.
- Evolution uses cumulative XP. Add `300 * new tech level` to current and maximum
  HP, preserving existing damage. Friendly casualties award rounded half-reward
  player XP; enemy casualties award 1.3-times-cost gold and twice that reward XP.
- Enemy selection uses zero-based unit IDs correctly. Preserve its 40-frame
  decisions, pending decision, six-outstanding-unit limit, tier unlocks at
  1500/5000 frames, 8000-frame evolution and tier-clock reset.
- Copy the individual turret events for all five enemy ages, including the late
  future-age events at 12000/20000 frames. Remove invented enemy specials.
- All troops move 0.7 Flash pixels/frame. Friendly units block each other using
  original hitzone widths. Target decisions refresh every five frames.
- Use original melee/shoot/shootwalk hit frames and random alternate attack
  cycles. Unit ranged attacks call direct damage; turret projectiles use their
  original projectile families, gravity, substeps and acceleration.
- Stone/medieval specials rain individual 200-damage projectiles. Renaissance
  healing lasts 600 frames and restores one HP/frame, including newly spawned
  allies. Modern bombs use 400 damage and the future sweep uses 1000 damage.
  These no longer inflict an invented unavoidable map-wide hit on bases.
  Recharge is 2000 frames and the initial charge is 1999 frames.
- Fire and explosive projectiles emit damaging falling fragments; oil emits
  falling drops. Projectile/corpse storage is reused, so 160 lifetime spawns no
  longer permanently stop production. IDs remain unique across slot reuse.
- Keep destroyed base state accessible to the result screen.

## Graphics and controls

- Preserve sprite alpha instead of flattening every sprite onto opaque black.
- Sort numbered source frames numerically when sampling HUD art.
- Use 3774 source animation frames (3306 unique images, 68 clips), including
  idle, walking, attacking, shooting, shooting while walking and death states.
  Sixteen types share source clips where the original does. Rasterization and
  ETC1A4 compression adapt them to 3DS memory and texture limits.
- Restore the decompiled forest backdrop. Fit the whole 1000px battlefield into
  the 400px top screen so both bases are visible at once, and align each frame's
  last opaque row to the ground line instead of its transparent canvas edge.
- Stream the original 110-second soundtrack in 16384-sample double-buffered
  chunks, so it keeps looping and resumes correctly after the music toggle.
  Rebuild the bottom HUD from the original strips at native size, with a single
  XP row and larger card labels.
- Correct interleaved base/turret/UI atlas indexing and enemy-age base art.
  Stack turret slots vertically and rotate turrets toward their targets.
- Keep help and menu dialogs rendering and accepting touch input. Confirmation
  dialogs suspend the simulation. Use elapsed time to drive the 40-Hz simulation.
- Include tex3ds PNG dependencies during incremental builds, retain intermediate
  atlases, and stop shipping a stale prebuilt atlas in place of its source PNGs.

## Verification

The host regression executable checks queue order, progression, attack timing,
casualty rewards, difficulty, every enemy turret schedule, special cooldown and
healing, slot transactions, corpse-slot reuse over 200 spawns, result state,
40/60-Hz caller equivalence, and 15 autoplay battles covering all ages/difficulties.

`tests/verify_flash_rules.py` independently parses the original EN/ES/TU/TS
tables and SWF frame-rate bytes, and compares them with the **compiled** port.

```sh
c++ -std=c++17 -Wall -Wextra -I3ds/source tests/sim_regression.cpp \
    3ds/source/sim.cpp -o sim-regression
./sim-regression
python tests/verify_flash_rules.py ./sim-regression /path/to/Age-of-War-Flash-Decompiled
```

Rebuild the Flash animation assets with Pillow, then use the normal devkitARM
build. Generated PNGs and descriptors are included so rebuilding the game does
not require the reference checkout.

```sh
python tools/build-flash-animations.py /path/to/Age-of-War-Flash-Decompiled
cd 3ds
source env.sh
make
./build-cia.sh
```

## Remaining differences

This is **not a certified pixel- or frame-identical reproduction**. The two
3DS screens require a different HUD arrangement. Character registration points,
Super Soldier filters, turret animation-trigger delay and ten-frame target
refresh, decorative particles, projectile boundary ordering and some sound cues
still need side-by-side runtime comparison. Projectile/actor pools impose finite
limits absent from Flash's dynamic movie clips. The future special preserves the
reference script's one-frame projectile lifetime; its exact displayed timeline
also needs a runtime comparison.

Real-device rendering, memory/performance, audio and input behavior remain
unverified. UI automation was stopped at the user's request before loading this
build in Azahar; no successful emulator gameplay verification is claimed.

The old contribution guide's claims about original saves/replays or individual
turret levels are not established by this decompilation and are not implemented
as assumed missing original features.
