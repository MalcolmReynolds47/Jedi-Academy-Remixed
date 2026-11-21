// Physics props do not execute the native actor trigger-touch loop. Query the
// engine broad phase after physics/joint motion, then clip the swept prop AABB
// against the oriented volume. This catches translation across a whole volume
// in one server frame. The envelope is conservative for rotating prop hulls;
// moving volumes use their current pose. GPL-2.0-only.
typedef struct {unsigned epoch;int valid;vec3_t origin,mins,maxs;} jarTriggerPropStart_t;
static jarTriggerPropStart_t triggerPropStart[MAX_GENTITIES];
static qboolean triggerPropFramePending;
static void TriggerPropReset(void){memset(triggerPropStart,0,sizeof(triggerPropStart));triggerPropFramePending=qfalse;}
static void TriggerPropBegin(void){
 triggerPropFramePending=qtrue;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){jarTriggerPropStart_t*s=&triggerPropStart[n];s->valid=PhysicsProp(n);
  if(s->valid){s->epoch=controllerEntityEpoch[n];VectorCopy(g_entities[n].r.currentOrigin,s->origin);VectorCopy(g_entities[n].r.mins,s->mins);VectorCopy(g_entities[n].r.maxs,s->maxs);}
 }
}
static qboolean TriggerPropSweep(gentity_t*volume,const vec3_t start,const vec3_t end,const vec3_t mins,const vec3_t maxs,vec3_t contact){
 vec3_t axes[3],delta,motion,half,tests[15];AnglesToAxis(volume->r.currentAngles,axes);int count=0;
 for(int a=0;a<3;++a){delta[a]=start[a]+.5f*(mins[a]+maxs[a])-volume->r.currentOrigin[a];half[a]=.5f*(maxs[a]-mins[a]);motion[a]=end[a]-start[a];}
 for(int a=0;a<3;++a){VectorClear(tests[count]);tests[count++][a]=1;VectorCopy(axes[a],tests[count++]);}
 for(int a=0;a<3;++a)for(int b=0;b<3;++b){vec3_t world={0};world[b]=1;CrossProduct(axes[a],world,tests[count++]);}
 float enter=0,leave=1,extent=(float)objects[volume->s.number].triggerExtent;
 for(int i=0;i<count;++i){float reach=.001f,d=DotProduct(delta,tests[i]),v=DotProduct(motion,tests[i]);for(int a=0;a<3;++a)reach+=half[a]*fabsf(tests[i][a])+extent*fabsf(DotProduct(axes[a],tests[i]));
  if(fabsf(v)<.000001f){if(fabsf(d)>reach)return qfalse;continue;}
  float a=(-reach-d)/v,b=(reach-d)/v;enter=fmaxf(enter,fminf(a,b));leave=fminf(leave,fmaxf(a,b));if(enter>leave)return qfalse;
 }
 VectorMA(start,enter,motion,contact);return qtrue;
}
static qboolean TriggerPushProp(gentity_t*source,gentity_t*prop,const vec3_t contact){
 int n=prop->s.number,root=source->s.number;jarObject_t*o=&objects[n];vec3_t velocity;
 if(!sandbox.integer||ControllerKind(root)!=22||!TriggerHelper(root)||!TriggerHelper(root)->r.linked||!objects[root].triggerActive||objects[root].copyPending||(source->flags&FL_INACTIVE)||root==n||objects[root].output[0]==n+1||!PhysicsProp(n)||!ForceMass(n)||!ForcePermission(objects[root].owner,n)||!TriggerPushReady(source))return qfalse;
 if(TransportPushVector(source,contact,NULL,velocity)!=1)return qfalse;
 // Set velocity, not a mass-scaled force: a jump pad has the same exit speed
 // for light and heavy props. Preserve pose, angular momentum and attachments.
 PhysicsVelocity(n,velocity);VectorCopy(velocity,o->velocity);VectorCopy(velocity,prop->s.pos.trDelta);prop->s.pos.trTime=level.time;
 if(velocity[2]>1)prop->s.groundEntityNum=ENTITYNUM_NONE;
 ++o->propertyRevision;++objects[root].propertyRevision;
 if(source->spawnflags&4){if(source->wait==-1){objects[root].triggerActive=0;SelectionChanged(root);}else if(source->wait>0)source->painDebounceTime=level.time;}
 TriggerSync(root); // Prop contacts run after the regular sensor publication.
 return qtrue;
}
static void TriggerPropFrame(void){
 if(!triggerPropFramePending)return;triggerPropFramePending=qfalse;
 if(!sandbox.integer||!trap->EntitiesInBox)return;
 qboolean any=qfalse;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==22&&TriggerHelper(n)&&TriggerHelper(n)->r.linked){any=qtrue;break;}if(!any)return;
 static int hits[MAX_GENTITIES];
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){if(!PhysicsProp(n)||!ForceMass(n))continue;gentity_t*prop=&g_entities[n];jarTriggerPropStart_t*s=&triggerPropStart[n];vec3_t start,mins,maxs,low,high;
  qboolean previous=s->valid&&s->epoch==controllerEntityEpoch[n];VectorCopy(previous?s->origin:prop->r.currentOrigin,start);
  for(int a=0;a<3;++a){mins[a]=previous?fminf(s->mins[a],prop->r.mins[a]):prop->r.mins[a];maxs[a]=previous?fmaxf(s->maxs[a],prop->r.maxs[a]):prop->r.maxs[a];low[a]=fminf(start[a],prop->r.currentOrigin[a])+mins[a];high[a]=fmaxf(start[a],prop->r.currentOrigin[a])+maxs[a];}
  int count=trap->EntitiesInBox(low,high,hits,MAX_GENTITIES);
  for(int i=0;i<count;++i){int h=hits[i];if(h<MAX_CLIENTS||h>=ENTITYNUM_WORLD)continue;int root=triggerParent[h]-1;
   if(root<MAX_CLIENTS||root>=ENTITYNUM_WORLD||ControllerKind(root)!=22||TriggerHelper(root)!=&g_entities[h])continue;
   vec3_t contact;if(TriggerPropSweep(&g_entities[root],start,prop->r.currentOrigin,mins,maxs,contact))TriggerPushProp(&g_entities[root],prop,contact);
  }
 }
}
