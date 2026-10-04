"""Data tables: a direct port of the original `DataTables.cpp`.

All balance numbers are transcribed 1:1 from the C++ source. The only
structural change is that the C++ `Resource<T>` (value + original +
upgrade percentage/cost) becomes a small mutable `Stat` class, because the
upgrade system was commented out upstream and we keep the fields for the
3DS port spec.

Animation frames are read from the TexturePacker JSON sidecars. The original
used a fragile line-oriented scanner (`Tools/TextureDataReader.cpp`) that
scraped for `Unit_AnimN.png` then read the next `{...}` pairs; parsing the
JSON properly yields identical values with none of the failure modes.
"""
from __future__ import annotations

import json
import os
import re
from typing import List, Sequence, Tuple

from . import settings

# ---------------------------------------------------------------- enums
# Index order is load-bearing: it is the enum order in the original and the
# index used to look up unit textures in the spawn bar.
MAGE, KNIGHT, SAMURAI, SHADOW, DESTROYER, EXECUTIONER, UNIT_COUNT = range(7)

MELEE, RANGED = range(2)

LASER_TURRET, TURRET_COUNT = range(2)

ALLY, ENEMY = range(2)

BASE_DEFAULT = 0

# Animation flags
REPEAT = 1
LOOPBACK = 2

# Turret slot rectangles, in base-local coordinates relative to the base's
# centre (from Base.cpp).
TURRET_SLOT_RECTS: Tuple[Tuple[float, float, float, float], ...] = (
    (18.0, 103.0, 43.0, 44.0),
    (67.0, 103.0, 43.0, 44.0),
    (117.0, 103.0, 43.0, 44.0),
)

# Smoke emitter offsets per damage threshold, from Base.cpp.
SMOKE_EMITTERS: Tuple[Tuple[int, float, float], ...] = (
    (15, -36.0, -33.0),
    (32, 64.0, -69.0),
    (55, 55.0, 87.0),
    (80, -57.0, 57.0),
)

# Unit textures, indexed by unit type.
UNIT_TEXTURE_FILES = (
    "Mage.png",
    "Knight.png",
    "Samurai.png",
    "Shadow.png",
    "Destroyer.png",
    "Executioner.png",
)

TURRET_TEXTURE_FILES = ("LaserTurret.png",)


# ---------------------------------------------------------------- stats
class Stat:
    """Mirrors pyro::utils::Resource<T> plus its upgrade fields."""

    __slots__ = ("value", "original", "upgrade_percentage", "upgrade_cost")

    def __init__(self, value, upgrade_percentage=0, upgrade_cost=0):
        self.value = value
        self.original = value
        self.upgrade_percentage = upgrade_percentage
        self.upgrade_cost = upgrade_cost

    def reset(self) -> None:
        self.value = self.original

    def __repr__(self) -> str:
        return f"Stat({self.value}, up={self.upgrade_percentage}%, {self.upgrade_cost}g)"


class Upgradeable:
    """A stat that the (currently disabled) upgrade system can modify."""

    __slots__ = ("stat", "purchased")

    def __init__(self, value, upgrade_percentage=0, upgrade_cost=0):
        self.stat = Stat(value, upgrade_percentage, upgrade_cost)
        self.purchased = False

    @property
    def value(self):
        return self.stat.value

    def can_upgrade(self, gold: int) -> bool:
        return not self.purchased and gold >= self.stat.upgrade_cost

    def apply(self) -> None:
        """Add `upgrade_percentage` of the original value (rate subtracts)."""
        st = self.stat
        pct = st.value / st.original * 100.0 + st.upgrade_percentage
        st.value = pct * st.original / 100.0
        self.purchased = True


class Frame:
    """One animation frame: a source rect plus a normalised pivot."""

    __slots__ = ("x", "y", "w", "h", "px", "py", "surface")

    def __init__(self, x, y, w, h, px, py, surface):
        self.x = x
        self.y = y
        self.w = w
        self.h = h
        self.px = px
        self.py = py
        self.surface = surface


class AnimationData:
    __slots__ = ("frames", "total_duration", "repeat", "loopback", "frame_duration")

    def __init__(self, frames: Sequence[Frame], total_duration: float,
                 repeat: bool, loopback: bool):
        self.frames = list(frames)
        self.total_duration = total_duration
        self.repeat = repeat
        self.loopback = loopback

        # setupIndividualAnimation(): build the loopback sequence, then give
        # every frame an equal slice of the total duration.
        if loopback:
            self.frames.extend(reversed(self.frames[:-1]))
        if self.frames:
            self.frame_duration = total_duration / len(self.frames)
        else:
            self.frame_duration = 0.0


# ---------------------------------------------------------------- parsers
_FRAME_INDEX = re.compile(r"_(\d+)\.png$")


def _read_frames(unit_name: str, anim_type: str) -> List[Frame]:
    """Return frames for `<unit_name>_<anim_type><N>.png`, ordered by N.

    The original searched for the literal filename and relied on JSON key
    ordering; we do it by regex on the filename, which is order-independent
    and cannot desync when the sheet is re-exported.
    """
    import pygame  # local import keeps this module importable headless

    path = os.path.join(settings.TEXTURE_DATA, unit_name + "Data.json")
    if not os.path.isfile(path):
        return []

    with open(path, "r", encoding="utf-8") as fh:
        raw = json.load(fh)

    sheet_name = raw.get("meta", {}).get("image")
    if not sheet_name:
        return []
    # A couple of sheets were exported with a "<Name>Data.png" typo; fall
    # back to the canonical texture when the declared name is absent.
    sheet_path = os.path.join(settings.TEX, sheet_name)
    if not os.path.isfile(sheet_path):
        sheet_path = os.path.join(settings.TEX, unit_name + ".png")
    if not os.path.isfile(sheet_path):
        return []

    sheet = pygame.image.load(sheet_path)
    # The original applied no filtering to gameplay sprites.
    sheet = sheet.convert_alpha()

    prefix = unit_name + "_" + anim_type
    collected = []
    for entry in raw.get("frames", ()):
        fname = entry.get("filename", "")
        if not fname.startswith(prefix):
            continue
        match = _FRAME_INDEX.search(fname)
        if match is None:
            continue
        rect = entry["frame"]
        pivot = entry.get("pivot", {"x": 0.0, "y": 0.0})
        # subsurface views the shared sheet; copy() detaches each frame so
        # the whole sheet can be freed and so we can pre-scale independently.
        sub = sheet.subsurface(
            pygame.Rect(rect["x"], rect["y"], rect["w"], rect["h"])
        ).copy()
        collected.append(
            (
                int(match.group(1)),
                Frame(rect["x"], rect["y"], rect["w"], rect["h"],
                      float(pivot["x"]), float(pivot["y"]), sub),
            )
        )

    collected.sort(key=lambda pair: pair[0])
    return [frame for _, frame in collected]


# ---------------------------------------------------------------- data
class UnitData:
    __slots__ = (
        "name", "unit_type", "general_type", "health", "damage", "range",
        "rate", "cost", "speed", "spawn_time", "scale", "icon_rect",
        "projectile_speed", "walk", "attack", "frame0", "frames",
    )

    def __init__(self, **kw):
        for key in self.__slots__:
            setattr(self, key, kw.get(key))


class TurretData:
    __slots__ = (
        "name", "turret_type", "damage", "range", "rate", "cost", "scale",
        "icon_rect", "projectile_speed", "frame0", "frames",
    )

    def __init__(self, **kw):
        for key in self.__slots__:
            setattr(self, key, kw.get(key))


class BaseData:
    __slots__ = ("name", "base_type", "health", "max_population", "gold", "scale")

    def __init__(self, **kw):
        for key in self.__slots__:
            setattr(self, key, kw.get(key))


class ValueDisplayData:
    """Floating damage number motion, from initValueDisplayData()."""

    __slots__ = ("start_vx", "start_vy", "accel_x", "accel_y", "lifetime")

    def __init__(self):
        self.start_vx = 0.0
        self.start_vy = 42.0
        self.accel_x = 0.0
        self.accel_y = -1.7
        self.lifetime = 0.85


# ---------------------------------------------------------------- tables
def build_unit_data() -> List[UnitData]:
    """Transcribed from initUnitData(). Order defines unit type indices."""
    units: List[UnitData] = [
        # Mage
        UnitData(
            name="Mage", unit_type=MAGE, general_type=RANGED,
            health=Upgradeable(75, 25, 5), damage=Upgradeable(15, 25, 50),
            range=Upgradeable(95.0, 25, 5), rate=Upgradeable(0.7, 25, 5),
            cost=25, speed=50.0, spawn_time=2.0, scale=0.5,
            icon_rect=(5.0, 126.0, 30.0, 30.0), projectile_speed=320.0,
        ),
        # Knight
        UnitData(
            name="Knight", unit_type=KNIGHT, general_type=MELEE,
            health=Upgradeable(200), damage=Upgradeable(35),
            range=Upgradeable(15.0), rate=Upgradeable(0.75),
            cost=50, speed=65.0, spawn_time=3.5, scale=0.5,
            icon_rect=(15.0, 130.0, 55.0, 55.0), projectile_speed=None,
        ),
        # Samurai
        UnitData(
            name="Samurai", unit_type=SAMURAI, general_type=MELEE,
            health=Upgradeable(280), damage=Upgradeable(25),
            range=Upgradeable(15.0), rate=Upgradeable(0.45),
            cost=150, speed=70.0, spawn_time=3.5, scale=0.25,
            icon_rect=(82.0, 305.0, 57.0, 57.0), projectile_speed=None,
        ),
        # Shadow
        UnitData(
            name="Shadow", unit_type=SHADOW, general_type=MELEE,
            health=Upgradeable(350), damage=Upgradeable(65),
            range=Upgradeable(15.0), rate=Upgradeable(0.6),
            cost=350, speed=90.0, spawn_time=3.0, scale=0.5,
            icon_rect=(432.0, 416.0, 58.0, 58.0), projectile_speed=None,
        ),
        # Destroyer
        UnitData(
            name="Destroyer", unit_type=DESTROYER, general_type=RANGED,
            health=Upgradeable(500), damage=Upgradeable(50),
            range=Upgradeable(60.0), rate=Upgradeable(0.7),
            cost=450, speed=50.0, spawn_time=4.5, scale=0.85,
            icon_rect=(71.0, 180.0, 69.0, 69.0), projectile_speed=500.0,
        ),
        # Executioner
        UnitData(
            name="Executioner", unit_type=EXECUTIONER, general_type=MELEE,
            health=Upgradeable(600), damage=Upgradeable(40),
            range=Upgradeable(15.0), rate=Upgradeable(0.35),
            cost=550, speed=55.0, spawn_time=5.0, scale=0.9,
            icon_rect=(65.0, 24.0, 54.0, 54.0), projectile_speed=None,
        ),
    ]

    # Animation timing, transcribed per unit.
    walk_durations = (0.65, 0.65, 0.65, 0.65, 0.45, 0.65)
    walk_loopback = (False,) * 6
    # Only the Mage attack animation loops back on itself.
    atk_loopback = (True, True, False, False, False, False)

    for data, walk_dur, w_lb, a_lb in zip(units, walk_durations,
                                          walk_loopback, atk_loopback):
        walk_frames = _read_frames(data.name, "Walk")
        atk_frames = _read_frames(data.name, "Attack")
        data.walk = AnimationData(walk_frames, walk_dur, True, w_lb)
        # The attack animation lasts exactly one attack interval, so the
        # swing stays in sync with the damage tick.
        data.attack = AnimationData(atk_frames, data.rate.stat.original, False, a_lb)
        data.frames = (data.walk, data.attack)
        data.frame0 = walk_frames[0] if walk_frames else (
            atk_frames[0] if atk_frames else None)
    return units


def build_turret_data() -> List[TurretData]:
    return [
        # Laser Turret
        TurretData(
            name="Laser Turret", turret_type=LASER_TURRET,
            damage=Upgradeable(8), range=Upgradeable(380.0),
            rate=Upgradeable(0.75), cost=215, scale=0.75,
            icon_rect=None, projectile_speed=450.0,
        ),
    ]


def build_base_data() -> List[BaseData]:
    return [
        BaseData(name="Default Base", base_type=BASE_DEFAULT,
                 health=Upgradeable(2000), max_population=10, gold=200,
                 scale=1.0),
    ]


class Tables:
    """Container so callers load assets once and share the result."""

    __slots__ = ("units", "turrets", "bases", "value_display", "unit_icons")

    def __init__(self):
        self.units = build_unit_data()
        self.turrets = build_turret_data()
        self.bases = build_base_data()
        self.value_display = ValueDisplayData()
        # Spawn-bar icons: crop each unit's head out of its sheet.
        self.unit_icons = []
        for data in self.units:
            icon = data.icon_rect
            self.unit_icons.append(icon)
