// Age of War - Nintendo 3DS port
// Top screen: battlefield.
// Bottom screen: status and touch HUD (1:1 with touch coordinates).
#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "sim.h"
#include "background_t3x.h"
#include "base_t3x.h"
#include "unit_bg_t3x.h"

// Bundled BCFNT. C2D_FontLoadSystem() is unusable on a KOR console: it
// deliberately returns NULL for the console's own region, and for a foreign
// region it mounts a system title that Korean firmware does not have.
// A BCFNT built from Noto Sans (SIL OFL 1.1) works on every region.
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
static C2D_SpriteSheet g_baseSheet;
static C2D_SpriteSheet g_unitBgSheet;
static C2D_Image g_backgroundImage;
static C2D_Image g_baseImage;
static C2D_Image g_unitBgImage;

// The console is kept only as a fallback for initialization errors. During
// normal play both screens are rendered as graphics targets.
static bool g_consoleUp = false;

static Sim g_sim;
static int g_mode = 0;          // 0 = train units, 1 = buy turrets
static int g_difficulty = 0;    // Game.DifficultySetting
static int g_selected = -1;     // unit id currently inspected, -1 = none
static float g_accum = 0.0f;
static float g_frameDt = 1.0f / 60.0f;   // last frame's real duration
static unsigned g_fps = 60;
static const float FIXED_DT = 1.0f / 50.0f;  // Unity FixedUpdate 0.02f

// Damage feedback without touching the simulation: remember the last health
// seen per unit id and flash the sprite when it drops.
#define FLASH_SLOTS 128
static int g_prevHp[FLASH_SLOTS];
static float g_flash[FLASH_SLOTS];
static int g_flashInit = 0;

static void noteDamage(Unit& u, float dt)
{
	if (!g_flashInit)
	{
		for (int i = 0; i < FLASH_SLOTS; i++)
		{
			g_prevHp[i] = -1;
			g_flash[i] = 0.0f;
		}
		g_flashInit = 1;
	}
	int slot = ((int)u.id % FLASH_SLOTS + FLASH_SLOTS) % FLASH_SLOTS;
	if (g_prevHp[slot] >= 0 && u.health < (float)g_prevHp[slot])
		g_flash[slot] = 0.2f;
	g_prevHp[slot] = (int)u.health;
	if (g_flash[slot] > 0.0f)
	{
		g_flash[slot] -= dt;
		if (g_flash[slot] < 0.0f)
			g_flash[slot] = 0.0f;
	}
}

// palette
static const u32 C_SKY      = C2D_Color32(0x5C, 0x8A, 0xC8, 0xFF);
static const u32 C_SKY_LOW  = C2D_Color32(0x9C, 0xC0, 0xE0, 0xFF);
static const u32 C_GROUND   = C2D_Color32(0x6B, 0x4A, 0x2A, 0xFF);
static const u32 C_GROUND2  = C2D_Color32(0x4E, 0x35, 0x1C, 0xFF);
static const u32 C_BASE     = C2D_Color32(0x8A, 0x86, 0x7C, 0xFF);
static const u32 C_BASE_E   = C2D_Color32(0x7A, 0x62, 0x62, 0xFF);
static const u32 C_PANEL    = C2D_Color32(0x1E, 0x24, 0x2E, 0xFF);
static const u32 C_PANEL_L  = C2D_Color32(0x33, 0x3C, 0x4A, 0xFF);
static const u32 C_TEXT     = C2D_Color32(0xF0, 0xF0, 0xF0, 0xFF);
static const u32 C_TEXT_DIM = C2D_Color32(0xA0, 0xA0, 0xA0, 0xFF);
static const u32 C_GOOD     = C2D_Color32(0x30, 0xC0, 0xFF, 0xFF);
static const u32 C_BAD      = C2D_Color32(0xFF, 0x40, 0x30, 0xFF);
static const u32 C_HP       = C2D_Color32(0x40, 0xE0, 0x40, 0xFF);
static const u32 C_HP_LOW   = C2D_Color32(0xE0, 0x40, 0x40, 0xFF);
static const u32 C_GOLD     = C2D_Color32(0xFF, 0xD0, 0x40, 0xFF);
static const u32 C_XP       = C2D_Color32(0xC0, 0xFF, 0x60, 0xFF);
static const u32 C_READY    = C2D_Color32(0x30, 0xA0, 0x40, 0xFF);
static const u32 C_LOCKED   = C2D_Color32(0x40, 0x40, 0x48, 0xFF);
static const u32 C_BAR      = C2D_Color32(0x20, 0x20, 0x20, 0xFF);

// age colors (0 caveman .. 4 future)
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
	return 50.0f + x * 15.0f;   // centred on the 400px top screen
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
	C2D_DrawRectSolid(x, y, 0.0f, w, h, enabled ? (active ? C_GOOD : C_PANEL_L) : C_LOCKED);
	C2D_DrawRectSolid(x, y, 0.0f, w, 1.0f, active ? C_TEXT : C_PANEL);
	C2D_DrawRectSolid(x, y + h - 1.0f, 0.0f, w, 1.0f, C_BAR);
	float tw = textWidth(label, scale);
	float tx = x + (w - tw) * 0.5f;
	if (tx < x + 2.0f) tx = x + 2.0f;
	drawText(label, tx, y + (h - 12.0f * scale) * 0.5f, scale,
	         enabled ? C_TEXT : C_TEXT_DIM);
}

static void drawHealthBar(float x, float y, float w, float ratio)
{
	if (ratio >= 1.0f)
		return;
	C2D_DrawRectSolid(x - 1.0f, y - 1.0f, 0.0f, w + 2.0f, 4.0f, C_BAR);
	C2D_DrawRectSolid(x, y, 0.0f, w * ratio, 2.0f, ratio > 0.35f ? C_HP : C_HP_LOW);
}

static void drawBattlefield(const Sim& s)
{
	if (g_backgroundImage.tex)
		C2D_DrawImageAt(g_backgroundImage, 0.0f, 0.0f, 0.0f);
	else
	{
		// fallback scene if the embedded artwork cannot be loaded
		C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, FIELDW, 70.0f, C_SKY);
		C2D_DrawRectSolid(0.0f, 70.0f, 0.0f, FIELDW, 68.0f, C_SKY_LOW);
		C2D_DrawRectSolid(0.0f, 138.0f, 0.0f, FIELDW, 26.0f, C_GROUND);
		C2D_DrawRectSolid(0.0f, 138.0f, 0.0f, FIELDW, 2.0f, C_GROUND2);
	}
	// level markers every world unit
	for (int i = 0; i <= WORLD_LENGTH; i++)
		C2D_DrawRectSolid(wx((float)i), 140.0f, 0.0f, 1.0f, 4.0f, C_GROUND2);
	// mid line: gives the eye a reference for how far each side has pushed
	C2D_DrawRectSolid(0.0f, 131.0f, 0.0f, FIELDW, 1.0f, C2D_Color32(0x3A, 0x2A, 0x18, 0xFF));
	(void)s;
}

static void drawBases(const Sim& s)
{
	for (int i = 0; i < s.unitCount; i++)
	{
		const Unit& b = s.units[i];
		if (!b.isBase)
			continue;
		float x = wx(b.x) - 15.0f;
		u32 col = (b.team == TEAM_GOOD) ? C_BASE : C_BASE_E;
		if (g_baseImage.tex)
			C2D_DrawImageAt(g_baseImage, x - 2.0f, 96.0f, 0.0f);
		else
		{
			C2D_DrawRectSolid(x, 88.0f, 0.0f, 30.0f, 52.0f, col);
			C2D_DrawRectSolid(x - 2.0f, 84.0f, 0.0f, 34.0f, 6.0f, C_GROUND2);
		}
		drawHealthBar(x + 2.0f, 80.0f, 26.0f, b.health / b.maxHealth);
		// turrets on this base
		int placed = 0;
		for (int t = 0; t < MAX_TURRETS; t++)
		{
			if (!s.turrets[b.team][t].alive)
				continue;
			float tx = x + 4.0f + (float)placed * 8.0f;
			float ty = 89.0f; // turret feet meet the base roof at y=96
			C2D_DrawRectSolid(tx, ty, 0.0f, 6.0f, 7.0f, AGE_COLOR[s.age < 4 ? s.age : 4]);
			// barrel
			float dir = (b.team == TEAM_BAD) ? -1.0f : 1.0f;
			C2D_DrawLine(tx + 3.0f, ty + 3.0f, C_BAR,
			             tx + 3.0f + dir * 8.0f, ty + 3.0f, C_BAR, 1.5f, 0.0f);
			placed++;
		}
		// expansion pads
		for (int e = 0; e <= s.expansionLevel[b.team] && e < MAX_TURRETS; e++)
		{
			if (s.turrets[b.team][e].alive)
				continue;
			C2D_DrawRectSolid(x + 2.0f + (float)e * 8.0f, 89.0f, 0.0f, 6.0f, 7.0f, C_LOCKED);
		}
	}
}

static void drawUnit(Unit& u, const Sim& s)
{
	const UnitDef& d = UNIT_DEFS[u.typeId];
	int age = u.typeId / 4;
	if (age > 4) age = 4;
	u32 band = (u.team == TEAM_BAD) ? C_BAD : C_GOOD;
	u32 body = (u.team == TEAM_BAD)
		? C2D_Color32(0xC0 - age * 0x18, 0x40 + age * 0x14, 0x34, 0xFF)
		: C2D_Color32(0x38 + age * 0x14, 0x74 + age * 0x14, 0xC4, 0xFF);

	float w = d.width * 15.0f;
	float h = d.height * 15.0f;
	if (w < 5.0f) w = 5.0f;
	if (h < 6.0f) h = 6.0f;
	float cx = wx(u.x);
	float y = 140.0f - h;
	float x = cx - w * 0.5f;

	// attack lunge: the swing/shoot window reads as motion instead of a
	// static overlap, which is what makes melee fights readable.
	bool swinging = (u.attackTimer > d.attackTime * 0.5f) || (u.rangedTimer > 0.0f);
	if (swinging)
		x += u.dir * 2.0f;

	bool selected = (u.id == g_selected);
	if (selected)
	{
		C2D_DrawCircleSolid(cx, 141.0f, 0.0f, 13.0f, C_GOLD);
		// reach: melee ring plus the thin dashed-feel ranged line
		float mr = d.meleeRange * P2U * 15.0f;
		if (mr > 2.0f)
			C2D_DrawCircleSolid(cx, 141.0f, 0.0f, mr, C2D_Color32(0xFF, 0xD0, 0x40, 0x40));
		if (d.rangedDamage > 0.0f)
		{
			float rr = d.rangedRange * P2U * 15.0f;
			C2D_DrawLine(cx, 141.0f, C2D_Color32(0xFF, 0xD0, 0x40, 0x30),
			             cx + u.dir * rr, 141.0f, C2D_Color32(0xFF, 0xD0, 0x40, 0x30), 1.0f, 0.0f);
		}
	}

	C2D_DrawRectSolid(x, y, 0.0f, w, h, body);
	// facing marker so the two teams' advance direction is unambiguous
	float nose = x + (u.dir > 0.0f ? w : 0.0f);
	C2D_DrawRectSolid(nose - (u.dir > 0.0f ? 2.0f : 0.0f), y + h * 0.25f, 0.0f, 2.0f, h * 0.5f, band);
	C2D_DrawRectSolid(x, y, 0.0f, w, 2.0f, band);

	if (d.rangedDamage > 0.0f && !swinging)
		C2D_DrawLine(cx, y + h * 0.5f, C2D_Color32(0x20, 0x20, 0x20, 0xC0),
		             cx + u.dir * 5.0f, y + h * 0.4f, C2D_Color32(0x20, 0x20, 0x20, 0xC0), 1.0f, 0.0f);

	int slot = ((int)u.id % FLASH_SLOTS + FLASH_SLOTS) % FLASH_SLOTS;
	if (g_flash[slot] > 0.0f)
		C2D_DrawRectSolid(x, y, 0.0f, w, h, C2D_Color32(0xFF, 0xFF, 0xFF,
		                                              (u8)(90.0f * (g_flash[slot] / 0.2f))));

	drawHealthBar(cx - w * 0.5f, y - 4.0f, w, u.health / u.maxHealth);
	if (u.healing)
		C2D_DrawCircleSolid(cx, y - 8.0f, 0.0f, 2.0f, C_HP);
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
		// streak along the travel direction so shots read as moving
		float sx = x - b.vx * 1.6f;
		C2D_DrawLine(sx, y, col, x, y, col, 1.0f, 0.0f);
		C2D_DrawCircleSolid(x, y, 0.0f, 1.5f, col);
	}
}

static void drawSpecial(const Sim& s)
{
	if (s.spell.kind == 0)
		return;
	for (int i = 0; i < s.unitCount; i++)
	{
		const Unit& u = s.units[i];
		if (!u.alive || u.team == s.spell.team || u.isBase)
			continue;
		if (s.spell.kind == 2)
		{
			C2D_DrawCircleSolid(wx(u.x), 90.0f, 0.0f, 3.0f, C_HP);
		}
		else
		{
			C2D_DrawLine(wx(BASE_FRIENDLY_X + 1.0f), 70.0f, C_GOLD,
			             wx(u.x), 70.0f, C_GOLD, 2.0f, 0.0f);
			C2D_DrawCircleSolid(wx(u.x), 70.0f, 0.0f, 3.0f, C_GOLD);
		}
	}
}

static void drawStatusBar(const Sim& s)
{
	C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, SCRW, 27.0f, C_PANEL);
	drawButton(3.0f, 3.0f, 48.0f, 21.0f, g_mode == 0 ? "UNIT" : "TURR", g_mode == 1, true, 0.48f);
	drawButton(54.0f, 3.0f, 44.0f, 21.0f, "AGE", false, s.age < AGE_FUTURE, 0.48f);
	drawButton(101.0f, 3.0f, 48.0f, 21.0f, "SPEC", false, s.specialTimer >= SPECIAL_COOLDOWN, 0.48f);
	drawButton(152.0f, 3.0f, 48.0f, 21.0f, "SLOT", false, s.expansionLevel[TEAM_GOOD] < 3, 0.48f);

	char buf[96];
	snprintf(buf, sizeof(buf), "%d", (int)s.coins);
	drawText(buf, 208.0f, 4.0f, 0.54f, C_GOLD);
	drawText("G", 208.0f + textWidth(buf, 0.54f) + 2.0f, 4.0f, 0.54f, C_GOLD);

	snprintf(buf, sizeof(buf), "%d", (int)s.exp);
	drawText(buf, 258.0f, 4.0f, 0.54f, C_XP);
	drawText("XP", 258.0f + textWidth(buf, 0.54f) + 2.0f, 4.0f, 0.54f, C_XP);

	drawText(AGE_NAMES[s.age], 208.0f, 15.0f, 0.48f, AGE_COLOR[s.age]);
}

static void drawTurretPanel(const Sim& s)
{
	// CanvasHud.M_TURRET shows three buttons: turrets[age * 3 + 0..2]
	const float gy = 86.0f;
	for (int i = 0; i < 3; i++)
	{
		const int type = s.age * 3 + i;
		const TurretDef& d = TURRET_DEFS[type];
		const float x = 2.0f + (float)i * 106.0f;
		const bool affordable = s.coins >= (float)d.cost;

		C2D_DrawRectSolid(x, gy + 2.0f, 0.0f, 100.0f, 46.0f, C_PANEL_L);
		C2D_DrawRectSolid(x, gy + 2.0f, 0.0f, 100.0f, 3.0f, AGE_COLOR[s.age]);

		drawText(d.name, x + 4.0f, gy + 8.0f, 0.46f, C_TEXT);

		char stat[48];
		snprintf(stat, sizeof(stat), "dmg %d  rng %d  %dt", d.damage, d.range, d.shotSpeed);
		drawText(stat, x + 4.0f, gy + 21.0f, 0.40f, C_TEXT_DIM);

		char cost[16];
		if (d.cost >= 1000)
			snprintf(cost, sizeof(cost), "%dk", d.cost / 1000);
		else
			snprintf(cost, sizeof(cost), "%d", d.cost);
		float cw = textWidth(cost, 0.55f);
		drawText(cost, x + 94.0f - cw, gy + 34.0f, 0.55f,
		         affordable ? C_GOLD : C_TEXT_DIM);
	}
}

static void drawUnitPanel(const Sim& s)
{
	const float gy = 86.0f;
	C2D_DrawRectSolid(0.0f, gy - 2.0f, 0.0f, SCRW, 2.0f, C_PANEL);
	if (g_mode == 1)
	{
		drawTurretPanel(s);
	}
	else
	{
	for (int i = 0; i < 16; i++)
	{
		int col = i % 4;
		int row = i / 4;
		float x = 3.0f + (float)col * 79.0f;
		float y = gy + (float)row * 37.0f;
		const UnitDef& d = UNIT_DEFS[i];
		bool affordable = s.coins >= (float)d.cost;
		bool unlocked = (i / 4) <= s.age;

		C2D_DrawRectSolid(x, y, 0.0f, 75.0f, 34.0f, unlocked ? C_PANEL_L : C_LOCKED);
		C2D_DrawRectSolid(x, y, 0.0f, 75.0f, 3.0f, AGE_COLOR[i / 4]);
		if (!unlocked)
			C2D_DrawRectSolid(x, y, 0.0f, 75.0f, 34.0f, C2D_Color32(0x20, 0x20, 0x28, 0xA0));

		// unit silhouette
		float uw = d.width * 6.0f, uh = d.height * 6.0f;
		if (uw > 26.0f) uw = 26.0f;
		if (uh > 12.0f) uh = 12.0f;
		C2D_DrawRectSolid(x + 4.0f, y + 18.0f, 0.0f, uw, uh,
		                  unlocked ? C_GOOD : C_LOCKED);
		float nameScale = 0.40f;
		float nameWidth = textWidth(d.name, nameScale);
		if (nameWidth > 67.0f)
			nameScale *= 67.0f / nameWidth;
		drawText(d.name, x + 4.0f, y + 5.0f, nameScale, unlocked ? C_TEXT : C_TEXT_DIM);

		// cost or a clear age gate for locked units
		char cost[16];
		if (!unlocked)
			snprintf(cost, sizeof(cost), "AGE %d", i / 4);
		else if (d.cost >= 1000)
			snprintf(cost, sizeof(cost), "%dk", d.cost / 1000);
		else
			snprintf(cost, sizeof(cost), "%d", d.cost);
		drawText(cost, x + 34.0f, y + 20.0f, 0.44f,
		         (affordable && unlocked) ? C_GOLD : C_TEXT_DIM);
	}
	}

	// production queue
	for (int i = 0; i < s.queueCount; i++)
	{
		float x = 5.0f + (float)i * 14.0f;
		C2D_DrawRectSolid(x, 234.0f, 0.0f, 12.0f, 4.0f, C_READY);
	}
}

static void drawBottomInfo(const Sim& s)
{
	Unit* fb = simBase(const_cast<Sim&>(s), TEAM_GOOD);
	Unit* eb = simBase(const_cast<Sim&>(s), TEAM_BAD);
	C2D_DrawRectSolid(0.0f, 27.0f, 0.0f, SCRW, 55.0f, C_PANEL);
	drawText("YOUR BASE", 6.0f, 30.0f, 0.48f, C_GOOD);
	drawText("ENEMY BASE", 168.0f, 30.0f, 0.48f, C_BAD);
	char buf[48];
	snprintf(buf, sizeof(buf), "%d/%d", fb ? (int)fb->health : 0,
	         fb ? (int)fb->maxHealth : 0);
	drawText(buf, 6.0f, 43.0f, 0.56f, C_TEXT);
	drawHealthBar(42.0f, 46.0f, 104.0f, fb ? fb->health / fb->maxHealth : 0.0f);
	snprintf(buf, sizeof(buf), "%d/%d", eb ? (int)eb->health : 0,
	         eb ? (int)eb->maxHealth : 0);
	drawText(buf, 168.0f, 43.0f, 0.56f, C_TEXT);
	drawHealthBar(204.0f, 46.0f, 104.0f, eb ? eb->health / eb->maxHealth : 0.0f);
	drawText("Touch card: deploy   SELECT: pause", 6.0f, 65.0f, 0.45f, C_TEXT_DIM);
}

static void drawMessage(const Sim& s)
{
	if (simMessageCount == 0)
		return;
	float w = textWidth(simMessages[0], 0.75f) + 12.0f;
	float x = (SCRW - w) * 0.5f;
	C2D_DrawRectSolid(x, 65.0f, 0.0f, w, 16.0f, C_BAR);
	drawText(simMessages[0], x + 6.0f, 69.0f, 0.62f, C_TEXT);
}

static void drawGameOver(const Sim& s)
{
	if (!s.gameOver)
		return;
	C2D_DrawRectSolid(0.0f, 90.0f, 0.0f, SCRW, 56.0f, C_BAR);
	const bool win = simBase(const_cast<Sim&>(s), TEAM_BAD)->health <= 0.0f;
	drawText(win ? "VICTORY!" : "DEFEAT", 130.0f, 100.0f, 1.25f, win ? C_HP : C_HP_LOW);
	drawText("A: restart   START: quit", 92.0f, 124.0f, 0.60f, C_TEXT);
}

// --- top screen ------------------------------------------------------------

static void drawTopStatus(const Sim& s, unsigned frame)
{
	Unit* eb = simBase(const_cast<Sim&>(s), TEAM_BAD);
	Unit* fb = simBase(const_cast<Sim&>(s), TEAM_GOOD);
	int need = (s.age < AGE_FUTURE) ? AGE_REQUIREMENTS[s.age + 1] : 0;
	C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, 400.0f, 240.0f, C_PANEL);
	C2D_DrawRectSolid(0.0f, 0.0f, 0.0f, 400.0f, 34.0f, C_GROUND2);
	drawText("AGE OF WAR", 18.0f, 8.0f, 1.35f, C_GOLD);
	drawText("3DS EDITION", 280.0f, 11.0f, 0.75f, C_TEXT_DIM);

	C2D_DrawRectSolid(16.0f, 48.0f, 0.0f, 176.0f, 72.0f, C_PANEL_L);
	drawText("YOUR BASE", 28.0f, 58.0f, 0.85f, C_GOOD);
	char buf[64];
	snprintf(buf, sizeof(buf), "HP  %d / %d", fb ? (int)fb->health : 0,
	         fb ? (int)fb->maxHealth : 0);
	drawText(buf, 28.0f, 78.0f, 0.9f, C_TEXT);
	drawHealthBar(28.0f, 98.0f, 148.0f, fb ? fb->health / fb->maxHealth : 0.0f);

	C2D_DrawRectSolid(240.0f, 48.0f, 0.0f, 144.0f, 72.0f, C_PANEL_L);
	drawText("ENEMY BASE", 252.0f, 58.0f, 0.85f, C_BAD);
	snprintf(buf, sizeof(buf), "HP  %d / %d", eb ? (int)eb->health : 0,
	         eb ? (int)eb->maxHealth : 0);
	drawText(buf, 252.0f, 78.0f, 0.9f, C_TEXT);
	drawHealthBar(252.0f, 98.0f, 116.0f, eb ? eb->health / eb->maxHealth : 0.0f);

	drawText("RESOURCES", 18.0f, 138.0f, 0.85f, C_TEXT_DIM);
	snprintf(buf, sizeof(buf), "GOLD %d", (int)s.coins);
	drawText(buf, 18.0f, 156.0f, 1.0f, C_GOLD);
	snprintf(buf, sizeof(buf), "EXP %d / %d", (int)s.exp, need);
	drawText(buf, 150.0f, 156.0f, 0.9f, C_XP);
	drawText(AGE_NAMES[s.age], 300.0f, 156.0f, 0.9f, AGE_COLOR[s.age]);

	snprintf(buf, sizeof(buf), "UNITS %d   QUEUE %d   SLOTS %d/4", s.unitCount,
	         s.queueCount, s.expansionLevel[TEAM_GOOD] + 1);
	drawText(buf, 18.0f, 180.0f, 0.85f, C_TEXT);
	if (s.specialTimer >= SPECIAL_COOLDOWN)
		drawText("SPECIAL READY", 18.0f, 202.0f, 0.9f, C_HP);
	else
	{
		snprintf(buf, sizeof(buf), "SPECIAL %.1fs", SPECIAL_COOLDOWN - s.specialTimer);
		drawText(buf, 18.0f, 202.0f, 0.9f, C_TEXT_DIM);
	}
	snprintf(buf, sizeof(buf), "FPS %u", g_fps);
	drawText(buf, 332.0f, 202.0f, 0.75f, C_TEXT_DIM);
	(void)frame;

	if (g_selected >= 0)
	{
		for (int i = 0; i < s.unitCount; i++)
		{
			if (s.units[i].id != g_selected)
				continue;
			const Unit& u = s.units[i];
			const UnitDef& d = UNIT_DEFS[u.typeId];
			C2D_DrawRectSolid(210.0f, 132.0f, 0.0f, 178.0f, 82.0f, C_BAR);
			drawText(d.name, 222.0f, 142.0f, 0.9f, C_GOLD);
			snprintf(buf, sizeof(buf), "%s  HP %d/%d", u.team == TEAM_BAD ? "ENEMY" : "ALLY",
			         (int)u.health, (int)u.maxHealth);
			drawText(buf, 222.0f, 162.0f, 0.75f, C_TEXT);
			snprintf(buf, sizeof(buf), "MELEE %d DMG  RANGE %d", (int)d.meleeDamage,
			         (int)(d.meleeRange * 100.0f));
			drawText(buf, 222.0f, 180.0f, 0.7f, C_TEXT_DIM);
			if (d.rangedDamage > 0.0f)
			{
				snprintf(buf, sizeof(buf), "RANGED %d DMG  RANGE %d", (int)d.rangedDamage,
				         (int)(d.rangedRange * 100.0f));
				drawText(buf, 222.0f, 196.0f, 0.7f, C_TEXT_DIM);
			}
			break;
		}
	}
	else if (!s.gameOver)
	{
		drawText("TOUCH A UNIT CARD TO DEPLOY", 218.0f, 218.0f, 0.62f, C_TEXT_DIM);
	}
}

// --- input -----------------------------------------------------------------

#define BTN_UNIT  0
#define BTN_AGE   1
#define BTN_SPEC  2
#define BTN_SLOT  3

// Nearest live unit to a battlefield tap, or -1. Units are drawn on the ground
// line at y 140, so the pick radius is generous vertically.
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
		float dy = (140.0f - UNIT_DEFS[u.typeId].height * 7.5f) - sy;
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
	if (py < 27.0f)
	{
		if (px < 52.0f) return BTN_UNIT;
		if (px < 100.0f) return BTN_AGE;
		if (px < 151.0f) return BTN_SPEC;
		if (px < 203.0f) return BTN_SLOT;
		return -1;
	}
	if (py >= 84.0f)
	{
		if (g_mode == 1)
		{
			// turret menu: three wide buttons
			if (px < 106.0f) return 100;
			if (px < 212.0f) return 101;
			if (px < 318.0f) return 102;
			return -1;
		}
		int col = (int)((px - 3.0f) / 79.0f);
		int row = (int)((py - 86.0f) / 37.0f);
		if (col >= 0 && col < 4 && row >= 0 && row < 4)
			return 100 + row * 4 + col;
	}
	return -1;
}

static void handleTap(float sx, float sy)
{
	// The battlefield itself is tappable: select the unit nearest the finger so
	// the player can read a unit's reach and state mid-fight.
	if (sy >= 27.0f && sy < 86.0f)
	{
		g_selected = pickUnit(sx, sy);
		return;
	}
	int id = hitButton(sx, sy);
	if (id < 0)
		return;
	switch (id)
	{
	case BTN_UNIT:
		g_mode = (g_mode == 0) ? 1 : 0;
		break;
	case BTN_AGE:
		simUpgradeAge(g_sim);
		break;
	case BTN_SPEC:
		simCastSpecial(g_sim);
		break;
	case BTN_SLOT:
		simAddExpansion(g_sim);
		break;
	default:
	{
		int slot = id - 100;
		if (slot < 0 || slot > 15)
			break;
		if (g_mode == 1)
		{
			// CanvasHud.M_TURRET: turrets[age * 3 + slot]
			if (slot > 2)
				break;
			simBuildTurret(g_sim, g_sim.age * 3 + slot);
			break;
		}
		if ((slot / 4) > g_sim.age)
		{
			simMessage("Reach a higher age first!");
			break;
		}
		simQueueUnit(g_sim, slot);
		break;
	}
	}
}

// Init failures are reported through the plain libctru console instead of
// crashing, so a failure on real hardware is still self-diagnosing.
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
	// C3D_Init must run before C2D_Init/C2D_Prepare: it allocates the
	// texenv/attr/buffer pools that C2D_Prepare writes to via C3D_GetTexEnv.
	// Without it C3D_GetTexEnv(4) returns NULL and C2D_Prepare data-aborts.
	if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))
		return fatalInit("C3D_Init");
	if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS))
		return fatalInit("C2D_Init");
	C2D_Prepare();

	g_font = C2D_FontLoadFromMem(aow3ds_font_bcfnt, aow3ds_font_bcfnt_size);
	if (!g_font)
		return fatalInit("C2D_FontLoadFromMem");
	C2D_FontSetFilter(g_font, GPU_LINEAR, GPU_LINEAR);
	// The HUD reparses many strings every frame. Keep enough room for both
	// screens and clear it before each frame so glyph data cannot accumulate.
	g_textBuf = C2D_TextBufNew(4096);
	if (!g_textBuf)
		return fatalInit("C2D_TextBufNew");
	g_bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	if (!g_bottom)
		return fatalInit("C2D_CreateScreenTarget");
	g_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	if (!g_top)
		return fatalInit("C2D_CreateScreenTarget(top)");

	// Load the original Age of War artwork embedded by the graphics pipeline.
	// Each sheet has a fallback renderer above, so a missing texture cannot
	// bring back the black-screen failure mode on real hardware.
	g_backgroundSheet = C2D_SpriteSheetLoadFromMem(background_t3x, background_t3x_size);
	g_baseSheet = C2D_SpriteSheetLoadFromMem(base_t3x, base_t3x_size);
	g_unitBgSheet = C2D_SpriteSheetLoadFromMem(unit_bg_t3x, unit_bg_t3x_size);
	if (g_backgroundSheet)
		g_backgroundImage = C2D_SpriteSheetGetImage(g_backgroundSheet, 0);
	if (g_baseSheet)
		g_baseImage = C2D_SpriteSheetGetImage(g_baseSheet, 0);
	if (g_unitBgSheet)
		g_unitBgImage = C2D_SpriteSheetGetImage(g_unitBgSheet, 0);

	simInit(g_sim, g_difficulty);
	touchPosition touch;
	bool wasDown = false;
	unsigned frame = 0;
	unsigned fpsTick = 0;
	unsigned fpsFrames = 0;

	while (aptMainLoop())
	{
		g_frameDt = 1.0f / 60.0f;
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break;
		if ((kDown & KEY_A) && g_sim.gameOver)
		{
			simInit(g_sim, g_difficulty);
			g_selected = -1;
			g_mode = 0;
			g_accum = 0.0f;
			continue;
		}
		if (kDown & KEY_SELECT)
			simTogglePause(g_sim);

		hidTouchRead(&touch);
		// KEY_TOUCH is the canonical 3DS touch test: hidScanInput() only sets
		// that bit when the digitizer reports a non-zero sample. touch.py is a
		// u16, so "py >= 0" would be a constant true.
		bool down = (hidKeysHeld() & KEY_TOUCH) != 0;
		if (down && !wasDown)
		{
		// libctru already reports bottom-screen coordinates in landscape
		// orientation (x: 0..319, y: 0..239). Rotating these values makes the
		// first card look like a higher-age card and causes the false gate text.
		float sx = (float)touch.px;
		float sy = (float)touch.py;
			handleTap(sx, sy);
		}
		wasDown = down;

		// fixed-step simulation, like Unity's FixedUpdate
		g_accum += 1.0f / 60.0f;
		int steps = 0;
		while (g_accum >= FIXED_DT && steps < 5)
		{
			simStep(g_sim, FIXED_DT);
			g_accum -= FIXED_DT;
			steps++;
		}
		if (g_accum > 1.0f)
			g_accum = 0.0f;

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		C2D_TextBufClear(g_textBuf);
		C2D_TargetClear(g_top, C_SKY);
		C2D_SceneBegin(g_top);
		drawBattlefield(g_sim);
		drawBases(g_sim);
		drawSpecial(g_sim);
		for (int i = 0; i < g_sim.unitCount; i++)
		{
			if (!g_sim.units[i].isBase && g_sim.units[i].alive)
				drawUnit(g_sim.units[i], g_sim);
		}
		drawBullets(g_sim);
		if (g_sim.paused)
			drawText("PAUSED", 132.0f, 80.0f, 1.5f, C_GOLD);
		drawGameOver(g_sim);
		C2D_TargetClear(g_bottom, C_PANEL);
		C2D_SceneBegin(g_bottom);
		drawStatusBar(g_sim);
		drawBottomInfo(g_sim);
		drawMessage(g_sim);
		drawUnitPanel(g_sim);
		C2D_Flush();
		C3D_FrameEnd(0);

		frame++;
	}

	C3D_RenderTargetDelete(g_top);
	C3D_RenderTargetDelete(g_bottom);
	C2D_TextBufDelete(g_textBuf);
	C2D_FontFree(g_font);
	if (g_unitBgSheet)
		C2D_SpriteSheetFree(g_unitBgSheet);
	if (g_baseSheet)
		C2D_SpriteSheetFree(g_baseSheet);
	if (g_backgroundSheet)
		C2D_SpriteSheetFree(g_backgroundSheet);
	C2D_Fini();
	gfxExit();
	return 0;
}
