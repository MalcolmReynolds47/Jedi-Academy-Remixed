// Raven Software / Activision / OpenJK contributors. GPL-2.0-only.
#include "g_logic_settings.h"
#include "g_subbsp_ownership.h"
#include "g_workshop.h"
#include <algorithm>
/*QUAKED target_random (.5 .5 .5) (-4 -4 -4) (4 4 4) USEONCE
Randomly fires off only one of it's targets each time used

USEONCE	set to never fire again
*/

void target_random_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
 if(G_LogicKind(*self)==jar::logic::Random&&G_JAR_WorkshopOwns(self)){
  jar::logic::State settings;
  if(!G_LogicRead(*self,settings)||!settings.enabled||!settings.remaining)return;
  gentity_t* choices[jar::logic::outputCount]{};
  int count=0;
  for(int slot=0;slot<jar::logic::outputCount;++slot){
   const char* name=G_LogicOutput(*self,slot);
   auto* target=name?G_Find(nullptr,FOFS(targetname),name):nullptr;
   if(!target||target==self||!G_JAR_WorkshopOwns(target)||target->e_UseFunc==useF_NULL||
      (target->svFlags&SVF_INACTIVE)||std::find(choices,choices+count,target)!=choices+count)continue;
   choices[count++]=target;
  }
  if(!count)return;
  auto* destination=choices[count==1?0:Q_irand(0,count-1)];
  if(!settings.repeat)self->count=1;
  const auto sourceLife=G_EntityLifetime(self),targetLife=G_EntityLifetime(destination);
  const auto actorLife=G_EntityLifetime(activator);
  G_ActivateBehavior(self,BSET_USE);
  // A native save can restore ordinary entities without a sub-BSP lifetime.
  // Free and reallocation still invalidate an in-use identity in that case.
  if(!self->inuse||sourceLife!=G_EntityLifetime(self)||
     !destination->inuse||targetLife!=G_EntityLifetime(destination))return;
  if(activator&&(!activator->inuse||G_EntityLifetime(activator)!=actorLife))activator=nullptr;
  GEntity_UseFunc(destination,self,G_LogicActivator(*self,activator));
  return;
 }
	int			t_count = 0, pick;
	gentity_t	*t = NULL;
	G_SubBSPCapture sectionScope(self);

	//gi.Printf("target_random %s used by %s (entnum %d)\n", self->targetname, activator->targetname, activator->s.number );
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{
		self->e_UseFunc = useF_NULL;
	}

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return;
	}

	if(t_count == 1)
	{
		G_UseTargets (self, activator);
		return;
	}

	//FIXME: need a seed
	pick = Q_irand(1, t_count);
	t_count = 0;
	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
		else
		{
			continue;
		}

		if (t == self)
		{
//				gi.Printf ("WARNING: Entity used itself.\n");
		}
		else if(t_count == pick)
		{
			if (t->e_UseFunc != useF_NULL)	// check can be omitted
			{
				GEntity_UseFunc(t, self, activator);
				return;
			}
		}

		if (!self->inuse)
		{
			gi.Printf("entity was removed while using targets\n");
			return;
		}
	}
}

void SP_target_random (gentity_t *self)
{
	self->e_UseFunc = useF_target_random_use;
}
