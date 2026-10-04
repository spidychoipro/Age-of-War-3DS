#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "sim.h"

#define MSG_MAX 8

char simMessages[MSG_MAX][64];
int simMessageCount = 0;

static float s_messageTimer;

// Base collider in Game.unity: size (3.799438, 3), offset (0.8997183, 0),
// so the attackable edges sit at centre - 1.0 and centre + 2.7997.
static const float BASE_EDGE_NEAR = -1.0f;
static const float BASE_EDGE_FAR = 2.7997f;

static float randf(void)
{
	return (float)rand() / (float)RAND_MAX;
}

void simMessage(const char* text)
{
	snprintf(simMessages[0], sizeof(simMessages[0]), "%s", text);
	simMessageCount = 1;
	s_messageTimer = 2.0f;
}

Unit* simBase(Sim& s, int team)
{
	for (int i = 0; i < s.unitCount; i++)
	{
		if (s.units[i].isBase && s.units[i].team == team)
			return &s.units[i];
	}
	return 0;
}

static Unit* spawnUnitRaw(Sim& s, int type, int team)
{
	if (s.unitCount >= MAX_UNITS)
		return 0;

	Unit& u = s.units[s.unitCount++];
	const UnitDef& d = UNIT_DEFS[type];
	memset(&u, 0, sizeof(u));
	u.team = team;
	u.typeId = type;
	u.id = s.currentId++;
	u.isBase = false;
	u.alive = true;
	u.dir = (team == TEAM_BAD) ? -1.0f : 1.0f;

	// Game.spawnUnit: good team at baseFriendly.bounds.max.x - 0.2,
	// bad team at baseEnemy.bounds.min.x + 0.2
	if (team == TEAM_BAD)
		u.x = BASE_ENEMY_X + BASE_EDGE_NEAR + 0.2f;
	else
		u.x = BASE_FRIENDLY_X + BASE_EDGE_FAR - 0.2f;

	// Unit.Start: reward + difficulty scaling (harder 1.3, impossible 2.0)
	float scale = 1.0f;
	if (team == TEAM_BAD)
	{
		if (s.difficulty == 1) scale = 1.3f;
		else if (s.difficulty == 2) scale = 2.0f;
	}
	u.maxHealth = d.health * scale;
	u.health = u.maxHealth;
	return &u;
}

void simQueueUnit(Sim& s, int type)
{
	if (s.paused || s.gameOver)
		return;
	if (type < 0 || type > 15)
		return;
	if (s.queueCount >= MAX_QUEUE)
	{
		simMessage("Your production queue is full");
		return;
	}
	const UnitDef& d = UNIT_DEFS[type];
	if (s.coins < d.cost && !s.isHack)
	{
		simMessage("You need more coins to train this unit");
		return;
	}
	if (!s.isHack)
		s.coins -= d.cost;
	QueuedUnit& q = s.queue[s.queueCount++];
	q.unitType = type;
	q.buildTime = (float)d.buildTime / 40.0f;
	q.timeQueued = 0.0f;
}

Unit* simClosestEnemy(Sim& s, const Unit& u)
{
	Unit* best = 0;
	float bestDist = 1e30f;
	for (int i = 0; i < s.unitCount; i++)
	{
		Unit& o = s.units[i];
		if (!o.alive || o.team == u.team)
			continue;
		float d = fabsf(o.x - u.x);
		if (d < bestDist)
		{
			bestDist = d;
			best = &o;
		}
	}
	return best;
}

static void removeUnit(Sim& s, int index)
{
	s.units[index] = s.units[s.unitCount - 1];
	s.unitCount--;
}

// Unit.damage: damage plus Random.value*2 jitter, coin/exp reward on death.
static void damageUnit(Sim& s, Unit& target, float amount, bool jitter)
{
	if (!target.alive || target.health <= 0.0f)
		return;

	target.health -= amount + (jitter ? randf() * 2.0f : 0.0f);
#ifdef SIM_TRACE
	printf("trace: unit %d team %d (base=%d) x=%.2f health %.1f -> %.1f\n",
	       target.id, target.team, target.isBase, target.x, target.maxHealth, target.health);
#endif

	if (target.health > 0.0f || target.isBase)
		return;

	target.alive = false;
	int mreward = (int)((float)UNIT_DEFS[target.typeId].cost * 1.3f);
	if (target.team == TEAM_BAD)
	{
		s.coins += mreward;
		s.exp += 2.0f * (float)mreward;
		if (s.exp > 999999.0f) s.exp = 999999.0f;
	}
	else
	{
		s.e_coins += mreward;
		s.exp += (float)((int)(mreward / 2));
		if (s.exp > 999999.0f) s.exp = 999999.0f;
	}
}

static void spawnBullet(Sim& s, float x, float y, float vx, float vy, float targetX, int team, int damage)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		if (s.bullets[i].alive)
			continue;
		Bullet& b = s.bullets[i];
		b.alive = true;
		b.x = x;
		b.y = y;
		b.vx = vx;
		b.vy = vy;
		b.targetX = targetX;
		b.team = team;
		b.damage = damage;
		return;
	}
}

// Base.addExpansion: the enemy never pays (Base.cs passes the coin check when
// team == 1) and the level is capped at 3.
static void enemyAddExpansion(Sim& s)
{
	if (s.expansionLevel[TEAM_BAD] < 3)
		s.expansionLevel[TEAM_BAD]++;
}

// Base.destroyTurret
static void enemyDestroyTurret(Sim& s, int pos)
{
	if (pos < 0 || pos >= MAX_TURRETS)
		return;
	Turret& t = s.turrets[TEAM_BAD][pos];
	t.alive = false;
	t.type = 0;
	t.st = 0;
}

// Base.setUpTurret: replaces whatever occupied the slot, then FixUpExpansions
// adds an expansion when the slot is beyond the current expansion level.
static void enemySetUpTurret(Sim& s, int pos, int type)
{
	if (pos < 0 || pos >= MAX_TURRETS || type < 0 || type > 14)
		return;
	Turret& t = s.turrets[TEAM_BAD][pos];
	t.type = type;
	t.team = TEAM_BAD;
	t.st = 0;
	t.alive = true;
	if (pos > s.expansionLevel[TEAM_BAD])
		enemyAddExpansion(s);
}

// EnemyAi.update turret schedule (lines 91-191). Every branch is an exact
// equality test on tech_timer, so these fire on precisely one tick.
static void enemyTurretSchedule(Sim& s)
{
	switch (s.techLevel)
	{
	case 1:
		if (s.techTimer == 1000.0f)
		{
			enemySetUpTurret(s, 0, 0);
			s.turretLevel++;
		}
		else if (s.techTimer == 4000.0f)
		{
			enemySetUpTurret(s, 0, 1);
			s.turretLevel++;
		}
		else if (s.techTimer == 6000.0f)
		{
			enemySetUpTurret(s, 0, 2);
		}
		break;

	case 2:
		if (s.techTimer == 1000.0f)
		{
			enemySetUpTurret(s, 0, 3);
			s.turretLevel++;
		}
		else if (s.techTimer == 4000.0f)
		{
			enemyAddExpansion(s);
			enemySetUpTurret(s, 0, 5);
			s.turretLevel++;
		}
		else if (s.techTimer == 6000.0f)
		{
			enemySetUpTurret(s, 1, 4);
		}
		break;

	case 3:
		if (s.techTimer == 1000.0f)
		{
			enemySetUpTurret(s, 0, 6);
			s.turretLevel++;
		}
		else if (s.techTimer == 4000.0f)
		{
			enemyAddExpansion(s);
			enemySetUpTurret(s, 1, 6);
			s.turretLevel++;
		}
		else if (s.techTimer == 6000.0f)
		{
			enemyDestroyTurret(s, 0);
			enemyDestroyTurret(s, 1);
			enemySetUpTurret(s, 2, 8);
		}
		break;

	case 4:
		if (s.techTimer == 5000.0f)
		{
			enemySetUpTurret(s, 0, 9);
			s.turretLevel++;
		}
		else if (s.techTimer == 7000.0f)
		{
			enemyAddExpansion(s);
			enemyDestroyTurret(s, 0);
			enemyDestroyTurret(s, 2);
			enemySetUpTurret(s, 1, 10);
			s.turretLevel++;
		}
		break;

	case 5:
		if (s.techTimer == 5000.0f)
		{
			enemySetUpTurret(s, 0, 12);
			s.turretLevel++;
		}
		else if (s.techTimer == 12000.0f)
		{
			enemyDestroyTurret(s, 0);
			enemyDestroyTurret(s, 1);
			enemyDestroyTurret(s, 2);
			enemySetUpTurret(s, 1, 13);
			s.turretLevel++;
		}
		else if (s.techTimer == 20000.0f)
		{
			enemyDestroyTurret(s, 0);
			enemyDestroyTurret(s, 1);
			enemyDestroyTurret(s, 2);
			enemySetUpTurret(s, 2, 14);
			s.turretLevel++;
		}
		break;
	}
}

static void enemyAiUpdate(Sim& s)
{
	s.techTimer += 1.0f;
	if (s.unitLevel == 1)
	{
		if (s.techTimer >= 1500.0f) s.unitLevel = 2;
	}
	else if (s.unitLevel == 2 && s.techTimer >= 5000.0f)
	{
		s.unitLevel = 3;
	}
	if (s.techTimer == 8000.0f && s.techLevel != 5)
	{
		s.techLevel++;
		Unit* base = simBase(s, TEAM_BAD);
		if (base)
		{
			base->health += BASE_AGE_HEALTH * (float)s.techLevel;
			base->maxHealth += BASE_AGE_HEALTH * (float)s.techLevel;
		}
		s.unitLevel = 1;
		s.techTimer = 0.0f;
	}

	int num = 0;
	s.aiTimer += 1.0f;
	if (s.aiTimer > s.stepTime)
	{
		num = (randf() < s.uf) ? 1 : 3;
		s.aiTimer = 0.0f;
		s.checkAction = true;
	}

	// getUOF(): number of living enemy units, capped at 6
	int enemyCount = 0;
	for (int i = 0; i < s.unitCount; i++)
		if (s.units[i].alive && s.units[i].team == TEAM_BAD) enemyCount++;

	if (s.checkAction && num == 1 && s.uTimer < -5.0f && enemyCount < 6)
	{
		int type = (int)floorf(randf() * (float)s.unitLevel + 1.0f);
		type += (s.techLevel - 1) * 3;
		s.willCreateUnit = type - 1;
		// unit_level tops out at 3 and techLevel at 5, so the AI never
		// reaches type 15 (the 150000-coin Super Soldier).
		if (s.willCreateUnit > 14) s.willCreateUnit = 14;
		if (s.willCreateUnit < 0) s.willCreateUnit = 0;
		s.uTimer = (float)UNIT_DEFS[s.willCreateUnit].buildTime;
		s.checkAction = false;
	}
	if (s.uTimer == 0.0f)
	{
		spawnUnitRaw(s, s.willCreateUnit, TEAM_BAD);
	}
	s.uTimer -= 1.0f;

	enemyTurretSchedule(s);
}

// Unit.FixedUpdate distance metric: centre to centre, or to the nearest
// collider edge when the target is a Base.
static float targetDistance(const Unit& u, const Unit& target)
{
	if (!target.isBase)
		return fabsf(target.x - u.x);
	return fminf(fabsf(target.x + BASE_EDGE_FAR - u.x), fabsf(target.x + BASE_EDGE_NEAR - u.x));
}

// Unit.attack / Unit.rangedAttack measure to the target's collider edges
// (bounds.max.x / bounds.min.x against our own centre) for every target type,
// and allow twice the range when the target is a Base.
static float attackDistance(const Unit& u, const Unit& target)
{
	float near, far;
	if (target.isBase)
	{
		near = target.x + BASE_EDGE_NEAR;
		far  = target.x + BASE_EDGE_FAR;
	}
	else
	{
		float half = UNIT_DEFS[target.typeId].width * 0.5f;
		near = target.x - half;
		far  = target.x + half;
	}
	return fminf(fabsf(far - u.x), fabsf(near - u.x));
}

// Unit.getClosestFriend: the single nearest ally spawned earlier (id < ours),
// excluding Bases. Only that one unit can block us.
static Unit* simClosestFriend(Sim& s, const Unit& u)
{
	Unit* best = 0;
	float bestDist = 1e30f;
	for (int i = 0; i < s.unitCount; i++)
	{
		Unit& f = s.units[i];
		if (!f.alive || f.isBase || f.team != u.team || f.id >= u.id)
			continue;
		float dist = fabsf(f.x - u.x);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = &f;
		}
	}
	return best;
}

static void unitFixedUpdate(Sim& s, Unit& u, float dt)
{
	if (!u.alive || u.isBase)
		return;

	const UnitDef& d = UNIT_DEFS[u.typeId];

	if (u.healing)
	{
		u.health += 1.0f;
		if (u.health > u.maxHealth) u.health = u.maxHealth;
	}

	Unit* target = simClosestEnemy(s, u);
	if (!target)
		return;

	float num = targetDistance(u, *target);
	bool inMelee  = num <  d.meleeRange * P2U;   // flag2
	bool inRanged = !inMelee && num < d.rangedRange * P2U;  // flag3

	// flag: free to advance. Cleared inside melee reach...
	bool canMove = !inMelee;
	// ...or when the closest earlier ally overlaps our padded bounds
	// (bounds.size.x + P2U * 25, so half of that is P2U * 12.5 on each side).
	Unit* ally = simClosestFriend(s, u);
	if (ally)
	{
		float myHalf     = d.width * 0.5f;
		float allyHalf   = UNIT_DEFS[ally->typeId].width * 0.5f;
		if (fabsf(ally->x - u.x) < myHalf + allyHalf + P2U * 12.5f)
			canMove = false;
	}

	if (u.attackTimer > 0.0f) u.attackTimer -= dt;
	if (u.rangedTimer > 0.0f) u.rangedTimer -= dt;

	// if (flag && !flag2) Translate(direction * speed * P2U * dt * 40).
	// inMelee already forces canMove false, so this is just canMove.
	if (canMove)
	{
		u.x += u.dir * 0.7f * P2U * dt * 40.0f;
		if (u.x < -2.0f) u.x = -2.0f;
		if (u.x > (float)WORLD_LENGTH + 2.0f) u.x = (float)WORLD_LENGTH + 2.0f;
	}

	// attack() / rangedAttack() are animation events, so they resolve
	// independently of movement: a unit in weapon range keeps walking
	// ("WalkShoot") while it shoots.
	float reach = attackDistance(u, *target);
	if (inMelee)
	{
		float meleeReach = d.meleeRange * P2U * (target->isBase ? 2.0f : 1.0f);
		if (u.attackTimer <= 0.0f && reach < meleeReach)
		{
			u.attackTimer = d.attackTime;
			damageUnit(s, *target, d.meleeDamage, true);
		}
	}
	else if (inRanged)
	{
		float rangedReach = d.rangedRange * P2U * (target->isBase ? 2.0f : 1.0f);
		if (u.rangedTimer <= 0.0f && reach < rangedReach)
		{
			u.rangedTimer = d.rangedAttackTime > 0.0f ? d.rangedAttackTime : d.attackTime;
			damageUnit(s, *target, d.rangedDamage, true);
			spawnBullet(s, u.x, GROUND_LEVEL + d.height * 0.6f, u.dir * 4.0f, 0.0f, target->x, u.team, 0);
		}
	}
}

static void turretFixedUpdate(Sim& s, int team, Turret& t)
{
	if (!t.alive)
		return;
	const TurretDef& d = TURRET_DEFS[t.type];
	float turretX = (team == TEAM_BAD) ? BASE_ENEMY_X - BASE_WIDTH * 0.5f
	                                   : BASE_FRIENDLY_X + BASE_WIDTH * 0.5f;

	Unit* best = 0;
	float bestDist = 1e30f;
	for (int i = 0; i < s.unitCount; i++)
	{
		Unit& u = s.units[i];
		if (!u.alive || u.team == team)
			continue;
		float dist = fabsf(u.x - turretX);
		if (dist < bestDist)
		{
			bestDist = dist;
			best = &u;
		}
	}

	t.st++;
	if (best && bestDist < (float)d.range * P2U && t.st >= d.shotSpeed)
	{
		t.st = 0;
		damageUnit(s, *best, (float)d.damage, false);
		float speed = 8.0f;
		spawnBullet(s, turretX, GROUND_LEVEL + 1.0f,
			(team == TEAM_BAD ? -speed : speed), 0.0f, best->x, team, 0);
	}
}

static void bulletUpdate(Sim& s, float dt)
{
	for (int i = 0; i < MAX_BULLETS; i++)
	{
		Bullet& b = s.bullets[i];
		if (!b.alive)
			continue;
		b.x += b.vx * dt;
		if ((b.vx > 0.0f && b.x >= b.targetX) || (b.vx < 0.0f && b.x <= b.targetX))
			b.alive = false;
	}
}

static void spellUpdate(Sim& s, float dt)
{
	if (s.spell.kind == 0)
		return;
	s.spell.timer -= dt;
	s.spell.tick -= dt;
	if (s.spell.tick <= 0.0f)
	{
		s.spell.tick = 0.2f;
		if (s.spell.kind == 2)
		{
			for (int i = 0; i < s.unitCount; i++)
			{
				Unit& u = s.units[i];
				if (u.alive && u.team == s.spell.team)
				{
					u.healing = true;
					u.health += 4.0f;
					if (u.health > u.maxHealth) u.health = u.maxHealth;
				}
			}
		}
		else
		{
			for (int i = 0; i < s.unitCount; i++)
			{
				Unit& u = s.units[i];
				if (u.alive && !u.isBase && u.team != s.spell.team)
					damageUnit(s, u, 45.0f, false);
			}
		}
	}
	if (s.spell.timer <= 0.0f)
	{
		if (s.spell.kind == 2)
		{
			for (int i = 0; i < s.unitCount; i++)
				s.units[i].healing = false;
		}
		s.spell.kind = 0;
	}
}

void simCastSpecial(Sim& s)
{
	if (s.paused || s.gameOver)
		return;
	if (s.specialTimer < SPECIAL_COOLDOWN)
	{
		simMessage("The special is not ready yet");
		return;
	}
	s.specialTimer = 0.0f;
	s.numGlobalSpellsUsed++;
	s.spell.team = TEAM_GOOD;
	// Base.special(): age 0 comets, 1 arrows, 2 heal, 3 bomber, 4 beam
	s.spell.kind = (s.age == 2) ? 2 : 1;
	s.spell.timer = (s.age == 2) ? 8.0f : 4.0f;
	s.spell.tick = 0.0f;
	simMessage("Special attack!");
}

void simUpgradeAge(Sim& s)
{
	if (s.paused || s.gameOver)
		return;
	if (s.age == AGE_FUTURE)
	{
		simMessage("Already at the final age!");
		return;
	}
	if (s.exp < (float)AGE_REQUIREMENTS[s.age + 1] && !s.isHack)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%d experience required to upgrade!", AGE_REQUIREMENTS[s.age + 1]);
		simMessage(buf);
		return;
	}
	s.age++;
	Unit* base = simBase(s, TEAM_GOOD);
	if (base)
	{
		base->health += BASE_AGE_HEALTH * (float)(s.age + 1);
		base->maxHealth += BASE_AGE_HEALTH * (float)(s.age + 1);
	}
	simMessage(AGE_NAMES[s.age]);
}

void simBuildTurret(Sim& s, int type)
{
	if (s.paused || s.gameOver)
		return;
	if (type < 0 || type > 14)
		return;
	const TurretDef& d = TURRET_DEFS[type];
	if (s.coins < d.cost && !s.isHack)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "You need %d coins", d.cost);
		simMessage(buf);
		return;
	}
	int slot = -1;
	for (int i = 0; i < MAX_TURRETS; i++)
	{
		if (i <= s.expansionLevel[TEAM_GOOD] && !s.turrets[TEAM_GOOD][i].alive)
		{
			slot = i;
			break;
		}
	}
	if (slot < 0)
	{
		simMessage("No free turret slot (buy an expansion)");
		return;
	}
	if (!s.isHack)
		s.coins -= d.cost;
	s.turrets[TEAM_GOOD][slot].type = type;
	s.turrets[TEAM_GOOD][slot].team = TEAM_GOOD;
	s.turrets[TEAM_GOOD][slot].st = 0;
	s.turrets[TEAM_GOOD][slot].alive = true;
	simMessage(d.name);
}

void simAddExpansion(Sim& s)
{
	if (s.paused || s.gameOver)
		return;
	if (s.expansionLevel[TEAM_GOOD] >= 3)
	{
		simMessage("All expansions are built");
		return;
	}
	int cost = EXPANSION_COSTS[s.expansionLevel[TEAM_GOOD]];
	if (s.coins < cost && !s.isHack)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "%d coins required for a new slot", cost);
		simMessage(buf);
		return;
	}
	if (!s.isHack)
		s.coins -= cost;
	s.expansionLevel[TEAM_GOOD]++;
	simMessage("New turret slot!");
}

void simTogglePause(Sim& s)
{
	if (s.gameOver)
		return;
	s.paused = !s.paused;
}

void simInit(Sim& s, int difficulty)
{
	memset(&s, 0, sizeof(s));
	s.coins = (float)START_COINS;
	s.exp = 0.0f;
	s.age = 0;
	s.difficulty = difficulty;
	s.currentId = 0;

	// bases
	Unit& fb = s.units[s.unitCount++];
	memset(&fb, 0, sizeof(fb));
	fb.team = TEAM_GOOD;
	fb.typeId = 0;
	fb.id = s.currentId++;
	fb.isBase = true;
	fb.alive = true;
	fb.x = BASE_FRIENDLY_X;
	fb.dir = 1.0f;
	fb.maxHealth = BASE_START_HEALTH;
	fb.health = fb.maxHealth;

	Unit& eb = s.units[s.unitCount++];
	memset(&eb, 0, sizeof(eb));
	eb.team = TEAM_BAD;
	eb.typeId = 0;
	eb.id = s.currentId++;
	eb.isBase = true;
	eb.alive = true;
	eb.x = BASE_ENEMY_X;
	eb.dir = -1.0f;
	eb.maxHealth = BASE_START_HEALTH;
	eb.health = eb.maxHealth;

	// EnemyAi.init()
	s.techTimer = 0.0f;
	s.aiTimer = 0.0f;
	s.uTimer = -1.0f;
	s.uf = 0.3f;
	s.stepTime = 40.0f;
	s.checkAction = false;
	s.unitLevel = 1;
	s.turretLevel = 0;
	s.techLevel = 1;
	s.willCreateUnit = 0;
	s.specialTimer = SPECIAL_COOLDOWN;
	s.spell.kind = 0;
}

void simStep(Sim& s, float dt)
{
	if (s.paused || s.gameOver)
		return;

	enemyAiUpdate(s);

	for (int i = 0; i < s.unitCount; i++)
		unitFixedUpdate(s, s.units[i], dt);

	for (int team = 0; team < 2; team++)
		for (int i = 0; i < MAX_TURRETS; i++)
			turretFixedUpdate(s, team, s.turrets[team][i]);

	bulletUpdate(s, dt);
	spellUpdate(s, dt);

	Unit* fb = simBase(s, TEAM_GOOD);
	Unit* eb = simBase(s, TEAM_BAD);
	if (fb) fb->healing = false;
	if (eb) eb->healing = false;

	// Base.FixedUpdate
	s.specialTimer += dt;
	if (s.specialTimer > SPECIAL_COOLDOWN) s.specialTimer = SPECIAL_COOLDOWN;

	// Game.LateUpdate: production queue
	if (s.queueCount > 0)
	{
		QueuedUnit& q = s.queue[0];
		q.timeQueued += dt;
		if (q.timeQueued > q.buildTime || s.isHack)
		{
			spawnUnitRaw(s, q.unitType, TEAM_GOOD);
			for (int i = 0; i < s.queueCount - 1; i++)
				s.queue[i] = s.queue[i + 1];
			s.queueCount--;
			if (s.queueCount > 0)
				s.queue[0].timeQueued = 0.0f;
		}
	}

	// Game.Update: win / lose
	if (fb && eb)
	{
		if (fb->health <= 0.0f || eb->health <= 0.0f)
		{
			if (!s.gameOver)
			{
				s.gameOver = true;
				s.gameOverTimer = 0.0f;
				simMessage(eb->health <= 0.0f && fb->health > 0.0f ? "VICTORY!" : "DEFEAT!");
			}
		}
	}
	if (s.gameOver)
		s.gameOverTimer += dt;

	// reap dead units
	for (int i = s.unitCount - 1; i >= 0; i--)
		if (!s.units[i].alive && !s.units[i].isBase)
			removeUnit(s, i);

	if (s.shake > 0.0f)
	{
		s.shake -= dt;
		if (s.shake < 0.0f) s.shake = 0.0f;
	}

	if (s_messageTimer > 0.0f)
	{
		s_messageTimer -= dt;
		if (s_messageTimer <= 0.0f)
		{
			simMessages[0][0] = 0;
			simMessageCount = 0;
		}
	}
}
