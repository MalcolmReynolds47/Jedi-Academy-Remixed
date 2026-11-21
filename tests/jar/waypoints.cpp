// Production navigator and graph, deterministic collision services, no game window.
#include "game/g_navigator.cpp"
#include "memory_save.h"
#include <deque>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
static std::deque<std::string> strings;
static int roomForSlot[MAX_GENTITIES]{};
gentity_t* G_SubBSPOwner(const gentity_t* e){
 return e&&roomForSlot[e->s.number]?&g_entities[roomForSlot[e->s.number]]:nullptr;
}
uint64_t G_SubBSPNamespace(const gentity_t* e){return e?roomForSlot[e->s.number]:0;}
bool G_SiegeKorribanRoom(const gentity_t* root){return root&&(root==&g_entities[900]||root==&g_entities[901]);}
// These unrelated rendering, file and movement services must never be invoked
// by the graph/controller fixture. A* and its collision queries are production.
static void Unexpected(){throw std::runtime_error("Unexpected external navigation service");}
void CG_DrawNode(vec3_t,int){Unexpected();}
void CG_DrawEdge(vec3_t,vec3_t,int){Unexpected();}
void CG_DrawRadius(vec3_t,unsigned,int){Unexpected();}
void CG_DrawCombatPoint(vec3_t,int){Unexpected();}
gentity_t*G_FindDoorTrigger(gentity_t*){Unexpected();return nullptr;}
qboolean G_EntIsBreakable(int,gentity_t*){Unexpected();return qfalse;}
qboolean G_CheckInSolidTeleport(const vec3_t&,gentity_t*){Unexpected();return qfalse;}
gentity_t*G_Find(gentity_t*,int,const char*){Unexpected();return nullptr;}
void G_SetOrigin(gentity_t*,const vec3_t){Unexpected();}
void TeleportPlayer(gentity_t*,vec3_t,vec3_t){Unexpected();}
void G_SoundOnEnt(gentity_t*,soundChannel_t,const char*){Unexpected();}
int INV_GoodieKeyCheck(gentity_t*){Unexpected();return 0;}
void TIMER_Set(gentity_t*,const char*,int){Unexpected();}
qboolean TIMER_Done(gentity_t*,const char*){Unexpected();return qfalse;}
qboolean NPC_Jumping(){Unexpected();return qfalse;}
qboolean NPC_TryJump(gentity_t*,float,float){Unexpected();return qfalse;}
qboolean NPC_TryJump(const vec3_t&,float,float){Unexpected();return qfalse;}
bool hfile::open(float,int,bool){Unexpected();return false;}
bool NAVDEBUG_showNodes=false,NAVDEBUG_showRadius=false,NAVDEBUG_showEdges=false,NAVDEBUG_showEnemyPath=false,NAVDEBUG_showCombatPoints=false,NAVDEBUG_showNavGoals=false,NAVDEBUG_showCollision=false,NAVDEBUG_showGrid=false,NAVDEBUG_showNearest=false,NAVDEBUG_showPointLines=false;
cvar_t*g_nav1=nullptr,*g_nav2=nullptr,*g_speed=nullptr;
int delayedShutDown=0;gentity_t*player=nullptr,*NPC=nullptr;gNPC_t*NPCInfo=nullptr;gclient_t*client=nullptr;usercmd_t ucmd{};
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
static bool wall=false,pit=false,blocked=false,narrow=false,lowCeiling=false;
static gentity_t&Make(int n,float x,float y=0,const char*type="waypoint"){
 auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString(type);e.targetname=G_NewString(("live_"+std::to_string(n)).c_str());VectorSet(e.currentOrigin,x,y,24);CHECK(G_WaypointSpawn(e));return e;
}
static bool Path(int from,int to,gentity_t*actor=nullptr){mUser.ClearActor();if(actor)mUser.SetActor(actor);mSearch.mStart=from;mSearch.mEnd=to;mGraph.astar(mSearch,mUser);return mSearch.success();}
int main(){return RunTests("live native waypoint graph, collision, edits, removal, saved settings and exhaustion",[]{
 globals.num_entities=1500;gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};gi.inPVS=[](const vec3_t,const vec3_t){return qtrue;};gi.Printf=[](const char*,...){};
 gi.trace=[](trace_t*t,const vec3_t start,const vec3_t low,const vec3_t high,const vec3_t end,int,int,EG2_Collision,int){
  *t={};t->fraction=1;t->entityNum=ENTITYNUM_NONE;VectorCopy(end,t->endpos);
  if(blocked){t->startsolid=qtrue;return;}
  if(narrow&&low&&low[0]<-20){t->startsolid=qtrue;return;}
  if(lowCeiling&&high&&high[2]>120){t->startsolid=qtrue;return;}
  if(wall&&((start[0]<64&&end[0]>64)||(end[0]<64&&start[0]>64))){t->fraction=.5f;t->entityNum=ENTITYNUM_WORLD;return;}
  if(end[2]<0&&start[2]>=0&&(!pit||start[0]<48||start[0]>80)){t->fraction=start[2]/(start[2]-end[2]);t->endpos[2]=0;t->plane.normal[2]=1;t->entityNum=ENTITYNUM_WORLD;}
 };
 mGraph.clear();mCells.clear();mRegion.clear();G_WaypointReset();
 auto&a=Make(10,0);auto&b=Make(11,128,0,"waypoint_small");G_WaypointFrame();
 CHECK(liveWaypointHandles.size()==2&&mGraph.size_nodes()==2&&mGraph.size_edges()==1);
 int ah=liveWaypointHandles[10],bh=liveWaypointHandles[11];CHECK(Path(ah,bh));CHECK(mGraph.get_node(bh).mRadius==2);
 wall=true;CHECK(!Path(ah,bh));wall=false;pit=true;CHECK(!Path(ah,bh));pit=false;CHECK(Path(ah,bh));
 gentity_t walker{};gNPC_t brain{};walker.NPC=&brain;VectorSet(walker.mins,-16,-16,-24);VectorSet(walker.maxs,16,16,32);narrow=true;CHECK(Path(ah,bh,&walker));walker.maxs[0]=64;CHECK(!Path(ah,bh,&walker));walker.maxs[0]=16;narrow=false;
 b.currentOrigin[2]=224;G_WaypointFrame();CHECK(!Path(liveWaypointHandles[10],liveWaypointHandles[11],&walker));brain.scriptFlags|=SCF_NAV_CAN_FLY;CHECK(Path(liveWaypointHandles[10],liveWaypointHandles[11],&walker));brain.scriptFlags=0;b.currentOrigin[2]=24;G_WaypointFrame();
 // Automatic links can be disabled; explicit primary/secondary routes can cycle.
 CHECK(G_WaypointSet(a,{0,true})&&G_WaypointSet(b,{0,true}));G_WaypointFrame();CHECK(!mGraph.size_edges());
 a.target=G_NewString(b.targetname);b.target2=G_NewString(a.targetname);G_WaypointFrame();CHECK(mGraph.size_edges()==1&&Path(liveWaypointHandles[10],liveWaypointHandles[11]));
 CHECK(G_WaypointSet(b,{0,false}));G_WaypointFrame();CHECK(liveWaypointHandles.size()==1&&!mGraph.size_edges());
 CHECK(G_WaypointSet(b,{128,true}));G_WaypointFrame();CHECK(liveWaypointHandles.size()==2);
 a.waypoint=ah;a.lastWaypoint=ah;a.noWaypointTime=999999;a.NPC=&brain;brain.homeWp=liveWaypointHandles[10];b.currentOrigin[1]=128;G_WaypointFrame();CHECK(!a.waypoint&&!a.lastWaypoint&&!a.noWaypointTime&&!brain.homeWp&&mGraph.get_node(liveWaypointHandles[11]).mPoint[1]==128);a.NPC=nullptr;
 // Native entity serialization saves properties, not transient graph handles.
 MemorySave save;ojk::SavedGameHelper io(&save);gentity_t stored{},loaded{};stored.wait=b.wait;stored.svFlags=b.svFlags;VectorCopy(b.currentOrigin,stored.currentOrigin);stored.sg_export(io);save.reset_buffer_offset();loaded.sg_import(io);io.ensure_all_data_read();CHECK(loaded.wait==128&&loaded.currentOrigin[1]==128);
 CHECK(G_WaypointRestore());b.wait=-1;CHECK(!G_WaypointRestore());b.wait=128;CHECK(G_WaypointRestore());
 a.inuse=qfalse;b.inuse=qfalse;G_WaypointFrame();CHECK(!mGraph.size_nodes()&&!mGraph.size_edges());
 // A map node/edge retains its identity while live nodes are added and removed.
 CWayNode authored{};authored.mPoint=CVec3(-100,0,4);authored.mRadius=20;authored.mType=NAV::PT_WAYNODE;authored.mFlags.clear();int original=mGraph.insert_node(authored);auto&c=Make(12,0);G_WaypointFrame();CHECK(mGraph.get_node(original).mPoint[0]==-100&&Path(original,liveWaypointHandles[12]));c.inuse=qfalse;G_WaypointFrame();CHECK(mGraph.size_nodes()==1&&mGraph.get_node(original).mPoint[0]==-100);
 // Coincident edits cannot overflow the fixed nearest-cell lists.
 for(int i=20;i<90;++i)Make(i,0);G_WaypointFrame();CHECK(liveWaypointHandles.size()==70);
 blocked=true;g_entities[20].currentOrigin[0]=1;G_WaypointFrame();CHECK(liveWaypointHandles.empty());blocked=false;
 for(int i=20;i<90;++i)g_entities[i].inuse=qfalse;G_WaypointFrame();
 // Dense live graphs stay within all edge/neighbor/cell capacities, including
 // more nodes than the optional native region cache can represent.
 for(int i=100;i<500;++i)Make(i,float((i-100)%20)*32,float((i-100)/20)*32);
 G_WaypointFrame();CHECK(liveWaypointHandles.size()==400&&mGraph.size_edges()<=NAV::NUM_EDGES-1&&!mRegion.size());
 for(auto it=mGraph.nodes_begin();it!=mGraph.nodes_end();++it)CHECK(mGraph.get_node_neighbors(it.index()).size()<=NAV::NUM_EDGES_PER_NODE);
 for(int i=100;i<500;++i)g_entities[i].inuse=qfalse;G_WaypointFrame();CHECK(mGraph.size_nodes()==1);
 // Inactive feature: restore does not disturb map caches or actor references.
 g_entities[1].waypoint=original;g_entities[1].wayedge=123;CHECK(G_WaypointRestore());G_WaypointFrame();CHECK(g_entities[1].waypoint==original&&g_entities[1].wayedge==123);
 while(mGraph.size_nodes()<NAV::NUM_NODES-1)mGraph.insert_node(authored);
 auto&full=g_entities[100];full.inuse=qtrue;full.s.number=100;full.classname=G_NewString("waypoint");CHECK(!G_WaypointSpawn(full)&&mGraph.size_nodes()==NAV::NUM_NODES-1);
 mGraph.clear();mCells.clear();mRegion.clear();G_WaypointReset();full.inuse=qfalse;
 authored.mPoint=CVec3(-60000,-60000,4);mGraph.insert_node(authored);auto&farA=Make(600,60000,60000);Make(601,60128,60000);G_WaypointFrame();
 CHECK(NAV::GetNearestNode(farA.currentOrigin)==liveWaypointHandles[600]);CHECK(Path(liveWaypointHandles[600],liveWaypointHandles[601]));
 // The native graph reset drops only transient registration, leaving saved
 // entity configuration to be reconstructed after map navigation loads.
 mGraph.clear();G_WaypointReset();CHECK(liveWaypointHandles.empty()&&G_WaypointRestore()&&liveWaypointHandles.size()==2);
 g_entities[600].inuse=qfalse;g_entities[601].inuse=qfalse;
 auto& hostA=Make(12,0);Make(13,128);
 hostA.target=G_NewString("t21");
 const auto addRoom=[&](int first,int owner){
  for(size_t i=0;i<ARRAY_LEN(jar::siegeKorriban::waypoints);++i){
   const auto& spec=jar::siegeKorriban::waypoints[i];
   auto& point=g_entities[first+i];
   point.inuse=qtrue;point.s.number=first+i;point.classname=G_NewString("waypoint");
   point.model=G_NewString(jar::siegeKorriban::waypointModel);
   point.targetname=G_NewString(spec.name);point.target=G_NewString(spec.target);
   CHECK(sscanf(spec.origin,"%f %f %f",point.currentOrigin,point.currentOrigin+1,point.currentOrigin+2)==3);
   point.currentOrigin[0]+=1512;point.currentOrigin[1]-=12952;point.currentOrigin[2]+=416;
   roomForSlot[first+i]=owner;
  }
 };
 addRoom(1000,900);addRoom(1100,901);
 G_WaypointFrame();
 CHECK(liveWaypointRoomNodes.size()==2&&liveWaypointRoomNodes[900].size()==18&&liveWaypointRoomNodes[901].size()==18);
 CHECK(mGraph.size_edges()==37&&NAV::GetNearestNode(hostA.currentOrigin)==liveWaypointHandles[12]);
 CHECK(Path(liveWaypointHandles[1000],liveWaypointHandles[1009])&&
       !Path(liveWaypointHandles[1000],liveWaypointHandles[1109])&&
       !Path(liveWaypointHandles[1000],liveWaypointHandles[12]));
 auto& rancor=g_entities[1200];rancor.s.number=1200;rancor.inuse=qtrue;rancor.s.time2=jar::siegeKorriban::rancorTag;
 gclient_t rancorClient{};rancor.client=&rancorClient;rancor.NPC=&brain;rancorClient.NPC_class=CLASS_RANCOR;
 VectorSet(rancor.mins,-60,-60,-24);VectorSet(rancor.maxs,60,60,136);
 roomForSlot[1200]=900;VectorCopy(g_entities[1000].currentOrigin,rancor.currentOrigin);
 G_SiegeKorribanRancorAllegiance(rancor);
 CHECK(rancorClient.playerTeam==TEAM_ENEMY&&rancorClient.enemyTeam==TEAM_PLAYER);
 CHECK(NAV::GetNearestNode(&rancor,true)==liveWaypointHandles[1000]);
 CHECK(Path(liveWaypointHandles[1000],liveWaypointHandles[1009],&rancor));
 CHECK(NAV::FindPath(&rancor,g_entities[1009].currentOrigin));
 lowCeiling=true;
 CHECK(!Path(liveWaypointHandles[1000],liveWaypointHandles[1009],&rancor));
 lowCeiling=false;
 CHECK(!NAV::FindPath(&rancor,liveWaypointHandles[1100])&&!NAV::FindPath(&rancor,liveWaypointHandles[12]));
 for(int i=1000;i<1018;++i)g_entities[i].inuse=qfalse;
 G_WaypointFrame();
 CHECK(liveWaypointRoomNodes.size()==1&&liveWaypointRoomNodes[901].size()==18&&
       mGraph.size_edges()==19&&NAV::GetNearestNode(&rancor,true)==WAYPOINT_NONE);
 mGraph.clear();G_WaypointReset();CHECK(G_WaypointRestore()&&liveWaypointRoomNodes[901].size()==18);
 g_entities[12].inuse=g_entities[13].inuse=qfalse;
 G_WaypointFrame();CHECK(mGraph.size_edges()==18&&NAV::GetNearestNode(hostA.currentOrigin)==WAYPOINT_NONE);
 rancor.NPC=nullptr;rancor.client=nullptr;
 });}
