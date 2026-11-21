// Actual scheduler, snapshot capture and journal paths with failing FS adapters.
static void AutoTick(unsigned milliseconds){level.time=(int)((unsigned)level.time+milliseconds);WorldFrame();}
static int AutoLatest(jarWorld_t*out){int best=-1;jarWorld_t candidate;for(int i=0;i<worldFileCount;++i)if(strstr(worldFiles[i].path,worldAutoSlot)&&WorldDecode(worldFiles[i].data,worldFiles[i].size,&candidate)){if(best<0||candidate.generation>out->generation){*out=candidate;best=i;}}return best;}
static void AutoSaveCases(void){
 memset(worldFiles,0,sizeof(worldFiles));worldFileCount=worldWrites=0;WorldFixture();assert(WorldInterval()==120&&worldAutoLoad.integer==1);
 gentity_t*prop=CopySpawn(1,0,100),*vehicle=CopySpawn(3,1,500);CopyReady(vehicle);objects[prop->s.number].heldBy=1;VectorSet(objects[prop->s.number].velocity,10,20,30);vehicle->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[1];VectorSet(vehicle->client->ps.velocity,40,50,60);
 CopyLink(0,prop->s.number,ENTITYNUM_WORLD);CopyLink(0,prop->s.number,0);
 WorldFrame();assert(worldTried&&!worldAutoBlocked&&WorldSlot(worldAutoSlot)&&worldWrites==0);
 AutoTick(119999);assert(worldWrites==0);AutoTick(1);assert(worldWrites==1);
 static jarWorld_t saved;int newest=AutoLatest(&saved);assert(newest>=0&&saved.copy.count==2&&saved.copy.ropes==1&&saved.generation==1);
 for(int i=0;i<2;++i){assert(saved.copy.node[i].frozen&&VectorLengthSquared(saved.velocity[i])==0);}assert(strstr(worldAutoStatus,"2 parked; 1 transient"));
 assert(objects[prop->s.number].heldBy==1&&!objects[prop->s.number].frozen&&objects[prop->s.number].velocity[1]==20&&vehicle->m_pVehicle->m_pPilot==(bgEntity_t*)&g_entities[1]&&vehicle->client->ps.velocity[1]==50);
 AutoTick(120000);assert(worldWrites==1&&strstr(worldAutoStatus,"unchanged"));
 objects[prop->s.number].mass=300;AutoTick(120000);assert(worldWrites==2&&AutoLatest(&saved)>=0&&saved.generation==2);
 byte previous[WORLD_BYTES];int previousSize=worldFiles[1].size;memcpy(previous,worldFiles[1].data,previousSize);
 objects[prop->s.number].mass=301;worldWriteFail=1;AutoTick(120000);assert(worldWrites==3&&strstr(worldAutoStatus,"write failed")&&!memcmp(previous,worldFiles[1].data,previousSize));
 worldWriteFail=0;AutoTick(120000);assert(worldWrites==4&&AutoLatest(&saved)>=0&&saved.generation==3&&saved.copy.node[0].mass==301);
 // Shutdown writes the latest live snapshot even before the next timer expires.
 objects[prop->s.number].mass=400;JAR_MP_Shutdown();assert(worldWrites==5&&AutoLatest(&saved)>=0&&saved.generation==4);JAR_MP_Shutdown();assert(worldWrites==5);
 char originalSlot[33];Q_strncpyz(originalSlot,worldAutoSlot,sizeof(originalSlot));
 WorldFixture();WorldFrame();assert(!worldAutoBlocked&&CopyCount(MAX_CLIENTS)==3);int loadedVehicle=-1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].owner==MAX_CLIENTS+1){if(g_entities[n].m_pVehicle){loadedVehicle=n;CopyReady(&g_entities[n]);assert(!VehicleOccupied(g_entities[n].m_pVehicle));}else if(objects[n].catalog)assert(objects[n].mass==400&&objects[n].frozen);}
 assert(loadedVehicle>=0&&objects[loadedVehicle].frozen);
 // Failed asynchronous native initialization cannot replace a good saved world
 // with the rolled-back or partially restored assembly.
 WorldFixture();WorldFrame();assert(CopyCount(MAX_CLIENTS)==3);for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(objects[n].copyPending){G_FreeEntity(&g_entities[n]);break;}
 assert(worldAutoBlocked&&!CopyCount(MAX_CLIENTS));int writes=worldWrites;AutoTick(120000);JAR_MP_Shutdown();assert(worldWrites==writes);
 // A trace-rejected startup load likewise blocks empty-world replacement.
 WorldFixture();blocked=1;WorldFrame();blocked=0;assert(worldAutoBlocked);AutoTick(120000);assert(worldWrites==writes);
 assert(WorldClear()&&!worldAutoBlocked);JAR_MP_Shutdown();assert(worldWrites==writes+1&&AutoLatest(&saved)>=0&&saved.copy.count==0);
 WorldFixture();WorldFrame();assert(!CopyCount(MAX_CLIENTS)&&!worldAutoBlocked); // Intentional clear persists.
 // Per-map and per-BSP names isolate snapshots, without embedding a map path.
 WorldFixture();worldChecksum=778;WorldFrame();assert(strcmp(originalSlot,worldAutoSlot)&&!CopyCount(MAX_CLIENTS));char otherSlot[33];Q_strncpyz(otherSlot,worldAutoSlot,sizeof(otherSlot));
 WorldFixture();Q_strncpyz(level.mapname,"maps/mp/ffa1.bsp",sizeof(level.mapname));WorldFrame();assert(strcmp(originalSlot,worldAutoSlot)&&strcmp(otherSlot,worldAutoSlot));
 // A first-ever short write can retry its own torn record. Unknown corrupt data
 // remains protected until the server explicitly clears or loads a world.
 WorldFixture();worldChecksum=999;WorldFrame();worldWriteFail=1;writes=worldWrites;AutoTick(120000);assert(worldWrites==writes+1);worldWriteFail=0;AutoTick(120000);assert(worldWrites==writes+2&&AutoLatest(&saved)>=0);
 // Controls update at runtime, including timer reset, clamping and disable.
 worldAutoSave.integer=1;AutoTick(1);assert(worldInterval==30);writes=worldWrites;CopySpawn(1,0,120);AutoTick(29999);assert(worldWrites==writes);AutoTick(1);assert(worldWrites==writes+1);
 worldAutoSave.integer=99999;AutoTick(1);assert(worldInterval==3600);worldAutoSave.integer=0;AutoTick(1);writes=worldWrites;CopySpawn(1,0,220);AutoTick(4000000);JAR_MP_Shutdown();assert(worldWrites==writes);
 worldAutoSave.integer=30;sandbox.integer=0;AutoTick(30000);JAR_MP_Shutdown();assert(worldWrites==writes);sandbox.integer=1;
 // Existing auto snapshots aren't overwritten when automatic loading is off.
 WorldFixture();worldAutoLoad.integer=0;WorldFrame();assert(worldAutoBlocked&&!CopyCount(MAX_CLIENTS));writes=worldWrites;AutoTick(120000);assert(worldWrites==writes);
 // Explicit named loads win over the automatic map snapshot.
 WorldFixture();CopySpawn(1,0,300);assert(WorldSave("chosen"));assert(WorldClear());WorldReset();Q_strncpyz(worldLoad.string,"chosen",sizeof(worldLoad.string));WorldFrame();assert(CopyCount(MAX_CLIENTS)==1&&!worldAutoBlocked);
 // A corrupt autosave is preserved after startup failure. An explicit clear
 // permits writing one new revision while retaining the other old revision.
 WorldFixture();WorldFrame();assert(WorldClear());JAR_MP_Shutdown();for(int i=0;i<worldFileCount;++i)if(strstr(worldFiles[i].path,worldAutoSlot))worldFiles[i].data[0]^=1;
 WorldFixture();WorldFrame();assert(worldAutoBlocked);writes=worldWrites;AutoTick(120000);assert(worldWrites==writes);assert(WorldClear());JAR_MP_Shutdown();assert(worldWrites==writes+1&&AutoLatest(&saved)>=0);
 // Unsigned elapsed time remains correct when the native millisecond clock wraps.
 worldAutoSave.integer=30;WorldFrame();worldLastAttempt=0xfffffff0u;level.time=20;writes=worldWrites;WorldFrame();assert(worldWrites==writes);
 puts("MP autosave: periodic/shutdown journals, held/occupied snapshots, transient ropes, restore protection, native failure, cleared worlds, namespaces, runtime controls and retry passed.");
}
