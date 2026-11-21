// Native Use, Touch and Think dispatch for owned contact volumes.
#include "g_contact_trigger.h"
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "g_contact_clocks.h"

static void PortableHurtDamage(gentity_t& trigger,gentity_t& actor);

static void ContactFire(gentity_t& trigger){
 trigger.nextthink=0;
 trigger.e_ThinkFunc=thinkF_NULL;
 auto* actor=trigger.activator;
 trigger.activator=nullptr;
 jar::contactTrigger::Settings settings;
 if(!G_ContactRead(trigger,settings)||settings.kind==jar::contactTrigger::Hurt||!settings.enabled||G_ContactSpent(trigger))return;
 if(settings.kind==jar::contactTrigger::Once)G_ContactSetSpent(trigger,true);
 else{
  const int jitter=settings.variation?Q_irand(-settings.variation,settings.variation):0;
  trigger.painDebounceTime=level.time+settings.interval+jitter;
 }
 const auto lifetime=G_EntityLifetime(&trigger),actorLife=G_EntityLifetime(actor);
 G_ActivateBehavior(&trigger,BSET_USE);
 if(!trigger.inuse||G_EntityLifetime(&trigger)!=lifetime)return;
 if(G_EntityLifetime(actor)!=actorLife)actor=nullptr;
 G_UseTargets(&trigger,G_LogicActivator(trigger,actor));
}

static void ContactUse(gentity_t& trigger,gentity_t* actor){
 jar::contactTrigger::Settings settings;
 if(!G_ContactRead(trigger,settings)||settings.kind==jar::contactTrigger::Hurt||!settings.enabled||G_ContactSpent(trigger)||
    G_ContactPending(trigger)||trigger.painDebounceTime>level.time)return;
 trigger.activator=actor&&actor->inuse?actor:nullptr;
 if(!settings.delay){ContactFire(trigger);return;}
 trigger.e_ThinkFunc=thinkF_multi_trigger_run;
 trigger.nextthink=level.time+settings.delay;
}

void G_ContactTouchAt(gentity_t& trigger,gentity_t* actor,const vec3_t origin){
 using namespace jar::contactTrigger;
 if(!G_LogicLiveActor(actor)||!(trigger.contents&CONTENTS_TRIGGER)||
    !G_LogicUseEnabled(trigger)||trigger.painDebounceTime>level.time||G_ContactPending(trigger))return;
 Settings settings;
 if(!G_ContactRead(trigger,settings)||
    (settings.subject==Player&&actor->s.number!=0)||
    (settings.subject==NPCs&&(actor->s.number==0||!actor->NPC))||
    (settings.useRequired&&!(actor->client->usercmd.buttons&BUTTON_USE)))return;
 if(!jar::volume::Contact(trigger.pos1,trigger.pos2,trigger.currentOrigin,trigger.currentAngles,
                          origin,actor->mins,actor->maxs))return;
 if(settings.kind==Hurt){
  if(actor->takedamage&&G_ContactClockReady(trigger,*actor,settings.interval))PortableHurtDamage(trigger,*actor);
 }else ContactUse(trigger,actor);
}
