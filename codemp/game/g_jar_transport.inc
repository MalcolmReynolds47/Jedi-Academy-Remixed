#include "jar_mp_teleport_volume.h"
// Owned spatial targets resolve object references, never authored map names.
// Native movement/event routines remain responsible for the resulting motion.
static qboolean TransportActor(gentity_t*source,gentity_t*actor,qboolean vehicles){
 if(actor&&actor->inuse&&actor->client&&actor->s.number<MAX_CLIENTS&&ControllerKind(source->s.number)==23&&actor->client->sess.sessionTeam==TEAM_SPECTATOR)
  return actor->client->pers.connected==CON_CONNECTED&&actor->client->ps.pm_type==PM_SPECTATOR&&!(actor->client->ps.pm_flags&PMF_FOLLOW)&&playerTools.integer&&!players[actor->s.number].noTools&&level.time>=players[actor->s.number].noGrabUntil;
 if(!actor||!actor->client||!ActorAlive(actor)||(!vehicles&&actor->m_pVehicle)||(!actor->m_pVehicle&&actor->client->ps.m_iVehicleNum)||JAR_MP_ActorControlled(actor))return qfalse;
 int n=actor->s.number;
 if(n<MAX_CLIENTS)return playerTools.integer&&!players[n].noTools&&level.time>=players[n].noGrabUntil;
 return ForcePermission(objects[source->s.number].owner,n);
}
static gentity_t*TransportDestination(gentity_t*source){
 int n=objects[source->s.number].output[0]-1,kind=ControllerKind(n);
 if(!JAR_ControllerPosition(kind)||objects[n].copyPending||(g_entities[n].flags&FL_INACTIVE)||!ForcePermission(objects[source->s.number].owner,n))return NULL;
 return &g_entities[n];
}
static float TransportTop(const gentity_t*destination){
 return JAR_ControllerStart(ControllerKind(destination->s.number))?0:destination->r.maxs[2];
}
static qboolean transportQuiet;
static void TransportMessage(gentity_t*actor,const char*text){if(!transportQuiet&&actor&&actor->client&&actor->s.number<MAX_CLIENTS)Tell(actor,text);}
static gentity_t*TransportMover(gentity_t*actor){
 if(!actor||!actor->client)return NULL;
 if(actor->m_pVehicle||!actor->client->ps.m_iVehicleNum)return actor;
 int n=actor->client->ps.m_iVehicleNum;
 if(n<MAX_CLIENTS||n>=ENTITYNUM_WORLD||!g_entities[n].inuse||!VehicleHasRider(&g_entities[n],actor))return NULL;
 return &g_entities[n];
}
#include "g_jar_vehicle_teleport.inc"
static int TransportPushVector(gentity_t*source,const vec3_t origin,gentity_t*actor,vec3_t velocity){
 int kind=ControllerKind(source->s.number);
 gentity_t*dest=TransportDestination(source);
 if(!dest&&objects[source->s.number].output[0]){TransportMessage(actor,"Push destination is unavailable or protected.");return -1;}
 if(kind==22){int result=JAR_PushVolumeVelocity(source->spawnflags,source->r.currentOrigin,source->r.currentAngles,dest?dest->r.currentOrigin:NULL,origin,source->speed,g_gravity.value,velocity);if(result==1)return 1;
  if(result==3)TransportMessage(actor,"Move the destination away from the push controller.");else if(result==4)TransportMessage(actor,"Arc mode needs a destination above the source and positive gravity.");else if(result==5)TransportMessage(actor,"Push trajectory exceeds 4000 units/second; move its destination closer.");return -1;
 }
 if(!dest){AngleVectors(source->r.currentAngles,velocity,NULL,NULL);VectorScale(velocity,source->speed,velocity);}
 else {
  VectorSubtract(dest->r.currentOrigin,kind==22&&(source->spawnflags&16)?origin:source->r.currentOrigin,velocity);
  if(source->spawnflags&(kind==22?4:2)){if(VectorNormalize(velocity)<.01f){TransportMessage(actor,"Move the destination away from the push controller.");return -1;}VectorScale(velocity,source->speed,velocity);}
  else {float height=velocity[2],gravity=g_gravity.value;if(height<=.01f||!isfinite(gravity)||gravity<=0){TransportMessage(actor,"Arc mode needs a destination above the source and positive gravity.");return -1;}float time=sqrtf(height/(.5f*gravity));velocity[0]/=time;velocity[1]/=time;velocity[2]=time*gravity;}
 }
 for(int a=0;a<3;++a)if(!isfinite(velocity[a])||fabsf(velocity[a])>4000){TransportMessage(actor,"Push trajectory exceeds 4000 units/second; move its destination closer.");return -1;}
 return 1;
}
int JAR_MP_PushVector(gentity_t*source,gentity_t*actor,vec3_t velocity){
 if(!source)return 0;int kind=ControllerKind(source->s.number);if(kind!=6&&kind!=22)return 0;
 if(kind==22&&(!objects[source->s.number].triggerActive||(source->flags&FL_INACTIVE)))return -1;
 gentity_t*mover=TransportMover(actor);
 if(!sandbox.integer||!TransportActor(source,mover,qtrue)){TransportMessage(actor,"Push requires active, consenting actors or a settled vehicle with permitted occupants, outside a hold.");return -1;}
 return TransportPushVector(source,mover->r.currentOrigin,actor,velocity);
}
qboolean JAR_MP_PushTarget(gentity_t*source,gentity_t*other,gentity_t*activator){
 if(!source||ControllerKind(source->s.number)!=6)return qfalse;
 vec3_t velocity;if(JAR_MP_PushVector(source,activator,velocity)<0)return qtrue;
 gentity_t*mover=TransportMover(activator);playerState_t*ps=&mover->client->ps;
 if(ps->pm_type!=PM_NORMAL&&ps->pm_type!=PM_FLOAT){TransportMessage(activator,"Push requires a moving actor or vehicle outside a freeze or scripted state.");return qtrue;}
 JAR_MP_NativePush(source,other,mover,velocity);
 if(mover->m_pVehicle){
  JAR_VehiclePushVelocity(ps,velocity);
  VectorCopy(ps->velocity,mover->s.pos.trDelta);mover->s.groundEntityNum=ps->groundEntityNum;
  ++objects[mover->s.number].propertyRevision;
 }
 TransportMessage(activator,mover->m_pVehicle?"Vehicle and occupants pushed; native driving continues.":"Pushed in the controller's direction.");return qtrue;
}
qboolean JAR_MP_TeleportTarget(gentity_t*source,gentity_t*actor){
 if(!source)return qfalse;int kind=ControllerKind(source->s.number);const int start=JAR_ControllerStart(kind);if(kind!=7&&kind!=23&&!start)return qfalse;
 if(start&&((source->flags&FL_INACTIVE)||objects[source->s.number].copyPending)){TransportMessage(actor,"The start marker is disabled.");return qtrue;}
 if(kind==23&&(!objects[source->s.number].triggerActive||(source->flags&FL_INACTIVE)||((source->spawnflags&1)&&(!actor||!actor->client||actor->client->sess.sessionTeam!=TEAM_SPECTATOR))))return qtrue;
 gentity_t*mover=TransportMover(actor);
 if(start&&mover&&mover->m_pVehicle){TransportMessage(actor,"Exit the vehicle to visit this start marker.");return qtrue;}
 if(!sandbox.integer||!TransportActor(source,mover,qtrue)){TransportMessage(actor,"Teleport requires active, consenting actors or a settled vehicle with permitted occupants, outside a hold.");return qtrue;}
 gentity_t*dest=start?source:TransportDestination(source);if(!dest){TransportMessage(actor,"Link an available destination marker before teleporting.");return qtrue;}
 if(mover->m_pVehicle){TeleportVehicle(source,mover,dest,actor);return qtrue;}
 // The visible destination body stays below the arrival hull; native teleport
 // adds one unit and emits its stock effects, angle/velocity and interpolation bit.
 vec3_t origin,final;
 if(!JAR_OwnedTeleportArrival(dest->r.currentOrigin,TransportTop(dest),actor->r.mins[2],origin,final))return qtrue;
 trace_t tr;trap->Trace(&tr,final,actor->r.mins,actor->r.maxs,final,actor->s.number,MASK_PLAYERSOLID,qfalse,0,0);
 if(tr.startsolid||tr.allsolid){TransportMessage(actor,"Destination is occupied or obstructed. Nothing teleported.");return qtrue;}
 if(!Room()){TransportMessage(actor,"Entity reserve reached; no teleport effects allocated.");return qtrue;}
 G_ActivateBehavior(source,BSET_USE);actor->client->ps.groundEntityNum=ENTITYNUM_NONE;TeleportPlayer(actor,origin,dest->r.currentAngles);TransportMessage(actor,start?"Teleported to the start marker.":"Teleported to the linked destination.");return qtrue;
}

#include "g_jar_teleport_prediction.inc"
