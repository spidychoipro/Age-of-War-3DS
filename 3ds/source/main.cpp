// Age of War - Nintendo 3DS port
// Top screen: battlefield. Bottom screen: original-style touch HUD.
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "sim.h"
#include "audio.h"
#include "art_indices.h"
#include "background_t3x.h"
#include "aow_t3x.h"
#include "aow3ds_font_bcfnt.h"

#define SCRW 320.0f
#define SCRH 240.0f
#define FIELDW 400.0f

static C2D_TextBuf g_textBuf;
static C2D_Text g_text;
static C2D_Font g_font;
static C3D_RenderTarget* g_bottom;
static C3D_RenderTarget* g_top;
static C2D_SpriteSheet g_backgroundSheet;
static C2D_SpriteSheet g_artSheet;
static C2D_Image g_backgroundImage;
static C2D_Image g_art[A_COUNT];

static bool g_consoleUp = false;
static Sim g_sim;
static int g_difficulty = 0;
static int g_menuDiff = 0;
static bool g_title = true;
static bool g_help = false;
static bool g_confirmMenu = false;
static int g_mode = 0;              // 0 = units, 1 = turrets
static int g_selected = -1;
static int g_placeType = -1;        // turret type awaiting placement
static bool g_sellMode = false;
static unsigned g_frame = 0;
static float g_accum = 0.0f;
static unsigned g_fps = 60;
static unsigned g_fpsFrames = 0;
static unsigned g_fpsTick = 0;
static float g_sfxCooldown = 0.0f;
static const float FIXED_DT = 1.0f / 50.0f;

#define FLASH_SLOTS 128
static int g_prevHp[FLASH_SLOTS];
static float g_flash[FLASH_SLOTS];
static bool g_flashInit = false;

static const u32 C_PANEL    = C2D_Color32(0x1E, 0x24, 0x2E, 0xFF);
static const u32 C_PANEL_L  = C2D_Color32(0x33, 0x3C, 0x4A, 0xFF);
static const u32 C_TEXT     = C2D_Color32(0xF8, 0xF2, 0xD8, 0xFF);
static const u32 C_TEXT_DIM = C2D_Color32(0xB0, 0xA8, 0x90, 0xFF);
static const u32 C_GOOD     = C2D_Color32(0x70, 0xD0, 0xA0, 0xFF);
static const u32 C_BAD      = C2D_Color32(0xFF, 0x60, 0x50, 0xFF);
static const u32 C_HP       = C2D_Color32(0x50, 0xE0, 0x60, 0xFF);
static const u32 C_HP_LOW   = C2D_Color32(0xE0, 0x40, 0x40, 0xFF);
static const u32 C_GOLD     = C2D_Color32(0xFF, 0xD0, 0x40, 0xFF);
static const u32 C_XP       = C2D_Color32(0xC0, 0xFF, 0x60, 0xFF);
static const u32 C_BAR      = C2D_Color32(0x18, 0x18, 0x18, 0xFF);

static const u32 AGE_COLOR[5] =
{
	C2D_Color32(0xC8, 0xA0, 0x60, 0xFF),
	C2D_Color32(0x90, 0xB0, 0x50, 0xFF),
	C2D_Color32(0x50, 0xB0, 0xE0, 0xFF),
	C2D_Color32(0x60, 0xD0, 0x70, 0xFF),
	C2D_Color32(0xD0, 0x60, 0xF0, 0xFF),
};

static float wx(float x)
{
	return 50.0f + x * 15.0f;
}

static void drawText(const char* str, float x, float y, float scale, u32 color)
{
	C2D_TextFontParse(&g_text, g_font, g_textBuf, str);
	C2D_DrawText(&g_text, 0, x, y, 0.0f, scale, scale, color);
}

static float textWidth(const char* str, float scale)
{
	C2D_TextFontParse(&g_text, g_font, g_textBuf, str);
	return (float)g_text.width * scale;
}

static void drawButton(float x, float y, float w, float h, const char* label,
                       bool active, bool enabled, float scale)
{
	C2D_DrawRectSolid(x, y, 0.0f, w, h, enabled ? (active ? C_GOOD : C_PANEL_L) : C_BAR);
	C2D_DrawRectSolid(x, y, 0.0f, w, 1.0f, active ? C_TEXT : C_PANEL);
	float tw = textWidth(label, scale);
	float tx = x + (w - tw) * 0.5f;
	if (tx < x + 2.0f) tx = x + 2.0f;
	drawText(label, tx, y + (h - 12.0f * scale) * 0.5f, scale,
	         enabled ? C_TEXT : C_TEXT_DIM);
}

static void drawHealthBar(float x, float y, float w, float ratio)
{
	if (ratio < 0.0f) ratio = 0.0f;
	if (ratio > 1.0f) ratio = 1.0f;
	C2D_DrawRectSolid(x - 1.0f, y - 1.0f, 0.0f, w + 2.0f, 4.0f, C_BAR);
	C2D_DrawRectSolid(x, y, 0.0f, w * ratio, 2.0f, ratio > 0.35f ? C_HP : C_HP_LOW);
}

static void drawSprite(int idx, float x, float y, float w, float h, bool flip)
{
	if (idx < 0 || idx >= A_COUNT || !g_art[idx].tex)
		return;
	float sx = w / (float)g_art[idx].subtex->width;
	float sy = h / (float)g_art[idx].subtex->height;
	if (flip)
		C2D_DrawImageAt(g_art[idx], x + w, y, 0.0f, NULL, -sx, sy);
	else
		C2D_DrawImageAt(g_art[idx], x, y, 0.0f, NULL, sx, sy);
}

static int unitAge(int type)
{
	return type < 15 ? type / 3 : 4;
}

static int unitKind(int type)
{
	return type < 15 ? type % 3 : 1;
}

#define ART_U(age, kind) ((age) * 24 + (kind) * 8)

static void noteDamage(Unit& u, float dt)
{
	if (!g_flashInit)
	{
		for (int i = 0; i < FLASH_SLOTS; i++)
		{
			g_prevHp[i] = -1;
			g_flash[i] = 0.0f;
		}
		g_flashInit = true;
	}
	int slot = ((int)u.id % FLASH_SLOTS + FLASH_SLOTS) % FLASH_SLOTS;
	if (g_prevHp[slot] >= 0 && u.health < (float)g_prevHp[slot])
	{
		g_flash[slot] = 0.18f;
		if (g_sfxCooldown <= 0.0f && !u.isBase)
		{
			audioPlay(u.typeId % 3 == 2 ? SFX_CATAPULT : SFX_HIT);
			g_sfxCooldown = 0.08f;
		}
	}
	g_prevHp[slot] = (int)u.health;
	if (g_flash[slot] > 0.0f)
	{
		g_flash[slot] -= dt;
		if (g_flash[slot] < 0.0f) g_flash[slot] = 0.0f;
	}
}

// --- battlefield -----------------------------------------------------------

static void drawBattlefield(const Sim& s)
{
	if (g_backgroundImage.tex)
		C2D_DrawImageAt(g_backgroundImage, 0.0f, 0.0f, 0.0f);
	else
		C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, FIELDW, SCRH, C_PANEL);

	// The original game's ground line is the visual anchor for every sprite.
	C2D_DrawRectSolid(0.0f, 140.0f, 0.0f, FIELDW, 1.0f, C2D_Color32(0x30, 0x22, 0x14, 0x80));
	if (s.age > 0)
	{
		char buf[24];
		snprintf(buf, sizeof(buf), "%s", AGE_NAMES[s.age]);
		drawText(buf, 8.0f, 6.0f, 0.55f, AGE_COLOR[s.age]);
	}
}

static void drawBases(const Sim& s)
{
	for (int i = 0; i < s.unitCount; i++)
	{
		const Unit& b = s.units[i];
		if (!b.isBase)
			continue;
		bool friendly = b.team == TEAM_GOOD;
		float cx = wx(b.x);
		int artBase = A_BASE_0 + (friendly ? s.age : (s.techLevel > 4 ? 4 : s.techLevel - 1));
		drawSprite(artBase, cx - 28.0f, 94.0f, 56.0f, 48.0f, !friendly);
		drawHealthBar(cx - 26.0f, 88.0f, 52.0f, b.health / b.maxHealth);

		int placed = 0;
		for (int t = 0; t < MAX_TURRETS; t++)
		{
			if (!s.turrets[b.team][t].alive)
				continue;
			int ta = A_TURRET_0_0 + s.turrets[b.team][t].type;
			float tx = cx - 20.0f + (float)placed * 13.0f;
			drawSprite(ta, tx, 80.0f, 24.0f, 24.0f, !friendly);
			placed++;
		}
		for (int e = 0; e <= s.expansionLevel[b.team] && e < MAX_TURRETS; e++)
		{
			if (s.turrets[b.team][e].alive)
				continue;
			C2D_DrawRectSolid(cx - 18.0f + (float)e * 13.0f, 88.0f, 0.0f, 10.0f, 6.0f,
			                  friendly ? C2D_Color32(0xFF, 0xD0, 0x40, 0x60) : C2D_Color32(0x40, 0x40, 0x40, 0x60));
		}
	}
}

static void drawUnit(Unit& u, const Sim& s)
{
	const UnitDef& d = UNIT_DEFS[u.typeId];
	int age = unitAge(u.typeId);
	int kind = unitKind(u.typeId);
	bool friendly = u.team == TEAM_GOOD;
	bool attacking = u.attackTimer > d.attackTime * 0.55f || u.rangedTimer > 0.0f;
	int anim = attacking ? 4 : 0;
	int frame = (int)((g_frame / (attacking ? 4u : 7u) + (unsigned)u.id) & 3u);
	int art = ART_U(age, kind) + anim + frame;

	float h = 46.0f;
	float w = h * 0.8f;
	float cx = wx(u.x);
	float y = 140.0f - h;
	if (attacking)
		cx += u.dir * 2.0f;

	if (u.id == g_selected)
	{
		C2D_DrawCircleSolid(cx, 140.0f, 0.0f, 13.0f, C_GOLD);
		float mr = d.meleeRange * P2U * 15.0f;
		if (mr > 2.0f)
			C2D_DrawCircleSolid(cx, 140.0f, 0.0f, mr, C2D_Color32(0xFF, 0xD0, 0x40, 0x30));
	}

	drawSprite(art, cx - w * 0.5f, y, w, h, !friendly);
	int slot = ((int)u.id % FLASH_SLOTS + FLASH_SLOTS) % FLASH_SLOTS;
	if (g_flash[slot] > 0.0f)
		C2D_DrawRectSolid(cx - w * 0.5f, y, 0.0f, w, h,
		                  C2D_Color32(0xFF, 0xFF, 0xFF, (u8)(110.0f * g_flash[slot] / 0.18f)));
	drawHealthBar(cx - w * 0.5f, y - 4.0f, w, u.health / u.maxHealth);
	if (u.healing)
		drawSprite(A_FX_HEAL, cx - 8.0f, y - 18.0f, 16.0f, 16.0f, false);
	(void)s;
}

static void drawBullets(const Sim& s)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (!s.bullets[i].alive)
			continue;
		const Bullet& b = s.bullets[i];
		float y = 140.0f - (b.y - GROUND_LEVEL) * 15.0f;
		float x = wx(b.x);
		u32 col = (b.team == TEAM_BAD) ? C_GOLD : C_TEXT;
		C2D_DrawLine(x - b.vx * 1.6f, y, col, x, y, col, 1.0f, 0.0f);
		C2D_DrawCircleSolid(x, y, 0.0f, 1.5f, col);
	}
}

static void drawSpecial(const Sim& s)
{
	if (s.spell.kind == 0)
		return;
	int base, count;
	switch (s.spell.kind)
	{
	case 1: base = A_FX_CAVE_0;   count = 4; break;
	case 2: base = A_FX_ARROW_0;  count = 4; break;
	case 4: base = A_FX_PLANE;    count = 1; break;
	case 5: base = A_FX_LASER_0;  count = 4; break;
	default: base = A_FX_HEAL;    count = 1; break;
	}
	int frame = (int)((g_frame / 4u) & 3u);
	int sprite = base + (count > 1 ? frame : 0);
	if (s.spell.kind == 3)
	{
		for (int i = 0; i < s.unitCount; i++)
			if (s.units[i].alive && s.units[i].team == s.spell.team)
				drawSprite(sprite, wx(s.units[i].x) - 10.0f, 100.0f, 20.0f, 20.0f, false);
		return;
	}
	if (s.spell.kind == 4)
	{
		drawSprite(sprite, 20.0f, 40.0f, 64.0f, 42.0f, false);
		C2D_DrawLine(20.0f, 70.0f, C_GOLD, 370.0f, 70.0f, C_GOLD, 1.0f, 0.0f);
		return;
	}
	for (int i = 0; i < 7; i++)
	{
		float x = 20.0f + (float)((i * 61 + (int)(g_frame / 3u)) % 360);
		float y = 16.0f + (float)((i * 37 + (int)(g_frame / 5u)) % 70);
		drawSprite(sprite, x, y, 26.0f, 26.0f, false);
	}
}

// --- bottom HUD ------------------------------------------------------------

static void drawHudTop(const Sim& s)
{
	drawSprite(A_UI_TOP, 0.0f, 0.0f, SCRW, 59.0f, false);
	drawButton(3.0f, 2.0f, 45.0f, 24.0f, g_mode == 0 ? "UNIT" : "TURRET", g_mode == 1, true, 0.42f);
	drawButton(50.0f, 2.0f, 45.0f, 24.0f, "AGE", false,
	           s.age < AGE_FUTURE && s.exp >= AGE_XP[s.age], 0.48f);
	drawButton(97.0f, 2.0f, 45.0f, 24.0f, "SPECIAL", false,
	           s.specialTimer >= SPECIAL_COOLDOWN && s.spell.kind == 0, 0.40f);
	drawButton(144.0f, 2.0f, 45.0f, 24.0f, "SLOTS", false,
	           s.expansionLevel[TEAM_GOOD] < 3 && s.coins >= EXPANSION_COST[s.expansionLevel[TEAM_GOOD]], 0.44f);
	drawButton(191.0f, 2.0f, 45.0f, 24.0f, "MENU", false, true, 0.44f);

	char buf[64];
	snprintf(buf, sizeof(buf), "%d", (int)s.coins);
	drawSprite(A_UI_COIN, 240.0f, 4.0f, 16.0f, 16.0f, false);
	drawText(buf, 258.0f, 3.0f, 0.52f, C_GOLD);
	snprintf(buf, sizeof(buf), "XP %d", (int)s.exp);
	drawText(buf, 240.0f, 22.0f, 0.48f, C_XP);
	drawText(AGE_NAMES[s.age], 240.0f, 38.0f, 0.46f, AGE_COLOR[s.age]);
}

static void drawQueue(const Sim& s)
{
	drawSprite(A_UI_ROOT, 0.0f, 60.0f, SCRW, 54.0f, false);
	for (int i = 0; i < s.queueCount; i++)
	{
		float x = 8.0f + (float)i * 22.0f;
		float p = s.queue[i].buildTime > 0.0f ? s.queue[i].timeQueued / s.queue[i].buildTime : 0.0f;
		if (p > 1.0f) p = 1.0f;
		C2D_DrawRectSolid(x, 100.0f, 0.0f, 18.0f, 5.0f, C_BAR);
		C2D_DrawRectSolid(x, 100.0f, 0.0f, 18.0f * p, 5.0f, C_HP);
	}
	if (s.queueCount == 0)
		drawText("Ready for orders", 8.0f, 98.0f, 0.45f, C_TEXT_DIM);
}

static void drawInfo(const Sim& s)
{
	Unit* fb = simBase(const_cast<Sim&>(s), TEAM_GOOD);
	Unit* eb = simBase(const_cast<Sim&>(s), TEAM_BAD);
	C2D_DrawRectSolid(0.0f, 114.0f, 0.0f, SCRW, 72.0f, C_PANEL);
	drawText("YOUR BASE", 6.0f, 116.0f, 0.48f, C_GOOD);
	drawText("ENEMY BASE", 166.0f, 116.0f, 0.48f, C_BAD);
	char buf[48];
	snprintf(buf, sizeof(buf), "%d / %d", fb ? (int)fb->health : 0, fb ? (int)fb->maxHealth : 0);
	drawText(buf, 6.0f, 129.0f, 0.50f, C_TEXT);
	drawHealthBar(108.0f, 132.0f, 54.0f, fb ? fb->health / fb->maxHealth : 0.0f);
	snprintf(buf, sizeof(buf), "%d / %d", eb ? (int)eb->health : 0, eb ? (int)eb->maxHealth : 0);
	drawText(buf, 166.0f, 129.0f, 0.50f, C_TEXT);
	drawHealthBar(268.0f, 132.0f, 46.0f, eb ? eb->health / eb->maxHealth : 0.0f);

	if (g_placeType >= 0 || g_sellMode)
	{
		drawText(g_sellMode ? "CANCEL to stop selling" : "CANCEL to stop building",
		         6.0f, 145.0f, 0.54f, C_GOLD);
	}
	else if (s.specialTimer >= SPECIAL_COOLDOWN && s.spell.kind == 0)
		drawText("Special: ready", 6.0f, 145.0f, 0.54f, C_HP);
	else
	{
		snprintf(buf, sizeof(buf), "Special: %.1fs", SPECIAL_COOLDOWN - s.specialTimer);
		drawText(buf, 6.0f, 145.0f, 0.52f, C_TEXT_DIM);
	}
	if (simMessageCount > 0)
	{
		float w = textWidth(simMessages[0], 0.56f) + 8.0f;
		C2D_DrawRectSolid(4.0f, 160.0f, 0.0f, w, 14.0f, C_BAR);
		drawText(simMessages[0], 8.0f, 162.0f, 0.56f, C_TEXT);
	}
	drawText("L/R mode   SELECT pause   A restart", 6.0f, 176.0f, 0.48f, C_TEXT_DIM);
}

static void drawCard(float x, float y, float w, float h, const char* label,
                     const char* cost, int art, bool enabled, u32 accent)
{
	C2D_DrawRectSolid(x, y, 0.0f, w, h, enabled ? C_PANEL_L : C_BAR);
	C2D_DrawRectSolid(x, y, 0.0f, w, 2.0f, accent);
	if (art >= 0)
		drawSprite(art, x + (w - 34.0f) * 0.5f, y + 3.0f, 34.0f, h - 17.0f, false);
	float lw = textWidth(label, 0.44f);
	if (lw > w - 4.0f)
	{
		float s = 0.44f * (w - 4.0f) / lw;
		drawText(label, x + 2.0f, y + h - 15.0f, s, enabled ? C_TEXT : C_TEXT_DIM);
	}
	else
		drawText(label, x + (w - lw) * 0.5f, y + h - 15.0f, 0.44f, enabled ? C_TEXT : C_TEXT_DIM);
	drawText(cost, x + 3.0f, y + 3.0f, 0.42f, enabled ? C_GOLD : C_TEXT_DIM);
}

static void drawMenu(const Sim& s)
{
	if (g_placeType >= 0 || g_sellMode)
	{
		drawSprite(A_UI_TURRETS_0, 0.0f, 186.0f, SCRW, 54.0f, false);
		for (int i = 0; i < MAX_TURRETS; i++)
		{
			bool open = i <= s.expansionLevel[TEAM_GOOD] && s.turrets[TEAM_GOOD][i].alive;
			char buf[24];
			if (g_sellMode)
			{
				snprintf(buf, sizeof(buf), open ? "SELL %d" : "EMPTY", i + 1);
				drawButton(2.0f + (float)i * 79.0f, 200.0f, 75.0f, 34.0f, buf,
				           false, open, 0.44f);
			}
			else
			{
				snprintf(buf, sizeof(buf), "SLOT %d", i + 1);
				drawButton(2.0f + (float)i * 79.0f, 200.0f, 75.0f, 34.0f, buf,
				           false, !s.turrets[TEAM_GOOD][i].alive && i <= s.expansionLevel[TEAM_GOOD], 0.44f);
			}
		}
		drawButton(2.0f, 178.0f, 70.0f, 20.0f, "CANCEL", false, true, 0.48f);
		return;
	}

	if (g_mode == 1)
	{
		drawSprite(A_UI_TURRETS_0 + s.age, 0.0f, 186.0f, SCRW, 54.0f, false);
		for (int i = 0; i < 3; i++)
		{
			int type = s.age * 3 + i;
			const TurretDef& d = TURRET_DEFS[type];
			char cost[16];
			snprintf(cost, sizeof(cost), "%d", d.cost);
			char label[32];
			snprintf(label, sizeof(label), "%s", d.name);
			drawCard(2.0f + (float)i * 79.0f, 200.0f, 75.0f, 38.0f, label, cost,
			         A_TURRET_0_0 + type, s.coins >= d.cost, AGE_COLOR[s.age]);
		}
		drawButton(242.0f, 200.0f, 76.0f, 38.0f, "SELL", false, true, 0.50f);
		return;
	}

	drawSprite(A_UI_UNITS_0 + s.age, 0.0f, 186.0f, SCRW, 54.0f, false);
	for (int i = 0; i < 3; i++)
	{
		int type = s.age * 3 + i;
		const UnitDef& d = UNIT_DEFS[type];
		char cost[16];
		snprintf(cost, sizeof(cost), "%d", d.cost);
		drawCard(2.0f + (float)i * 79.0f, 200.0f, 75.0f, 38.0f, d.name, cost,
		         ART_U(s.age, i), s.coins >= d.cost && s.queueCount < MAX_QUEUE, AGE_COLOR[s.age]);
	}
	if (s.age == AGE_FUTURE)
	{
		drawCard(242.0f, 200.0f, 76.0f, 38.0f, "SUPER", "150k",
		         ART_U(4, 1), s.coins >= 150000.0f && s.queueCount < MAX_QUEUE, C_GOLD);
	}
	else
	{
		drawButton(242.0f, 200.0f, 76.0f, 38.0f, "LOCKED", false, false, 0.44f);
	}
}

static void drawGameOver(const Sim& s)
{
	if (!s.gameOver)
		return;
	bool win = simBase(const_cast<Sim&>(s), TEAM_BAD)->health <= 0.0f;
	C2D_DrawRectSolid(0.0f, 118.0f, 0.0f, SCRW, 78.0f, C_BAR);
	drawText(win ? "Victory!" : "Defeat!", 108.0f, 124.0f, 0.95f, win ? C_HP : C_BAD);
	drawText(win ? "You destroyed the enemy base." : "Maybe you didn't try hard enough?",
	         12.0f, 146.0f, 0.50f, C_TEXT);
	drawButton(90.0f, 166.0f, 140.0f, 26.0f, "PLAY AGAIN", false, true, 0.50f);
}

static void drawHelp()
{
	drawSprite(A_UI_MENU_BG, 0.0f, 0.0f, SCRW, SCRH, false);
	C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, SCRW, SCRH, C2D_Color32(0x00, 0x00, 0x00, 0xD8));
	drawText("HOW TO PLAY", 96.0f, 8.0f, 0.75f, C_GOLD);
	static const char* lines[] =
	{
		"Destroy the enemy base to win.",
		"Kill units to earn XP and reach 5 ages.",
		"Each age has its own special attack.",
		"Specials need time to recharge.",
		"Buy and sell turrets with money.",
		"Bases can't be repaired. Choose well.",
	};
	for (int i = 0; i < 6; i++)
		drawText(lines[i], 10.0f, 36.0f + (float)i * 26.0f, 0.50f, C_TEXT);
	drawButton(110.0f, 204.0f, 100.0f, 24.0f, "BACK", false, true, 0.52f);
}

static void drawConfirmMenu()
{
	C2D_DrawRectSolid(0.0f, 78.0f, 0.0f, SCRW, 82.0f, C_BAR);
	drawText("Return to menu?", 68.0f, 88.0f, 0.70f, C_GOLD);
	drawButton(36.0f, 122.0f, 112.0f, 28.0f, "YES", false, true, 0.54f);
	drawButton(172.0f, 122.0f, 112.0f, 28.0f, "NO", false, true, 0.54f);
}

static void drawTitle()
{
	drawSprite(A_UI_MENU_BG, 0.0f, 0.0f, SCRW, SCRH, false);
	C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, SCRW, SCRH, C2D_Color32(0x00, 0x00, 0x00, 0x90));
	if (g_help)
	{
		drawHelp();
		return;
	}
	drawSprite(A_UI_LOGO, 40.0f, 12.0f, 240.0f, 86.0f, false);
	drawText("AGE OF WAR", 88.0f, 98.0f, 0.85f, C_GOLD);
	static const char* names[3] = { "NORMAL", "HARDER", "IMPOSSIBLE" };
	for (int i = 0; i < 3; i++)
	{
		drawButton(58.0f, 124.0f + (float)i * 24.0f, 204.0f, 22.0f, names[i],
		           g_menuDiff == i, true, 0.54f);
	}
	drawButton(58.0f, 200.0f, 204.0f, 22.0f, "HOW TO PLAY", false, true, 0.50f);
	drawButton(236.0f, 4.0f, 80.0f, 18.0f, audioMusicOn() ? "MUSIC ON" : "MUSIC OFF",
	           false, audioReady(), 0.40f);
	drawText("L/R choose   A start", 90.0f, 228.0f, 0.46f, C_TEXT_DIM);
}

// --- input -----------------------------------------------------------------

#define BTN_UNIT    0
#define BTN_AGE     1
#define BTN_SPEC    2
#define BTN_SLOT    3
#define BTN_MENU    4
#define BTN_CANCEL  5
#define BTN_BACK    6
#define BTN_YES     7
#define BTN_NO      8
#define BTN_RESTART 9
#define BTN_MUSIC   10

static int pickUnit(float sx, float sy)
{
	int best = -1;
	float bestD = 26.0f;
	for (int i = 0; i < g_sim.unitCount; i++)
	{
		const Unit& u = g_sim.units[i];
		if (!u.alive || u.isBase)
			continue;
		float dx = wx(u.x) - sx;
		float dy = (140.0f - 23.0f) - sy;
		float d = dx * dx + dy * dy;
		if (d < bestD * bestD)
		{
			bestD = d;
			best = u.id;
		}
	}
	return best;
}

static int hitButton(float px, float py)
{
	if (g_title)
	{
		if (g_help)
		{
			if (py >= 204.0f && py < 228.0f && px >= 110.0f && px < 210.0f) return BTN_BACK;
			return -1;
		}
		if (py < 24.0f && px >= 236.0f) return BTN_MUSIC;
		if (py >= 124.0f && py < 194.0f)
		{
			int id = (int)((py - 124.0f) / 24.0f);
			if (id >= 0 && id < 3) return 100 + id;
		}
		if (py >= 200.0f && py < 222.0f) return 103;
		return -1;
	}
	if (g_confirmMenu)
	{
		if (py >= 122.0f && py < 150.0f) return px < 148.0f ? BTN_YES : BTN_NO;
		return -1;
	}
	if (g_sim.gameOver)
	{
		if (py >= 166.0f && py < 192.0f && px >= 90.0f && px < 230.0f) return BTN_RESTART;
		return -1;
	}
	if (py < 26.0f)
	{
		if (px < 48.0f) return BTN_UNIT;
		if (px < 95.0f) return BTN_AGE;
		if (px < 142.0f) return BTN_SPEC;
		if (px < 189.0f) return BTN_SLOT;
		if (px < 236.0f) return BTN_MENU;
		return -1;
	}
	if (g_placeType >= 0 || g_sellMode)
	{
		if (py >= 178.0f && py < 200.0f && px < 72.0f) return BTN_CANCEL;
		if (py >= 200.0f)
		{
			int col = (int)((px - 2.0f) / 79.0f);
			if (col >= 0 && col < 4) return 200 + col;
		}
		return -1;
	}
	if (py >= 200.0f)
	{
		int col = (int)((px - 2.0f) / 79.0f);
		if (col >= 0 && col < 3) return 100 + col;
		if (px >= 242.0f) return 103;
	}
	return -1;
}

static void restartGame()
{
	simInit(g_sim, g_difficulty);
	g_selected = -1;
	g_mode = 0;
	g_placeType = -1;
	g_sellMode = false;
	g_confirmMenu = false;
	g_accum = 0.0f;
}

static void handleTap(float sx, float sy)
{
	if (g_title)
	{
		int id = hitButton(sx, sy);
		if (id == BTN_BACK)
			g_help = false;
		else if (id == BTN_MUSIC)
			audioSetMusic(!audioMusicOn());
		else if (id == 103)
			g_help = true;
		else if (id >= 100 && id < 103)
		{
			g_menuDiff = id - 100;
			g_difficulty = g_menuDiff;
			restartGame();
			g_title = false;
			g_help = false;
			audioPlay(SFX_SWORD);
		}
		return;
	}
	if (g_confirmMenu)
	{
		int id = hitButton(sx, sy);
		if (id == BTN_YES)
		{
			g_title = true;
			g_help = false;
			g_confirmMenu = false;
			g_placeType = -1;
			g_sellMode = false;
			g_selected = -1;
		}
		else if (id == BTN_NO)
			g_confirmMenu = false;
		return;
	}
	if (g_sim.gameOver)
	{
		if (hitButton(sx, sy) == BTN_RESTART)
		{
			restartGame();
			audioPlay(SFX_SWORD);
		}
		return;
	}

	int id = hitButton(sx, sy);
	if (id == BTN_CANCEL)
	{
		g_placeType = -1;
		g_sellMode = false;
		return;
	}
	if (id >= 200 && id < 204)
	{
		int slot = id - 200;
		if (g_sellMode)
			simSellTurret(g_sim, slot);
		else if (g_placeType >= 0)
			simPlaceTurret(g_sim, slot, g_placeType);
		g_placeType = -1;
		g_sellMode = false;
		return;
	}
	if (id >= 0)
	{
		switch (id)
		{
		case BTN_UNIT:
			g_mode = (g_mode == 0) ? 1 : 0;
			g_placeType = -1;
			g_sellMode = false;
			break;
		case BTN_AGE:
			simUpgradeAge(g_sim);
			audioPlay(SFX_SWORD);
			break;
		case BTN_SPEC:
			simCastSpecial(g_sim);
			audioPlay(SFX_SPECIAL);
			break;
		case BTN_SLOT:
			simAddExpansion(g_sim);
			audioPlay(SFX_HIT);
			break;
		case BTN_MENU:
			g_confirmMenu = true;
			break;
		default:
		{
			int slot = id - 100;
			if (g_mode == 1)
			{
				if (slot == 3)
				{
					g_sellMode = true;
					g_placeType = -1;
				}
				else if (slot >= 0 && slot < 3)
				{
					g_placeType = g_sim.age * 3 + slot;
					g_sellMode = false;
				}
			}
			else if (slot >= 0 && slot < 3)
			{
				simQueueUnit(g_sim, g_sim.age * 3 + slot);
				audioPlay(SFX_WHOOSH);
			}
			else if (slot == 3 && g_sim.age == AGE_FUTURE)
			{
				simQueueUnit(g_sim, 15);
				audioPlay(SFX_WHOOSH);
			}
			break;
		}
		}
		return;
	}
	// Tapping the battlefield selects a unit for inspection.
	if (sy >= 58.0f && sy < 114.0f)
		g_selected = pickUnit(sx, sy);
}

static int fatalInit(const char* what)
{
	if (!g_consoleUp)
	{
		consoleInit(GFX_TOP, NULL);
		g_consoleUp = true;
	}
	printf("\n  Age of War 3DS\n\n  init failed:\n  %s\n\n  press START to exit\n", what);
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		if (hidKeysDown() & KEY_START) break;
		gfxSwapBuffers();
	}
	return 1;
}

int main(int argc, char** argv)
{
	(void)argc; (void)argv;
	gfxInitDefault();
	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
		return fatalInit("C3D_Init");
	if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS))
		return fatalInit("C2D_Init");
	C2D_Prepare();

	g_font = C2D_FontLoadFromMem(aow3ds_font_bcfnt, aow3ds_font_bcfnt_size);
	if (!g_font)
		return fatalInit("C2D_FontLoadFromMem");
	C2D_FontSetFilter(g_font, GPU_LINEAR, GPU_LINEAR);
	g_textBuf = C2D_TextBufNew(4096);
	if (!g_textBuf)
		return fatalInit("C2D_TextBufNew");
	g_bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	g_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	if (!g_bottom || !g_top)
		return fatalInit("C2D_CreateScreenTarget");

	g_backgroundSheet = C2D_SpriteSheetLoadFromMem(background_t3x, background_t3x_size);
	if (g_backgroundSheet)
		g_backgroundImage = C2D_SpriteSheetGetImage(g_backgroundSheet, 0);
	g_artSheet = C2D_SpriteSheetLoadFromMem(aow_t3x, aow_t3x_size);
	if (g_artSheet)
	{
		for (int i = 0; i < A_COUNT; i++)
			g_art[i] = C2D_SpriteSheetGetImage(g_artSheet, i);
	}
	else
		return fatalInit("aow.t3x");

	audioInit();
	simInit(g_sim, g_difficulty);

	touchPosition touch;
	bool wasDown = false;
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
		{
			if (g_title)
				break;
			g_confirmMenu = !g_confirmMenu;
			g_placeType = -1;
			g_sellMode = false;
			continue;
		}
		if (g_title)
		{
			if (kDown & KEY_B)
				g_help = !g_help;
			if (g_help)
				continue;
			if (kDown & KEY_LEFT) g_menuDiff = (g_menuDiff + 2) % 3;
			if (kDown & KEY_RIGHT) g_menuDiff = (g_menuDiff + 1) % 3;
			if (kDown & KEY_A)
			{
				g_difficulty = g_menuDiff;
				restartGame();
				g_title = false;
				audioPlay(SFX_SWORD);
			}
		}
		else
		{
			if (g_confirmMenu)
			{
				if (kDown & KEY_A)
				{
					g_title = true;
					g_help = false;
					g_confirmMenu = false;
					g_selected = -1;
				}
				else if (kDown & KEY_B)
					g_confirmMenu = false;
				continue;
			}
			if ((kDown & KEY_A) && g_sim.gameOver)
			{
				restartGame();
				audioPlay(SFX_SWORD);
				continue;
			}
			if (kDown & KEY_SELECT)
				simTogglePause(g_sim);
			if (kDown & KEY_L) g_mode = 0;
			if (kDown & KEY_R) g_mode = 1;
		}

		hidTouchRead(&touch);
		bool down = (hidKeysHeld() & KEY_TOUCH) != 0;
		if (down && !wasDown)
			handleTap((float)touch.px, (float)touch.py);
		wasDown = down;

		if (!g_title)
		{
			g_accum += 1.0f / 60.0f;
			int steps = 0;
			while (g_accum >= FIXED_DT && steps < 5)
			{
				simStep(g_sim, FIXED_DT);
				g_accum -= FIXED_DT;
				steps++;
			}
			if (g_accum > 1.0f) g_accum = 0.0f;
			for (int i = 0; i < g_sim.unitCount; i++)
				noteDamage(g_sim.units[i], 1.0f / 60.0f);
			if (g_sfxCooldown > 0.0f) g_sfxCooldown -= 1.0f / 60.0f;
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TextBufClear(g_textBuf);
		C2D_TargetClear(g_top, C_PANEL);
		C2D_SceneBegin(g_top);
		drawBattlefield(g_sim);
		drawSpecial(g_sim);
		drawBases(g_sim);
		if (!g_title)
		{
			for (int i = 0; i < g_sim.unitCount; i++)
				if (!g_sim.units[i].isBase && g_sim.units[i].alive)
					drawUnit(g_sim.units[i], g_sim);
			drawBullets(g_sim);
			if (g_sim.paused)
				drawText("PAUSED", 160.0f, 80.0f, 1.3f, C_GOLD);
		}
		else
		{
			drawSprite(A_UI_LOGO, 80.0f, 70.0f, 240.0f, 90.0f, false);
		}

		C2D_TargetClear(g_bottom, C_PANEL);
		C2D_SceneBegin(g_bottom);
		if (g_title)
		{
			drawTitle();
		}
		else
		{
			drawHudTop(g_sim);
			drawQueue(g_sim);
			drawInfo(g_sim);
			drawMenu(g_sim);
			drawGameOver(g_sim);
			if (g_sim.paused && !g_sim.gameOver)
				drawText("PAUSED - SELECT to resume", 60.0f, 100.0f, 0.55f, C_GOLD);
			if (g_confirmMenu)
				drawConfirmMenu();
		}
		C2D_Flush();
		C3D_FrameEnd(0);

		g_frame++;
		g_fpsFrames++;
		g_fpsTick++;
		if (g_fpsTick >= 60)
		{
			g_fps = g_fpsFrames;
			g_fpsFrames = 0;
			g_fpsTick = 0;
		}
	}

	audioExit();
	C3D_RenderTargetDelete(g_top);
	C3D_RenderTargetDelete(g_bottom);
	C2D_TextBufDelete(g_textBuf);
	C2D_FontFree(g_font);
	if (g_artSheet) C2D_SpriteSheetFree(g_artSheet);
	if (g_backgroundSheet) C2D_SpriteSheetFree(g_backgroundSheet);
	C2D_Fini();
	gfxExit();
	return 0;
}
