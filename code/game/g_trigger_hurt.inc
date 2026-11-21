// Raven Software / Activision / OpenJK contributors; GPL-2.0-only.
#include "g_bsp_section.h"
#include "g_contact_trigger.h"
#include "g_subbsp_ownership.h"
#include "g_bsp_section_hurt.inc"

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.1 .5 .1) ? START_OFF PLAYERONLY SILENT NO_PROTECTION LOCKCAM FALLING ELECTRICAL INACTIVE MULTIPLE
Any entity that touches this will be hurt.
It does dmg points of damage each server frame

PLAYERONLY		only the player is hurt by it
SILENT			supresses playing the sound
NO_PROTECTION	*nothing* stops the damage
LOCKCAM			Falling death results in camera locking in place
FALLING			Forces a falling scream and anim
ELECTRICAL		does electrical damage
INACTIVE		Cannot be triggered until used by a target_activate
MULTIPLE        multiple entities can touch this trigger in a single frame *and* if needed, the trigger can have a wait of > 0

"dmg"			default 5 (whole numbers only)
"delay"			How many seconds it takes to get from 0 to "dmg" (default is 0)
"wait"			Use in instead of "SLOW" - determines how often the player gets hurt, 0.1 is every frame, 1.0 is once per second.  -1 will stop after one use
"count"			If set, FALLING death causes a fade to black in this many milliseconds (default is 10000 = 10 seconds)
"NPC_targetname" - If set, only an NPC with a matching NPC_targetname will trip this trigger
"noise"         sound to play when it hurts something ( default: "sound/world/electro" )
*/
void hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {

	if (G_ContactKind(*self)==jar::contactTrigger::Hurt) {
		if(self->svFlags&SVF_INACTIVE)return;
		const auto lifetime=G_EntityLifetime(self);
		G_ActivateBehavior(self,BSET_USE);
		jar::contactTrigger::Settings settings;
		if(self->inuse&&G_EntityLifetime(self)==lifetime&&G_ContactRead(*self,settings)){
			settings.armed=!settings.armed;
			G_ContactSettings(*self,settings);
		}
		return;
	}

	G_ActivateBehavior(self,BSET_USE);

	//FIXME: Targeting the trigger will toggle its on / off state???
	if ( self->linked ) {
		gi.unlinkentity( self );
	} else {
		gi.linkentity( self );
	}
}

void trigger_hurt_reset (gentity_t *self)
{
	self->attackDebounceTime = 0;
	self->e_ThinkFunc = thinkF_NULL;
}
extern void JET_FlyStart(gentity_t* actor);

static void ApplyHurtDamage(gentity_t& trigger,gentity_t& actor,int damage,int damageFlags){
	const int flags=trigger.spawnflags;
	const int fadeDuration=trigger.count;
	const bool player=actor.s.number==0;
	const auto life=G_EntityLifetime(&actor);
	if ((flags&64)&&actor.client) {
		actor.s.powerups|=1<<PW_SHOCKED;
		actor.client->ps.powerups[PW_SHOCKED]=level.time+1000;
	}
	if (flags&32) {
		if(actor.NPC&&actor.client&&
		   (actor.client->NPC_class==CLASS_BOBAFETT||actor.client->NPC_class==CLASS_ROCKETTROOPER)){
			JET_FlyStart(&actor);
			return;
		}
		G_Damage(&actor,&trigger,&trigger,nullptr,nullptr,damage,damageFlags|DAMAGE_NO_ARMOR,MOD_FALLING);
	} else {
		G_Damage(&actor,&trigger,&trigger,nullptr,nullptr,damage,damageFlags,MOD_TRIGGER_HURT);
	}
	if(!actor.inuse||G_EntityLifetime(&actor)!=life||actor.health>0)return;
	if((flags&32)&&player){
		if(fadeDuration){
			extern void CGCam_Fade(vec4_t source,vec4_t dest,float duration);
			vec4_t src={0,0,0,0},dst={0,0,0,1};
			CGCam_Fade(src,dst,fadeDuration);
		}
		if(flags&16){
			cg.overrides.active|=CG_OVERRIDE_3RD_PERSON_CDP;
			cg.overrides.thirdPersonCameraDamp=0;
		}
		if(actor.client)actor.client->ps.pm_flags|=PMF_SLOW_MO_FALL;
	}
	if((flags&64)&&actor.client)actor.client->ps.powerups[PW_SHOCKED]=level.time+10000;
}

static void PortableHurtDamage(gentity_t& trigger,gentity_t& actor){
	ApplyHurtDamage(trigger,actor,trigger.damage,0);
}

void hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if(G_ContactKind(*self)==jar::contactTrigger::Hurt){
		if(other)G_ContactTouchAt(*self,other,other->currentOrigin);
		return;
	}

	int		dflags;
	int		actualDmg = self->damage;

	if ( self->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return;
	}

	if ( !other->takedamage )
	{
		return;
	}

	if( level.time < self->painDebounceTime + self->wait  ) // normal 'wait' check
	{
		if( self->spawnflags & 2048 ) // MULTIPLE - allow multiple entities to touch this trigger in one frame
		{
			if ( self->painDebounceTime && level.time > self->painDebounceTime ) // if we haven't reached the next frame continue to let ents touch the trigger
			{
				return;
			}
		}
		else // only allowing one ent per frame to touch trigger
		{
			return;
		}
	}

	// if the player has already activated this trigger this frame
	if( other && !other->s.number && self->aimDebounceTime == level.time )
	{
		return;
	}


	if ( self->spawnflags & 2 )
	{//player only
		if ( other->s.number )
		{
			return;
		}
	}

	if ( self->NPC_targetname && self->NPC_targetname[0] )
	{//I am for you, Kirk
		if ( other->script_targetname && other->script_targetname[0] )
		{//must have a name
			if ( Q_stricmp( self->NPC_targetname, other->script_targetname ) != 0 )
			{//not the right guy to fire me off
				return;
			}
		}
		else
		{//no name?  No trigger.
			return;
		}
	}

	// play sound
	if ( !(self->spawnflags & 4) )
	{
		G_Sound( other, self->noise_index );
	}

	if ( self->spawnflags & 8 )
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}

	if ( self->delay )
	{//Increase dmg over time
		if ( self->attackDebounceTime < self->delay )
		{//FIXME: this is for the entire trigger, not per person, so if someone else jumped in after you were in it for 5 seconds, they'd get damaged faster
			actualDmg = floor( (float)(self->damage * self->attackDebounceTime / self->delay) );
		}
		self->attackDebounceTime += FRAMETIME;

		self->e_ThinkFunc = thinkF_trigger_hurt_reset;
		self->nextthink = level.time + FRAMETIME*2;
	}

	if(actualDmg){
		const auto lifetime=G_EntityLifetime(self);
		const bool player=other->s.number==0;
		ApplyHurtDamage(*self,*other,actualDmg,dflags);
		if(!self->inuse||G_EntityLifetime(self)!=lifetime)return;
		if(player)self->aimDebounceTime=level.time;
		self->painDebounceTime=level.time;
	}

	if ( self->wait < 0 )
	{
		self->e_TouchFunc = touchF_NULL;
	}
}

void SP_trigger_hurt( gentity_t *self )
{
    // The stock MP hazards are silent; SP's default shock sound is not a
    // retail asset. Scope the change to this admitted stock section profile.
    const auto* root = G_SubBSPOwner(self);
    if (root && jar::bspSection::Portable(*root)) {
        const auto mode=jar::bspSection::Resource(root->model)->mode;
        if(mode==jar::bspSection::Mode::Hurt||mode==jar::bspSection::Mode::Shaft||mode==jar::bspSection::Mode::Rift||
           mode==jar::bspSection::Mode::Korriban||mode==jar::bspSection::Mode::Hilltops||
           mode==jar::bspSection::Mode::DropZone||mode==jar::bspSection::Mode::Wasteland)
            self->spawnflags |= 4;
    }

	char	buffer[MAX_QPATH];
	char	*s;

	InitTrigger (self);

	if ( !( self->spawnflags & 4 ))
	{
		G_SpawnString( "noise", "sound/world/electro", &s );

		Q_strncpyz( buffer, s, sizeof(buffer) );
		self->noise_index = G_SoundIndex(buffer);
	}

	self->e_TouchFunc = touchF_hurt_touch;

	if ( !self->damage ) {
		self->damage = 5;
	}

	self->delay *= 1000;
	self->wait *= 1000;

	self->contents = CONTENTS_TRIGGER;

	if ( self->targetname ) {//NOTE: for some reason, this used to be: if(self->spawnflags&2)
		self->e_UseFunc = useF_hurt_use;
	}

	// link in to the world if starting active
	if ( !(self->spawnflags & 1) )
	{
		gi.linkentity (self);
	}
	else // triggers automatically get linked into the world by SetBrushModel, so we have to unlink it here
	{
		gi.unlinkentity(self);
	}
}

