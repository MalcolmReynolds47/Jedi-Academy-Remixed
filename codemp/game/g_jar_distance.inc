// Native Sandbox distance checks with two activation outputs and two inputs.
static qboolean DistanceEndpoint(int source,int endpoint){
 if(endpoint<MAX_CLIENTS||endpoint>=ENTITYNUM_WORLD||endpoint==source||
    !g_entities[endpoint].inuse||!objects[endpoint].owner||!objects[endpoint].catalog||
    objects[endpoint].copyPending)return qfalse;
 const int owner=objects[source].owner;
 if(owner==MAX_CLIENTS+1)return objects[endpoint].owner==owner;
 if(owner<1||owner>MAX_CLIENTS)return qfalse;
 gentity_t*player=&g_entities[owner-1];
 return player->inuse&&player->client&&player->client->pers.connected==CON_CONNECTED&&Edit(player,endpoint);
}

static int DistanceInputStatus(int source,int slot){
 const int link=objects[source].distanceInput[slot];
 if(!link)return 0;
 return link>0&&DistanceEndpoint(source,link-1)?1:2;
}

static int* DistanceLinkField(int source,int role){
 return role<JAR_DISTANCE_REFERENCE?&objects[source].output[role]:
        &objects[source].distanceInput[role-JAR_DISTANCE_REFERENCE];
}

static void DistanceLinkChange(gentity_t*player,int source,int role,int next){
 int*link=DistanceLinkField(source,role);
 if(*link==next)return;
 jarPropertyUndo_t*undo=&players[player->s.number].propertyUndo;
 memset(undo,0,sizeof(*undo));
 undo->entity=source;
 undo->field=JAR_PROPERTY_DISTANCE_LINK;
 undo->identity=ContextIdentity(source);
 undo->revision=++objects[source].propertyRevision;
 undo->distanceRole=role;
 undo->distanceBefore=*link;
 undo->distanceEpoch=*link>0?controllerEntityEpoch[*link-1]:0;
 *link=next;
 SelectionChanged(source);
}

static qboolean DistanceLinkUndo(int source,const jarPropertyUndo_t*undo){
 const int role=undo->distanceRole;
 int link=undo->distanceBefore;
 if(link>0){
  const int endpoint=link-1;
  if(!DistanceEndpoint(source,endpoint)||controllerEntityEpoch[endpoint]!=undo->distanceEpoch)
   link=role<JAR_DISTANCE_REFERENCE?0:JAR_DISTANCE_LINK_MISSING;
  else if(role<JAR_DISTANCE_REFERENCE&&ControllerReaches(endpoint,source))return qfalse;
 }
 *DistanceLinkField(source,role)=link;
 ++objects[source].propertyRevision;
 return qtrue;
}

static void DistanceUse(gentity_t*trigger,gentity_t*other,gentity_t*activator){
 const int n=trigger->s.number;
 if(!sandbox.integer||(trigger->flags&FL_INACTIVE)||objects[n].copyPending||
    !JAR_MP_DistanceSettingsValid((int)trigger->speed,trigger->spawnflags))return;
 const int referenceLink=objects[n].distanceInput[0],subjectLink=objects[n].distanceInput[1];
 const gentity_t*reference=trigger;
 const gentity_t*npc=NULL;
 if(referenceLink){
  if(referenceLink>0&&DistanceEndpoint(n,referenceLink-1))reference=&g_entities[referenceLink-1];
  else{
   if(activator&&other==activator&&activator->s.number<MAX_CLIENTS)Tell(activator,"Distance reference is unavailable. Relink or reset it in Context.");
   return;
  }
 }
 if(trigger->spawnflags&JAR_DISTANCE_NPC){
  if(subjectLink>0&&DistanceEndpoint(n,subjectLink-1))npc=&g_entities[subjectLink-1];
  if(!npc||!npc->client||!npc->NPC){
   if(activator&&other==activator&&activator->s.number<MAX_CLIENTS)Tell(activator,"Link an NPC subject before checking this distance.");
   return;
  }
 }

 const float rangeSquared=trigger->speed*trigger->speed;
 qboolean inside=npc&&npc->health>0&&DistanceSquared(reference->r.currentOrigin,npc->r.currentOrigin)<rangeSquared;
 if((trigger->spawnflags&JAR_DISTANCE_PLAYER)&&!inside){
  for(int c=0;c<level.maxclients;++c){
   const gentity_t*player=&g_entities[c];
   if(!player->inuse||!player->client||player->health<=0||
      player->client->pers.connected!=CON_CONNECTED||player->client->sess.sessionTeam==TEAM_SPECTATOR)continue;
   if(DistanceSquared(reference->r.currentOrigin,player->client->ps.origin)<rangeSquared){inside=qtrue;break;}
  }
 }
 trigger->flags|=FL_INACTIVE;
 ControllerCaptureActivator(trigger,activator);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
 JAR_MP_UseTargets(trigger,activator,inside?"@jar_primary":"@jar_secondary");
}
