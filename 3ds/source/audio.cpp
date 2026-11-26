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
	const int MUSIC_BUFFERS = 2;
	const u32 MUSIC_CHUNK = 16384;

	bool g_ready = false;
	bool g_musicReady = false;
	Result g_lastResult = 0;
	void* g_musicAlloc = NULL;
	const void* g_musicData = NULL;
	u32 g_musicSamples = 0;
	u32 g_nextMusicChunk = 0;
	void* g_sfxAlloc[SFX_COUNT] = { 0 };
	const void* g_sfxData[SFX_COUNT] = { 0 };
	ndspWaveBuf g_musicBuf[MUSIC_BUFFERS];
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

	void setMusicMix()
	{
		float mix[12] = {0};
		mix[0] = 0.55f;
		mix[1] = 0.55f;
		ndspChnSetMix(MUSIC_CHANNEL, mix);
	}

	void queueMusicChunk(int slot)
	{
		if (!g_musicReady || !g_musicData || g_musicSamples == 0)
			return;
		u32 start = g_nextMusicChunk * MUSIC_CHUNK;
		if (start >= g_musicSamples)
			start = 0;
		u32 remaining = g_musicSamples - start;
		u32 count = remaining < MUSIC_CHUNK ? remaining : MUSIC_CHUNK;
		count &= ~15u;
		if (count == 0)
		{
			start = 0;
			count = MUSIC_CHUNK;
		}
		ndspWaveBuf& buf = g_musicBuf[slot];
		buf.data_vaddr = (const void*)((const u8*)g_musicData + (size_t)start * 2);
		buf.nsamples = count;
		buf.looping = false;
		buf.status = NDSP_WBUF_FREE;
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &buf);
		g_nextMusicChunk = start / MUSIC_CHUNK + 1;
		if (g_nextMusicChunk * MUSIC_CHUNK >= g_musicSamples)
			g_nextMusicChunk = 0;
	}

	void restartMusic()
	{
		ndspChnWaveBufClear(MUSIC_CHANNEL);
		resetChannel(MUSIC_CHANNEL);
		setMusicMix();
		memset(g_musicBuf, 0, sizeof(g_musicBuf));
		g_nextMusicChunk = 0;
		queueMusicChunk(0);
		queueMusicChunk(1);
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
	memset(g_musicBuf, 0, sizeof(g_musicBuf));
	memset(g_sfxBuf, 0, sizeof(g_sfxBuf));

	const void* music = copyToLinear(music_pcm, music_pcm_size, &g_musicAlloc);
	if (music)
	{
		g_musicData = music;
		g_musicSamples = (music_pcm_size / 2) & ~15u;
		g_musicReady = true;
		restartMusic();
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
	g_musicData = NULL;
	g_musicSamples = 0;
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

void audioUpdate()
{
	if (!g_ready || !g_musicReady || !g_musicOn)
		return;
	for (int i = 0; i < MUSIC_BUFFERS; i++)
	{
		if (g_musicBuf[i].status == NDSP_WBUF_DONE)
			queueMusicChunk(i);
	}
}

void audioSetMusic(bool on)
{
	if (!g_ready || !g_musicReady || g_musicOn == on)
		return;
	g_musicOn = on;
	if (on)
		restartMusic();
	else
		ndspChnWaveBufClear(MUSIC_CHANNEL);
}
