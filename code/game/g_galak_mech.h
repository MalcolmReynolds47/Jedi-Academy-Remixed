// Restored original mech behavior, separate from Academy vehicle pilots.
// GPL-2.0-only.
#pragma once
void NPC_GalakMech_Precache(void);
void NPC_GalakMech_Init(gentity_t *ent);
void NPC_BSGM_Default(void);
void GM_Dying(gentity_t *self);
void NPC_GM_Pain(gentity_t *self,gentity_t *inflictor,gentity_t *attacker,const vec3_t point,int damage,int mod,int hitLoc);
void NPC_GalakMech_ClearBeam(gentity_t *self);
void NPC_GalakMech_UpdateMuzzle(gentity_t *self);
