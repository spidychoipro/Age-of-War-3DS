#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "sim.h"
#include "audio.h"

const char* simMessages[MAX_MESSAGES] = { "", "", "", "" };
int simMessageCount = 0;

namespace
{
	float randomFloat() { return (float)rand() / ((float)RAND_MAX + 1.0f); }
	float difficultyScale(int d) { return d == 2 ? 2.0f : (d == 1 ? 1.3f : 1.0f); }
	void push(const char* msg)
	{
		for (int i = MAX_MESSAGES - 1; i > 0; i--) simMessages[i] = simMessages[i - 1];
		simMessages[0] = msg;
		if (simMessageCount < MAX_MESSAGES) simMessageCount++;
	}
	float halfWidth(const Unit& u) { return (u.isBase ? 170.95f : UNIT_WIDTH[u.typeId]) * FLASH_PIXEL * 0.5f; }
	float height(const Unit& u) { return (u.isBase ? 76.5f : UNIT_HEIGHT[u.typeId]) * FLASH_PIXEL; }
	Unit* spawnUnit(Sim& s, int team, int type)
	{
		int slot = 2;
		while (slot < s.unitCount && (s.units[slot].alive || s.units[slot].deathTimer > 0.0f)) slot++;
		if (slot >= MAX_UNITS) return NULL;
		if (slot == s.unitCount) s.unitCount++;
		Unit& u = s.units[slot];
		memset(&u, 0, sizeof(u));
		u.id = s.nextUnitId++; u.team = team; u.typeId = type; u.alive = true;
		u.dir = team == TEAM_GOOD ? 1.0f : -1.0f;
		u.x = SPAWN_X[team]; u.y = GROUND_LEVEL;
		u.maxHealth = UNIT_DEFS[type].health * (team == TEAM_BAD ? difficultyScale(s.difficulty) : 1.0f);
		u.health = u.maxHealth;
		u.scanFrames=3; u.targetId=-1;
		u.nearestDistance=u.enemyDistance=300.0f*FLASH_PIXEL;
		u.stopDistance=100.0f*FLASH_PIXEL;
		return &u;
	}
	void damageUnit(Sim& s, Unit& u, float amount)
	{
		if (!u.alive || s.gameOver) return;
		u.health -= amount;
		if (u.health > 0.0f) return;
		u.alive = false;
		if (u.isBase)
		{
			s.gameOver = true;
			push(u.team == TEAM_BAD ? "Enemy base destroyed!" : "Your base was destroyed!");
			audioPlay(SFX_EXPLOSION);
			return;
		}
		u.deathTimer = 80.0f / FLASH_FPS;
		int reward = (int)floorf(UNIT_DEFS[u.typeId].cost * 1.3f + 0.5f);
		if (u.team == TEAM_BAD)
		{
			s.coins += reward; s.exp += reward * 2;
			if (s.enemyUnitsOut > 0) s.enemyUnitsOut--;
		}
		else
		{
			s.enemyCash += reward;
			s.exp += (int)floorf(reward / 2.0f + 0.5f);
		}
		audioPlay(u.typeId == 11 ? SFX_DIE_TANK : SFX_DIE);
	}
	void startCycle(Unit& u)
	{
		const AttackCycle& c = u.action == ACTION_MELEE ? MELEE_CYCLES[u.typeId] :
			(u.action == ACTION_SHOOT ? SHOOT_CYCLES[u.typeId] : SHOOT_WALK_CYCLES[u.typeId]);
		bool alternate = c.alternateLength > 0 && randomFloat() < 0.5f;
		u.hitFrame = alternate ? c.alternateHit : c.hit;
		u.cycleFrames = alternate ? c.alternateLength : c.length;
		u.animationStart = 1;
		if (u.action == ACTION_MELEE && c.alternateLength > 0)
		{
			u.animationStart = alternate ? (u.typeId == 6 ? 39 : (u.typeId == 13 || u.typeId == 15 ? 33 : 50)) : 2;
		}
		u.actionFrame = 0;
	}
	void updateUnit(Sim& s, Unit& u)
	{
		if (u.isBase) return;
		if (!u.alive) { u.deathTimer = fmaxf(0.0f, u.deathTimer - FLASH_DT); return; }
		u.healing = s.spell.kind == 3 && u.team == TEAM_GOOD;
		if (u.healing) u.health = fminf(u.maxHealth, u.health + 1.0f);
		Unit* enemy = NULL;
		if (u.scanFrames <= 0)
		{
			u.nearestDistance=u.enemyDistance=300.0f*FLASH_PIXEL;
			u.targetId=-1;
			for (int i = s.unitCount - 1; i >= 0; i--)
			{
				Unit& t=s.units[i];
				if (!t.alive || &t==&u) continue;
				float distance=(t.x-u.x)*u.dir;
				if (distance<=0.0f) continue;
				if (distance<u.nearestDistance)
				{
					u.nearestDistance=distance;
					u.stopDistance=halfWidth(u)+halfWidth(t)+2.0f*FLASH_PIXEL;
				}
				float edge=distance-halfWidth(t);
				if (t.team!=u.team && edge<u.enemyDistance) { u.enemyDistance=edge; u.targetId=t.id; }
			}
			u.scanFrames=5;
		}
		for (int i=0;i<s.unitCount;i++)
			if (s.units[i].id==u.targetId && s.units[i].alive) { enemy=&s.units[i]; break; }
		bool moving = u.nearestDistance > u.stopDistance;
		const UnitDef& d = UNIT_DEFS[u.typeId];
		int action = moving ? ACTION_WALK : ACTION_IDLE;
		if (enemy && fabsf(u.enemyDistance) < d.meleeRange)
		{
			if (!moving) action = ACTION_MELEE;
		}
		else if (enemy && d.rangedDamage > 0 && fabsf(u.enemyDistance) < d.rangedRange)
			action = moving ? ACTION_SHOOT_WALK : ACTION_SHOOT;
		if (moving) u.x += u.dir * 0.7f * FLASH_PIXEL;
		u.vx = moving ? u.dir * 0.7f * FLASH_PIXEL * FLASH_FPS : 0.0f;
		if (u.action != action) { u.action = action; startCycle(u); }
		if (action >= ACTION_MELEE)
		{
			u.actionFrame++;
			if (u.actionFrame == u.hitFrame && enemy)
			{
				// Flash unit shots call ranged_hit directly; they are not projectiles.
				// The musketeer's melee animation also calls ranged_hit().
				bool ranged = action != ACTION_MELEE || u.typeId == 7;
				float amount = ranged ? d.rangedDamage : d.meleeDamage;
				if (u.team == TEAM_BAD) amount *= difficultyScale(s.difficulty);
				damageUnit(s, *enemy, amount - randomFloat() * 2.0f);
				audioPlay(ranged ? (u.typeId >= 12 ? SFX_FUTURE : SFX_RANGE_MEDIVAL) : SFX_SWORD);
			}
			if (u.actionFrame >= u.cycleFrames) startCycle(u);
		}
		u.attackTimer = action == ACTION_MELEE ? (u.cycleFrames - u.actionFrame) * FLASH_DT : 0.0f;
		u.rangedTimer = action >= ACTION_SHOOT ? (u.cycleFrames - u.actionFrame) * FLASH_DT : 0.0f;
		u.scanFrames--;
	}
	Bullet* newBullet(Sim& s, int kind, float x, float y, float vx, float vy, float damage, int team)
	{
		for (int i = 0; i < MAX_BULLETS; i++)
		{
			Bullet& b = s.bullets[i];
			if (b.alive) continue;
			memset(&b, 0, sizeof(b));
			b.alive = true; b.kind = kind; b.team = team;
			b.x = x; b.y = y;
			// Velocities use simulation units per Flash frame.
			b.vx = vx; b.vy = vy; b.damage = damage; b.life = 20.0f;
			return &b;
		}
		return NULL;
	}
	void bulletHit(Sim& s, Bullet& b)
	{
		for (int i = s.unitCount - 1; i >= 0; i--)
		{
			Unit& u = s.units[i];
			if (!u.alive || u.team == b.team) continue;
			if (fabsf(u.x - b.x) <= halfWidth(u) && b.y >= u.y && b.y <= u.y + height(u))
			{
				damageUnit(s, u, b.damage); b.alive = false;
				return;
			}
		}
	}
	void impactFragments(Sim& s, const Bullet& b, bool ground)
	{
		if (b.kind!=4 && b.kind!=7) return;
		int count=b.kind==7 && ground ? 4 : 3;
		for (int i=0;i<count;i++)
		{
			float angle=(60+rand()%60)*0.01745329252f;
			newBullet(s,b.kind==4 ? 16 : 17,b.x,fmaxf(b.y,GROUND_LEVEL),
				cosf(angle)*7*FLASH_PIXEL,(sinf(angle)*7-randomFloat())*FLASH_PIXEL,
				b.kind==4 ? 10.0f : 30.0f,b.team);
		}
	}
	void updateBullets(Sim& s)
	{
		for (int i = 0; i < MAX_BULLETS; i++)
		{
			Bullet& b = s.bullets[i];
			if (!b.alive) continue;
			b.frame++; b.life -= FLASH_DT;
			int steps = b.kind >= 9 && b.kind <= 12 ? 8 : (b.kind >= 6 && b.kind <= 8 ? 4 : 1);
			if (b.kind == 5)
			{
				if (b.frame % 2 == 0)
				{
					Bullet* drop=newBullet(s,15,b.x+(randomFloat()*2-1)*FLASH_PIXEL,
						b.y+(randomFloat()*2-1)*FLASH_PIXEL,0.0f,0.0f,b.damage,b.team);
					if (drop) drop->gravity=FLASH_PIXEL;
				}
				if (b.frame >= 40) b.alive = false;
				continue;
			}
			for (int j = 0; j < steps && b.alive; j++)
			{
				if (b.kind >= 9 && b.kind <= 12)
				{
					b.x += b.vx; b.y += b.vy;
					b.vx *= 1.01f; b.vy *= 1.01f;
				}
				else
				{
					float g = (b.kind==24 || b.kind==25 || (b.kind>=15 && b.kind<=17)) ? 0.2f : (b.kind>=6 && b.kind<=8 ? 0.02f : 0.05f);
					b.gravity += g * FLASH_PIXEL;
					if (b.kind == 24 || b.kind == 25)
					{
						b.gravity = fminf(b.gravity, 15.0f * FLASH_PIXEL); b.vx /= 1.02f;
					}
					b.x += b.vx; b.y += b.vy - b.gravity;
				}
				// Fire/explosive fragments only hurt units on their falling arc.
				bool ground=b.y<GROUND_LEVEL;
				if ((b.kind!=16 && b.kind!=17) || b.vy-b.gravity<0.0f) bulletHit(s,b);
				if (ground || b.x < -2.0f || b.x > 22.0f || b.life <= 0.0f) b.alive = false;
				if (!b.alive) { Bullet impact=b; impactFragments(s,impact,ground); break; }
				// This self-removal is present in bulletspecial5's original script.
				if (b.kind == 25) b.alive = false;
			}
		}
	}
	void updateTurrets(Sim& s)
	{
		for (int team = 0; team < 2; team++)
			for (int i = 0; i < MAX_TURRETS; i++)
			{
				TurretSlot& slot = s.turrets[team][i];
				if (!slot.alive) continue;
				const TurretDef& def = TURRET_DEFS[slot.type];
				float x = (team == TEAM_GOOD ? 52.0f : 948.0f) * FLASH_PIXEL;
				static const float offsets[4] = {80.0f,125.0f,172.0f,220.0f};
				float y = GROUND_LEVEL + offsets[i] * FLASH_PIXEL;
				Unit* target = NULL;
				float best = def.range;
				for (int j = s.unitCount - 1; j >= 0; j--)
				{
					Unit& u = s.units[j];
					if (!u.alive || u.team == team) continue;
					float dx = u.x - x, dy = u.y - y, distance = sqrtf(dx*dx + dy*dy);
					if (distance < best) { best = distance; target = &u; }
				}
				if (!target) continue;
				float angle = atan2f(target->y + height(*target)/1.3f - y, target->x - x);
				int kind = TURRET_BULLET[slot.type];
				if (kind==5) angle=team==TEAM_GOOD ? 0.0f : 3.14159265359f;
				slot.rotation = angle; slot.cooldown -= FLASH_DT;
				if (slot.cooldown > 0.00001f) continue;
				slot.cooldown = def.cooldown;
				float speed = kind >= 9 ? 0.2f : 7.0f;
				float vx = cosf(angle)*speed*FLASH_PIXEL, vy = sinf(angle)*speed*FLASH_PIXEL;
				float offset = kind == 2 ? 4.0f : (kind == 9 ? 200.0f : (kind == 12 ? 100.0f : (kind >= 10 ? 42.0f : 5.0f)));
				if (kind == 3 || kind == 4) y += fabsf(vx)*5.0f;
				else if (kind == 5) { x += (team == TEAM_GOOD ? 116.0f : -116.0f)*FLASH_PIXEL; y -= 7.0f*FLASH_PIXEL; }
				else { x += vx*offset; y += vy*offset; }
				// Original difficulty scales troops, not turret damage.
				newBullet(s, kind, x, y, vx, vy, def.damage, team);
				audioPlay(SFX_TURRET);
			}
	}
	void updateSpell(Sim& s)
	{
		Spell& sp = s.spell;
		if (!sp.kind) return;
		sp.frame++;
		int duration = sp.kind == 3 ? 600 : (sp.kind == 4 ? 276 : 200);
		sp.timer = (duration-sp.frame)/FLASH_FPS;
		if (sp.kind == 1 || sp.kind == 2)
		{
			int interval = sp.kind == 1 ? 9 : 5;
			if (sp.frame % interval == 0)
			{
				float angle = (80 + rand()%20)*0.01745329252f;
				newBullet(s,20+sp.kind,(200+rand()%500)*FLASH_PIXEL,GROUND_LEVEL+525*FLASH_PIXEL,
					cosf(angle)*10*FLASH_PIXEL,-sinf(angle)*10*FLASH_PIXEL,200.0f,TEAM_GOOD);
			}
		}
		else if (sp.kind == 4)
		{
			sp.x += 4*FLASH_PIXEL;
			if (sp.frame%15 == 0 && sp.x < 700*FLASH_PIXEL)
				newBullet(s,24,sp.x,GROUND_LEVEL+225*FLASH_PIXEL,4*FLASH_PIXEL,-FLASH_PIXEL,400.0f,TEAM_GOOD);
		}
		else if (sp.kind == 5 && sp.frame%5 == 0 && sp.x < 700*FLASH_PIXEL)
		{
			newBullet(s,25,sp.x+30*FLASH_PIXEL,GROUND_LEVEL+5*FLASH_PIXEL,4*FLASH_PIXEL,-FLASH_PIXEL,1000.0f,TEAM_GOOD);
			sp.x += 50*FLASH_PIXEL;
		}
		if (sp.frame >= duration) sp.kind = 0;
	}
	void enemyTurret(Sim& s, int slot, int type)
	{
		TurretSlot& t=s.turrets[TEAM_BAD][slot];
		memset(&t,0,sizeof(t)); t.alive=true; t.type=type; t.cooldown=TURRET_DEFS[type].cooldown;
		t.rotation=3.14159265359f;
	}
	void updateEnemyAI(Sim& s)
	{
		int frame = (int)lroundf(s.enemyUnitTimer*FLASH_FPS)+1;
		if (frame == 8000 && s.enemyTechLevel < AGE_COUNT)
		{
			s.enemyTechLevel++;
			float added=BASE_HEALTH_PER_AGE*s.enemyTechLevel;
			s.units[1].maxHealth+=added; s.units[1].health+=added;
			s.enemyUnitLevel=1; frame=0;
		}
		s.enemyUnitTimer=frame/FLASH_FPS; s.enemyAgeTimer=(8000-frame)/FLASH_FPS;
		if (frame == 1500) s.enemyUnitLevel=2;
		if (frame == 5000) s.enemyUnitLevel=3;
		// Literal schedule from DefineSprite_33_base_comp; no enemy specials.
		switch (s.enemyTechLevel)
		{
		case 1:
			if (frame==1000) enemyTurret(s,0,0);
			if (frame==4000) enemyTurret(s,0,1);
			if (frame==6000) enemyTurret(s,0,2);
			break;
		case 2:
			if (frame==1000) enemyTurret(s,0,3);
			if (frame==4000) { s.expansionLevel[1]=1; enemyTurret(s,0,5); }
			if (frame==6000) enemyTurret(s,1,4);
			break;
		case 3:
			if (frame==1000) enemyTurret(s,0,6);
			if (frame==4000) { s.expansionLevel[1]=2; enemyTurret(s,1,6); }
			if (frame==6000) { s.turrets[1][0].alive=false; s.turrets[1][1].alive=false; enemyTurret(s,2,8); }
			break;
		case 4:
			if (frame==5000) enemyTurret(s,0,9);
			if (frame==7000) { s.expansionLevel[1]=2; s.turrets[1][2].alive=false; s.turrets[1][0].alive=false; enemyTurret(s,1,10); }
			break;
		case 5:
			if (frame==5000) enemyTurret(s,0,12);
			if (frame==12000 || frame==20000)
			{
				for (int i=0;i<3;i++) s.turrets[1][i].alive=false;
				if (frame==12000) enemyTurret(s,1,13);
				else { s.expansionLevel[1]=3; enemyTurret(s,3,14); }
			}
			break;
		}
		if (s.frame%40==0) s.enemyWantsUnit=randomFloat()<0.3f;
		if (s.enemyWantsUnit && s.enemyPendingType<0 && s.enemySpawnCooldown < -5*FLASH_DT && s.enemyUnitsOut<6)
		{
			s.enemyPendingType=(s.enemyTechLevel-1)*3+rand()%s.enemyUnitLevel;
			s.enemySpawnCooldown=UNIT_DEFS[s.enemyPendingType].buildTime;
			s.enemyUnitsOut++; s.enemyWantsUnit=false;
		}
		if (s.enemyPendingType>=0 && s.enemySpawnCooldown<0.00001f)
		{
			if (spawnUnit(s,TEAM_BAD,s.enemyPendingType)) s.enemyPendingType=-1;
		}
		s.enemySpawnCooldown=((int)lroundf(s.enemySpawnCooldown*FLASH_FPS)-1)/FLASH_FPS;
	}
}

void simPushMessage(const char* msg) { push(msg); }
void simClearMessages()
{
	for (int i=0;i<MAX_MESSAGES;i++) simMessages[i]="";
	simMessageCount=0;
}
void simInit(Sim& s, int difficulty)
{
	memset(&s,0,sizeof(s));
	s.difficulty=difficulty<0 ? 0 : (difficulty>2 ? 2 : difficulty);
	s.coins=175; s.enemyCash=100; s.techLevel=1; s.enemyTechLevel=1;
	s.enemyUnitLevel=1; s.enemyPendingType=-1; s.enemySpawnCooldown=-FLASH_DT;
	s.enemyAgeTimer=8000/FLASH_FPS; s.specialTimer=SPECIAL_COOLDOWN-FLASH_DT;
	s.nextUnitId=2; s.unitCount=2;
	for (int team=0;team<2;team++)
	{
		Unit& b=s.units[team];
		b.id=team; b.team=team; b.alive=true; b.isBase=true;
		b.x=BASE_X[team]; b.y=GROUND_LEVEL;
		b.maxHealth=b.health=BASE_HEALTH[team]; b.dir=team==TEAM_GOOD ? 1.0f : -1.0f;
	}
	simClearMessages();
}
void simTogglePause(Sim& s) { s.paused=!s.paused; }
void simStep(Sim& s, float dt)
{
	if (s.paused || s.gameOver || dt<=0.0f) return;
	s.frameAccumulator+=dt;
	while (s.frameAccumulator+0.000001f>=FLASH_DT && !s.gameOver)
	{
		s.frameAccumulator-=FLASH_DT; s.frame++; s.elapsed=s.frame/FLASH_FPS;
		s.specialTimer=fminf(SPECIAL_COOLDOWN,((int)lroundf(s.specialTimer*FLASH_FPS)+1)/FLASH_FPS);
		updateEnemyAI(s);
		if (s.queueCount>0)
		{
			QueuedUnit& q=s.queue[0]; q.timeQueued=((int)lroundf(q.timeQueued*FLASH_FPS)+1)/FLASH_FPS;
			if (q.timeQueued+0.00001f>=q.buildTime && spawnUnit(s,TEAM_GOOD,q.type))
			{
				for (int j=0;j+1<s.queueCount;j++) s.queue[j]=s.queue[j+1];
				s.queueCount--;
			}
		}
		for (int i=2;i<s.unitCount && !s.gameOver;i++) updateUnit(s,s.units[i]);
		if (s.gameOver) break;
		updateSpell(s); updateBullets(s);
		if (!s.gameOver) updateTurrets(s);
	}
}
bool simQueueUnit(Sim& s, int type)
{
	if (s.paused || s.gameOver || type<0 || type>=16 || s.queueCount>=MAX_QUEUE) return false;
	if ((type==15 && s.age!=AGE_FUTURE) || (type<15 && type/3!=s.age)) return false;
	const UnitDef& d=UNIT_DEFS[type];
	if (s.coins<d.cost) return false;
	s.coins-=d.cost;
	QueuedUnit& q=s.queue[s.queueCount++];
	q.type=type; q.buildTime=d.buildTime; q.timeQueued=0;
	return true;
}
bool simUpgradeAge(Sim& s)
{
	if (s.paused || s.gameOver || s.age>=AGE_FUTURE || s.exp<AGE_XP[s.age]) return false;
	// XP is cumulative in Flash; evolution never spends it.
	s.age++; s.techLevel=s.age+1;
	float added=BASE_HEALTH_PER_AGE*s.techLevel;
	s.units[0].maxHealth+=added; s.units[0].health+=added;
	push("New age reached!"); audioPlay(SFX_SWORD);
	return true;
}
bool simAddExpansion(Sim& s)
{
	int level=s.expansionLevel[TEAM_GOOD];
	if (s.paused || s.gameOver || level>=3 || s.coins<EXPANSION_COST[level]) return false;
	s.coins-=EXPANSION_COST[level]; s.expansionLevel[TEAM_GOOD]++;
	push("Turret slot added!"); return true;
}
bool simCastSpecial(Sim& s)
{
	if (s.paused || s.gameOver || s.spell.kind || s.specialTimer+0.00001f<SPECIAL_COOLDOWN) return false;
	memset(&s.spell,0,sizeof(s.spell));
	s.spell.kind=s.age+1; s.spell.team=TEAM_GOOD;
	s.spell.timer=(s.age==2 ? 600.0f : (s.age==3 ? 276.0f : 200.0f))/FLASH_FPS;
	s.spell.x=(s.age==3 ? -100.0f : 150.0f)*FLASH_PIXEL;
	s.specialTimer=0; push("Special attack!"); audioPlay(SFX_SPECIAL);
	return true;
}
bool simPlaceTurret(Sim& s, int slot, int type)
{
	if (s.paused || s.gameOver || slot<0 || slot>=MAX_TURRETS || type<0 || type>=15) return false;
	if (type/3!=s.age || slot>s.expansionLevel[0] || s.turrets[0][slot].alive || s.coins<TURRET_DEFS[type].cost) return false;
	s.coins-=TURRET_DEFS[type].cost;
	TurretSlot& t=s.turrets[0][slot];
	memset(&t,0,sizeof(t)); t.alive=true; t.type=type; t.cooldown=TURRET_DEFS[type].cooldown;
	push("Turret built"); audioPlay(SFX_HIT); return true;
}
bool simSellTurret(Sim& s, int slot)
{
	if (s.paused || s.gameOver || slot<0 || slot>=MAX_TURRETS) return false;
	TurretSlot& t=s.turrets[0][slot];
	if (!t.alive) return false;
	s.coins+=(int)floorf(TURRET_DEFS[t.type].cost/2.0f+0.5f);
	memset(&t,0,sizeof(t)); push("Turret sold"); return true;
}
Unit* simBase(Sim& s, int team) { return &s.units[team]; }
