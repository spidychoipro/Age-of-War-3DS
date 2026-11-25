#pragma once

#include "data.h"

// Simulation state for the original-style game loop. Kept POD-like so the
// whole state can be reset with a single call and shared with the renderer.
#define MAX_UNITS 160
#define MAX_BULLETS 128
#define MAX_QUEUE 8
#define MAX_TURRETS 4
#define MAX_MESSAGES 4

#define SPECIAL_COOLDOWN 45.0f

enum Side { TEAM_GOOD = 0, TEAM_BAD = 1 };

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
};

struct Spell
{
	int kind;
	int team;
	float timer;
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
	float specialTimer;
	float spawnCooldown[2];
	float enemySpawnCooldown;
	float enemySpecialTimer;
	float enemyAgeTimer;
	int enemyCash;
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
