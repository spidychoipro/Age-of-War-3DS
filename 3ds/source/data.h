#pragma once

// Source: Flash frame_13/DoAction.as, DefineSprite_699_ennemy and SWF header.
// One simulation unit is 50 original Flash pixels; the SWF runs at 40 fps.
static const float FLASH_FPS = 40.0f;
static const float FLASH_DT = 1.0f / FLASH_FPS;
static const float FLASH_PIXEL = 1.0f / 50.0f;
#define AGE_CAVE 0
#define AGE_KNIGHT 1
#define AGE_MEDIVAL 2
#define AGE_MILTARY 3
#define AGE_FUTURE 4
#define AGE_COUNT 5

static const char* const AGE_NAMES[AGE_COUNT] =
{
	"Stone Age", "Medieval Age", "Renaissance", "Modern Age", "Future"
};

// SWF EV = [4000, 14000, 45000, 200000]; indexed by the current age.
static const int AGE_XP[AGE_COUNT] = { 4000, 14000, 45000, 200000, 0 };
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
	float attackTime; // melee animation cycle (first variant)
	float buildTime;
};

// ES is shared by both sides; only enemy units receive difficulty scaling.
static const UnitDef UNIT_DEFS[16] =
{
	{ "Club man",          15,   55,  16,   0, 0.4f, 0.0f, 40/FLASH_FPS,  40/FLASH_FPS },
	{ "Slingshot man",     25,   42,  10,   8, 0.4f, 2.0f, 40/FLASH_FPS,  40/FLASH_FPS },
	{ "Dino rider",       100,  160,  40,   0, 0.9f, 0.0f, 45/FLASH_FPS, 100/FLASH_FPS },
	{ "Sword man",         50,  100,  35,   0, 0.4f, 0.0f, 48/FLASH_FPS,  70/FLASH_FPS },
	{ "Archer",            75,   80,  20,   9, 0.4f, 2.6f, 48/FLASH_FPS,  50/FLASH_FPS },
	{ "Knight",           500,  300,  60,   0, 1.2f, 0.0f, 52/FLASH_FPS, 100/FLASH_FPS },
	{ "Dueler",           200,  200,  79,   0, 0.5f, 0.0f, 37/FLASH_FPS, 100/FLASH_FPS },
	{ "Musketeer",        400,  160,  40,  20, 0.5f, 2.6f, 46/FLASH_FPS, 100/FLASH_FPS },
	{ "Cannoneer",       1000,  600, 120,   0, 0.5f, 0.0f, 78/FLASH_FPS, 200/FLASH_FPS },
	{ "Melee Infantry",  1500,  350, 100,   0, 0.5f, 0.0f, 30/FLASH_FPS, 100/FLASH_FPS },
	{ "Infantry",        2000,  300,  60,  30, 0.5f, 2.6f, 21/FLASH_FPS, 100/FLASH_FPS },
	{ "Tank",            7000, 1200, 300,   0, 2.0f, 0.0f, 63/FLASH_FPS, 300/FLASH_FPS },
	{ "God's Blade",     5000, 1000, 250,   0, 0.8f, 0.0f, 37/FLASH_FPS, 100/FLASH_FPS },
	{ "Blaster",         6000,  800, 130,  80, 0.8f, 2.6f, 32/FLASH_FPS, 100/FLASH_FPS },
	{ "War machine",    20000, 3000, 600,   0, 2.0f, 0.0f, 90/FLASH_FPS, 300/FLASH_FPS },
	{ "Super Soldier", 150000, 4000, 400, 400, 0.8f, 3.0f, 40/FLASH_FPS, 100/FLASH_FPS },
};

// Hitzone widths/heights in Flash pixels, from DefineSprite_695.
static const float UNIT_WIDTH[16] = {31.6f,31.6f,81.4f,31.6f,31.6f,98.4f,33.5f,33.5f,33.5f,33.5f,33.5f,173.5f,44.5f,44.5f,116.5f,44.5f};
static const float UNIT_HEIGHT[16] = {44.3f,44.3f,62.3f,44.3f,44.3f,69.0f,51.3f,51.3f,51.3f,51.3f,51.3f,57.3f,69.05f,69.05f,57.05f,69.05f};

struct AttackCycle { int hit; int length; int alternateHit; int alternateLength; };
// Hit frame and loop length, including the random attack1/attack2 timelines.
static const AttackCycle MELEE_CYCLES[16] = {
	{20,40,0,0},{20,40,0,0},{21,45,0,0},{33,48,28,51},
	{38,48,31,51},{23,52,0,0},{19,37,25,47},{13,46,0,0},
	{27,78,0,0},{14,30,0,0},{7,21,0,0},{30,63,0,0},
	{23,37,0,0},{21,32,13,25},{13,90,0,0},{21,32,13,25}
};
static const AttackCycle SHOOT_CYCLES[16] = {
	{0,0,0,0},{26,32,0,0},{0,0,0,0},{0,0,0,0},
	{15,40,0,0},{0,0,0,0},{0,0,0,0},{13,46,0,0},
	{0,0,0,0},{0,0,0,0},{7,21,0,0},{0,0,0,0},
	{0,0,0,0},{6,14,0,0},{0,0,0,0},{6,14,0,0}
};
static const AttackCycle SHOOT_WALK_CYCLES[16] = {
	{0,0,0,0},{30,43,0,0},{0,0,0,0},{0,0,0,0},
	{16,40,0,0},{0,0,0,0},{0,0,0,0},{1,48,0,0},
	{0,0,0,0},{0,0,0,0},{3,48,0,0},{0,0,0,0},
	{0,0,0,0},{21,38,0,0},{0,0,0,0},{21,38,0,0}
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
	{ "Rock slingshot",      100,    30/FLASH_FPS,  12, 7.0f },
	{ "Egg automatic",       200,    11/FLASH_FPS,   5, 6.0f },
	{ "Primitive catapult",  500,    70/FLASH_FPS,  25, 8.0f },
	{ "Catapult",            500,    70/FLASH_FPS,  40, 8.0f },
	{ "Fire catapult",       750,    70/FLASH_FPS,  50, 8.0f },
	{ "Oil",                1000,   100/FLASH_FPS,   4, 6.0f },
	{ "Small cannon",       1500,    70/FLASH_FPS,  30, 10.0f },
	{ "Large cannon",       3000,    70/FLASH_FPS,  70, 10.0f },
	{ "Explosives cannon",  6000,    70/FLASH_FPS, 100, 10.0f },
	{ "Single turret",      7000,    40/FLASH_FPS,  70, 10.0f },
	{ "Rocket turret",      9000,    50/FLASH_FPS, 100, 10.0f },
	{ "Double turret",     14000,    22/FLASH_FPS,  60, 10.0f },
	{ "Titanium shooter",  24000,    40/FLASH_FPS, 100, 8.0f },
	{ "Laser cannon",      40000,    10/FLASH_FPS,  40, 10.0f },
	{ "Ion ray",          100000,    10/FLASH_FPS,  60, 10.0f },
};
static const int TURRET_BULLET[15] = {1,2,3,3,4,5,6,6,7,8,9,8,12,10,11};

static const float P2U = 0.45f;
static const float GROUND_LEVEL = 2.0f;
static const float BASE_X[2] = { 1.0f, 19.0f };
static const float SPAWN_X[2] = { 3.0f, 17.2f };
// The SWF starts both bases at 500 and adds 300 * new age on every evolution.
static const float BASE_HEALTH[2] = { 500.0f, 500.0f };
static const float BASE_HEALTH_PER_AGE = 300.0f;
