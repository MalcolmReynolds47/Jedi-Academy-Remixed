// Server-owned sight checks and one-shot delivery for Sandbox visibility markers.
static qboolean VisiblePlayer(gentity_t*marker,gentity_t*player,float cosine){
 const gclient_t*client=player->client;
 if(!player->inuse||!client||client->pers.connected!=CON_CONNECTED||player->health<=0||
    client->sess.sessionTeam==TEAM_SPECTATOR||
    ((marker->spawnflags&JAR_VISIBLE_FORCE_SIGHT)&&!(client->ps.fd.forcePowersActive&(1<<FP_SEE))))
  return qfalse;

 vec3_t eye,forward;
 VectorCopy(client->ps.origin,eye);
 eye[2]+=client->ps.viewheight;
 AngleVectors(client->ps.viewangles,forward,NULL,NULL);
 if(!JAR_VisibleFacing(marker->r.currentOrigin,eye,forward,(int)marker->speed,cosine)||
    !trap->InPVS(eye,marker->r.currentOrigin))return qfalse;
 if(marker->spawnflags&JAR_VISIBLE_IGNORE_WALLS)return qtrue;

 trace_t trace;
 trap->Trace(&trace,eye,NULL,NULL,marker->r.currentOrigin,player->s.number,MASK_OPAQUE,qfalse,0,0);
 return !trace.startsolid&&!trace.allsolid&&(trace.fraction==1||trace.entityNum==marker->s.number);
}

static void VisibleFrame(void){
 // Round-robin admission bounds trace work even when many markers arm at once.
 static int cursor=MAX_CLIENTS;
 if(!sandbox.integer||level.numConnectedClients<=0)return;
 const int slots=level.num_entities-MAX_CLIENTS;
 int budget=32;
 for(int checked=0;checked<slots&&budget>0;++checked){
  if(cursor>=level.num_entities)cursor=MAX_CLIENTS;
  const int n=cursor++;
  gentity_t*marker=&g_entities[n];
  if(!marker->inuse||marker->use!=VisibleUse||!marker->genericValue1||
     (marker->flags&FL_INACTIVE)||marker->timestamp>level.time||
     !objects[n].owner||objects[n].copyPending)continue;

  --budget;
  marker->timestamp=level.time+JAR_VISIBLE_SAMPLE_MS;
  const float cosine=JAR_VisibleCosine(marker->damage);
  for(int c=0;c<level.maxclients;++c){
   gentity_t*player=&g_entities[c];
   if(!VisiblePlayer(marker,player,cosine))continue;
   VisibleArm(marker,0);
   ControllerCaptureActivator(marker,player);
   ++objects[n].propertyRevision;
   SelectionChanged(n);
   JAR_MP_UseTargets(marker,player,"@jar_primary");
   break;
  }
 }
}
