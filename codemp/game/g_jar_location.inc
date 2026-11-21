// Portable labels share the native team chat and overlay location indices.
// GPL-2.0-only.
static locationData_t portableLocations[MAX_LOCATIONS];
static int locationOwners[MAX_LOCATIONS]; // Entity number plus one; zero is free.

static int LocationSlot(int entity){
 const int slot=g_entities[entity].genericValue3;
 return slot>0&&slot<MAX_LOCATIONS&&locationOwners[slot]==entity+1?slot:0;
}

static void LocationForget(int entity){
 const int slot=LocationSlot(entity);
 if(!slot)return;
 locationOwners[slot]=0;
 memset(&portableLocations[slot],0,sizeof(portableLocations[slot]));
 trap->SetConfigstring(CS_LOCATIONS+slot,"");
}

static void LocationUse(gentity_t *entity,gentity_t *other,gentity_t *activator){
 const int n=entity->s.number;
 if(!sandbox.integer||objects[n].copyPending)return;
 entity->flags^=FL_INACTIVE;
 SelectionChanged(n);
}

static qboolean LocationApply(int entity,const jarControllerState_t *state){
 const int slot=LocationSlot(entity);
 if(!slot||!JAR_ControllerStateValid(ControllerKind(entity),state))return qfalse;
 gentity_t *marker=&g_entities[entity];
 locationData_t *location=&portableLocations[slot];
 if(strcmp(location->message,state->text)){
  Q_strncpyz(location->message,state->text,sizeof(location->message));
  trap->SetConfigstring(CS_LOCATIONS+slot,location->message);
 }
 location->count=state->flags;
 marker->speed=(float)state->speed;
 if(state->inactive)marker->flags|=FL_INACTIVE;else marker->flags&=~FL_INACTIVE;
 return qtrue;
}

static void LocationState(int entity,jarControllerState_t *state){
 const int slot=LocationSlot(entity);
 if(!slot)return;
 const gentity_t *marker=&g_entities[entity];
 const locationData_t *location=&portableLocations[slot];
 Q_strncpyz(state->text,location->message,sizeof(state->text));
 state->flags=location->count;
 state->speed=(int)marker->speed;
 state->inactive=(marker->flags&FL_INACTIVE)!=0;
}

static qboolean LocationInit(gentity_t *entity,int kind){
 // The map's locations occupy the first indices and are immutable at runtime.
 int slot=level.locations.num+1;
 while(slot<MAX_LOCATIONS&&locationOwners[slot])++slot;
 if(slot>=MAX_LOCATIONS)return qfalse;
 locationOwners[slot]=entity->s.number+1;
 entity->genericValue3=slot;
 entity->use=LocationUse;
 portableLocations[slot]=(locationData_t){0};
 portableLocations[slot].cs_index=slot;
 entity->message=portableLocations[slot].message;
 jarControllerState_t defaults={0};
 Q_strncpyz(defaults.text,"Sandbox location",sizeof(defaults.text));
 if(kind==JAR_LOCATION_VOLUME_CONTROLLER)defaults.speed=128;
 return LocationApply(entity->s.number,&defaults);
}

locationData_t *JAR_MP_Location(const gentity_t *subject,locationData_t *authored,float authoredDistance){
 if(!sandbox.integer)return authored;
 const float *origin=subject->r.currentOrigin;
 locationData_t *visible=authored,*nearest=NULL,*volume=NULL;
 float visibleDistance=authoredDistance,nearestDistance=INFINITY,volumeDistance=INFINITY;
 for(int slot=level.locations.num+1;slot<MAX_LOCATIONS;++slot){
  const int n=locationOwners[slot]-1;
  if(n<MAX_CLIENTS||n>=level.num_entities||objects[n].copyPending)continue;
  const gentity_t *marker=&g_entities[n];
  const int kind=ControllerKind(n);
  if(!JAR_ControllerLocation(kind)||(marker->flags&FL_INACTIVE))continue;
  const float distance=DistanceSquared(origin,marker->r.currentOrigin);
  if(!isfinite(distance))continue;
  locationData_t *location=&portableLocations[slot];
  VectorCopy(marker->r.currentOrigin,location->origin);
  if(kind==JAR_LOCATION_VOLUME_CONTROLLER){
   if(distance<volumeDistance&&JAR_VolumeContact(marker->r.currentOrigin,marker->r.currentAngles,
      marker->speed,origin,subject->r.mins,subject->r.maxs)){volume=location;volumeDistance=distance;}
   continue;
  }
  if(distance<nearestDistance){nearest=location;nearestDistance=distance;}
  if(distance<visibleDistance&&trap->InPVS(origin,marker->r.currentOrigin)){
   visible=location;visibleDistance=distance;
  }
 }
 return volume?volume:visible?visible:nearest;
}
