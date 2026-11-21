// Local single-player Siege objective entities. GPL-2.0-only.
#pragma once

namespace jar { namespace siege {
constexpr int itemTag=0x4a534749;
constexpr int returnDelay=20000;
constexpr int offRadar=8;
constexpr int immediate=1<<18;
constexpr int noPickup=1<<19;
constexpr int forceLimit=1<<20;
constexpr int pickupOnce=1<<21;
constexpr int picked=1<<22;
constexpr int delivered=1<<23;
constexpr int requiresItem=1<<24;
constexpr char itemClass[]="misc_siege_item";
constexpr char objectiveClass[]="info_siege_objective";
constexpr char endClass[]="target_siege_end";
constexpr char crystalModel[]="models/map_objects/mp/crystal_blue.md3";
constexpr char crystalIcon[]="gfx/mp/siegeicons/korriban/blue_crystal.tga";
constexpr char crystalReturn[]="mp/bluecrystalrespawn.efx";
inline bool Item(const gentity_t& e){
    return e.inuse&&e.s.time2==itemTag&&e.classname&&!strcmp(e.classname,itemClass);
}
} }

void SP_misc_siege_item(gentity_t*);
void SP_info_siege_objective(gentity_t*);
void SP_target_siege_end(gentity_t*);
void G_SiegeRestore();
void G_SiegeForget(const gentity_t*);
bool G_SiegeForceLimited(const gentity_t&);
bool G_SiegeTryDeliver(gentity_t&,gentity_t*);
bool G_SiegeKorribanRoom(const gentity_t*);
bool G_SiegeKorribanPrepareBrush(gentity_t&);
bool G_SiegeKorribanPrepareMedia(gentity_t&);
bool G_SiegeKorribanFinishMedia(gentity_t&);
bool G_SiegeKorribanHeavyHit(const gentity_t&,int mod);
bool G_SiegeKorribanStart(gentity_t&);
bool G_SiegeKorribanEntry(const gentity_t&);
void G_SiegeKorribanEntered(gentity_t&,gentity_t&);
bool G_SiegeKorribanPrint(gentity_t&);
