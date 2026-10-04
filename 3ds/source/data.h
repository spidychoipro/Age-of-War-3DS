// Game data extracted from the decompiled Age of War (PS Vita) project.
//   Units:  Assets/Resources/units/*.prefab   (health, damage, ranges, collider size)
//           Assets/Scripts/Assembly-CSharp/Game.cs (names, cost, buildTime)
//   Turrets: Assets/Scene/Game.unity           (turrets[] TurretData array)
#pragma once

struct UnitDef
{
	const char* name;
	float health;
	float meleeDamage;
	float rangedDamage;
	float meleeRange;
	float rangedRange;
	float attackTime;
	float rangedAttackTime;
	int cost;
	int buildTime;
	float width;
	float height;
};

static const UnitDef UNIT_DEFS[16] =
{
	{ "Clubman",         55.0f,  16.0f,   0.0f,  20.0f,   0.0f, 0.5f, 0.0f,     15,   40, 0.535f, 1.148f },
	{ "Slingshot Man",   42.0f,  10.0f,   8.0f,  20.0f, 100.0f, 0.5f, 0.7f,     25,   40, 0.614f, 1.146f },
	{ "Dino Rider",     160.0f,  40.0f,   0.0f,  45.0f,   0.0f, 0.5f, 0.0f,    100,  100, 1.961f, 1.487f },
	{ "Swordman",       100.0f,  35.0f,   0.0f,  20.0f,   0.0f, 0.5f, 0.0f,     50,   70, 1.457f, 2.721f },
	{ "Archer",          80.0f,  20.0f,   9.0f,  20.0f, 130.0f, 0.5f, 0.0f,     75,   50, 1.185f, 2.684f },
	{ "Knight",         300.0f,  60.0f,   0.0f,  60.0f,   0.0f, 0.5f, 0.0f,    500,  100, 3.794f, 3.622f },
	{ "Dueler",         200.0f,  79.0f,   0.0f,  25.0f,   0.0f, 0.5f, 0.0f,    200,  100, 0.744f, 1.287f },
	{ "Mousquettere",   160.0f,  40.0f,  20.0f,  25.0f, 130.0f, 0.5f, 0.0f,    400,  100, 0.590f, 1.370f },
	{ "Canoneer",       600.0f, 120.0f,   0.0f,  25.0f,   0.0f, 0.5f, 0.0f,   1000,  200, 0.643f, 1.360f },
	{ "Melee Infantry", 350.0f, 100.0f,   0.0f,  25.0f,   0.0f, 0.5f, 0.0f,   1500,  100, 1.854f, 3.038f },
	{ "Infantry",       300.0f,  60.0f,  30.0f,  25.0f, 130.0f, 0.5f, 0.0f,   2000,  100, 2.029f, 3.015f },
	{ "Tank",          1200.0f, 300.0f,   0.0f, 100.0f,   0.0f, 0.5f, 0.0f,   7000,  300, 6.867f, 3.041f },
	{ "God's Blade",   1000.0f, 250.0f,   0.0f,  40.0f,   0.0f, 0.5f, 0.0f,   5000,  100, 1.944f, 3.262f },
	{ "Blaster",        800.0f, 130.0f,  80.0f,  40.0f, 130.0f, 0.5f, 0.0f,   6000,  100, 2.079f, 3.224f },
	{ "War Machine",   3000.0f, 600.0f,   0.0f, 100.0f,   0.0f, 0.5f, 0.0f,  20000,  300, 6.365f, 3.143f },
	{ "Super Soldier", 5000.0f, 400.0f, 400.0f,  40.0f, 150.0f, 0.5f, 0.0f, 150000,  100, 2.079f, 3.224f },
};

struct TurretDef
{
	const char* name;
	int shotSpeed;
	int bulletId;
	int damage;
	int range;
	int cost;
};

static const TurretDef TURRET_DEFS[15] =
{
	{ "Rock Slignshot",   60,  1,  12, 350,   100 },
	{ "Egg Automatic",    11,  2,   5, 300,   200 },
	{ "Primitive Catapult",70, 3,  25, 400,   500 },
	{ "Catapult",          70,  3,  40, 400,   500 },
	{ "Fire Catapult",     70,  4,  50, 400,   750 },
	{ "Oil",              100,  5,   4, 300,  1000 },
	{ "Small Canon",       70,  6,  30, 500,  1500 },
	{ "Large Canon",       70,  6,  70, 500,  3000 },
	{ "Explosives Canon",  70,  7, 100, 500,  6000 },
	{ "Single Turret",     40,  8,  70, 500,  7000 },
	{ "Rocket Turret",     50,  9, 100, 500,  9000 },
	{ "Double Turret",     22,  8,  60, 500, 14000 },
	{ "Titanium Shooter",  40, 10, 100, 400, 24000 },
	{ "LazerCanon",        10, 11,  40, 500, 40000 },
	{ "IonRay",            10, 12,  60, 500,100000 },
};

// Base.ageRequirements
static const int AGE_REQUIREMENTS[5] = { 500, 4000, 14000, 45000, 200000 };

// Base.addExpansion
static const int EXPANSION_COSTS[3] = { 1000, 3000, 7500 };

static const char* const AGE_NAMES[5] =
{
	"Caveman", "Medieval", "Renaissance", "WW2", "Future"
};

// Game.cs constants
static const float P2U = 0.025f;          // pixel -> world unit
static const float GROUND_LEVEL = -3.85f;
static const float BASE_START_HEALTH = 500.0f;
static const float BASE_AGE_HEALTH = 300.0f;
static const float SPECIAL_COOLDOWN = 50.0f;
static const int START_COINS = 175;
static const int MAX_QUEUE = 5;
static const int AGE_FUTURE = 4;
