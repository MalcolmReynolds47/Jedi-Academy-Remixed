// Read-only server diagnostics of actual spawned NPC state and native geometry.
// GPL-2.0-only.
static qboolean ActorInfo(const char *command) {
 if(Q_stricmp(command,"jar_actor_info"))return qfalse;
 char text[32],*end;trap->Argv(1,text,sizeof(text));unsigned long id=strtoul(text,&end,10);
 if(trap->Argc()!=2||!*text||*end||id>UINT_MAX){Tell(NULL,"Server console: jar_actor_info CATALOG_ID");return qtrue;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];
  if(!e->inuse||!e->client||!objects[n].owner||objects[n].catalog<1||objects[n].catalog>JAR_MP_CATALOG_COUNT||jarMPCatalog[objects[n].catalog-1].id!=id)continue;
  char model[MAX_QPATH]={0},gla[MAX_QPATH]={0};vec3_t low={0},high={0};int frames=0;
  trap->GetConfigstring(CS_MODELS+e->s.modelindex,model,sizeof(model));char*skin=strchr(model,'*');if(skin)*skin=0;
  if(model[0]=='$'&&e->m_pVehicle&&e->m_pVehicle->m_pVehicleInfo&&e->m_pVehicle->m_pVehicleInfo->model)Com_sprintf(model,sizeof(model),"models/players/%s/model.glm",e->m_pVehicle->m_pVehicleInfo->model);
  if(e->ghoul2)trap->G2API_GetGLAName(e->ghoul2,0,gla);
  qboolean valid=trap->JAR_ModelPoseBounds&&trap->JAR_ModelPoseBounds(model,0,&frames,low,high);
  Tell(NULL,va("Actor %u entity %d class %d team %d enemy %d gravity %d health %d ammo %d model %s gla %s pose %d frames %d bounds %.6f %.6f %.6f %.6f %.6f %.6f",
   (unsigned)id,n,e->client->NPC_class,e->client->playerTeam,e->client->enemyTeam,e->client->ps.gravity,e->health,e->count,model,gla,valid,frames,low[0],low[1],low[2],high[0],high[1],high[2]));
 }
 return qtrue;
}
