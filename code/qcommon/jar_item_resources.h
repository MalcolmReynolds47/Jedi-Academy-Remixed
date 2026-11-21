// Stock resources for legacy retail pickup paths. GPL-2.0-only.
// Include after the native game/item enums. Kept explicit for catalog generation.
#ifndef JAR_ITEM_RESOURCES_H
#define JAR_ITEM_RESOURCES_H
#include <cstring>
namespace jar { namespace itemResources {
struct Replacement {const char *name,*original,*model;int type,tag;};
static const Replacement replacements[]={
 {"holocron_force_heal","models/map_objects/force_holocrons/heal.md3","models/map_objects/mp/lt_heal.md3",IT_HOLOCRON,FP_HEAL},
 {"holocron_force_levitation","models/map_objects/force_holocrons/jump.md3","models/map_objects/mp/force_jump.md3",IT_HOLOCRON,FP_LEVITATION},
 {"holocron_force_speed","models/map_objects/force_holocrons/speed.md3","models/map_objects/mp/force_speed.md3",IT_HOLOCRON,FP_SPEED},
 {"holocron_force_push","models/map_objects/force_holocrons/push.md3","models/map_objects/mp/force_push.md3",IT_HOLOCRON,FP_PUSH},
 {"holocron_force_pull","models/map_objects/force_holocrons/pull.md3","models/map_objects/mp/force_pull.md3",IT_HOLOCRON,FP_PULL},
 {"holocron_force_telepathy","models/map_objects/force_holocrons/telepathy.md3","models/map_objects/mp/lt_telepathy.md3",IT_HOLOCRON,FP_TELEPATHY},
 {"holocron_force_grip","models/map_objects/force_holocrons/grip.md3","models/map_objects/mp/dk_grip.md3",IT_HOLOCRON,FP_GRIP},
 {"holocron_force_lightning","models/map_objects/force_holocrons/lightning.md3","models/map_objects/mp/dk_lightning.md3",IT_HOLOCRON,FP_LIGHTNING},
 {"holocron_force_saberthrow","models/map_objects/force_holocrons/saberthrow.md3","models/map_objects/mp/saber_throw.md3",IT_HOLOCRON,FP_SABERTHROW},
 {"item_bacta","models/items/bacta.md3","models/map_objects/mp/bacta.md3",IT_HEALTH,ITM_MEDPAK_PICKUP},
 {"weapon_stun_baton","models/weapons2/stun_baton/stunbaton_w.glm","models/weapons2/stun_baton/baton_w.glm",IT_WEAPON,WP_STUN_BATON},
 {"weapon_jawa","models/weapons2/jawa/jawa_gun.md3","models/weapons2/briar_pistol/briar_pistol_w.glm",IT_WEAPON,WP_JAWA},
 {"item_enviro","models/items/shield.md3","models/map_objects/mp/shield.md3",IT_HOLDABLE,INV_PORTABLE_SHIELD},
};
inline const Replacement* Find(const char*name){if(name)for(const auto&r:replacements)if(!std::strcmp(name,r.name))return &r;return nullptr;}
}}
#endif
