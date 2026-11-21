// Native dispensing/recharge plus production commands, copies and world codec.
static void StationTick(gentity_t*e,int ms){level.time+=ms;e->think(e);}
static void StationCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0];p->client->ps.stats[STAT_MAX_HEALTH]=100;
 for(int i=AMMO_BLASTER;i<AMMO_MAX;++i)ammoData[i].max=100;
 const char*names[]={"misc_ammo_floor_unit","misc_shield_floor_unit","misc_model_shield_power_converter","misc_model_ammo_power_converter","misc_model_health_power_converter"};
 gentity_t*station[5];
 for(int i=0;i<5;++i){
  gentity_t*e=station[i]=ControllerAt(names[i],0,100+i*200);assert(e&&ControllerKind(e->s.number)==24+i&&e->use==StationUse&&e->think==StationThink&&(e->r.svFlags&SVF_PLAYER_USABLE)&&PhysicsProp(e->s.number)&&e->count==200&&e->genericValue4==200&&e->genericValue5==100);
  p->health=p->client->ps.stats[STAT_HEALTH]=50;p->client->ps.stats[STAT_ARMOR]=0;memset(p->client->ps.ammo,0,sizeof(p->client->ps.ammo));p->client->pers.cmd.buttons=BUTTON_USE;
  GlobalUse(e,p,p);assert(e->count<200&&e->setTime>=level.time+100&&e->activator==p);
  if(i==0||i==3)assert(p->client->ps.ammo[AMMO_BLASTER]==(i==0?5:10));else if(i==4)assert(p->health==55&&p->client->ps.stats[STAT_HEALTH]==55&&e->count==195);else assert(p->client->ps.stats[STAT_ARMOR]==2&&e->count==198);
  int charge=e->count;GlobalUse(e,p,p);assert(e->count==charge);StationTick(e,101);assert(e->count==charge);GlobalUse(e,p,p);assert(e->count<charge);
  // Release USE ends the native loop; recharge adds one unit per configured tick.
  p->client->pers.cmd.buttons=0;charge=e->count;StationTick(e,101);assert(!e->s.loopSound&&!e->activator&&e->count==charge+1);StationTick(e,50);assert(e->count==charge+1);StationTick(e,51);assert(e->count==charge+2);
  ControllerSelect(e);Command(0,"jar_property","controller_count_300");assert(e->count==300&&e->genericValue4==300);Command(0,"jar_property","property_undo");assert(e->genericValue4==200&&e->count==charge+2);
  Command(0,"jar_property","controller_charge_0");assert(!e->count);Command(0,"jar_property","controller_nodrain");assert(e->genericValue12==1);e->setTime=0;p->health=50;p->client->ps.stats[STAT_ARMOR]=0;memset(p->client->ps.ammo,0,sizeof(p->client->ps.ammo));GlobalUse(e,p,p);assert(!e->count&&(i==4?p->health==55:i==1||i==2?p->client->ps.stats[STAT_ARMOR]==2:p->client->ps.ammo[AMMO_BLASTER]>0));
  Command(0,"jar_property","controller_nodrain");Command(0,"jar_property","controller_reset");Command(0,"jar_property","controller_delay_500");assert(e->count==200&&e->genericValue5==500);
  const char*bad[]={"controller_count_0","controller_count_2147483648","controller_charge_201","controller_delay_0","controller_delay_-1","controller_charge_nan"};for(int j=0;j<6;++j)Command(0,"jar_property",bad[j]);assert(e->count==200&&e->genericValue4==200&&e->genericValue5==500);
 }
 // A restored/world-owned station may serve consenting players; restrictions
 // are rechecked on every direct use and recharge think, not cached at spawn.
 gentity_t*health=station[4];int n=health->s.number;objects[n].owner=MAX_CLIENTS+1;worldEditing.integer=1;health->setTime=0;p->health=50;GlobalUse(health,p,p);assert(p->health==55);objects[n].owner=1;
 for(int refusal=0;refusal<5;++refusal){p->health=50;health->setTime=0;players[0].noTools=refusal==0;playerTools.integer=refusal==1?0:1;health->flags=refusal==2?FL_INACTIVE:0;objects[n].copyPending=refusal==3;p->client->sess.sessionTeam=refusal==4?TEAM_SPECTATOR:TEAM_FREE;GlobalUse(health,p,p);assert(p->health==50);}
 players[0].noTools=0;playerTools.integer=1;health->flags=0;objects[n].copyPending=0;p->client->sess.sessionTeam=TEAM_FREE;health->setTime=0;p->client->pers.cmd.buttons=BUTTON_USE;GlobalUse(health,p,p);assert(health->activator==p);++controllerClientEpoch[0];StationTick(health,1);assert(!health->activator&&!health->s.loopSound);
 // Native class armor limits, double ammo and owned NPC recipients stay native.
 gentity_t*shield=station[1];level.gametype=GT_SIEGE;bgSiegeClasses[1].maxarmor=20;p->client->siegeClass=1;p->client->ps.stats[STAT_ARMOR]=19;shield->setTime=0;GlobalUse(shield,p,p);assert(p->client->ps.stats[STAT_ARMOR]==20);level.gametype=GT_FFA;p->client->siegeClass=-1;
 gentity_t*ammo=station[0];ammo->setTime=0;p->client->ps.eFlags|=EF_DOUBLE_AMMO;p->client->ps.ammo[AMMO_BLASTER]=199;GlobalUse(ammo,p,p);assert(p->client->ps.ammo[AMMO_BLASTER]==200);p->client->ps.eFlags&=~EF_DOUBLE_AMMO;
 gentity_t*npc=CopySpawn(2,0,1800);CopyReady(npc);npc->health=40;npc->client->ps.stats[STAT_MAX_HEALTH]=100;health->setTime=0;GlobalUse(health,npc,npc);assert(npc->health==45);objects[npc->s.number].owner=2;health->setTime=0;GlobalUse(health,npc,npc);assert(npc->health==45);G_FreeEntity(npc);StationTick(health,1);assert(!health->activator);
 int remainingCharge=health->count;health->count=0;health->setTime=0;p->health=50;GlobalUse(health,p,p);assert(p->health==50&&!health->count);health->count=remainingCharge;
 // Links use the real activator, and disable stops playback immediately.
 gentity_t*relay=ControllerAt("target_relay",0,1300),*off=ControllerAt("target_deactivate",0,1500);ControllerLink(p,relay->s.number,qfalse);ControllerLink(p,n,qfalse);health->setTime=0;p->health=50;ControllerShot(p,relay->s.number,qfalse);assert(p->health==55&&health->s.loopSound);
 ControllerLink(p,off->s.number,qfalse);ControllerLink(p,n,qfalse);ControllerShot(p,off->s.number,qfalse);assert((health->flags&FL_INACTIVE)&&!health->s.loopSound);int charge=health->count;StationTick(health,1000);assert(health->count==charge);
 ControllerSelect(health);Command(0,"jar_property","controller_enabled");health->genericValue8=level.time+400;health->setTime=level.time+90;
 CopyAssembly(p,n);assert(CopyControllersValid(&copies[0]));static jarWorld_t world,decoded;static byte bytes[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&world));int size=WorldEncode(&world,bytes),at=0;assert(WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 int h=-1;for(int i=0;i<world.copy.count;++i){int index;const jarMPEntry_t*entry=Entry(world.copy.node[i].id,&index);if(!strcmp(entry->name,names[4]))h=i;}assert(h>=0&&world.copy.node[h].controllerRemaining==400&&world.copy.node[h].stationUseRemaining==90);
 memcpy(bad,bytes,size);at=WORLD_HEADER+h*WORLD_NODE+280;WorldPut(bad,&at,0x80000000u);WorldTestHash(bad,size);decoded.generation=777;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==777);assert(WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));int found=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(JAR_ControllerStation(ControllerKind(i))){++found;gentity_t*e=&g_entities[i];assert(e->use==StationUse&&e->think==StationThink&&e->genericValue4==200&&e->genericValue5==500&&!e->s.loopSound&&!e->activator);if(ControllerKind(i)==28){assert(e->count==charge&&e->genericValue8==level.time+400&&e->setTime==level.time+90);health=e;}}
 assert(found==5);p->health=50;GlobalUse(health,p,p);assert(p->health==50);level.time+=91;GlobalUse(health,p,p);assert(p->health==55);assert(WorldClear());
 assert(PlaceCopy(p,&copies[0],(float[3]){1000,0,100},NULL,NULL));int pasted=players[0].selected;assert(g_entities[pasted].inuse);Undo(p,qfalse);assert(!g_entities[pasted].inuse&&WorldClear());
 // JRW16's former 280-byte records remain readable and default the new delay.
 gentity_t*plain=CopySpawn(1,0,100);assert(plain&&WorldCapture(&world));size=WorldEncode(&world,bytes);memcpy(bad,bytes,WORLD_HEADER);at=0;WorldPut(bad,&at,0x4757524au);memcpy(bad+WORLD_HEADER,bytes+WORLD_HEADER,280);size=WORLD_HEADER+280+4;WorldTestHash(bad,size);assert(WorldDecode(bad,size,&decoded)&&!decoded.copy.node[0].stationUseRemaining);assert(WorldClear());
 // Unowned map callbacks retain native behavior: this extraction did not
 // silently change health-station drain or the floor-ammo frame-rate quirk.
 gentity_t*map=G_Spawn();map->count=200;p->health=50;health_power_converter_use(map,p,p);assert(p->health==55&&map->count==200);map->setTime=0;memset(p->client->ps.ammo,0,sizeof(p->client->ps.ammo));ammo_generic_power_converter_use(map,p,p);ammo_generic_power_converter_use(map,p,p);assert(p->client->ps.ammo[AMMO_BLASTER]==10);G_FreeEntity(map);PhysicsReset();
 puts("MP stations: five native dispensing routes, recharge and current consent, direct use and links, typed settings/Undo, empty/infinite supply, stale clients, copies/JRW17 pending cooldown and JRW16 compatibility passed.");
}
