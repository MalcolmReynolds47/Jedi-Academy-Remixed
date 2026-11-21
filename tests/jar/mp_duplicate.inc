// Production clipboard/transactions with deterministic native parser/trace imports.
static int CopyCount(int owner){int count=0;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner==owner+1)++count;return count;}
static int LiveCount(void){int count=0;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse)++count;return count;}
static unsigned CopyId(int kind){for(unsigned n=0;n<JAR_MP_CATALOG_COUNT;++n)if(jarMPCatalog[n].kind==kind)return jarMPCatalog[n].id;assert(0);return 0;}
static gentity_t*CopySpawn(int kind,int owner,float x){vec3_t point={0,0,80},angles={0,0,0};point[0]=x;return SpawnAt(&g_entities[owner],CopyId(kind),point,angles);}
static gentity_t*CopyLink(int owner,int first,int second){
 gentity_t*e=G_Spawn();Track(e,owner,-1);objects[e->s.number].first=first+1;objects[e->s.number].second=second+1;objects[e->s.number].length=90;
 VectorSet(objects[e->s.number].anchorA,1,2,3);VectorSet(objects[e->s.number].anchorB,4,5,6);e->s.generic1=251;return e;
}
static void CopyReady(gentity_t*e){e->health=e->client->ps.stats[STAT_HEALTH]=100;e->s.eFlags&=~EF_NODRAW;JAR_MP_CopyNPCReady(e);}
static qboolean copyFloorDestination;
static void CopyFloorTrace(trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,
                           int pass,int mask,int capsule,int flags,int lod){
 memset(hit,0,sizeof(*hit));hit->fraction=1;VectorCopy(end,hit->endpos);
 if(mins)return;
 const float dz=start[2]-end[2];
 if(fabsf(dz-4)<.01f){
  if(start[2]<0||end[2]>0||(start[0]>250&&!copyFloorDestination))return;
  hit->fraction=start[2]/dz;hit->endpos[2]=0;
 }else if(!VectorCompare(start,end)){
  hit->fraction=.5f;VectorSet(hit->endpos,500,0,0);
 }else return;
 hit->entityNum=ENTITYNUM_WORLD;hit->plane.normal[2]=1;
}
static void CopyWorldCases(void){
 ActorFixture();
 imports.Trace=CopyFloorTrace;
 gentity_t* player=&g_entities[0];
 for(int kind=0;kind<=2;++kind){
  gentity_t* body=CopySpawn(1,0,100);assert(body);
  const int first=kind==2?ENTITYNUM_WORLD:body->s.number;
  const int second=kind==2?body->s.number:ENTITYNUM_WORLD;
  gentity_t* entity=CopyLink(0,first,second);
  jarObject_t* link=&objects[entity->s.number];
  link->jointKind=kind;
  VectorSet(kind==2?link->anchorA:link->anchorB,100,0,0);
  VectorSet(kind==2?link->anchorB:link->anchorA,0,0,kind?-80:0);
  if(kind){link->length=0;link->jointA[3]=link->jointB[3]=1;}
  else {VectorSet(link->jointB,0,0,1);link->jointB[3]=1;}
  CopyAssembly(player,body->s.number);
  assert(copies[0].count==1&&copies[0].ropes==1&&copies[0].rope[0].copyWorldNormal[2]==1);
  G_FreeEntity(body);
  const int live=LiveCount();
  copyFloorDestination=qfalse;PasteAssembly(player);assert(LiveCount()==live);
  copyFloorDestination=qtrue;PasteAssembly(player);assert(LiveCount()==live+2);
  assert(g_entities[players[0].selected].r.currentOrigin[2]==80);
  for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&g_entities[n].s.generic1==251){
   const jarObject_t* pasted=&objects[n];
   const float* point=kind==2?pasted->anchorA:pasted->anchorB;
   assert(pasted->jointKind==kind&&point[0]==500&&point[2]==0);
  }
  Undo(player,qfalse);assert(LiveCount()==live);
 }
 imports.Trace=Trace;
}
static void DuplicateCases(void){
 ActorFixture();int c=0;gentity_t*a=CopySpawn(1,0,100),*b=CopySpawn(1,0,170),*other=CopySpawn(1,1,300);assert(a&&b&&other);
 int an=a->s.number,bn=b->s.number;gentity_t*rope=CopyLink(0,an,bn);CopyLink(0,an,ENTITYNUM_WORLD);
 objects[an].mass=200;objects[an].frozen=1;objects[an].noGravity=1;objects[an].thruster=1;VectorSet(a->r.currentAngles,20,60,5);
 players[0].tool=3;aimed=an;Tool(&g_entities[0],qtrue);assert(copies[0].count==2&&copies[0].ropes==1&&copies[1].count==0);
 assert(copies[0].node[0].mass==200&&copies[0].node[0].frozen&&copies[0].node[0].thruster&&copies[0].node[1].offset[0]==70);
 assert(copies[0].rope[0].a[0]==1&&copies[0].rope[0].b[2]==6&&copies[0].rope[0].length==90);
 // Every connected member AND rope requires current copy permission.
 objects[bn].owner=2;CopyAssembly(&g_entities[1],an);assert(!copies[1].count);objects[bn].owner=1;
 objects[rope->s.number].owner=2;CopyAssembly(&g_entities[1],an);assert(!copies[1].count);sharedEditing.integer=1;CopyAssembly(&g_entities[1],an);assert(copies[1].count==2);sharedEditing.integer=0;objects[rope->s.number].owner=1;
 // The snapshot survives source deletion/reuse and keeps properties at copy time.
 G_FreeEntity(a);G_FreeEntity(b);assert(copies[0].count==2);a=CopySpawn(1,0,500);assert(a->s.number==an);objects[an].mass=5;
 int before=CopyCount(0);aimed=ENTITYNUM_WORLD;Tool(&g_entities[0],qfalse);assert(CopyCount(0)==before+3);int root=players[0].selected;unsigned group=objects[root].order;
 assert(root!=an&&objects[root].mass==200&&objects[root].frozen&&objects[root].thruster&&objects[root].noGravity&&g_entities[root].r.currentAngles[YAW]==60);
 int partner=-1,newRope=-1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==1&&objects[n].order==group){if(g_entities[n].s.generic1==251)newRope=n;else if(n!=root)partner=n;}
 assert(partner>=0&&newRope>=0&&g_entities[partner].r.currentOrigin[0]-g_entities[root].r.currentOrigin[0]==70);
 assert(objects[newRope].first==root+1&&objects[newRope].second==partner+1&&objects[newRope].length==90&&g_entities[newRope].s.time2==90);
 assert(g_entities[root].r.linked&&g_entities[partner].r.linked);Undo(&g_entities[0],qfalse);assert(CopyCount(0)==before&&g_entities[an].inuse&&other->inuse);
 // Other clients' immutable clipboards produce their OWN editable groups.
 PasteAssembly(&g_entities[1]);assert(CopyCount(1)==4&&CopyCount(0)==before);Undo(&g_entities[1],qfalse);assert(CopyCount(1)==1);
 // Preflight rejects collisions, missing assets and capacities without mutations.
 before=LiveCount();blocked=1;PasteAssembly(&g_entities[0]);assert(LiveCount()==before);blocked=0;
 missingModel=1;PasteAssembly(&g_entities[0]);assert(LiveCount()==before);missingModel=0;
 // Fail after first member allocation; rollback removes only provisional members.
 traceCalls=0;blockAt=4;PasteAssembly(&g_entities[0]);assert(LiveCount()==before&&copies[0].count==2&&g_entities[an].inuse);blockAt=0;
 // Filling the entity pool cannot evict originals or partially paste.
 byte occupiedBefore[MAX_GENTITIES];for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n){occupiedBefore[n]=(byte)g_entities[n].inuse;g_entities[n].inuse=qtrue;}
 PasteAssembly(&g_entities[0]);assert(objects[an].mass==5);for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD;++n)g_entities[n].inuse=occupiedBefore[n];
 CopyReset(1);assert(!copies[1].count&&copies[0].count==2);
 // Pickups retain quantities and an item-rule rejection rolls the WHOLE group back.
 ActorFixture();unsigned baton=0;int batonIndex=0;for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(!strcmp(jarMPCatalog[i].name,"weapon_stun_baton")){baton=jarMPCatalog[i].id;batonIndex=i;break;}
 vec3_t point={180,0,80},angles={0,0,0};a=CopySpawn(1,0,100);b=SpawnAt(&g_entities[0],baton,point,angles);assert(b);b->count=17;CopyLink(0,a->s.number,b->s.number);CopyAssembly(&g_entities[0],a->s.number);
 before=LiveCount();itemDisabled=1;PasteAssembly(&g_entities[0]);assert(LiveCount()==before&&a->inuse&&b->inuse);itemDisabled=0;PasteAssembly(&g_entities[0]);root=players[0].selected;group=objects[root].order;
 int copiedPickup=-1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==1&&objects[n].order==group&&objects[n].catalog==batonIndex+1)copiedPickup=n;
 assert(copiedPickup>=0&&g_entities[copiedPickup].count==17);Undo(&g_entities[0],qfalse);assert(LiveCount()==before);
 // Native actors join mixed assemblies. Apply health/freeze AFTER native begin.
 ActorFixture();a=CopySpawn(1,0,100);b=CopySpawn(2,0,170);assert(b&&b->NPC);CopyReady(b);b->health=b->client->ps.stats[STAT_HEALTH]=175;BeginActor(&g_entities[0],b);objects[b->s.number].frozen=1;
 CopyLink(0,a->s.number,b->s.number);CopyAssembly(&g_entities[0],a->s.number);assert(copies[0].count==2&&copies[0].node[1].health==175&&copies[0].node[1].frozen);
 before=LiveCount();PasteAssembly(&g_entities[0]);root=players[0].selected;group=objects[root].order;int copiedActor=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].order==group&&objects[n].copyPending)copiedActor=n;
 assert(copiedActor>=0&&!actors[copiedActor].active&&g_entities[copiedActor].health==0);
 vec3_t savedOrigin,savedAngles;VectorCopy(objects[copiedActor].copyOrigin,savedOrigin);VectorCopy(objects[copiedActor].copyAngles,savedAngles);
 g_entities[copiedActor].r.currentOrigin[2]-=8;g_entities[copiedActor].client->ps.origin[2]-=8;g_entities[copiedActor].r.currentAngles[YAW]+=45;
 CopyReady(&g_entities[copiedActor]);assert(!objects[copiedActor].copyPending&&g_entities[copiedActor].health==175&&objects[copiedActor].frozen&&actors[copiedActor].active);
 assert(VectorCompare(savedOrigin,g_entities[copiedActor].r.currentOrigin)&&VectorCompare(savedAngles,g_entities[copiedActor].r.currentAngles));
 int goal=g_entities[copiedActor].NPC->tempGoal->s.number;Undo(&g_entities[0],qfalse);assert(!g_entities[goal].inuse&&!g_entities[copiedActor].r.linked&&LiveCount()==before);
 // Delayed failure and timeout remove every group member, including native goals.
 PasteAssembly(&g_entities[0]);group=objects[players[0].selected].order;blocked=1;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].order==group&&objects[n].copyPending){CopyReady(&g_entities[n]);break;}blocked=0;assert(LiveCount()==before);
 PasteAssembly(&g_entities[0]);root=players[0].selected;group=objects[root].order;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].order==group&&objects[n].copyPending){G_FreeEntity(&g_entities[n]);break;}assert(LiveCount()==before);
 PasteAssembly(&g_entities[0]);level.time+=3001;CopyFrame();assert(LiveCount()==before);
 PasteAssembly(&g_entities[0]);cleanupDisconnect.integer=0;JAR_MP_Disconnect(0);assert(LiveCount()==before&&!copies[0].count); // Pending groups never transfer to another connection.
 // If another member fails after a copied vehicle has been boarded, defer
 // group removal until its remaining occupant leaves. Never erase their body.
 ActorFixture();a=CopySpawn(3,0,100);b=CopySpawn(2,0,240);CopyReady(a);CopyReady(b);CopyLink(0,a->s.number,b->s.number);CopyAssembly(&g_entities[0],a->s.number);before=LiveCount();PasteAssembly(&g_entities[0]);root=players[0].selected;group=objects[root].order;CopyReady(&g_entities[root]);g_entities[root].m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[1];
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(n!=root&&objects[n].order==group&&objects[n].copyPending){G_FreeEntity(&g_entities[n]);break;}
 CopyFrame();assert(g_entities[root].inuse&&objects[root].copyPending==-1);g_entities[root].m_pVehicle->m_pPilot=NULL;CopyFrame();assert(!g_entities[root].inuse&&LiveCount()==before);
 // Vehicles restart their native definition; occupied vehicles are not copied.
 ActorFixture();a=CopySpawn(3,0,200);CopyReady(a);a->health=230;a->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[0];CopyAssembly(&g_entities[0],a->s.number);assert(!copies[0].count);a->m_pVehicle->m_pPilot=NULL;
 CopyAssembly(&g_entities[0],a->s.number);assert(copies[0].count==1);before=LiveCount();PasteAssembly(&g_entities[0]);root=players[0].selected;CopyReady(&g_entities[root]);assert(g_entities[root].health==230);
 gentity_t*droid=CopySpawn(2,0,220);int dn=droid->s.number;droid->client->ps.m_iVehicleNum=root;g_entities[root].m_pVehicle->m_pDroidUnit=(bgEntity_t*)droid;
 // Native droids are helpers, not an independent Undo entry.
 memset(&objects[dn],0,sizeof(objects[dn]));
 g_entities[root].m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[1];int withDroid=LiveCount();Undo(&g_entities[0],qfalse);assert(LiveCount()==withDroid);Undo(&g_entities[0],qtrue);assert(LiveCount()==withDroid);g_entities[root].m_pVehicle->m_pPilot=NULL;
 // A vehicle's own native droid is not a player passenger and does not block Undo.
 Undo(&g_entities[0],qfalse);assert(!g_entities[dn].inuse&&LiveCount()==before);
 a->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[1];cleanupDisconnect.integer=1;JAR_MP_Disconnect(0);assert(a->inuse&&objects[a->s.number].owner==MAX_CLIENTS+1&&!copies[0].count);a->m_pVehicle->m_pPilot=NULL;
 JAR_MP_Init();assert(!copies[0].count&&!copies[1].count);
 CopyWorldCases();
 puts("MP assembly copying: properties, rope graph, permissions, placement, rollback, group Undo, native readiness/helpers and connection lifetime passed.");
}
