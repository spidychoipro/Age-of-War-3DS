#!/usr/bin/env bash
# Build aow3ds and package it as a dev (unititled) CIA.
set -euo pipefail

export DEVKITPRO="${DEVKITPRO:-$HOME/devkitpro}"
export DEVKITARM="$DEVKITPRO/devkitARM"
export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH"

cd "$(dirname "$0")"

UNIQUE_ID=0x0003FF3F

make -j"$(nproc)" "$@" ELF="$PWD/aow3ds.elf" 3dsx="$PWD/aow3ds.3dsx"

# bannertool (carstene1ns C++ v1.2.3) only accepts image/audio/output and
# requires a WAV (not a CWAV). Titles come from the RSF instead.
[ -f silence.wav ] || python3 -c "import wave;w=wave.open('silence.wav','wb');w.setnchannels(1);w.setsampwidth(2);w.setframerate(8000);w.writeframes(b'\0\0'*4000);w.close()"

bannertool makebanner \
	-i banner.png \
	-a silence.wav \
	-o banner.bnr

# makebanner only writes the banner image/audio. The title shown by 3DS menus
# and emulators lives in the SMDH, so generate it explicitly.
bannertool makesmdh \
	-s "Age of War" \
	-l "Age of War 3DS" \
	-p "homebrew" \
	-i icon.png \
	-o icon.smdh

# makerom v0.17 RSF: only the keys accepted by rsf_settings.c.
cat > aow3ds.rsf <<EOF
BasicInfo:
  Title             : "Age of War 3DS"
  CompanyCode       : "AO"
  ProductCode       : "CTR-P-AOWV"
  ContentType       : "Application"
  Logo              : "homebrew"
SystemControlInfo:
  AppType           : 1
  StackSize         : 0x40000
  SaveDataSize      : 0K
  JumpId            : 0
  RemasterVersion   : 0
TitleInfo:
  Category          : Application
  UniqueId          : ${UNIQUE_ID}
  Version           : 0
EOF

# Build the NCCH first, then package it with a fixed content id. Without the
# explicit :0, makerom picks a random TMD content id and Azahar names the
# installed file after it (e.g. 411f1d75.app).
makerom -f cxi -o aow3ds.cxi \
	-rsf aow3ds.rsf \
	-desc App:33 \
	-target d \
	-elf aow3ds.elf \
	-icon icon.smdh \
	-banner banner.bnr

makerom -f cia -o aow3ds.cia \
	-rsf aow3ds.rsf \
	-target d \
	-content aow3ds.cxi:0:0 \
	-icon icon.smdh \
	-banner banner.bnr

ls -la aow3ds.cia
