"""Tunables and performance targets.

Targets a low-power laptop: AMD Ryzen 3 7320U (4c/8t @ 2.4GHz) + Radeon 610M
integrated graphics, 16GB RAM, 1024x768 display.
"""
from __future__ import annotations

import os

# ---------------------------------------------------------------- paths
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "Assets")
TEX = os.path.join(ASSETS, "Textures")
FONTS = os.path.join(ASSETS, "Fonts")
MUSIC = os.path.join(ASSETS, "Music")
SOUNDS = os.path.join(ASSETS, "Sounds")
TEXTURE_DATA = os.path.join(ASSETS, "TextureData")

GAME_TITLE = "Age of War"

# ---------------------------------------------------------------- video
# The original shipped a 1280x720 window. We default to the real desktop
# size and allow a fullscreen toggle at runtime.
DEFAULT_WINDOW = (1024, 768)
DEFAULT_FULLSCREEN = True

# World is derived from the background texture: 1.5x wider than tall.
BG_WIDTH_SCALE = 1.5

VSYNC = 1  # 1 = wait for vblank, saves a whole CPU core on a 4-core U-series

# ---------------------------------------------------------------- sim
# Fixed timestep decoupled from render. 60Hz matches the original's feel
# and keeps per-step float work bounded and predictable.
SIM_HZ = 60
SIM_DT = 1.0 / SIM_HZ
MAX_FRAME_TIME = 0.25  # clamp huge hitches (alt-tab, GC) to avoid spiral of death
MAX_STEPS_PER_FRAME = 5

# ---------------------------------------------------------------- perf
# Adaptive resolution: if the rolling average frame time stays above
# TARGET_FRAME_MS the internal render scale steps down. Costs one scaled
# blit per frame, so it is only paid when actually needed.
TARGET_FRAME_MS = 1000.0 / 60.0
SLOW_FRAME_MS = 1000.0 / 50.0
ADAPTIVE_SAMPLE_FRAMES = 90
RENDER_SCALES = (1.0, 0.85, 0.72, 0.6)

# Per-emitter particle cap. The original used 172 per emitter with up to
# 4 emitters = 688 alpha blits/frame, which is the single most expensive
# thing in the scene on integrated graphics.
SMOKE_PARTICLES_PER_EMITTER = 110
SMOKE_MAX_EMITTERS = 4

# Damage numbers: hard cap so a turret volley cannot flood the display.
MAX_DAMAGE_NUMBERS = 40

# ---------------------------------------------------------------- audio
# 22050Hz mono keeps decode cost and memory low; the 610M shares system RAM
# with the CPU so we avoid stereo buffers we do not need.
MIXER_FREQUENCY = 22050
MIXER_SIZE = -16
MIXER_CHANNELS = 1
MIXER_BUFFER = 512
MUSIC_VOLUME = 0.55
SFX_VOLUME = 0.8

# ---------------------------------------------------------------- balance
# Gold awarded for a kill, as a percentage of the unit's cost (125% in
# the original).
KILL_GOLD_PERCENT = 125

# Number of turret placement slots on each base.
TURRET_SLOTS = 3

# Max units queued behind the one currently spawning.
UNIT_QUEUE_SIZE = 5
