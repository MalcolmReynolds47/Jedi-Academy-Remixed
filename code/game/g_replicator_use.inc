// Native Replicator Use and Think callbacks. GPL-2.0-only.
#include "g_sandbox_replicator.h"
void misc_replicator_item_remove ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if(G_SandboxReplicator(*self)){
		if((self->svFlags&SVF_INACTIVE)||!activator||!activator->inuse||!activator->client||activator->health<=0)return;
		const int maxHealth=activator->client->ps.stats[STAT_MAX_HEALTH];
		if(activator->health<maxHealth){
			activator->health+=std::min(G_ReplicatorHeal(*self),maxHealth-activator->health);
			activator->client->ps.stats[STAT_HEALTH]=activator->health;
		}
		G_ReplicatorApplyPhase(*self,{},level.time);
		return;
	}
	self->s.eFlags |= EF_NODRAW;
	//self->contents = 0;
	self->s.modelindex = 0;
	self->e_UseFunc = useF_misc_replicator_item_spawn;
	//FIXME: pickup sound?
	if ( activator && activator->client )
	{
		activator->health += 5;
		if ( activator->health > activator->client->ps.stats[STAT_MAX_HEALTH] )	// Past max health
		{
			activator->health = activator->client->ps.stats[STAT_MAX_HEALTH];
		}
	}
}

void misc_replicator_item_finish_spawn( gentity_t *self )
{
	if(G_SandboxReplicator(*self)){
		const int variant=self->s.modelindex==self->fly_sound_debounce_time?1:0;
		G_ReplicatorApplyPhase(*self,{jar::replicator::Ready,0,variant},level.time);
		return;
	}
	//self->contents = CONTENTS_ITEM;
	//FIXME: blinks out for a couple frames when transporter effect is done?
	self->e_UseFunc = useF_misc_replicator_item_remove;
}

void misc_replicator_item_spawn ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if(G_SandboxReplicator(*self)){
		if(self->svFlags&SVF_INACTIVE)return;
		G_ReplicatorModels(*self);
		const int delay=G_ReplicatorDelay(*self);
		self->delay=delay;
		G_ReplicatorApplyPhase(*self,{jar::replicator::Forming,delay,Q_irand(0,1)},level.time);
		return;
	}
	switch ( Q_irand( 1, self->count ) )
	{
	case 1:
		self->s.modelindex = self->bounceCount;
		break;
	case 2:
		self->s.modelindex = self->fly_sound_debounce_time;
		break;
	case 3:
		self->s.modelindex = self->painDebounceTime;
		break;
	case 4:
		self->s.modelindex = self->disconnectDebounceTime;
		break;
	case 5:
		self->s.modelindex = self->attackDebounceTime;
		break;
	case 6://max
		self->s.modelindex = self->pushDebounceTime;
		break;
	}
	self->s.eFlags &= ~EF_NODRAW;
	self->e_ThinkFunc = thinkF_misc_replicator_item_finish_spawn;
	self->nextthink = level.time + 4000;//shorter?
	self->e_UseFunc = useF_NULL;

	gentity_t *tent = G_TempEntity( self->currentOrigin, EV_REPLICATOR );
	if (tent)
	{
		tent->owner = self;
	}
}

