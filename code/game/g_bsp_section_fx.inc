// A portable fx_runner keeps native timing and damage. Only registration and
// placement differ from the map-start path. GPL-2.0-only.
#include "g_bsp_section_fx.h"
#include "qcommon/jar_angular_trajectory.h"
static int BSPSectionFXSound(const gentity_t& e)
{
    // Model-less ET_GENERAL and these saved indices are set only by the
    // admitted section adapter. Native fxID is serialized via its wpIndex alias.
    return e.s.eType==ET_GENERAL && !e.s.modelindex && e.classname &&
        !strcmp(e.classname,"fx_runner") && e.spawnflags==jar::bspSectionFX::damageFlag &&
        e.noise_index>0 && e.noise_index<MAX_SOUNDS ? e.noise_index : 0;
}
bool G_BSPSectionInitializeFX(gentity_t& e,const vec3_t rootAngles,const gentity_t* aim)
{
    using namespace jar::bspSectionFX;
    if(!e.inuse || e.bmodel || e.contents || e.s.eType!=ET_MOVER || e.s.modelindex ||
       e.e_ThinkFunc!=thinkF_fx_runner_link || !e.classname || strcmp(e.classname,"fx_runner") ||
       e.target2 || !e.fxFile || bool(e.target)!=bool(aim) ||
       e.fxID<=0 || e.fxID>=MAX_FX) return false;
    const bool fire=e.spawnflags==damageFlag&&!strcmp(e.fxFile,effect)&&e.soundSet&&!strcmp(e.soundSet,soundSet)&&!e.targetname;
    const bool oneShot=e.spawnflags==dustFlag&&!strcmp(e.fxFile,dust)&&!e.soundSet&&e.targetname&&aim;
    if(!fire&&!oneShot)return false;
    vec3_t angles;
    if(aim){
        vec3_t direction;VectorSubtract(aim->currentOrigin,e.currentOrigin,direction);
        if(!VectorNormalize(direction))return false;
        vectoangles(direction,angles);
    }else{
        const vec3_t up={-90,0,0};
        jar::angular::RebaseAngles(up,vec3_origin,rootAngles,angles);
    }
    const int sound=fire?G_SoundIndex(loop):0;
    if(fire&&(sound<=0||sound>=MAX_SOUNDS))return false;
    // The authored torch set has only a middle loop. Map startup does not
    // load that set for a section added later. Use the very same retail wave
    // through native sound configstrings, which are carried by full saves.
    if(e.soundSet&&gi.bIsFromZone(e.soundSet,TAG_G_ALLOC)) gi.Free(e.soundSet);
    e.soundSet=nullptr;
    if(gi.bIsFromZone(e.fxFile,TAG_G_ALLOC)) gi.Free(e.fxFile);
    e.fxFile=nullptr;
    e.s.eType=ET_GENERAL;
    e.noise_index=sound;
    G_SetAngles(&e,angles);
    if(oneShot){
        // The native timer can run before the ordinary 400 ms link think.
        // Resolve the complete room first and arm all dust emitters now.
        e.e_UseFunc=useF_fx_runner_use;
        e.e_ThinkFunc=thinkF_NULL;
        e.nextthink=-1;
    }
    return true;
}
