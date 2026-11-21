// Reuse the existing sensor: only eligible client movement is predicted.
// Player and pilot masks are distinct; the server rechecks every native touch.
// GPL-2.0-only.
static void TriggerPublishPush(gentity_t*e,gentity_t*h){
 h->r.svFlags|=SVF_NOCLIENT;h->s.generic1=JAR_MP_PUSH_VOLUME;
 h->s.trickedentindex=h->s.trickedentindex2=h->s.trickedentindex3=h->s.trickedentindex4=0;
 if(ControllerKind(e->s.number)!=22||!sandbox.integer||!objects[e->s.number].triggerActive||(e->flags&FL_INACTIVE)||objects[e->s.number].copyPending)return;
 gentity_t*dest=TransportDestination(e);
 if((objects[e->s.number].output[0]&&!dest)||((e->spawnflags&16)&&!dest))return;
 entityState_t*s=&h->s;s->modelindex=objects[e->s.number].triggerExtent;s->frame=e->spawnflags;s->time=e->painDebounceTime;s->time2=(int)e->wait;s->speed=e->speed;s->bolt2=dest?1:0;s->otherEntityNum=e->s.number;s->angles2[0]=g_gravity.value;
 if(dest)VectorCopy(dest->r.currentOrigin,s->origin2);else VectorClear(s->origin2);
 s->pos.trType=s->apos.trType=TR_INTERPOLATE;s->pos.trTime=s->apos.trTime=level.time;VectorCopy(e->r.currentOrigin,s->pos.trBase);VectorCopy(e->r.currentAngles,s->apos.trBase);
 VectorClear(s->pos.trDelta);VectorClear(s->apos.trDelta);
 if(!objects[e->s.number].frozen||objects[e->s.number].heldBy){VectorCopy(objects[e->s.number].velocity,s->pos.trDelta);VectorCopy(e->s.apos.trDelta,s->apos.trDelta);}
 if(!JAR_PushVolumeValid(s))return;
 if(!(e->spawnflags&16)){vec3_t velocity;if(JAR_PushVolumeVelocity(e->spawnflags,e->r.currentOrigin,e->r.currentAngles,dest?dest->r.currentOrigin:NULL,e->r.currentOrigin,e->speed,g_gravity.value,velocity)!=1)return;}
 for(int c=0;c<MAX_CLIENTS;++c){
  gentity_t*p=&g_entities[c];
  if(TransportActor(e,p,qfalse)){
   if(c<16)s->trickedentindex|=1u<<c;else s->trickedentindex2|=1u<<(c-16);
  }
  gentity_t*v=TransportMover(p);
  if(v&&v->m_pVehicle&&v->m_pVehicle->m_pPilot==(bgEntity_t*)p&&TransportActor(e,v,qtrue)){
   if(c<16)s->trickedentindex3|=1u<<c;else s->trickedentindex4|=1u<<(c-16);
  }
 }
 if(s->trickedentindex||s->trickedentindex2||s->trickedentindex3||s->trickedentindex4)h->r.svFlags&=~SVF_NOCLIENT;
}
