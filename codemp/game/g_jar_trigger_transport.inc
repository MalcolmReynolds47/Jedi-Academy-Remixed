// Portable volumes reuse native push/teleport touch paths and owned destinations.
// The visible body's origin2 remains its prop hull; native push data lives only
// on the derived sensor, which is never a client-writable destination. GPL-2.0-only.
extern void trigger_push_touch(gentity_t*,gentity_t*,trace_t*);
extern void trigger_teleporter_touch(gentity_t*,gentity_t*,trace_t*);
static qboolean TriggerPushReady(gentity_t*e){
 return !(e->spawnflags&4)||!(level.time<e->painDebounceTime+e->wait&&(!(e->spawnflags&2048)||(e->painDebounceTime&&level.time>e->painDebounceTime)));
}
static void TriggerPushTouch(gentity_t*e,gentity_t*h,gentity_t*actor,trace_t*trace){
 gentity_t*mover=TransportMover(actor);vec3_t velocity;
 if(JAR_MP_PushVector(e,actor,velocity)!=1)return;
 playerState_t*ps=&mover->client->ps;
 if(e->spawnflags&4){
  if(ps->pm_type!=PM_NORMAL)return;
  if(!TriggerPushReady(e))return;
 }else if(ps->pm_type!=PM_NORMAL&&ps->pm_type!=PM_JETPACK&&ps->pm_type!=PM_FLOAT)return;
 h->spawnflags=e->spawnflags;h->flags=e->flags;h->wait=e->wait;h->speed=e->speed;h->painDebounceTime=e->painDebounceTime;
 if(e->spawnflags&16)VectorCopy(TransportDestination(e)->r.currentOrigin,h->s.origin2);
 else if(e->spawnflags&4)VectorScale(velocity,1/e->speed,h->s.origin2);
 else VectorCopy(velocity,h->s.origin2);
 trigger_push_touch(h,mover,trace);
 ++objects[e->s.number].propertyRevision;
 e->painDebounceTime=h->painDebounceTime;
 if(!h->touch){objects[e->s.number].triggerActive=0;h->touch=TriggerTouch;}
 if(mover->m_pVehicle){JAR_VehiclePushVelocity(ps,ps->velocity);VectorCopy(ps->velocity,mover->s.pos.trDelta);mover->s.groundEntityNum=ps->groundEntityNum;++objects[mover->s.number].propertyRevision;}
}
#include "g_jar_trigger_props.inc"
static void TriggerTransportTouch(gentity_t*e,gentity_t*h,gentity_t*actor,trace_t*trace){
 // Passive volumes must not flood the HUD every movement frame. Explicit
 // target-controller activations retain their existing result messages.
 qboolean previous=transportQuiet;transportQuiet=qtrue;
 if(ControllerKind(e->s.number)==22)TriggerPushTouch(e,h,actor,trace);
 else trigger_teleporter_touch(e,actor,trace);
 transportQuiet=previous;
}
