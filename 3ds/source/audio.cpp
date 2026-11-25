#include <3ds.h>
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
		u32 samples;
	};

	const Clip kSfx[SFX_COUNT] =
	{
		{ sfx_hit_pcm,           sfx_hit_pcm_size / 2 },
		{ sfx_sword_pcm,         sfx_sword_pcm_size / 2 },
		{ sfx_whoosh_pcm,        sfx_whoosh_pcm_size / 2 },
		{ sfx_catapult_pcm,      sfx_catapult_pcm_size / 2 },
		{ sfx_explosion_pcm,     sfx_explosion_pcm_size / 2 },
		{ sfx_die_pcm,           sfx_die_pcm_size / 2 },
		{ sfx_die_tank_pcm,      sfx_die_tank_pcm_size / 2 },
		{ sfx_range_medival_pcm, sfx_range_medival_pcm_size / 2 },
		{ sfx_range_miltary_pcm, sfx_range_miltary_pcm_size / 2 },
		{ sfx_special_pcm,       sfx_special_pcm_size / 2 },
		{ sfx_future_pcm,        sfx_future_pcm_size / 2 },
		{ sfx_turret_pcm,        sfx_turret_pcm_size / 2 },
		{ sfx_fire_pcm,          sfx_fire_pcm_size / 2 },
	};

	const int SFX_CHANNELS = 6;
	const int MUSIC_CHANNEL = 0;

	bool g_ready = false;
	ndspWaveBuf g_musicBuf;
	ndspWaveBuf g_sfxBuf[SFX_CHANNELS];
	int g_nextChannel = 1;
	bool g_musicOn = true;

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

bool audioMusicOn()
{
	return g_musicOn;
}

void audioInit()
{
	if (R_FAILED(ndspInit()))
		return;

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	memset(&g_musicBuf, 0, sizeof(g_musicBuf));
	memset(g_sfxBuf, 0, sizeof(g_sfxBuf));

	resetChannel(MUSIC_CHANNEL);
	g_musicBuf.data_vaddr = music_pcm;
	g_musicBuf.nsamples = music_pcm_size / 2;
	g_musicBuf.looping = true;
	g_musicBuf.status = NDSP_WBUF_FREE;
	float musicMix[12] = {0};
	musicMix[0] = 0.32f;
	musicMix[1] = 0.32f;
	ndspChnSetMix(MUSIC_CHANNEL, musicMix);
	ndspChnWaveBufAdd(MUSIC_CHANNEL, &g_musicBuf);

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
	g_ready = false;
}

void audioPlay(SfxId id)
{
	if (!g_ready || id < 0 || id >= SFX_COUNT)
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
	buf.data_vaddr = kSfx[id].data;
	buf.nsamples = kSfx[id].samples;
	buf.looping = false;
	buf.status = NDSP_WBUF_FREE;
	ndspChnWaveBufAdd(ch, &buf);
}

void audioSetMusic(bool on)
{
	if (!g_ready || g_musicOn == on)
		return;
	g_musicOn = on;
	if (on)
		ndspChnWaveBufAdd(MUSIC_CHANNEL, &g_musicBuf);
	else
		ndspChnWaveBufClear(MUSIC_CHANNEL);
}
