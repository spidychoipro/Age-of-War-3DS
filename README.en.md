[English README](README.en.md) | [한국어 README](README.md)

# Age of War 3DS

A 2D defense game rebuilt for the Nintendo 3DS (cartridge build, devkitARM). Ported with C++/libctru, and a Python (pygame) reimplementation of the same pieces is kept in this repo too.

> [!NOTE]
> **An unofficial fan port, made by one person.** Not affiliated with the original developers or publishers; the original name is only used for attribution.
>
> The layout and the balance are now based on the original, but it still isn't finished and hardware testing isn't done yet. It's playable start to finish.
>
> If you want it closer to the real game, see [CONTRIBUTING.md](CONTRIBUTING.md). What's left is listed under [Differences from the original](#differences-from-the-original) below.

## Differences from the original

The original SWF runs at 40 fps. This revision compares its decompilation with the port and corrects all 16 unit / 15 turret rows, sequential training, cumulative XP evolution, casualty rewards, enemy production/evolution/turret schedules, animation hit frames and specials. Graphics preserve alpha and use the original forest background and 3,774 source animation frames.

Full visual and timing parity is not claimed. The dual-screen HUD, registration points and filters, some turret/particle/sound timing, and real-device behavior still need comparison. See [`FLASH_PARITY.md`](FLASH_PARITY.md) for the changes, automated checks and remaining differences. `pygame/` remains an unfinished data-only implementation.

The separate [`Age-of-War-Flash-Decompiled`](https://github.com/spidychoipro/Age-of-War-Flash-Decompiled) repository holds the decompilation and original SWF. See [`THIRD_PARTY.md`](THIRD_PARTY.md) for asset provenance and rights.

## Install

Prebuilt binaries are in the releases.

| File | Description |
| --- | --- |
| `aow3ds.cia` | CIA for custom firmware (Luma3DS etc.) |
| `aow3ds.3dsx` | For Homebrew Launcher |

### Option 1: QR code (FBI)

1. Open FBI.
2. `Remote Install` → `Scan QR code`.
3. Display `install/aow3ds-qr.png` from the repo root (keep the white background, as large as you can manage).
4. The 3DS camera reads the URL; press `Yes` to download and install.

The QR encodes a direct download link to the CIA. As long as the 3DS is online, no SD card is involved.

### Option 2: SD card

Drop `aow3ds.cia` anywhere on the SD card, then FBI → `SD` → pick the file → `Install and delete CIA`.

## Build (3DS)

You need devkitPro's 3DS environment (`devkitARM`, `libctru`, `citro2d`). Do this on WSL, Linux or macOS.

```sh
cd 3ds
source env.sh
make clean
make
./build-cia.sh
```

`build-cia.sh` packages the CIA with `makerom` and builds the banner with `bannertool`.
The unique ID is `0x0003FF3F` (a development title ID), so it only installs on custom firmware with signature patches.

Controls and a longer build description are in [`3ds/README.md`](3ds/README.md).

## Folder structure

```
3ds/                    libctru (C++) port. This is the version that actually runs
   gfx/aow/             original sprites resized for the 3DS
   data/*.pcm           original music/SFX (22050 Hz mono PCM16)
   source/audio.cpp     ndsp playback
assets/audio/           original music source (1035.mp3), used for PCM conversion
pygame/                 Python reimplementation. Only the data tables are ported so far (WIP)
install/                QR code PNG + a scan page in HTML
tools/build-assets.ps1  regenerates the 3DS t3x/PCM assets from an original asset checkout
```

The full decompilation is kept in a separate repo (`Age-of-War-Flash-Decompiled`). Check it out under `reference/flash-decompiled/` locally if you need to compare behavior; it is not committed here.

## Asset credits

- Background, base, unit card art and font: resized from the public release assets of [`OtemPsych/Age-of-War`](https://github.com/OtemPsych/Age-of-War).
- Unit/turret/UI/effect sprites and music/SFX: converted from the original assets in [`apiotrowski255/age-of-war`](https://github.com/apiotrowski255/age-of-war).
- Flash decompilation: the Age of War folder from [`erupturatis/Decompiled-flash-games-archive`](https://github.com/erupturatis/Decompiled-flash-games-archive), kept as-is in [`Age-of-War-Flash-Decompiled`](https://github.com/spidychoipro/Age-of-War-Flash-Decompiled), including the original SWF.

Those sources have no license or unresolved rights. None of these assets are covered by this repo's MIT license, and they sit here with their rights unresolved. Details in [`THIRD_PARTY.md`](THIRD_PARTY.md).

The script that generated `3ds/data/aow3ds_font.bcfnt` isn't in this repo. Rebuilding it would need a separate BCFNT conversion tool.

`tools/build-assets.ps1` takes an original asset checkout as `-Source` and regenerates `3ds/gfx/aow/`, `3ds/data/*.pcm` and `3ds/source/art_indices.h`. It needs `magick` and `ffmpeg`. The converted assets are committed, so you normally don't have to run it.

The 3DS code here is an independent implementation, not the original game code moved over. `reference/` is only kept to compare behaviour against the original. Build artifacts go to the releases only and are never committed.

## Status

- 3DS: original-sprite battle on the top screen, base HP / resources / age / production UI on the bottom. 16 unit types, 15 turret types, 5 ages, 3 difficulties, age specials and original audio. Playable through to the end.
- Python: only `src/settings.py`, `src/gamedata.py` and assets. There's no entry point (`main.py`), so it doesn't run.

## Contributing

Issues and PRs are welcome in English or Korean.

- Bug → open an issue first, fix on `fix/issue-<number>`, put `Closes #<number>` in the PR body
- Feature → skip the issue, open a PR straight from `feat/<summary>`
- Build steps, running it, branch/commit conventions, encoding pitfalls → [`CONTRIBUTING.md`](CONTRIBUTING.md)
- Not sure whether something is welcome? Just ask in an issue. This repo is small enough that the questions become the docs

Only commit third-party assets whose license you can name. Open an issue first if it's unclear.

## License

MIT for the source code ([`LICENSE`](LICENSE)). Not applicable to the third-party assets, see [`THIRD_PARTY.md`](THIRD_PARTY.md).

