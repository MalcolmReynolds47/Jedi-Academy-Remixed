// Native trace, server authority and real helper/copy/world lifetimes.
// Collision and native health damage are deterministic engine boundaries.
static int laserHit,laserSolid,laserPass,laserMiss;static vec3_t laserStart,laserEnd;
static void LaserTraceFixture(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 if(!mins&&mask==(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)){
  memset(t,0,sizeof(*t));t->fraction=laserMiss?1:.5f;t->entityNum=laserHit;t->startsolid=laserSolid;
  VectorCopy(start,laserStart);VectorCopy(end,laserEnd);laserPass=pass;for(int a=0;a<3;++a)t->endpos[a]=start[a]+t->fraction*(end[a]-start[a]);return;
 }
 RigidTrace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);
}
static void LaserTick(gentity_t*h){ControllerTick(h,FRAMETIME);}
static gentity_t*LaserFixture(void){
 RigidFixture();imports.SendServerCommand=Send;imports.Trace=LaserTraceFixture;laserDamageCalls=laserDestroySource=laserSolid=laserMiss=0;laserHit=ENTITYNUM_WORLD;
 for(int c=0;c<3;++c)g_entities[c].takedamage=qtrue;
 gentity_t*e=ControllerAt("target_laser",0,100);assert(e&&LaserHelper(e->s.number));return e;
}
static void LaserCases(void){
 gentity_t*e=LaserFixture(),*p=&g_entities[0],*h=LaserHelper(e->s.number);int n=e->s.number;
 assert(h->s.eType==ET_BEAM&&h->r.linked&&!h->r.contents&&!objects[h->s.number].owner&&e->damage==1&&objects[n].laserActive);
 assert(laserPass==n&&fabsf(Distance(laserStart,laserEnd)-2048)<.01f&&h->s.origin2[0]==laserStart[0]+1024&&!laserDamageCalls);
 assert(h->r.maxs[0]==1028&&h->r.mins[0]==-4&&VectorCompare(h->s.pos.trBase,h->s.origin));
 // Slot zero and creator damage are valid; every player retains tool consent.
 laserHit=0;LaserTick(h);assert(laserDamageCalls==1&&laserVictim==0&&laserAttacker==n&&laserAmount==1);
 ControllerShot(p,n,qfalse);assert(!objects[n].laserActive&&!h->r.linked&&!h->nextthink);ControllerShot(p,n,qfalse);assert(objects[n].laserActive&&h->r.linked);LaserTick(h);assert(laserAttacker==0);
 int calls=laserDamageCalls;
 for(int refusal=0;refusal<8;++refusal){
  laserHit=0;laserMiss=refusal==0;laserSolid=refusal==1;playerTools.integer=refusal==2?0:1;players[0].noTools=refusal==3;players[0].noGrabUntil=refusal==4?level.time+10000:0;g_entities[0].takedamage=refusal==5?qfalse:qtrue;
  if(refusal==6)laserHit=ENTITYNUM_NONE;if(refusal==7)laserHit=ENTITYNUM_WORLD;
  LaserFrame();LaserTick(h);assert(laserDamageCalls==calls);
 }
 laserMiss=laserSolid=players[0].noTools=players[0].noGrabUntil=0;playerTools.integer=1;g_entities[0].takedamage=qtrue;
 // A moved/rotated emitter updates the beam endpoints without a damage pulse.
 laserHit=ENTITYNUM_WORLD;Move(e,(float[3]){300,50,100});VectorSet(e->r.currentAngles,0,90,0);LaserFrame();assert(laserDamageCalls==calls&&fabsf(laserEnd[0]-laserStart[0])<.01f&&fabsf(laserEnd[1]-laserStart[1]-2048)<.01f);
 // Primary tracks current hull centers, including ordinary props; it does not
 // use stale s.origin. Secondary, map and cyclic links are refused.
 gentity_t*t=CopySpawn(1,0,600);int target=t->s.number;VectorSet(t->s.origin,-9000,-9000,-9000);SwitchLink(p,e,t,0);LaserFrame();vec3_t expected;for(int a=0;a<3;++a)expected[a]=t->r.currentOrigin[a]+.5f*(t->r.mins[a]+t->r.maxs[a])-h->s.origin[a];VectorNormalize(expected);assert(DotProduct(expected,h->movedir)>.99999f);
 Move(t,(float[3]){300,800,250});LaserFrame();for(int a=0;a<3;++a)expected[a]=t->r.currentOrigin[a]+.5f*(t->r.mins[a]+t->r.maxs[a])-h->s.origin[a];VectorNormalize(expected);assert(DotProduct(expected,h->movedir)>.99999f);
 players[0].linkOutput=1;ControllerLink(p,n,qfalse);ControllerLink(p,target,qfalse);assert(!objects[n].output[1]);players[0].linkOutput=0;
 t->takedamage=qtrue;laserHit=target;objects[target].owner=2;LaserFrame();assert(!h->r.linked&&!h->nextthink&&objects[n].laserActive);sharedEditing.integer=1;LaserFrame();LaserTick(h);assert(h->r.linked&&laserDamageCalls==calls+1);sharedEditing.integer=0;objects[target].owner=1;
 t->flags|=FL_INACTIVE;LaserFrame();assert(!h->r.linked);t->flags&=~FL_INACTIVE;objects[target].copyPending=1;LaserFrame();assert(!h->r.linked);objects[target].copyPending=0;LaserFrame();assert(h->r.linked);
 // Typed properties are bounded, support harmless visual-only beams, and keep
 // pending tick timing through Undo instead of causing damage during editing.
 ControllerSelect(e);Command(0,"jar_property","controller_damage_5000");assert(e->damage==5000);Command(0,"jar_property","property_undo");assert(e->damage==1);Command(0,"jar_property","controller_damage_0");calls=laserDamageCalls;LaserTick(h);assert(h->r.linked&&laserDamageCalls==calls);
 for(const char**a=(const char*[]){"controller_damage_-1","controller_damage_32768","controller_damage_nan","controller_damage_1.5","controller_text_41","controller_mode",NULL};*a;++a){Command(0,"jar_property",*a);assert(e->damage==0&&h->r.linked);}
 Command(0,"jar_property","controller_damage_37");Command(0,"jar_property","controller_enabled");assert(!h->r.linked&&objects[n].laserActive);Command(0,"jar_property","property_undo");assert(h->r.linked);LaserTick(h);assert(laserAmount==37);
 // Native switch controllers immediately suppress/resume the derived beam.
 gentity_t*off=ControllerAt("target_deactivate",0,1000),*on=ControllerAt("target_activate",0,1200);SwitchLink(p,off,e,0);SwitchLink(p,on,e,0);
 ControllerShot(p,off->s.number,qfalse);assert(!h->r.linked&&objects[n].laserActive);ControllerShot(p,on->s.number,qfalse);assert(h->r.linked);
 sandbox.integer=0;LaserFrame();assert(!h->r.linked);sandbox.integer=1;LaserFrame();assert(h->r.linked);
 // Recomputed checksums do not allow invalid damage or non-laser damage fields.
 static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&world));int size=WorldEncode(&world,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 for(int i=0;i<world.copy.count;++i){int at=WORLD_HEADER+i*WORLD_NODE+268;memcpy(bad,bytes,size);WorldPut(bad,&at,i==0?32768:1);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);}
 CopyAssembly(p,n);assert(copies[0].count==4&&CopyControllersValid(&copies[0]));assert(WorldClear());for(int i=MAX_CLIENTS;i<level.num_entities;++i)assert(!laserParent[i]);
 assert(PlaceCopy(NULL,&world.copy,vec3_origin,world.velocity,world.angular));for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(ControllerKind(i)==13)e=&g_entities[i];n=e->s.number;h=LaserHelper(n);assert(h&&objects[n].laserActive&&e->damage==37&&objects[n].output[0]);
 worldEditing.integer=1;laserHit=0;LaserTick(h);assert(laserVictim==0&&laserAttacker==n);players[0].noTools=1;calls=laserDamageCalls;LaserTick(h);assert(laserDamageCalls==calls);players[0].noTools=0;
 assert(WorldClear()&&PlaceCopy(p,&copies[0],(float[3]){1000,0,80},NULL,NULL));n=players[0].selected;e=&g_entities[n];h=LaserHelper(n);assert(ControllerKind(n)==13&&e->damage==37&&h);
 target=objects[n].output[0]-1;G_FreeEntity(&g_entities[target]);assert(!objects[n].output[0]&&!objects[n].laserActive&&!h->r.linked);t=G_Spawn();assert(t->s.number==target);LaserFrame();assert(!h->r.linked);G_FreeEntity(t);
 // A lost helper is never confused with a replacement in the same slot.
 int helper=h->s.number;G_FreeEntity(h);t=G_Spawn();assert(t->s.number==helper);ControllerShot(p,n,qfalse);assert(LaserHelper(n)&&LaserHelper(n)!=t);G_FreeEntity(e);assert(t->inuse&&!laserParent[helper]);G_FreeEntity(t);assert(WorldClear());
 // Damage can synchronously free the source and reuse its slot. No stale
 // relink/nextthink write can mutate that new entity on callback return.
 e=LaserFixture();n=e->s.number;h=LaserHelper(n);laserHit=1;laserDestroySource=1;LaserTick(h);assert(!strcmp(g_entities[n].classname,"replacement")&&!g_entities[n].r.linked&&!g_entities[n].nextthink&&!objects[n].laserHelper);laserDestroySource=0;G_FreeEntity(&g_entities[n]);
 // Retained server-owned beams do not credit a newly connected slot occupant.
 e=LaserFixture();n=e->s.number;h=LaserHelper(n);ControllerShot(p,n,qfalse);ControllerShot(p,n,qfalse);cleanupDisconnect.integer=0;JAR_MP_Disconnect(0);laserHit=1;LaserTick(h);assert(laserVictim==1&&laserAttacker==n&&objects[n].owner==MAX_CLIENTS+1);assert(WorldClear());
 // Context Firing and Undo restart attribute damage to the current editor.
 e=LaserFixture();n=e->s.number;h=LaserHelper(n);ControllerShot(p,n,qfalse);sharedEditing.integer=1;players[1].selected=n;Command(1,"jar_property","controller_playing");laserHit=0;LaserTick(h);assert(laserAttacker==1);
 Command(1,"jar_property","controller_playing");assert(!objects[n].laserActive);Command(1,"jar_property","property_undo");LaserTick(h);assert(objects[n].laserActive&&laserAttacker==1);assert(WorldClear());
 imports.Trace=RigidTrace;PhysicsReset();puts("MP laser: native 2048-unit trace, slot-zero/self damage, eight refusal cases, current-position aiming, visual-only mode, switches, property Undo, JRW12, copy/cleanup, helper/source reuse and connection attribution passed.");
}
