#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim.h"
#include "audio.h"

const char* simMessages[MAX_MESSAGES] = { "", "", "", "" };
int simMessageCount = 0;

namespace
{
	const float SPAWN_X_MARGIN = 1.6f;
	const float SPECIAL_TIMER = SPECIAL_COOLDOWN;
	const float ENEMY_SPECIAL_FIRST = 28.0f;
	// SWF: step_time = 40 frames, uf = 0.3, u_timer < -5, uof < 6.
	// The fixed step is 50 Hz, so one Flash frame is 1/50 second here.
	const float ENEMY_STEP_TIME = 40.0f / 50.0f;
	const float ENEMY_SPAWN_CHANCE = 0.3f;
	const float ENEMY_TIER2_TIME = 1500.0f / 50.0f;
	const float ENEMY_TIER3_TIME = 5000.0f / 50.0f;
	const float ENEMY_AGE_TIME = 8000.0f / 50.0f;
	const int ENEMY_MAX_UNITS = 6;

	void push(const char* msg)
	{
		for (int i = MAX_MESSAGES - 1; i > 0; i--)
			simMessages[i] = simMessages[i - 1];
		simMessages[0] = msg;
		simMessageCount = simMessageCount < MAX_MESSAGES ? simMessageCount + 1 : MAX_MESSAGES;
	}

	float difficultyScale(int difficulty)
	{
		// SWF: Harder multiplies enemy health and damage by 1.3, Impossible by 2.
		return difficulty == 2 ? 2.0f : (difficulty == 1 ? 1.3f : 1.0f);
	}

	float clamp(float v, float lo, float hi)
	{
		return v < lo ? lo : (v > hi ? hi : v);
	}

	Unit* spawnUnit(Sim& s, int team, int type)
	{
		if (s.unitCount >= MAX_UNITS || type < 0 || type >= 16)
			return NULL;
		Unit& u = s.units[s.unitCount++];
		memset(&u, 0, sizeof(u));
		u.id = s.unitCount + (int)(s.elapsed * 1000.0f) % 1000;
		u.team = team;
		u.typeId = type;
		u.alive = true;
		u.dir = team == TEAM_GOOD ? 1.0f : -1.0f;
		u.x = BASE_X[team] + u.dir * SPAWN_X_MARGIN;
		u.y = GROUND_LEVEL;
		const UnitDef& d = UNIT_DEFS[type];
		float scale = team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f;
		u.maxHealth = d.health * scale;
		u.health = u.maxHealth;
		return &u;
	}

	Unit* findBase(Sim& s, int team)
	{
		for (int i = 0; i < s.unitCount; i++)
			if (s.units[i].alive && s.units[i].isBase && s.units[i].team == team)
				return &s.units[i];
		return NULL;
	}

	void spawnBase(Sim& s, int team)
	{
		Unit& b = s.units[s.unitCount++];
		memset(&b, 0, sizeof(b));
		b.id = s.unitCount;
		b.team = team;
		b.typeId = 0;
		b.alive = true;
		b.isBase = true;
		b.x = BASE_X[team];
		b.y = GROUND_LEVEL;
		b.dir = team == TEAM_GOOD ? 1.0f : -1.0f;
		b.maxHealth = BASE_HEALTH[team];
		b.health = b.maxHealth;
	}

	void fireBullet(Sim& s, int team, float x, float y, float tx, float ty,
	                float damage, float speed)
	{
		for (int i = 0; i < MAX_BULLETS; i++)
		{
			Bullet& b = s.bullets[i];
			if (b.alive)
				continue;
			float dx = tx - x;
			float dy = ty - y;
			float len = sqrtf(dx * dx + dy * dy);
			if (len < 0.001f) len = 1.0f;
			b.alive = true;
			b.team = team;
			b.x = x;
			b.y = y;
			b.vx = dx / len * speed;
			b.vy = dy / len * speed;
			b.damage = damage;
			b.life = 2.5f;
			return;
		}
	}

	void damageUnit(Sim& s, Unit& target, float amount, bool fromSpell)
	{
		if (!target.alive)
			return;
		target.health -= amount;
		if (target.health <= 0.0f)
		{
			target.alive = false;
			if (target.isBase)
			{
				s.gameOver = true;
				push(target.team == TEAM_BAD ? "Enemy base destroyed!" : "Your base was destroyed!");
				audioPlay(SFX_EXPLOSION);
			}
			else
			{
				// SWF: reward = Math.round(EN[id][1] * 1.3); XP = reward * 2.
				const UnitDef& d = UNIT_DEFS[target.typeId];
				int reward = (int)roundf(d.cost * 1.3f);
				if (target.team == TEAM_BAD)
				{
					s.coins += reward;
					s.exp += reward * 2;
				}
				else
				{
					s.enemyCash += reward;
					if (s.enemyUnitsOut > 0) s.enemyUnitsOut--;
				}
				audioPlay(target.typeId % 3 == 2 ? SFX_DIE_TANK : SFX_DIE);
			}
		}
		(void)fromSpell;
	}

	Unit* findTarget(Sim& s, const Unit& u, float range, float dx)
	{
		Unit* best = NULL;
		float bestDx = range;
		for (int i = 0; i < s.unitCount; i++)
		{
			Unit& t = s.units[i];
			if (!t.alive || t.team == u.team)
				continue;
			float d = (t.x - u.x) * dx;
			if (d < -0.4f || d > bestDx)
				continue;
			if (fabsf(t.x - u.x) > range + 0.3f)
				continue;
			best = &t;
			bestDx = d;
		}
		return best;
	}

	void updateUnit(Sim& s, Unit& u, float dt)
	{
		if (!u.alive || u.isBase)
			return;
		const UnitDef& d = UNIT_DEFS[u.typeId];
		if (u.stunTimer > 0.0f) u.stunTimer -= dt;
		if (u.poisonTimer > 0.0f)
		{
			u.poisonTimer -= dt;
			damageUnit(s, u, u.poisonDamage * dt, true);
			if (!u.alive) return;
		}
		if (u.healing)
		{
			u.health = fminf(u.health + u.maxHealth * 0.06f * dt, u.maxHealth);
			if (u.health >= u.maxHealth) u.healing = false;
		}
		if (u.attackTimer > 0.0f) u.attackTimer -= dt;
		if (u.rangedTimer > 0.0f) u.rangedTimer -= dt;

		Unit* enemy = findTarget(s, u, d.meleeRange, u.dir);
		if (enemy && u.attackTimer <= 0.0f)
		{
			float damage = d.meleeDamage * (u.team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f);
			damageUnit(s, *enemy, damage, false);
			u.attackTimer = d.attackTime;
			audioPlay(SFX_SWORD);
			return;
		}

		Unit* ranged = findTarget(s, u, d.rangedRange, u.dir);
		if (ranged && d.rangedDamage > 0 && u.rangedTimer <= 0.0f)
		{
			float damage = d.rangedDamage * (u.team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f);
			fireBullet(s, u.team, u.x, u.y + 0.7f, ranged->x, ranged->y + 0.5f,
			           damage, 7.0f + (float)u.typeId * 0.1f);
			u.rangedTimer = d.attackTime * 1.2f;
			audioPlay(u.typeId >= 12 ? SFX_FUTURE : (u.typeId >= 6 ? SFX_RANGE_MILTARY : SFX_RANGE_MEDIVAL));
			return;
		}

		Unit* blocker = findTarget(s, u, 0.45f, u.dir);
		if (!blocker)
			u.x += u.dir * (0.9f + (u.typeId % 3) * 0.1f) * dt;

		Unit* base = findBase(s, u.team == TEAM_GOOD ? TEAM_BAD : TEAM_GOOD);
		if (base && fabsf(base->x - u.x) < d.meleeRange && u.attackTimer <= 0.0f)
		{
			float damage = d.meleeDamage * (u.team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f);
			damageUnit(s, *base, damage, false);
			u.attackTimer = d.attackTime;
		}
		u.x = clamp(u.x, 0.2f, 19.8f);
	}

	void updateBullets(Sim& s, float dt)
	{
		for (int i = 0; i < MAX_BULLETS; i++)
		{
			Bullet& b = s.bullets[i];
			if (!b.alive) continue;
			b.x += b.vx * dt;
			b.y += b.vy * dt;
			b.life -= dt;
			if (b.life <= 0.0f || b.x < 0.0f || b.x > 20.0f || b.y < GROUND_LEVEL - 0.5f)
			{
				b.alive = false;
				continue;
			}
			for (int j = 0; j < s.unitCount; j++)
			{
				Unit& u = s.units[j];
				if (!u.alive || u.team == b.team) continue;
				if (fabsf(u.x - b.x) < 0.4f && fabsf(u.y + 0.6f - b.y) < 0.8f)
				{
					damageUnit(s, u, b.damage, false);
					b.alive = false;
					audioPlay(SFX_HIT);
					break;
				}
			}
		}
	}

	void updateTurrets(Sim& s, float dt)
	{
		for (int team = 0; team < 2; team++)
		{
			Unit* base = findBase(s, team);
			if (!base) continue;
			for (int i = 0; i < MAX_TURRETS; i++)
			{
				TurretSlot& slot = s.turrets[team][i];
				if (!slot.alive || slot.type < 0 || slot.type >= 15) continue;
				const TurretDef& def = TURRET_DEFS[slot.type];
				slot.cooldown -= dt;
				Unit* target = NULL;
				float bestD = def.range;
				for (int j = 0; j < s.unitCount; j++)
				{
					Unit& u = s.units[j];
					if (!u.alive || u.team == team) continue;
					float d = fabsf(u.x - base->x);
					if (d < bestD)
					{
						bestD = d;
						target = &u;
					}
				}
				if (!target || slot.cooldown > 0.0f) continue;
				float dmg = def.damage * (team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f);
				fireBullet(s, team, base->x, 3.4f, target->x, target->y + 0.5f, dmg, 9.0f);
				slot.cooldown = def.cooldown;
				audioPlay(SFX_TURRET);
			}
		}
	}

	void updateSpell(Sim& s, float dt)
	{
		if (s.spell.kind == 0) return;
		s.spell.timer -= dt;
		if (s.spell.timer > 0.0f) return;

		int kind = s.spell.kind;
		int team = s.spell.team;
		if (kind == 1 || kind == 2 || kind == 4 || kind == 5)
		{
			float damage = (kind == 4 ? 220.0f : (kind == 5 ? 320.0f : 140.0f));
			damage *= team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f;
			for (int i = 0; i < s.unitCount; i++)
			{
				Unit& u = s.units[i];
				if (u.alive && u.team != team)
					damageUnit(s, u, damage, true);
			}
			audioPlay(SFX_EXPLOSION);
		}
		else if (kind == 3)
		{
			for (int i = 0; i < s.unitCount; i++)
			{
				Unit& u = s.units[i];
				if (u.alive && u.team == team && !u.isBase)
					u.healing = true;
			}
			audioPlay(SFX_SPECIAL);
		}
		s.spell.kind = 0;
	}

	void updateEnemyAI(Sim& s, float dt)
	{
		if (s.gameOver) return;
		s.enemySpawnCooldown -= dt;
		s.enemyStepTimer -= dt;
		s.enemyAgeTimer -= dt;
		s.enemyUnitTimer += dt;
		s.enemySpecialTimer -= dt;

		// SWF: unit_level goes 1 -> 2 at 1500 frames, 2 -> 3 at 5000 frames.
		if (s.enemyUnitLevel == 1 && s.enemyUnitTimer >= ENEMY_TIER2_TIME)
			s.enemyUnitLevel = 2;
		if (s.enemyUnitLevel == 2 && s.enemyUnitTimer >= ENEMY_TIER3_TIME)
			s.enemyUnitLevel = 3;

		// The computer evolves on its own 8000-frame clock, not with the player.
		if (s.enemyAgeTimer <= 0.0f && s.enemyTechLevel < AGE_COUNT)
		{
			s.enemyTechLevel++;
			s.enemyAgeTimer = ENEMY_AGE_TIME;
			Unit* base = findBase(s, TEAM_BAD);
			if (base)
			{
				base->maxHealth += BASE_HEALTH_PER_AGE * s.enemyTechLevel;
				base->health = base->maxHealth;
			}
			// eaddons grows with the age in the SWF.
			s.expansionLevel[TEAM_BAD] = s.enemyTechLevel - 1;
		}

		if (s.enemySpecialTimer <= 0.0f)
		{
			s.spell.kind = 1 + ((s.enemyTechLevel - 1) % 5);
			s.spell.team = TEAM_BAD;
			s.spell.timer = 1.2f;
			s.enemySpecialTimer = 45.0f + (3 - s.difficulty) * 8.0f;
			push("Enemy used a special attack!");
		}

		// Every 40 frames there is a 30% chance to queue one unit. No more
		// than six enemy units may be outstanding, and a new one is only
		// queued once u_timer has been below -5 for a bit.
		if (s.enemyStepTimer <= 0.0f)
		{
			s.enemyStepTimer += ENEMY_STEP_TIME;
			if (s.enemyPendingType < 0 && s.enemySpawnCooldown < -0.1f &&
			    s.enemyUnitsOut < ENEMY_MAX_UNITS && (rand() % 100) < 30)
			{
				int type = (rand() % s.enemyUnitLevel) + 1 + (s.enemyTechLevel - 1) * 3;
				if (type >= 16) type = 15;
				s.enemyPendingType = type;
				s.enemySpawnCooldown = UNIT_DEFS[type].buildTime;
				s.enemyUnitsOut++;
			}
		}
		if (s.enemyPendingType >= 0 && s.enemySpawnCooldown <= 0.0f)
		{
			spawnUnit(s, TEAM_BAD, s.enemyPendingType);
			s.enemyPendingType = -1;
		}

		// The SWF's AI builds turrets on a fixed age schedule and its cash
		// deduction uses a misspelled variable, so they are effectively free.
		int wantSlots = s.enemyTechLevel - 1;
		if (wantSlots > MAX_TURRETS - 1) wantSlots = MAX_TURRETS - 1;
		for (int i = 0; i <= wantSlots; i++)
		{
			if (s.turrets[TEAM_BAD][i].alive) continue;
			int type = (s.enemyTechLevel - 1) * 3 + (i % 3);
			if (type > 14) type = 14;
			s.turrets[TEAM_BAD][i].alive = true;
			s.turrets[TEAM_BAD][i].type = type;
		}
	}
}

void simPushMessage(const char* msg)
{
	push(msg);
}

void simClearMessages()
{
	for (int i = 0; i < MAX_MESSAGES; i++) simMessages[i] = "";
	simMessageCount = 0;
}

void simInit(Sim& s, int difficulty)
{
	memset(&s, 0, sizeof(s));
	s.difficulty = clamp(difficulty, 0, 2);
	// SWF create_game(): cash = 175, e_cash = 100.
	s.coins = 175.0f;
	s.age = AGE_CAVE;
	s.techLevel = 1;
	s.enemyTechLevel = 1;
	s.enemyCash = 100.0f;
	s.spawnCooldown[0] = 0.0f;
	s.spawnCooldown[1] = 0.0f;
	s.enemySpawnCooldown = 0.0f;
	s.enemyStepTimer = ENEMY_STEP_TIME;
	s.enemySpecialTimer = ENEMY_SPECIAL_FIRST;
	s.enemyAgeTimer = ENEMY_AGE_TIME;
	s.enemyUnitTimer = 0.0f;
	s.enemyPendingType = -1;
	s.enemyUnitLevel = 1;
	s.enemyUnitsOut = 0;
	s.expansionLevel[TEAM_GOOD] = 0;
	s.expansionLevel[TEAM_BAD] = 0;
	s.spell.kind = 0;
	simClearMessages();
	spawnBase(s, TEAM_GOOD);
	spawnBase(s, TEAM_BAD);
	s.turrets[TEAM_GOOD][0].alive = true;
	s.turrets[TEAM_GOOD][0].type = 0;
	s.turrets[TEAM_BAD][0].alive = true;
	s.turrets[TEAM_BAD][0].type = 0;
}

void simTogglePause(Sim& s)
{
	s.paused = !s.paused;
}

void simStep(Sim& s, float dt)
{
	if (s.paused || s.gameOver) return;
	s.elapsed += dt;
	if (s.specialTimer < SPECIAL_COOLDOWN)
		s.specialTimer = fminf(SPECIAL_COOLDOWN, s.specialTimer + dt);

	updateEnemyAI(s, dt);
	updateSpell(s, dt);

	for (int i = 0; i < s.queueCount;)
	{
		QueuedUnit& q = s.queue[i];
		q.timeQueued += dt;
		if (q.timeQueued >= q.buildTime)
		{
			spawnUnit(s, TEAM_GOOD, q.type);
			for (int j = i; j + 1 < s.queueCount; j++) s.queue[j] = s.queue[j + 1];
			s.queueCount--;
		}
		else
			i++;
	}

	for (int i = 0; i < s.unitCount; i++)
		updateUnit(s, s.units[i], dt);
	updateBullets(s, dt);
	updateTurrets(s, dt);
}

bool simQueueUnit(Sim& s, int type)
{
	if (s.paused || s.gameOver || type < 0 || type >= 16 || s.queueCount >= MAX_QUEUE)
		return false;
	const UnitDef& d = UNIT_DEFS[type];
	if (s.coins < d.cost) return false;
	s.coins -= d.cost;
	QueuedUnit& q = s.queue[s.queueCount++];
	q.type = type;
	q.buildTime = d.buildTime;
	q.timeQueued = 0.0f;
	return true;
}

bool simUpgradeAge(Sim& s)
{
	if (s.paused || s.gameOver || s.age >= AGE_FUTURE || s.exp < AGE_XP[s.age])
		return false;
	s.exp -= AGE_XP[s.age];
	s.age++;
	Unit* base = findBase(s, TEAM_GOOD);
	if (base)
	{
		base->maxHealth += BASE_HEALTH_PER_AGE * s.age;
		base->health += BASE_HEALTH_PER_AGE * s.age;
		if (base->health > base->maxHealth) base->health = base->maxHealth;
	}
	push("New age reached!");
	audioPlay(SFX_SWORD);
	return true;
}

bool simAddExpansion(Sim& s)
{
	int level = s.expansionLevel[TEAM_GOOD];
	if (s.paused || s.gameOver || level >= 3 || s.coins < EXPANSION_COST[level])
		return false;
	s.coins -= EXPANSION_COST[level];
	s.expansionLevel[TEAM_GOOD]++;
	push("Turret slot added!");
	return true;
}

bool simCastSpecial(Sim& s)
{
	if (s.paused || s.gameOver || s.spell.kind != 0 || s.specialTimer < SPECIAL_TIMER)
		return false;
	s.spell.kind = 1 + s.age;
	s.spell.team = TEAM_GOOD;
	s.spell.timer = 1.2f;
	s.specialTimer = 0.0f;
	push("Special attack!");
	audioPlay(SFX_SPECIAL);
	return true;
}

bool simPlaceTurret(Sim& s, int slot, int type)
{
	if (s.paused || s.gameOver || slot < 0 || slot >= MAX_TURRETS || type < 0 || type >= 15)
		return false;
	if (slot > s.expansionLevel[TEAM_GOOD] || s.turrets[TEAM_GOOD][slot].alive)
		return false;
	if (s.coins < TURRET_DEFS[type].cost) return false;
	s.coins -= TURRET_DEFS[type].cost;
	s.turrets[TEAM_GOOD][slot].alive = true;
	s.turrets[TEAM_GOOD][slot].type = type;
	push("Turret built");
	audioPlay(SFX_HIT);
	return true;
}

bool simSellTurret(Sim& s, int slot)
{
	if (s.paused || s.gameOver || slot < 0 || slot >= MAX_TURRETS)
		return false;
	TurretSlot& t = s.turrets[TEAM_GOOD][slot];
	if (!t.alive) return false;
	s.coins += TURRET_DEFS[t.type].cost / 2;
	t.alive = false;
	t.type = 0;
	push("Turret sold");
	return true;
}

Unit* simBase(Sim& s, int team)
{
	return findBase(s, team);
}
