// Portable panels use the native portal PVS path and preserve the authored map.
static void PortalState(int n,jarControllerState_t* state) {
 const gentity_t* e=&g_entities[n];
 state->speed=e->genericValue1;
 state->pushSpeed=e->genericValue2;
 state->active=e->genericValue3;
 state->flags=e->spawnflags;
 state->inactive=!!(e->flags&FL_INACTIVE);
}

static void PortalSync(gentity_t* panel) {
 panel->s.time=JAR_PORTAL_VIEW_OFF;
 panel->r.svFlags&=~SVF_PORTAL;
 if(!sandbox.integer||objects[panel->s.number].copyPending||!panel->genericValue3||(panel->flags&FL_INACTIVE))return;
 if(panel->spawnflags&1){panel->s.time=JAR_PORTAL_VIEW_MIRROR;return;}
 const jarObject_t* object=&objects[panel->s.number];
 const int camera=object->output[0]-1;
 if(ControllerKind(camera)!=JAR_PORTAL_CAMERA_CONTROLLER||!ForcePermission(object->owner,camera)||
    objects[camera].copyPending)return;
 const gentity_t* view=&g_entities[camera];
 if(!view->genericValue3||(view->flags&FL_INACTIVE))return;
 VectorCopy(view->r.currentOrigin,panel->s.origin);
 VectorCopy(view->r.currentAngles,panel->s.angles);
 const int target=objects[camera].output[0]-1;
 if(!(view->spawnflags&1)&&target>=MAX_CLIENTS&&target<ENTITYNUM_WORLD&&
    ForcePermission(objects[camera].owner,target)&&!objects[target].copyPending){
  vec3_t direction;
  VectorSubtract(g_entities[target].r.currentOrigin,view->r.currentOrigin,direction);
  if(VectorLengthSquared(direction)>.001f){
   vectoangles(direction,panel->s.angles);
   panel->s.angles[ROLL]=view->r.currentAngles[ROLL];
  }
 }
 panel->s.time=JAR_PORTAL_VIEW_CAMERA;
 panel->r.svFlags|=SVF_PORTAL;
}

static int PortalSurfaceCount(void) {
 int count=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)
  if(g_entities[n].inuse&&JAR_MP_PortalSurface(&g_entities[n].s))++count;
 return count;
}

static qboolean PortalApply(int n,const jarControllerState_t* state) {
 const int kind=ControllerKind(n);
 if(!JAR_ControllerStateValid(kind,state))return qfalse;
 gentity_t* e=&g_entities[n];
 vec3_t mins,maxs;
 if(kind==JAR_PORTAL_SURFACE_CONTROLLER){
  JAR_MP_PortalHull(state->speed,state->pushSpeed,e->r.currentAngles,mins,maxs);
  if(state->speed!=e->genericValue1||state->pushSpeed!=e->genericValue2){
   trace_t trace;
   trap->Trace(&trace,e->r.currentOrigin,mins,maxs,e->r.currentOrigin,n,MASK_SOLID,qfalse,JAR_TRACE_SPAWN_OVERLAP,0);
   if(trace.startsolid||trace.allsolid)return qfalse;
  }
 }
 e->genericValue1=state->speed;
 e->genericValue2=state->pushSpeed;
 e->genericValue3=state->active;
 e->spawnflags=state->flags;
 if(state->inactive)e->flags|=FL_INACTIVE;else e->flags&=~FL_INACTIVE;
 if(kind==JAR_PORTAL_SURFACE_CONTROLLER){
  e->s.speed=state->speed;
  e->s.eventParm=state->pushSpeed;
  VectorCopy(mins,e->r.mins);VectorCopy(maxs,e->r.maxs);
  PhysicsPublish(e);
  PortalSync(e);
  trap->LinkEntity((sharedEntity_t*)e);
 }
 return qtrue;
}

static void PortalUse(gentity_t* e,gentity_t* other,gentity_t* actor) {
 const int n=e->s.number;
 if(!sandbox.integer||objects[n].copyPending||(e->flags&FL_INACTIVE))return;
 if(other==actor&&actor&&actor->client&&
    (actor->s.number<0||actor->s.number>=MAX_CLIENTS||!Edit(actor,n)))return;
 jarControllerState_t state={0};
 PortalState(n,&state);
 state.active=!state.active;
 PortalApply(n,&state);
 ++objects[n].propertyRevision;
 SelectionChanged(n);
}

static void PortalInit(gentity_t* e,int kind,const jarControllerState_t* saved) {
 const qboolean surface=kind==JAR_PORTAL_SURFACE_CONTROLLER;
 e->s.bolt2=surface?JAR_MP_PORTAL_SURFACE_TAG:JAR_MP_PORTAL_CAMERA_TAG;
 e->r.contents=CONTENTS_ITEM;
 e->r.svFlags|=SVF_PLAYER_USABLE;
 e->use=PortalUse;
 e->genericValue3=1;
 if(surface){
  e->genericValue1=e->s.speed=saved?saved->speed:JAR_PORTAL_WIDTH;
  e->genericValue2=e->s.eventParm=saved?saved->pushSpeed:JAR_PORTAL_HEIGHT;
  e->s.modelindex=0;
 }
}
