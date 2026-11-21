// Native callbacks, extracted for headless lifetime/timer tests. GPL-2.0-only.
#include "g_target_scriptrunner.h"
#include <algorithm>
#include <climits>
#include "g_scriptrunner_properties.inc"
void G_ScriptRunnerForget(gentity_t*entity){
 if(!entity)return;
 for(int i=0;i<std::min(globals.num_entities,int(ENTITYNUM_WORLD));++i){auto&e=g_entities[i];
  if(e.inuse&&e.activator==entity&&jar::scriptRunner::Class(e.classname)){
   e.activator=nullptr;
   if(e.e_ThinkFunc==thinkF_scriptrunner_run&&(e.spawnflags&1)){e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;}
  }
 }
}
static int ScriptRunnerTime(float milliseconds){
 if(!std::isfinite(milliseconds)||milliseconds<=0)return 0;
 return int(std::min(double(INT_MAX),double(level.time)+double(milliseconds)));
}
void scriptrunner_run(gentity_t*self){
 // Consume the delayed callback before scheduling cooldown. A cooldown must
 // not invoke this callback again through G_RunThink.
 self->e_ThinkFunc=thinkF_NULL;self->nextthink=0;
 if(self->count!=-1&&self->count<=0){self->e_UseFunc=useF_NULL;return;}
 if(!self->behaviorSet[BSET_USE])return;
 const bool onActivator=(self->spawnflags&1)!=0;auto*receiver=onActivator?self->activator:self;
 if(!receiver||!receiver->inuse)return;
 if(receiver->m_iIcarusID==IIcarusInterface::ICARUS_INVALID){
  if(!receiver->script_targetname||!receiver->script_targetname[0]){
   static unsigned sequence=0;char name[64];
   do{Com_sprintf(name,sizeof(name),"jar_script_%u",++sequence);}while(G_Find(nullptr,FOFS(script_targetname),name));
   char*replacement=G_NewString(name);
   if(receiver->script_targetname&&gi.bIsFromZone(receiver->script_targetname,TAG_G_ALLOC))gi.Free(receiver->script_targetname);
   receiver->script_targetname=replacement;
  }
  if(!Quake3Game()->ValidEntity(receiver))return;
  Quake3Game()->InitEntity(receiver);
  if(receiver->m_iIcarusID==IIcarusInterface::ICARUS_INVALID)return;
 }
 // Script execution may destroy/reuse either entity. Commit all bookkeeping
 // first and retain the script name independently of its native allocation.
 const std::string script=self->behaviorSet[BSET_USE];
 if(self->count>0)--self->count;
 if(!self->count)self->e_UseFunc=useF_NULL;
 self->nextthink=ScriptRunnerTime(self->wait);
 if(onActivator)Quake3Game()->RunScript(receiver,script.c_str());
 else G_ActivateBehavior(self,BSET_USE);
}
void target_scriptrunner_use(gentity_t*self,gentity_t*other,gentity_t*activator){
 if(self->nextthink>level.time||(self->count!=-1&&self->count<=0))return;
 if((self->spawnflags&1)&&(!activator||!activator->inuse))return;
 self->activator=activator;G_SetEnemy(self,other);
 if(self->delay>0){self->e_ThinkFunc=thinkF_scriptrunner_run;self->nextthink=ScriptRunnerTime(float(self->delay));}
 else scriptrunner_run(self);
}
#include "g_scriptrunner_spawn.inc"
