// Native entity allocation with a reserve for critical engine/game work.
// GPL-2.0-only. Optional callers must handle NULL; world/none are never allocated.
static gentity_t* G_AllocateEntity(int limit){
 const int end=level.num_entities<limit?level.num_entities:limit;
 for(int force=0;force<2;++force){
  for(int n=MAX_CLIENTS;n<end;++n){gentity_t*e=&g_entities[n];
   if(e->inuse||(!force&&e->freetime>level.startTime+2000&&level.time-e->freetime<1000))continue;
   G_InitGentity(e);return e;
  }
  if(level.num_entities<limit){
   gentity_t*e=&g_entities[level.num_entities++];
   trap->LocateGameData((sharedEntity_t*)level.gentities,level.num_entities,sizeof(gentity_t),&level.clients[0].ps,sizeof(level.clients[0]));
   G_InitGentity(e);return e;
  }
 }
 return NULL;
}
gentity_t* G_TrySpawn(void){return G_AllocateEntity(ENTITYNUM_MAX_NORMAL-JAR_MP_ENTITY_RESERVE);}
gentity_t* G_Spawn(void){
 gentity_t*e=G_AllocateEntity(ENTITYNUM_MAX_NORMAL);
 if(!e)trap->Error(ERR_DROP,"JA:R: critical entity capacity exhausted (%d slots)",ENTITYNUM_MAX_NORMAL);
 return e;
}
