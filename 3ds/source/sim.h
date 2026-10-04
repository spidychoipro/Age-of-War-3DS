// Core simulation, mirroring the fixed-step logic of the decompiled Unity scripts.
#pragma once

#include "data.h"

static const int MAX_UNITS = 96;
static const int MAX_TURRETS = 4;
static const int MAX_BULLETS = 48;
static const int WORLD_LENGTH = 20;   // Game.LEVEL_LENGTH
static const float BASE_FRIENDLY_X = 0.0f;
static const float BASE_ENEMY_X = (float)WORLD_LENGTH;
static const float BASE_WIDTH = 2.0f;
static const int TEAM_GOOD = 0;
static const int TEAM_BAD = 1;

struct Unit
{
	int team;
	int typeId;
	int id;
	float x;
	float health;
	float maxHealth;
	float dir;
	float attackTimer;
	float rangedTimer;
	bool alive;
	bool isBase;
	bool healing;
};

struct Turret
{
	int type;
	int team;
	int st;          // ticks since last shot
	bool alive;
};

struct Bullet
{
	float x;
	float y;
	float vx;
	float vy;
	float targetX;
	int team;
	int damage;
	bool alive;
};

struct QueuedUnit
{
	int unitType;
	float timeQueued;
	float buildTime;
};

struct Spell
{
	int kind;       // 0 none, 1 beam/bomb, 2 heal
	float timer;
	float tick;
	int team;
};

struct Sim
{
	// economy
	float coins;
	float exp;
	float e_coins;
	int age;
	int difficulty;
	bool paused;
	bool gameOver;
	float gameOverTimer;
	int currentId;
	bool isHack;

	Unit units[MAX_UNITS];
	int unitCount;
	Turret turrets[2][MAX_TURRETS];
	int expansionLevel[2];
	Bullet bullets[MAX_BULLETS];

	QueuedUnit queue[MAX_QUEUE];
	int queueCount;

	// enemy AI (EnemyAi.cs)
	float techTimer;
	float aiTimer;
	float uTimer;
	float uf;
	float stepTime;
	int unitLevel;
	int turretLevel;
	int techLevel;
	int willCreateUnit;
	bool checkAction;

	float specialTimer;
	int numGlobalSpellsUsed;
	Spell spell;

	// presentation
	float shake;
	float messageTimer;
};

extern char simMessages[8][64];
extern int simMessageCount;

void simInit(Sim& s, int difficulty);
void simStep(Sim& s, float dt);
void simQueueUnit(Sim& s, int type);
void simUpgradeAge(Sim& s);
void simBuildTurret(Sim& s, int type);
void simAddExpansion(Sim& s);
void simCastSpecial(Sim& s);
void simTogglePause(Sim& s);
void simMessage(const char* text);

Unit* simClosestEnemy(Sim& s, const Unit& u);
Unit* simBase(Sim& s, int team);
