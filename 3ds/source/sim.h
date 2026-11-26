#pragma once

#include "data.h"

// Simulation state for the original-style game loop. Kept POD-like so the
// whole state can be reset with a single call and shared with the renderer.
#define MAX_UNITS 160
#define MAX_BULLETS 128
#define MAX_QUEUE 5   // SWF tray: one training plus four waiting
#define MAX_TURRETS 4
#define MAX_MESSAGES 4

#define SPECIAL_COOLDOWN 50.0f // 2000 frames at the SWF's 40 fps

enum Side { TEAM_GOOD = 0, TEAM_BAD = 1 };
enum UnitAction { ACTION_IDLE, ACTION_WALK, ACTION_MELEE, ACTION_SHOOT, ACTION_SHOOT_WALK };

struct Unit
{
	int id;
	int team;
	int typeId;
	bool alive;
	bool isBase;
	float x;
	float y;
	float vx;
	float dir;
	float health;
	float maxHealth;
	float attackTimer;
	float rangedTimer;
	float stunTimer;
	float poisonTimer;
	float poisonDamage;
	bool healing;
	bool spawnedOffspring;
	int action;
	int actionFrame;
	int hitFrame;
	int cycleFrames;
	int animationStart;
	float deathTimer;
	int scanFrames;
	int targetId;
	float nearestDistance;
	float enemyDistance;
	float stopDistance;
};

struct Bullet
{
	bool alive;
	int team;
	float x;
	float y;
	float vx;
	float vy;
	float damage;
	float life;
	int kind; // 1..12 turret bullets, 21/22/24/25 specials
	float gravity;
	int frame;
};

struct QueuedUnit
{
	int type;
	float buildTime;
	float timeQueued;
};

struct TurretSlot
{
	bool alive;
	int type;
	float cooldown;
	float rotation;
};

struct Spell
{
	int kind;
	int team;
	float timer;
	int frame;
	float x;
};

struct Sim
{
	int difficulty;
	int age;
	int techLevel;
	int enemyTechLevel;
	float coins;
	float exp;
	bool paused;
	bool gameOver;
	float elapsed;
	float frameAccumulator;
	int frame;
	int nextUnitId;
	float specialTimer;
	float spawnCooldown[2];
	float enemySpawnCooldown;   // u_timer in the SWF; negative when ready
	float enemyStepTimer;       // 40-frame decision tick
	float enemySpecialTimer;
	float enemyAgeTimer;
	float enemyUnitTimer;       // unlocks tier 2 at 1500 frames, tier 3 at 5000
	float enemyCash;
	int enemyPendingType;
	int enemyUnitLevel;
	int enemyUnitsOut;
	bool enemyWantsUnit;
	Unit units[MAX_UNITS];
	int unitCount;
	Bullet bullets[MAX_BULLETS];
	QueuedUnit queue[MAX_QUEUE];
	int queueCount;
	TurretSlot turrets[2][MAX_TURRETS];
	int expansionLevel[2];
	Spell spell;
};

extern const char* simMessages[MAX_MESSAGES];
extern int simMessageCount;

void simPushMessage(const char* msg);
void simClearMessages();
void simInit(Sim& s, int difficulty);
void simStep(Sim& s, float dt);
void simTogglePause(Sim& s);

bool simQueueUnit(Sim& s, int type);
bool simUpgradeAge(Sim& s);
bool simAddExpansion(Sim& s);
bool simCastSpecial(Sim& s);
bool simPlaceTurret(Sim& s, int slot, int type);
bool simSellTurret(Sim& s, int slot);

Unit* simBase(Sim& s, int team);
