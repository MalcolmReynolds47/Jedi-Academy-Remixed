// Native pickup dispatch with explicit imported eligibility/item effects.
static int pickupAllowed=1,pickupEffects,pickupEvents,scoreCalls,scorePoints,scoreClient;
qboolean BG_CanItemBeGrabbed(int gametype,const entityState_t*item,const playerState_t*ps){return pickupAllowed;}
qboolean CheckItemCanBePickedUpByNPC(gentity_t*item,gentity_t*picker){return qfalse;}
void G_LogPrintf(const char*fmt,...){}
void G_ScaleNetHealth(gentity_t*e){}
static int PickupEffect(gentity_t*e,gentity_t*p){++pickupEffects;return 30;}
int Pickup_Weapon(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Ammo(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Armor(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Health(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Powerup(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Holdable(gentity_t*e,gentity_t*p){return PickupEffect(e,p);}
int Pickup_Team(gentity_t*e,gentity_t*p){assert(0);return 0;}
void RespawnItem(gentity_t*e){assert(0);}
void BG_AddPredictableEventToPlayerstate(int event,int parm,playerState_t*ps){assert(event==EV_ITEM_PICKUP);++pickupEvents;}
void G_AddPredictableEvent(gentity_t*e,int event,int parm){assert(0);}
void AddScore(gentity_t*p,vec3_t origin,int score){++scoreCalls;scorePoints=score;scoreClient=p->s.number;p->client->ps.persistant[PERS_SCORE]+=score;}
#define ITMSF_ALLOWNPC 4
#include "game/g_jar_native_pickup.inc"

vmCvar_t g_forcePowerDisable;
qboolean HasSetSaberOnly(void){return qfalse;}
void Use_Item(gentity_t*e,gentity_t*other,gentity_t*activator){assert(0);}
#define ITMSF_SUSPEND 1
#include "game/g_jar_native_item_finish.inc"
