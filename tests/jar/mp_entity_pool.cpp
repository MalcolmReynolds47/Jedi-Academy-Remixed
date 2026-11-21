#include "game/g_local.h"
#include <cassert>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <sstream>
gentity_t g_entities[MAX_GENTITIES];level_locals_t level{};static gclient_t client{};static gameImport_t imports{};gameImport_t*trap=&imports;
static int located;
static void Locate(sharedEntity_t*,int n,int,playerState_t*,int){assert(n<=ENTITYNUM_WORLD);located=n;}
static void QDECL Fail(int,const char*,...){throw std::runtime_error("capacity exhausted");}
void G_InitGentity(gentity_t*e){e->inuse=qtrue;e->s.number=int(e-g_entities);}
#include "../../codemp/game/g_jar_entity_pool.inc"
#include "../../codemp/game/g_jar_g2_queue.inc"
#include "../../codemp/game/g_jar_client_storage.inc"
#include "../../codemp/game/g_jar_npc_storage.inc"
static std::vector<int> removals;
static void Send(int client,const char*message){assert(client==-1&&strlen(message)<512);std::istringstream in(message);std::string command;in>>command;assert(command=="kg2");int id;while(in>>id)removals.push_back(id);}
int main(){
 level.gentities=g_entities;level.clients=&client;level.num_entities=MAX_CLIENTS;level.startTime=0;level.time=5000;imports.LocateGameData=Locate;imports.Error=Fail;
 for(int n=MAX_CLIENTS;n<ENTITYNUM_WORLD-JAR_MP_ENTITY_RESERVE;++n){auto*e=G_TrySpawn();assert(e==&g_entities[n]&&e->s.number==n);}
 assert(!G_TrySpawn()&&located==ENTITYNUM_WORLD-JAR_MP_ENTITY_RESERVE);
 for(int i=0;i<JAR_MP_ENTITY_RESERVE;++i)assert(G_Spawn()==&g_entities[ENTITYNUM_WORLD-JAR_MP_ENTITY_RESERVE+i]);
 assert(!G_TrySpawn()&&level.num_entities==ENTITYNUM_WORLD&&!g_entities[ENTITYNUM_WORLD].inuse&&!g_entities[ENTITYNUM_NONE].inuse);
 bool failed=false;try{G_Spawn();}catch(const std::runtime_error&){failed=true;}assert(failed);
 g_entities[15000].inuse=qfalse;g_entities[15000].freetime=level.time;assert(G_TrySpawn()==&g_entities[15000]);assert(!G_TrySpawn());
 g_entities[ENTITYNUM_WORLD-1].inuse=qfalse;assert(!G_TrySpawn());assert(G_Spawn()==&g_entities[ENTITYNUM_WORLD-1]);
 assert(level.num_entities==ENTITYNUM_WORLD);
 imports.SendServerCommand=Send;for(int i=0;i<600;++i)G_KillG2Queue(12000+i);while(gG2KillNum)G_SendG2KillQueue();
 assert(removals.size()==600);for(int i=0;i<600;++i)assert(removals[i]==12000+i);
 for(int i=MAX_CLIENTS;i<ENTITYNUM_WORLD;++i){gclient_t*cl=nullptr;G_CreateFakeClient(i,&cl);assert(cl==&jarClientStorage[i]);cl->ps.clientNum=i;gNPC_t*npc=New_NPC_t(i);assert(npc==&jarNPCStorage[i]);npc->shotTime=i;}
 for(int i=MAX_CLIENTS;i<ENTITYNUM_WORLD;++i){gclient_t*cl=nullptr;G_CreateFakeClient(i,&cl);assert(cl->ps.clientNum==i&&gNPCPtrs[i]->shotTime==i);assert(New_NPC_t(i)->shotTime==0);}
 for(int i:{-1,ENTITYNUM_WORLD,ENTITYNUM_NONE,MAX_GENTITIES}){gclient_t*cl=&client;G_CreateFakeClient(i,&cl);assert(!cl&&!New_NPC_t(i));}
 puts("MP native allocation: 16384 slots, optional reserve, critical exhaustion and high-water reuse passed.");
}
