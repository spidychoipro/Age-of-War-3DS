#pragma once

// Names, costs and progression values mirror the decompiled Flash game
// (frame_13/DoAction.as and DoAction_3.as). The visuals are 3DS-sized, but the
// economy and age curve are the original ones.
#define AGE_CAVE 0
#define AGE_KNIGHT 1
#define AGE_MEDIVAL 2
#define AGE_MILTARY 3
#define AGE_FUTURE 4
#define AGE_COUNT 5

static const char* const AGE_NAMES[AGE_COUNT] =
{
	"Stone Age", "Bronze Age", "Middle Age", "Industrial Age", "Future"
};

// Required experience to advance from the indexed age to the next one.
static const int AGE_XP[AGE_COUNT] = { 500, 4000, 14000, 45000, 200000 };
static const int EXPANSION_COST[3] = { 1000, 3000, 7500 };

struct UnitDef
{
	const char* name;
	int cost;
	int health;
	int meleeDamage;
	int rangedDamage;
	float meleeRange;
	float rangedRange;
	float attackTime;
	float buildTime;
};

// The Flash game stores enemy stats separately; the player's troops use the
// same values with the original per-age health/damage curve.
static const UnitDef UNIT_DEFS[16] =
{
	{ "Club man",          15,     55,   16,   0,   0.65f,  0.0f, 1.00f, 0.80f },
	{ "Slingshot man",     25,     42,   10,   8,   0.65f, 3.20f, 1.30f, 0.80f },
	{ "Dino rider",       100,    160,   40,   0,   0.75f,  0.0f, 1.10f, 2.00f },
	{ "Sword man",         50,    110,   26,   0,   0.65f,  0.0f, 0.90f, 1.40f },
	{ "Archer",            75,     84,   16,  18,   0.65f, 3.40f, 1.20f, 1.00f },
	{ "Knight",           500,    340,   65,   0,   0.75f,  0.0f, 0.85f, 2.00f },
	{ "Dueler",           200,    130,   22,  22,   0.70f, 3.00f, 0.85f, 2.00f },
	{ "Musketeer",        400,    180,   30,  38,   0.70f, 4.00f, 1.25f, 2.00f },
	{ "Cannoneer",       1000,    240,   20,  75,   0.70f, 4.20f, 1.60f, 4.00f },
	{ "Melee Infantry",  1500,    480,  125,   0,   0.80f,  0.0f, 0.90f, 2.00f },
	{ "Infantry",        2000,    400,   65,  70,   0.75f, 4.00f, 1.00f, 2.00f },
	{ "Tank",            7000,   1400,  200, 120,   0.90f, 4.50f, 1.30f, 6.00f },
	{ "God's Blade",     5000,    800,  180,   0,   0.80f,  0.0f, 0.70f, 2.00f },
	{ "Blaster",         6000,    560,   60, 140,   0.75f, 4.80f, 1.10f, 2.00f },
	{ "War machine",    20000,   3600,  450, 300,   1.00f, 5.00f, 1.30f, 6.00f },
	{ "Super Soldier", 150000,  10000, 1000, 800,   1.00f, 5.50f, 0.60f, 2.00f },
};

struct TurretDef
{
	const char* name;
	int cost;
	float cooldown;
	int damage;
	float range;
};

// Flash TU/TS tables.
static const TurretDef TURRET_DEFS[15] =
{
	{ "Rock slingshot",      100,    0.60f,  12, 3.5f },
	{ "Egg automatic",       200,    0.22f,   5, 3.0f },
	{ "Primitive catapult",  500,    1.40f,  25, 4.0f },
	{ "Catapult",            500,    1.40f,  40, 4.0f },
	{ "Fire catapult",       750,    1.40f,  50, 4.0f },
	{ "Oil",                1000,    2.00f,   4, 3.0f },
	{ "Small cannon",       1500,    1.40f,  30, 5.0f },
	{ "Large cannon",       3000,    1.40f,  70, 5.0f },
	{ "Explosives cannon",  6000,    1.40f, 100, 5.0f },
	{ "Single turret",      7000,    0.80f,  70, 5.0f },
	{ "Rocket turret",      9000,    1.00f, 100, 5.0f },
	{ "Double turret",     14000,    0.44f,  60, 5.0f },
	{ "Titanium shooter",  24000,    0.80f, 100, 4.0f },
	{ "Laser cannon",      40000,    0.20f,  40, 5.0f },
	{ "Ion ray",          100000,    0.20f,  60, 5.0f },
};

static const float P2U = 0.45f;
static const float GROUND_LEVEL = 2.0f;
static const float BASE_X[2] = { 1.0f, 18.0f };
static const float BASE_HEALTH[2] = { 8000.0f, 8000.0f };
