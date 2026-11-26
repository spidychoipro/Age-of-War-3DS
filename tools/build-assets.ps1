# Convert the original Age of War assets from a reference checkout into
# textures and raw PCM that the 3DS build embeds.
#
#   pwsh tools/build-assets.ps1 -Source "path\to\age of war sprites"
#
# Requires ImageMagick (magick) and ffmpeg on PATH. Output goes to
# 3ds/gfx/aow/ and 3ds/data/, and regenerates 3ds/source/art_indices.h.

param(
    [Parameter(Mandatory = $true)][string]$Source,
    [string]$Repo = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$gfx = Join-Path $Repo '3ds\gfx\aow'
$data = Join-Path $Repo '3ds\data'
$srcDir = Join-Path $Repo '3ds\source'
New-Item -ItemType Directory -Force -Path $gfx | Out-Null
New-Item -ItemType Directory -Force -Path $data | Out-Null
Get-ChildItem -File -Path $gfx | Remove-Item -Force

$names = [System.Collections.Generic.List[string]]::new()
$lines = [System.Collections.Generic.List[string]]::new()

function Add-Sprite {
    param([string]$Name, [string]$File, [int]$W, [int]$H)
    if ($env:AOW_DEBUG) { Write-Host "sprite $Name <- $File" }
    if (-not (Test-Path -LiteralPath $File)) { throw "missing asset: [$File]" }
    $out = Join-Path $gfx ($Name + '.png')
    # Composite over opaque black so fully transparent pixels carry no RGB.
    # Otherwise tex3ds can bleed white halos into the atlas padding.
    & magick -size "${W}x${H}" xc:black '(' $File -background none -resize "${W}x${H}" -gravity center -extent "${W}x${H}" ')' -compose Over -composite PNG32:$out
    if ($LASTEXITCODE -ne 0) { throw "magick failed for $File" }
    $names.Add($Name)
    $lines.Add("#define $Name $($names.Count - 1)")
}

function Pick-Frames {
    param([string]$Dir, [int]$Count)
    $files = @(Get-ChildItem -File -Filter '*.png' -LiteralPath $Dir | Sort-Object Name)
    if ($files.Count -eq 0) { throw "no frames in $Dir" }
    $result = @()
    if ($Count -eq 1) { return @($files[0].FullName) }
    for ($i = 0; $i -lt $Count; $i++) {
        $idx = [int][math]::Round(($i * ($files.Count - 1)) / [double]($Count - 1))
        $result += $files[$idx].FullName
    }
    return $result
}

$ages = @('cave', 'knight', 'medival', 'miltary', 'future')
$kinds = @('melee', 'range', 'tank')

# 15 unit archetypes, four walk and four attack frames each. The 16th unit
# (Super Soldier) reuses the future ranged art, as it does in the original.
for ($a = 0; $a -lt 5; $a++) {
    for ($k = 0; $k -lt 3; $k++) {
        $base = 'A_U_{0:d2}_{1}' -f $a, $k
        $unitDir = Join-Path $Source ('units\' + $ages[$a] + '\' + $kinds[$k])
        $walk = Pick-Frames (Join-Path $unitDir 'walk') 4
        $atkDir = Join-Path $unitDir 'attack'
        if (-not (Test-Path -LiteralPath $atkDir)) { $atkDir = Join-Path $unitDir 'idle_attack' }
        if (-not (Test-Path -LiteralPath $atkDir)) { $atkDir = Join-Path $unitDir 'walk_attack' }
        $attack = Pick-Frames $atkDir 4
        for ($f = 0; $f -lt 4; $f++) {
            Add-Sprite ($base + '_W' + $f) $walk[$f] 48 60
        }
        for ($f = 0; $f -lt 4; $f++) {
            Add-Sprite ($base + '_A' + $f) $attack[$f] 48 60
        }
    }
}

for ($a = 0; $a -lt 5; $a++) {
    Add-Sprite ('A_BASE_' + $a) (Join-Path $Source ('bases\' + $ages[$a] + '\base\base.png')) 56 48
    for ($t = 1; $t -le 3; $t++) {
        $dir = Join-Path $Source ('bases\' + $ages[$a] + '\turret_' + $t)
        $frames = @(Pick-Frames $dir 1)
        Add-Sprite ('A_TURRET_' + $a + '_' + ($t - 1)) $frames[0] 40 40
    }
}

$ui = Join-Path $Source 'ui'
Add-Sprite 'A_UI_ROOT' (Join-Path $ui 'root_ui.png') 320 44
Add-Sprite 'A_UI_TOP' (Join-Path $ui 'top_ui.png') 320 48
Add-Sprite 'A_UI_COIN' (Join-Path $ui 'coin.png') 20 20
Add-Sprite 'A_UI_SELL' (Join-Path $ui 'sell.png') 24 40
Add-Sprite 'A_UI_MENU_BG' (Join-Path $ui 'menu.png') 320 180
for ($i = 1; $i -le 5; $i++) {
    Add-Sprite ('A_UI_UNITS_' + ($i - 1)) (Join-Path $ui ('units_buttons{0:d4}.png' -f $i)) 320 40
    Add-Sprite ('A_UI_TURRETS_' + ($i - 1)) (Join-Path $ui ('all_turret_UI{0:d4}.png' -f $i)) 320 40
    Add-Sprite ('A_UI_SPECIAL_' + ($i - 1)) (Join-Path $ui ('special_buttons{0:d4}.png' -f $i)) 120 40
}

$fx = Join-Path $Source 'effects'
$fxSets = @(
    @{ Name = 'EXPLOSION'; Dir = 'explosion'; Count = 4 },
    @{ Name = 'CAVE'; Dir = 'cave_special_projectile'; Count = 4 },
    @{ Name = 'ARROW'; Dir = 'knight_special_arrow'; Count = 4 },
    @{ Name = 'LASER'; Dir = 'future_special'; Count = 4 }
)
foreach ($set in $fxSets) {
    $frames = Pick-Frames (Join-Path $fx $set.Dir) $set.Count
    for ($f = 0; $f -lt $set.Count; $f++) {
        Add-Sprite ('A_FX_' + $set.Name + '_' + $f) $frames[$f] 32 32
    }
}
Add-Sprite 'A_FX_HEAL' (Join-Path $fx 'heal\medival_special_heal.png') 32 32
Add-Sprite 'A_UI_LOGO' (Join-Path $fx 'logo\main menu0001.png') 192 72
$plane = @(Pick-Frames (Join-Path $fx 'miltary_special_plane') 1)
Add-Sprite 'A_FX_PLANE' $plane[0] 48 32

# tex3ds atlas descriptor. rgba4444 keeps the sheets small enough for CIA/3DSX
# while retaining the transparency the original sprites need.
$list = @('--atlas -f rgba4444 -z auto', '')
foreach ($name in $names) { $list += ('aow/' + $name + '.png') }
Set-Content -LiteralPath (Join-Path $Repo '3ds\gfx\aow.t3s') -Value $list -Encoding UTF8

$header = @('// Generated by tools/build-assets.ps1. Do not edit by hand.', '') + $lines + @('', "#define A_COUNT $($names.Count)")
Set-Content -LiteralPath (Join-Path $srcDir 'art_indices.h') -Value $header -Encoding UTF8

# Audio: 22.05 kHz mono signed 16-bit little endian, the format ndsp consumes
# directly. No extra decoder is needed on the console.
$audio = Join-Path $Source 'audio'
$sounds = @(
    @{ Name = 'music'; Path = (Join-Path $Repo 'assets\audio\1035.mp3') },
    @{ Name = 'sfx_hit'; File = 'sfx\stab_01.mp3' },
    @{ Name = 'sfx_sword'; File = 'sfx\sword_clash_01.mp3' },
    @{ Name = 'sfx_whoosh'; File = 'sfx\whoosh_01.mp3' },
    @{ Name = 'sfx_catapult'; File = 'sfx\catapult.mp3' },
    @{ Name = 'sfx_explosion'; File = 'sfx\explosion_01.mp3' },
    @{ Name = 'sfx_die'; File = 'sfx\die_01.mp3' },
    @{ Name = 'sfx_die_tank'; File = 'sfx\cave_tank_die.mp3' },
    @{ Name = 'sfx_range_medival'; File = 'sfx\medival_range_attack.mp3' },
    @{ Name = 'sfx_range_miltary'; File = 'sfx\miltary_range_attack.mp3' },
    @{ Name = 'sfx_special'; File = 'sfx\miltary_special_attack.mp3' },
    @{ Name = 'sfx_future'; File = 'sfx\future_tank_attack.mp3' },
    @{ Name = 'sfx_turret'; File = 'sfx\cave_turret_2_attack.mp3' },
    @{ Name = 'sfx_fire'; File = 'sfx\fire_01.mp3' }
)
foreach ($sound in $sounds) {
    $in = if ($sound.ContainsKey('Path')) { $sound.Path } else { Join-Path $audio $sound.File }
    if (-not (Test-Path -LiteralPath $in)) { throw "missing sound: $in" }
    $out = Join-Path $data ($sound.Name + '.pcm')
    & ffmpeg -hide_banner -loglevel error -y -i $in -ac 1 -ar 22050 -f s16le $out
    if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed for $in" }
}

Write-Host "Generated $($names.Count) sprites and $($sounds.Count) sounds."
