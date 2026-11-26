#include <3ds.h>
#include <3ds/allocator/linear.h>
#include <string.h>

#include "audio.h"

#include "music_pcm.h"
#include "sfx_hit_pcm.h"
#include "sfx_sword_pcm.h"
#include "sfx_whoosh_pcm.h"
#include "sfx_catapult_pcm.h"
#include "sfx_explosion_pcm.h"
#include "sfx_die_pcm.h"
#include "sfx_die_tank_pcm.h"
#include "sfx_range_medival_pcm.h"
#include "sfx_range_miltary_pcm.h"
#include "sfx_special_pcm.h"
#include "sfx_future_pcm.h"
#include "sfx_turret_pcm.h"
#include "sfx_fire_pcm.h"

namespace
{
	struct Clip
	{
		const void* data;
		u32 size;
		u32 samples;
	};

	const Clip kSfx[SFX_COUNT] =
	{
		{ sfx_hit_pcm,           sfx_hit_pcm_size,           sfx_hit_pcm_size / 2 },
		{ sfx_sword_pcm,         sfx_sword_pcm_size,         sfx_sword_pcm_size / 2 },
		{ sfx_whoosh_pcm,        sfx_whoosh_pcm_size,        sfx_whoosh_pcm_size / 2 },
		{ sfx_catapult_pcm,      sfx_catapult_pcm_size,      sfx_catapult_pcm_size / 2 },
		{ sfx_explosion_pcm,     sfx_explosion_pcm_size,     sfx_explosion_pcm_size / 2 },
		{ sfx_die_pcm,           sfx_die_pcm_size,           sfx_die_pcm_size / 2 },
		{ sfx_die_tank_pcm,      sfx_die_tank_pcm_size,      sfx_die_tank_pcm_size / 2 },
		{ sfx_range_medival_pcm, sfx_range_medival_pcm_size, sfx_range_medival_pcm_size / 2 },
		{ sfx_range_miltary_pcm, sfx_range_miltary_pcm_size, sfx_range_miltary_pcm_size / 2 },
		{ sfx_special_pcm,       sfx_special_pcm_size,       sfx_special_pcm_size / 2 },
		{ sfx_future_pcm,        sfx_future_pcm_size,        sfx_future_pcm_size / 2 },
		{ sfx_turret_pcm,        sfx_turret_pcm_size,        sfx_turret_pcm_size / 2 },
		{ sfx_fire_pcm,          sfx_fire_pcm_size,          sfx_fire_pcm_size / 2 },
	};

	const int SFX_CHANNELS = 6;
	const int MUSIC_CHANNEL = 0;

	bool g_ready = false;
	bool g_musicReady = false;
	Result g_lastResult = 0;
	void* g_musicAlloc = NULL;
	void* g_sfxAlloc[SFX_COUNT] = { 0 };
	const void* g_sfxData[SFX_COUNT] = { 0 };
	ndspWaveBuf g_musicBuf;
	ndspWaveBuf g_sfxBuf[SFX_CHANNELS];
	int g_nextChannel = 1;
	bool g_musicOn = true;

	// NDSP reads wave buffers by physical address. ELF .rodata is not part of
	// the linear mapping, so osConvertVirtToPhys() returns 0 for it and the DSP
	// plays silence. Copy every clip into the linear heap first.
	void* copyToLinear(const void* src, u32 size, void** allocation)
	{
		void* raw = linearAlloc(size + 16);
		if (!raw)
			return NULL;
		u32 aligned = ((u32)raw + 15u) & ~15u;
		memcpy((void*)aligned, src, size);
		*allocation = raw;
		return (void*)aligned;
	}

	void resetChannel(int ch)
	{
		ndspChnReset(ch);
		ndspChnSetInterp(ch, NDSP_INTERP_LINEAR);
		ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
		ndspChnSetRate(ch, 22050.0f);
		float mix[12] = {0};
		mix[0] = 0.9f;
		mix[1] = 0.9f;
		ndspChnSetMix(ch, mix);
	}
}

bool audioReady()
{
	return g_ready;
}

bool audioMusicReady()
{
	return g_musicReady;
}

bool audioMusicOn()
{
	return g_musicOn;
}

unsigned audioLastResult()
{
	return (unsigned)g_lastResult;
}

void audioInit()
{
	g_lastResult = ndspInit();
	if (R_FAILED(g_lastResult))
		return;

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	memset(&g_musicBuf, 0, sizeof(g_musicBuf));
	memset(g_sfxBuf, 0, sizeof(g_sfxBuf));

	const void* music = copyToLinear(music_pcm, music_pcm_size, &g_musicAlloc);
	if (music)
	{
		resetChannel(MUSIC_CHANNEL);
		g_musicBuf.data_vaddr = music;
		g_musicBuf.nsamples = (music_pcm_size / 2) & ~15u;
		g_musicBuf.looping = true;
		g_musicBuf.status = NDSP_WBUF_FREE;
		float musicMix[12] = {0};
		musicMix[0] = 0.55f;
		musicMix[1] = 0.55f;
		ndspChnSetMix(MUSIC_CHANNEL, musicMix);
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &g_musicBuf);
		g_musicReady = true;
	}

	for (int i = 0; i < SFX_COUNT; i++)
		g_sfxData[i] = copyToLinear(kSfx[i].data, kSfx[i].size, &g_sfxAlloc[i]);

	g_ready = true;
}

void audioExit()
{
	if (!g_ready)
		return;
	ndspChnWaveBufClear(MUSIC_CHANNEL);
	for (int i = 0; i < SFX_CHANNELS; i++)
		ndspChnWaveBufClear(1 + i);
	ndspExit();
	if (g_musicAlloc) linearFree(g_musicAlloc);
	for (int i = 0; i < SFX_COUNT; i++)
		if (g_sfxAlloc[i]) linearFree(g_sfxAlloc[i]);
	g_musicAlloc = NULL;
	for (int i = 0; i < SFX_COUNT; i++)
	{
		g_sfxAlloc[i] = NULL;
		g_sfxData[i] = NULL;
	}
	g_musicReady = false;
	g_ready = false;
}

void audioPlay(SfxId id)
{
	if (!g_ready || id < 0 || id >= SFX_COUNT || !g_sfxData[id])
		return;

	int ch = g_nextChannel;
	g_nextChannel++;
	if (g_nextChannel >= 1 + SFX_CHANNELS)
		g_nextChannel = 1;

	// Reuse only a channel whose sample has finished; this keeps rapid hits
	// from truncating each other.
	if (g_sfxBuf[ch - 1].status == NDSP_WBUF_QUEUED ||
	    g_sfxBuf[ch - 1].status == NDSP_WBUF_PLAYING)
	{
		return;
	}

	resetChannel(ch);
	ndspWaveBuf& buf = g_sfxBuf[ch - 1];
	buf.data_vaddr = g_sfxData[id];
	buf.nsamples = kSfx[id].samples & ~15u;
	buf.looping = false;
	buf.status = NDSP_WBUF_FREE;
	ndspChnWaveBufAdd(ch, &buf);
}

void audioSetMusic(bool on)
{
	if (!g_ready || !g_musicReady || g_musicOn == on)
		return;
	g_musicOn = on;
	if (on)
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &g_musicBuf);
	else
		ndspChnWaveBufClear(MUSIC_CHANNEL);
}
