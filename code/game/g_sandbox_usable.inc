// Authoritative state transitions for the model-backed Sandbox controller. GPL-2.0-only.
#include "g_sandbox_usable.h"
#include "g_subbsp_ownership.h"

static void sandboxUsableOutput(gentity_t& e,gentity_t* actor,bool secondary){
 const auto life=G_EntityLifetime(&e),actorLife=G_EntityLifetime(actor);
 G_ActivateBehavior(&e,BSET_USE);
 if(!life||G_EntityLifetime(&e)!=life)return;
 auto* activator=actorLife&&G_EntityLifetime(actor)==actorLife?actor:&e;
 const char* output=secondary?e.target2:e.target;
 if(output&&*output)G_UseTargets2(&e,activator,output);
}

bool G_SandboxUsableActivate(gentity_t& e,gentity_t* actor,bool direct){
 using namespace jar::sandboxUsable;
 Settings s;
 if(!G_SandboxUsableRead(e,s)||!s.enabled||s.spent||s.remaining)return false;
 if(direct){
  if(!actor||!actor->inuse||!actor->client||actor->health<=0)return false;
  if(actor->NPC?!(s.direct&NpcOnly):!(s.direct&PlayerOnly))return false;
 }
 if(s.pending){
  s.pending=false;
  return G_SandboxUsableSet(e,s);
 }
 if(s.mode==Pulse){
  s.spent=s.reuse==0;s.remaining=s.reuse;s.health=s.capacity;
  if(!G_SandboxUsableSet(e,s))return false;
  sandboxUsableOutput(e,actor,false);
  return true;
 }
 s.solid=!s.solid;
 if(s.solid)s.health=s.capacity;
 if(!G_SandboxUsableSet(e,s)){
  s.solid=false;s.pending=true;
  if(!G_SandboxUsableSet(e,s))return false;
  e.activator=G_EntityLifetime(actor)?actor:nullptr;
  return true;
 }
 sandboxUsableOutput(e,actor,s.solid);
 return true;
}

static void sandboxUsableReturn(gentity_t& e){
 jar::sandboxUsable::Settings s;
 if(!G_SandboxUsableRead(e,s)||!s.pending)return;
 if(!s.enabled){G_SandboxUsableRefresh(e);return;}
 auto* actor=e.activator;
 s.solid=true;s.pending=false;s.health=s.capacity;
 if(!G_SandboxUsableSet(e,s)){G_SandboxUsableRefresh(e);return;}
 sandboxUsableOutput(e,actor,true);
}

static void sandboxUsableDamage(gentity_t& e,gentity_t* attacker,bool lethal){
 using namespace jar::sandboxUsable;
 if(e.damage==NoDamage||(!lethal&&e.damage!=OnHit))return;
 if(lethal)e.health=0;
 G_SandboxUsableActivate(e,attacker,false);
}
