// Host-only deterministic checks. No libctru or DSP firmware required.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "sim.h"
#include "audio.h"

void audioPlay(SfxId) {}
static bool near(float a, float b) { return fabsf(a-b)<0.002f; }
static void tick(Sim& s, int frames) { for (int i=0;i<frames;i++) simStep(s,FLASH_DT); }
static void quiet(Sim& s)
{
	s.enemyUnitsOut=6; s.enemyPendingType=-1;
	s.units[0].health=s.units[1].health=1000000000;
}
static Unit& troop(Sim& s, int side, int type, float x)
{
	Unit& u=s.units[s.unitCount++];
	memset(&u,0,sizeof(u));
	u.id=s.nextUnitId++; u.team=side; u.typeId=type; u.alive=true;
	u.x=x; u.y=GROUND_LEVEL; u.dir=side==TEAM_GOOD ? 1.0f : -1.0f;
	u.health=u.maxHealth=UNIT_DEFS[type].health;
	return u;
}
static void initialAndQueue()
{
	Sim s; simInit(s,0); quiet(s);
	assert(near(s.coins,175));
	assert(!s.turrets[0][0].alive && !s.turrets[1][0].alive);
	assert(!simQueueUnit(s,3));
	assert(simQueueUnit(s,0) && simQueueUnit(s,1));
	tick(s,39); assert(s.unitCount==2 && s.queueCount==2);
	assert(near(s.queue[1].timeQueued,0));
	tick(s,1); assert(s.unitCount==3 && s.queueCount==1);
	assert(near(s.queue[0].timeQueued,0));
	tick(s,40); assert(s.unitCount==4 && s.queueCount==0);
	float before=s.elapsed; s.paused=true; tick(s,100); assert(s.elapsed==before);
}
static void progression()
{
	Sim s; simInit(s,0);
	s.units[0].health=200; s.exp=4000;
	assert(simUpgradeAge(s));
	assert(s.age==1 && s.techLevel==2 && near(s.exp,4000));
	assert(near(s.units[0].health,800) && near(s.units[0].maxHealth,1100));
	assert(!simUpgradeAge(s));
	s.exp=200000;
	for(int i=0;i<3;i++) assert(simUpgradeAge(s));
	assert(s.age==4 && near(s.units[0].maxHealth,4700));
	assert(!simUpgradeAge(s));
}
static void combatAndRewards()
{
	Sim s; simInit(s,0); quiet(s);
	Unit& a=troop(s,0,0,8.0f);
	Unit& b=troop(s,1,0,8.66f);
	tick(s,19); assert(near(a.health,55) && near(b.health,55));
	tick(s,1); assert(a.health>39 && a.health<=41 && b.health>39 && b.health<=41);
	// Friendly ranks must stop behind one another instead of overlapping.
	Sim ranks; simInit(ranks,0); quiet(ranks);
	Unit& front=troop(ranks,0,0,8.5f);
	Unit& back=troop(ranks,0,0,8.0f);
	tick(ranks,1); assert(near(back.x,8.0f)); assert(front.x>8.5f);
	// Both kinds of casualties award player XP, but only enemy losses pay cash.
	Sim rewards; simInit(rewards,0); quiet(rewards);
	Unit& victim=troop(rewards,1,0,8.0f); victim.health=1;
	Bullet& shot=rewards.bullets[0];
	shot.alive=true; shot.team=0; shot.kind=21; shot.x=8; shot.y=GROUND_LEVEL+0.2f; shot.damage=20; shot.life=1;
	tick(rewards,1); assert(!victim.alive); assert(near(rewards.coins,195) && near(rewards.exp,40));
	Unit& ally=troop(rewards,0,0,10.0f); ally.health=1;
	shot.alive=true; shot.team=1; shot.x=10; shot.y=GROUND_LEVEL+0.2f; shot.vx=shot.vy=shot.gravity=0;
	tick(rewards,1); assert(!ally.alive); assert(near(rewards.coins,195) && near(rewards.exp,50));
}
static void aiSchedule()
{
	Sim s; simInit(s,0); quiet(s);
	tick(s,999); assert(!s.turrets[1][0].alive);
	tick(s,1); assert(s.turrets[1][0].alive && s.turrets[1][0].type==0);
	tick(s,500); assert(s.enemyUnitLevel==2);
	tick(s,2500); assert(s.turrets[1][0].type==1);
	tick(s,1000); assert(s.enemyUnitLevel==3);
	tick(s,1000); assert(s.turrets[1][0].type==2);
	float old=s.units[1].health;
	tick(s,2000); assert(s.enemyTechLevel==2 && s.enemyUnitLevel==1);
	assert(near(s.units[1].maxHealth,1100));
	// Use a small damaged health value to check that evolution does not refill.
	s.units[1].health=123; s.enemyUnitTimer=7999/FLASH_FPS;
	tick(s,1); assert(s.enemyTechLevel==3 && near(s.units[1].health,1023));
	assert(s.spell.kind==0); (void)old;
	// First enemy must be the club man, not the slingshot man.
	Sim spawn; simInit(spawn,2);
	while (spawn.unitCount==2 && spawn.frame<1000) tick(spawn,1);
	assert(spawn.unitCount>2 && spawn.units[2].typeId==0 && near(spawn.units[2].maxHealth,110));
}
static void specialsAndSlots()
{
	Sim s; simInit(s,0); quiet(s); tick(s,1);
	assert(simCastSpecial(s)); assert(!simCastSpecial(s));
	tick(s,200); assert(s.spell.kind==0);
	assert(!simCastSpecial(s));
	tick(s,1800); assert(simCastSpecial(s));
	Sim heal; simInit(heal,0); quiet(heal); heal.age=2; heal.techLevel=3; tick(heal,1);
	Unit& u=troop(heal,0,6,8); u.health=50;
	assert(simCastSpecial(heal)); tick(heal,20); assert(near(u.health,70));
	Unit& fresh=troop(heal,0,6,7); fresh.health=100;
	tick(heal,20); assert(near(fresh.health,120));
	assert(heal.spell.kind==3); tick(heal,560); assert(heal.spell.kind==0);
	Sim slots; simInit(slots,0); slots.coins=20000;
	assert(!simPlaceTurret(slots,1,0)); assert(!simPlaceTurret(slots,0,3));
	assert(simPlaceTurret(slots,0,0)); assert(!simPlaceTurret(slots,0,0));
	float paid=slots.coins; assert(simSellTurret(slots,0)); assert(near(slots.coins,paid+50));
	assert(simAddExpansion(slots) && simAddExpansion(slots) && simAddExpansion(slots));
	assert(!simAddExpansion(slots)); assert(simPlaceTurret(slots,3,2));
}
static void longSessionAndResults()
{
	Sim s; simInit(s,0); quiet(s);
	int lastId=0;
	for (int n=0;n<200;n++)
	{
		s.coins=1000; assert(simQueueUnit(s,0)); tick(s,40);
		int found=0;
		for (int j=2;j<s.unitCount;j++) if(s.units[j].alive) { found++; assert(s.units[j].id>lastId); lastId=s.units[j].id; s.units[j].alive=false; s.units[j].deathTimer=0; }
		assert(found==1 && s.unitCount==3);
	}
	Sim end; simInit(end,0); quiet(end);
	end.units[1].health=1;
	Unit& a=troop(end,0,0,17.0f); (void)a;
	tick(end,100); assert(end.gameOver && simBase(end,1)!=NULL && simBase(end,1)->health<=0);
	assert(!simQueueUnit(end,0));
}
static void allAgesAndFrameRates()
{
	// Hit every original turret schedule event, including the late future age.
	const int events[][4] = {
		{2,1000,0,3},{2,4000,0,5},{2,6000,1,4},
		{3,1000,0,6},{3,4000,1,6},{3,6000,2,8},
		{4,5000,0,9},{4,7000,1,10},
		{5,5000,0,12},{5,12000,1,13},{5,20000,3,14}
	};
	for (const auto& e:events)
	{
		Sim s; simInit(s,0); quiet(s); s.enemyTechLevel=e[0]; s.enemyUnitTimer=(e[1]-1)/FLASH_FPS;
		tick(s,1); assert(s.turrets[1][e[2]].alive && s.turrets[1][e[2]].type==e[3]);
	}
	Sim a,b; simInit(a,0); simInit(b,0); quiet(a); quiet(b);
	srand(7); tick(a,400);
	srand(7); for(int i=0;i<600;i++) simStep(b,1.0f/60.0f);
	assert(a.frame==400 && b.frame==400 && near(a.elapsed,b.elapsed));
	// Specials are projectiles, not an unavoidable map-wide base hit.
	Sim spell; simInit(spell,0); quiet(spell); tick(spell,1);
	float hp=spell.units[1].health; assert(simCastSpecial(spell));
	tick(spell,400); assert(near(spell.units[1].health,hp));
	// Autoplay battles through every age/difficulty to exercise all actions.
	for (int difficulty=0;difficulty<3;difficulty++)
		for (int age=0;age<5;age++)
		{
			Sim s; simInit(s,difficulty); s.age=age; s.techLevel=s.enemyTechLevel=age+1;
			s.coins=1000000;
			for(int i=0;i<5;i++) assert(simQueueUnit(s,age*3+i%3));
			assert(simPlaceTurret(s,0,age*3));
			tick(s,5000);
			assert(s.unitCount<=MAX_UNITS);
			for(int i=0;i<s.unitCount;i++) assert(std::isfinite(s.units[i].health) && std::isfinite(s.units[i].x));
		}
}
int main(int argc,char** argv)
{
	if(argc>1 && strcmp(argv[1],"--tables")==0)
	{
		printf("FPS %.0f\n",FLASH_FPS);
		for(int i=0;i<16;i++) { const UnitDef& d=UNIT_DEFS[i]; printf("U %d %d %d %d %d %.3f %.3f %.3f\n",i,d.cost,d.health,d.meleeDamage,d.rangedDamage,d.meleeRange,d.rangedRange,d.buildTime); }
		for(int i=0;i<15;i++) { const TurretDef& d=TURRET_DEFS[i]; printf("T %d %d %.3f %d %.3f %d\n",i,d.cost,d.cooldown,d.damage,d.range,TURRET_BULLET[i]); }
		return 0;
	}
	srand(1);
	initialAndQueue(); progression(); combatAndRewards(); aiSchedule(); specialsAndSlots(); longSessionAndResults();
	allAgesAndFrameRates();
	puts("PASS: queue, progression, combat, rewards, all AI schedules, specials, slots, 200-unit session, results, 15 autoplay battles, frame rates");
}
