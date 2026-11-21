// Exercise the production file format and engine filesystem/console boundary.
typedef struct {char path[MAX_QPATH];byte data[WORLD_BYTES];int size;} testWorldFile_t;
static testWorldFile_t worldFiles[32];static int worldFileCount,worldOpens,worldCloses,worldWrites,worldWriteFail,worldShortRead,worldArgs=2;
static unsigned worldChecksum=777;static char worldMessage[512];
static int WorldTestOpen(const char*path,fileHandle_t*out,fsMode_t mode){
 if(strncmp(path,"jar/worlds/",11))return Open(path,out,mode);++worldOpens;
 int i;for(i=0;i<worldFileCount;++i)if(!strcmp(path,worldFiles[i].path))break;
 if(i==worldFileCount){if(mode==FS_READ){*out=0;return -1;}assert(i<32);Q_strncpyz(worldFiles[i].path,path,MAX_QPATH);++worldFileCount;}
 *out=i+10;if(mode==FS_WRITE){worldFiles[i].size=0;return 0;}return worldFiles[i].size;
}
static int WorldTestRead(void*data,int size,fileHandle_t f){if(f==1)return Read(data,size,f);assert(f>=10&&f<10+worldFileCount);testWorldFile_t*file=&worldFiles[f-10];assert(size<=file->size);if(worldShortRead)--size;memcpy(data,file->data,size);return size;}
static int WorldTestWrite(const void*data,int size,fileHandle_t f){assert(f>=10&&f<10+worldFileCount&&size<=WORLD_BYTES);++worldWrites;if(worldWriteFail)size/=2;testWorldFile_t*file=&worldFiles[f-10];memcpy(file->data,data,size);file->size=size;return size;}
static void WorldTestClose(fileHandle_t f){if(f>=10)++worldCloses;else Close(f);}
static void WorldTestCvar(const char*key,char*out,int size){
 if(!strcmp(key,"sv_jarMapBottom")){Q_strncpyz(out,"-65536",size);return;}
 assert(!strcmp(key,"sv_mapChecksum"));snprintf(out,size,"%d",(int)worldChecksum);
}
static int WorldTestArgc(void){return worldArgs;}
static void WorldTestPrint(const char*fmt,...){va_list args;va_start(args,fmt);vsnprintf(worldMessage,sizeof(worldMessage),fmt,args);va_end(args);}
static void WorldFixture(void){ActorFixture();Q_strncpyz(level.mapname,"maps/mp/ffa3.bsp",sizeof(level.mapname));worldWriteFail=worldShortRead=0;worldChecksum=777;}
static void WorldTestHash(byte*data,int size){int at=size-4;unsigned hash=WorldHash(data,at);WorldPut(data,&at,hash);}
static void WorldCases(void){
 imports.FS_Open=WorldTestOpen;imports.FS_Read=WorldTestRead;imports.FS_Write=WorldTestWrite;imports.FS_Close=WorldTestClose;imports.Print=WorldTestPrint;imports.Cvar_VariableStringBuffer=WorldTestCvar;imports.Argc=WorldTestArgc;
 WorldFixture();worldChecksum=0x85fcb9a9u;assert(WorldChecksum()==worldChecksum);
 Q_strncpyz(level.mapname,"maps/gm_construct.bsp",sizeof(level.mapname));
 assert(WorldChecksum()==0x7ee6cc66u);worldChecksum=0x85fcb9a8u;assert(WorldChecksum()==worldChecksum);
 WorldFixture();gentity_t*prop=CopySpawn(1,0,100),*npc=CopySpawn(2,1,240);CopyReady(npc);npc->health=npc->client->ps.stats[STAT_HEALTH]=173;BeginActor(&g_entities[1],npc);objects[npc->s.number].frozen=1;objects[npc->s.number].thruster=1;objects[npc->s.number].thrustBy=2;
 objects[prop->s.number].mass=200;objects[prop->s.number].frozen=1;objects[prop->s.number].noGravity=1;objects[prop->s.number].thruster=1;VectorSet(objects[prop->s.number].velocity,12,34,0);
 CopyLink(0,prop->s.number,npc->s.number);gentity_t*worldRope=CopyLink(1,prop->s.number,ENTITYNUM_WORLD);VectorSet(objects[worldRope->s.number].anchorB,500,300,100);
 // File records are bounded and transactional even when a checksum is recomputed.
 static jarWorld_t captured,decoded;static byte good[WORLD_BYTES],bad[WORLD_BYTES];assert(WorldCapture(&captured));assert(captured.copy.count==2&&captured.copy.ropes==2&&captured.copy.rope[1].second==-1);
 int size=WorldEncode(&captured,good);assert(size==WORLD_HEADER+2*WORLD_NODE+2*WORLD_ROPE+4&&WorldDecode(good,size,&decoded));assert(decoded.copy.node[1].health==173&&decoded.copy.node[1].thruster&&decoded.copy.rope[1].b[0]==500&&decoded.velocity[0][1]==34);
 // Spin roundtrip and old v1 records default to zero angular motion.
 captured.angular[0][2]=1.25f;int spinning=WorldEncode(&captured,bad);assert(WorldDecode(bad,spinning,&decoded)&&decoded.angular[0][2]==1.25f);
 int angularAt=WORLD_HEADER+88;WorldPut(bad,&angularAt,0x7fc00000u);WorldTestHash(bad,spinning);assert(!WorldDecode(bad,spinning,&decoded));
 memcpy(bad,good,WORLD_HEADER);int cursor=0;WorldPut(bad,&cursor,0x3157524au);cursor=WORLD_HEADER;
 for(int i=0;i<captured.copy.count;++i){memcpy(bad+cursor,good+WORLD_HEADER+i*WORLD_NODE,88);cursor+=88;}
 for(int r=0;r<captured.copy.ropes;++r){memcpy(bad+cursor,good+WORLD_HEADER+captured.copy.count*WORLD_NODE+r*WORLD_ROPE,36);cursor+=36;}cursor+=4;WorldTestHash(bad,cursor);
 assert(WorldDecode(bad,cursor,&decoded)&&VectorLength(decoded.angular[0])==0&&decoded.velocity[0][1]==34);
 const int positions[]={0,12,WORLD_HEADER,WORLD_HEADER+4,WORLD_HEADER+24,WORLD_HEADER+2*WORLD_NODE};
 const unsigned values[]={0,MAX_GENTITIES+1,0,2,0x7fc00000u,10000};
 for(int i=0;i<6;++i){memcpy(bad,good,size);int at=positions[i];WorldPut(bad,&at,values[i]);WorldTestHash(bad,size);decoded.generation=42;assert(!WorldDecode(bad,size,&decoded)&&decoded.generation==42);}
 memcpy(bad,good,size);memset(bad+20,'a',MAX_QPATH);WorldTestHash(bad,size);assert(!WorldDecode(bad,size,&decoded));assert(!WorldDecode(good,size-1,&decoded)&&!WorldDecode(good,size+1,&decoded));
 int opens=worldOpens;for(const char**v=(const char*[]){"","../bad","bad/name","bad;quit","bad name","abcdefghijklmnopqrstuvwxyz123456789",NULL};*v;++v){assert(!WorldSave(*v)&&!WorldLoad(*v));}assert(worldOpens==opens);
 assert(WorldSave("test"));assert(worldFileCount==2||worldFileCount==1); // Missing reads do not create revisions.
 objects[prop->s.number].mass=250;assert(WorldSave("test"));assert(worldFileCount==2);
 // A partial write only damages the older revision. Latest verified data survives.
 worldWriteFail=1;assert(!WorldSave("test"));worldWriteFail=0;assert(worldFiles[1].size==size);
 assert(!WorldLoad("test")&&CopyCount(0)==2&&CopyCount(1)==2); // Never overwrite a live world implicitly.
 // Holding or occupying any saved entity refuses the whole save.
 objects[prop->s.number].heldBy=1;int writes=worldWrites;assert(!WorldSave("test")&&worldWrites==writes);objects[prop->s.number].heldBy=0;
 assert(WorldClear());for(int n=0;n<3;++n){g_entities[n].inuse=qfalse;g_entities[n].client=NULL;}
 assert(WorldLoad("test"));int loadedProp=-1,loadedNpc=-1,ropes=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner){assert(objects[n].owner==MAX_CLIENTS+1);if(g_entities[n].s.generic1==251){++ropes;assert(objects[n].first>0&&objects[n].second>0);}else if(g_entities[n].client)loadedNpc=n;else loadedProp=n;}
 assert(loadedProp>=0&&loadedNpc>=0&&ropes==2&&objects[loadedProp].mass==250&&objects[loadedProp].velocity[1]==34&&objects[loadedNpc].copyPending);
 writes=worldWrites;assert(!WorldSave("test")&&worldWrites==writes);CopyReady(&g_entities[loadedNpc]);assert(g_entities[loadedNpc].health==173&&objects[loadedNpc].frozen&&objects[loadedNpc].thruster&&objects[loadedNpc].thrustBy==MAX_CLIENTS+1&&actors[loadedNpc].controller==MAX_CLIENTS+1);assert(JAR_MP_ActorThink(&g_entities[loadedNpc],NULL));
 g_entities[0].inuse=qtrue;g_entities[0].client=&clients[0];assert(!Edit(&g_entities[0],loadedProp));worldEditing.integer=1;assert(Edit(&g_entities[0],loadedProp));worldEditing.integer=0;
 assert(WorldSave("test")); // Replaces the torn older revision with generation 3.
 assert(WorldClear());worldFiles[0].data[0]^=1;assert(WorldLoad("test"));assert(strstr(worldMessage,"Invalid other revision ignored"));assert(WorldClear());
 worldChecksum=778;assert(!WorldLoad("test")&&!CopyCount(MAX_CLIENTS));worldChecksum=777;Q_strncpyz(level.mapname,"maps/mp/ffa1.bsp",sizeof(level.mapname));assert(!WorldLoad("test"));Q_strncpyz(level.mapname,"maps/mp/ffa3.bsp",sizeof(level.mapname));
 // Both corrupt revisions are preserved instead of overwriting unknown content.
 byte saved=worldFiles[1].data[0];worldFiles[1].data[0]^=1;writes=worldWrites;assert(!WorldLoad("test")&&!WorldSave("test")&&worldWrites==writes);worldFiles[1].data[0]=saved;
 worldShortRead=1;assert(!WorldLoad("test"));worldShortRead=0;
 blocked=1;assert(!WorldLoad("test")&&!CopyCount(MAX_CLIENTS));blocked=0;
 // No player command forwards the administrative file/clear operations.
 for(const char**v=(const char*[]){"jar_world_save","jar_world_load","jar_world_clear","jar_world_status",NULL};*v;++v)assert(!JAR_MP_Command(&g_entities[0],*v));
 Q_strncpyz(argument,"test",sizeof(argument));worldArgs=3;writes=worldWrites;assert(JAR_MP_ServerCommand("jar_world_save")&&worldWrites==writes);worldArgs=2;assert(JAR_MP_ServerCommand("jar_world_load")&&CopyCount(MAX_CLIENTS)==4);assert(JAR_MP_ServerCommand("jar_world_status")&&strstr(worldMessage,"2 objects, 2 attachments"));worldArgs=1;assert(JAR_MP_ServerCommand("jar_world_clear")&&!CopyCount(MAX_CLIENTS));assert(!JAR_MP_ServerCommand("jar_world_save_extra"));
 // Optional map-start restore happens once per map and does not need a player.
 WorldReset();Q_strncpyz(worldLoad.string,"test",sizeof(worldLoad.string));WorldFrame();assert(CopyCount(MAX_CLIENTS)==4);assert(WorldClear());WorldFrame();assert(!CopyCount(MAX_CLIENTS));worldLoad.string[0]=0;
 // Authored entities and players survive clear; occupied vehicles block it.
 gentity_t*authored=G_Spawn();gentity_t*vehicle=CopySpawn(3,0,400);CopyReady(vehicle);vehicle->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[0];assert(!WorldClear()&&authored->inuse&&vehicle->inuse);vehicle->m_pVehicle->m_pPilot=NULL;assert(WorldClear()&&authored->inuse&&g_entities[0].inuse);
 assert(WorldSave("empty"));assert(WorldLoad("empty"));assert(worldCloses>20);
 // Frozen vehicles also restore without a human controller, with native armor
 // and orientation available before the frame can resume vehicle dynamics.
 vehicle=CopySpawn(3,0,400);CopyReady(vehicle);BeginActor(&g_entities[0],vehicle);objects[vehicle->s.number].frozen=1;vehicle->health=230;VectorSet(vehicle->r.currentAngles,20,40,15);JAR_MP_ActorThink(vehicle,NULL);
 assert(WorldSave("vehicle")&&WorldClear()&&WorldLoad("vehicle"));int vehicleSlot=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==MAX_CLIENTS+1&&g_entities[n].m_pVehicle)vehicleSlot=n;
 assert(vehicleSlot>=0);vehicle=&g_entities[vehicleSlot];CopyReady(vehicle);assert(objects[vehicleSlot].frozen&&actors[vehicleSlot].controller==MAX_CLIENTS+1&&vehicle->m_pVehicle->m_iArmor==230);
 assert(JAR_MP_ActorThink(vehicle,NULL)&&vehicle->client->ps.pm_type==PM_FREEZE&&vehicle->client->ps.vehOrientation[ROLL]==15);assert(WorldClear());
 puts("MP world persistence: bounded codec, journal recovery, filesystem failure, map identity, server-only commands, world anchors, playerless restore and native readiness passed.");
}
