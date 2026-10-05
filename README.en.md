[English README](README.en.md) | [한국어 README](README.md)

# Age of War 3DS

A 2D defense game rebuilt for the Nintendo 3DS (cartridge build, devkitARM). Ported with C++/libctru, and a Python (pygame) reimplementation of the same pieces is kept in this repo too.

> [!NOTE]
> **An unofficial fan port, made by one person.** Not affiliated with the original developers or publishers; the original name is only used for attribution.
>
> The layout and the balance are a fair bit off from the original and it isn't finished, but it's playable start to finish.
>
> If you want it closer to the real game, see [CONTRIBUTING.md](CONTRIBUTING.md). What's missing is listed under [Differences from the original](#differences-from-the-original) below.

## Differences from the original

The balance tables (`3ds/source/data.h`) are the original's, and the UI does show all of them — 16 unit types, 15 turret types, 5 ages (Caveman → Medieval → Renaissance → WW2 → Future).

What's not there yet:

- Difficulty selection. The rules are in `sim.cpp` with three levels defined (harder ×1.3, impossible ×2.0), but `main.cpp` hardcodes `g_difficulty = 0`, so there's no way to pick one in-game.
- Saving and replays. The repo is never touched and the CIA sets `SaveDataSize: 0K`, so progress isn't kept anywhere.
- Music and sound effects. There is no audio code in `3ds/source/` at all.
- Turret upgrades. Only slot expansion (`simAddExpansion`) exists.
- The `pygame/` side. Just the data tables, so it doesn't run.

If you know the original well, filling these in would be the most useful thing you could do. Where to start is in [CONTRIBUTING.md](CONTRIBUTING.md#making-it-closer-to-the-original).

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
3ds/       libctru (C++) port. This is the version that actually runs
pygame/    Python reimplementation. Only the data tables are ported so far (WIP)
install/   QR code PNG + a scan page in HTML
```

## Asset credits

The background, base and unit card art and the font are resized and converted from the public release assets of [`OtemPsych/Age-of-War`](https://github.com/OtemPsych/Age-of-War) for 3DS. The original music and sound effects are not wired up yet because of file size.

That source repo ships no license. So those assets are not covered by this repo's MIT license, and they sit here with their rights unresolved. Details in [`THIRD_PARTY.md`](THIRD_PARTY.md).

The script that generated `3ds/data/aow3ds_font.bcfnt` isn't in this repo. Rebuilding it would need a separate BCFNT conversion tool.

The code here is an independent implementation, not the original game code moved over. Build artifacts go to the releases only and are never committed.

## Status

- 3DS: battle on the top screen, base HP / resources / age / production UI on the bottom. 16 unit types, 15 turret types, 5 ages. Playable through to the end.
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

