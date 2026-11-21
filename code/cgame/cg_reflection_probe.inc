// Submit the complete owned set, independent of camera PVS. Client owner IDs
// distinguish spawn, restore and renderer lifetimes. GPL-2.0-only.
#include "game/g_reflection_probe.h"
#include <map>
#include <string>
struct CGReflectionOwner {std::string identity;unsigned token;};
static std::map<int,CGReflectionOwner> reflectionOwners;
static unsigned reflectionNextOwner=0,reflectionSeenEpoch=0;
void CG_JAR_ReflectionClear(){
    reflectionOwners.clear();reflectionSeenEpoch=0;
    cgi_R_SetReflectionProbes(nullptr,0);
    cgi_Cvar_Set("jar_reflectionStatus","No active probes");
}
void CG_JAR_ReflectionSync(){
    const unsigned epoch=G_ReflectionEpoch();
    if(epoch!=reflectionSeenEpoch){reflectionOwners.clear();reflectionSeenEpoch=epoch;}
    std::map<int,CGReflectionOwner> current;
    jar::reflection::Probe probes[jar::reflection::limit]{};int count=0;
    bool overflow=false;
    for(int i=1;i<globals.num_entities;++i){const auto& e=g_entities[i];jar::reflection::Settings s;
        if(!G_ReflectionRead(e,s)||!s.enabled||(e.svFlags&SVF_INACTIVE)||(e.s.eFlags&EF_NODRAW)||!e.targetname)continue;
        if(count==jar::reflection::limit){overflow=true;break;}
        const auto old=reflectionOwners.find(i);CGReflectionOwner owner;
        if(old!=reflectionOwners.end()&&old->second.identity==e.targetname)owner=old->second;
        else {if(++reflectionNextOwner==0)++reflectionNextOwner;owner={e.targetname,reflectionNextOwner};}
        current.emplace(i,owner);
        auto& p=probes[count];p.owner=owner.token;p.revision=static_cast<unsigned>(e.count);p.radius=s.radius;
        VectorCopy(e.currentOrigin,p.origin);if(jar::reflection::Valid(p))++count;
    }
    reflectionOwners.swap(current);
    if(overflow){cgi_R_SetReflectionProbes(nullptr,0);cgi_Cvar_Set("jar_reflectionStatus","Reflection capacity exceeded");return;}
    const int result=cgi_R_SetReflectionProbes(probes,count);
    const char* status;
    if(!count)status="No active probes";
    else if(result==jar::reflection::Unsupported)status="Requires Rend2 renderer";
    else if(result==jar::reflection::Disabled)status="Enable Cubemap reflections in Graphics";
    else if(result==jar::reflection::NoWorld)status="Waiting for the map";
    else if(result<0)status="Reflection submission rejected";
    else status=va("Reflections ready: %d / %d",result,count);
    cgi_Cvar_Set("jar_reflectionStatus",status);
}
