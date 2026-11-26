// Minimal ndsp player for the original Age of War soundtrack and effects.
// Audio is optional: if the DSP firmware is unavailable the game keeps running.
#pragma once

enum SfxId
{
	SFX_HIT = 0,
	SFX_SWORD,
	SFX_WHOOSH,
	SFX_CATAPULT,
	SFX_EXPLOSION,
	SFX_DIE,
	SFX_DIE_TANK,
	SFX_RANGE_MEDIVAL,
	SFX_RANGE_MILTARY,
	SFX_SPECIAL,
	SFX_FUTURE,
	SFX_TURRET,
	SFX_FIRE,
	SFX_COUNT
};

void audioInit();
void audioExit();
void audioPlay(SfxId id);
void audioSetMusic(bool on);
bool audioReady();
bool audioMusicReady();
bool audioMusicOn();
unsigned audioLastResult();
