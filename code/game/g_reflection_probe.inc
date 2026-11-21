// Authored map probes remain renderer-owned. Sandbox probes have removable
// stock markers and native saved settings. GPL-2.0-only.
#include "g_reflection_probe.h"
static unsigned reflectionEpoch=1;
unsigned G_ReflectionEpoch(){return reflectionEpoch;}
int G_ReflectionCount(const gentity_t* skip){
    int count=0;
    for(int i=1;i<globals.num_entities;++i)if(&g_entities[i]!=skip&&G_ReflectionPortable(g_entities[i]))++count;
    return count;
}
void reflection_probe_use(gentity_t* e,gentity_t*,gentity_t*){
    jar::reflection::Settings s;
    if(!e||(e->svFlags&SVF_INACTIVE)||!G_ReflectionRead(*e,s))return;
    s.enabled=!s.enabled;G_ReflectionSet(*e,s);
}
bool G_ReflectionSpawn(gentity_t& e){
    if(!G_ReflectionPortable(e)||G_ReflectionCount(&e)>=jar::reflection::limit)return false;
    G_SpawnFloat("radius","512",&e.radius);
    e.count=1;
    jar::reflection::Settings s;if(!G_ReflectionRead(e,s))return false;
    e.s.eType=ET_GENERAL;e.e_UseFunc=useF_reflection_probe_use;
    e.s.modelindex=G_ModelIndex("models/map_objects/imperial/crate1.md3");
    VectorSet(e.mins,-21,-21,0);VectorSet(e.maxs,21,21,22);e.contents=CONTENTS_ITEM;
    e.takedamage=qfalse;e.svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;
    G_SetOrigin(&e,e.s.origin);G_SetAngles(&e,e.s.angles);gi.linkentity(&e);
    return true;
}
void SP_misc_cubemap(gentity_t* e){
    if(!G_ReflectionSpawn(*e))G_FreeEntity(e);
}
bool G_ReflectionRestore(){
    if(++reflectionEpoch==0)++reflectionEpoch;
    int count=0;
    for(int i=1;i<globals.num_entities;++i){auto& e=g_entities[i];if(!G_ReflectionPortable(e))continue;
        jar::reflection::Settings s;jar::reflection::Probe p{};p.owner=1;VectorCopy(e.currentOrigin,p.origin);p.radius=e.radius;
        if(++count>jar::reflection::limit||!G_ReflectionRead(e,s)||!jar::reflection::Valid(p)||
            e.s.eType!=ET_GENERAL||e.e_UseFunc!=useF_reflection_probe_use)return false;
        e.svFlags|=SVF_BROADCAST|SVF_PLAYER_USABLE;gi.linkentity(&e);
    }
    return true;
}
