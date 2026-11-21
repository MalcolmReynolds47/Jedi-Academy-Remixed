// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
#include "g_logic_settings.h"
#include "g_workshop.h"
#include <algorithm>

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent )
{
 if(G_LogicKind(*ent)==jar::logic::Delay&&G_JAR_WorkshopOwns(ent)){
  auto* activator=ent->activator;
  ent->activator=nullptr;
  if(!(ent->svFlags&SVF_INACTIVE))
   G_UseTargets(ent,G_LogicActivator(*ent,activator));
  return;
 }
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
 if(G_LogicKind(*ent)==jar::logic::Delay&&G_JAR_WorkshopOwns(ent)){
  jar::logic::State value;
  if(!G_LogicRead(*ent,value)||!value.enabled)return;
  const int delay=value.delayMs+(value.jitterMs?Q_irand(-value.jitterMs,value.jitterMs):0);
  ent->nextthink=level.time+std::max(1,delay);
  ent->e_ThinkFunc=thinkF_Think_Target_Delay;
  ent->activator=activator&&activator->inuse?activator:nullptr;
  G_ActivateBehavior(ent,BSET_USE);
  return;
 }
	G_ActivateBehavior(ent,BSET_USE);

	ent->nextthink = level.time + ( ent->wait + ent->random * Q_flrand(-1.0f, 1.0f) ) * 1000;
	ent->e_ThinkFunc = thinkF_Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent )
{
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) )
	{
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait )
	{
		ent->wait = 1;
	}

	ent->e_UseFunc = useF_Use_Target_Delay;
}
