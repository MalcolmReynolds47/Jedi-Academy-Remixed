// ICARUS entity-name index; authored names and script files remain unchanged.
// Resolve namespace at lookup time, including after native save restoration.
// GPL-2.0-only.
#include "g_subbsp_ownership.h"
static std::string JarScriptName(const char*name){char key[1024];Q_strncpyz(key,name?name:"",sizeof(key));Q_strupr(key);return key;}
static void JarForgetScriptEntity(entitylist_t&index,const gentity_t*e){
    for(auto it=index.begin();it!=index.end();)if(it->second==e->s.number)it=index.erase(it);else ++it;
}
static void JarAssociateScriptEntity(entitylist_t&index,const gentity_t*e){
    JarForgetScriptEntity(index,e);
    if(e->script_targetname&&e->script_targetname[0])index.emplace(JarScriptName(e->script_targetname),e->s.number);
}
static int JarScriptEntityByName(const entitylist_t&index,const char*name){
    if(!name||!name[0])return -1;
    const auto key=JarScriptName(name);
    const auto range=index.equal_range(key);int found=-1;
    for(auto it=range.first;it!=range.second;++it){const int n=it->second;if(n<0||n>=ENTITYNUM_WORLD||n>=globals.num_entities)continue;const auto&e=g_entities[n];
        if(e.inuse&&e.s.number==n&&e.m_iIcarusID!=IIcarusInterface::ICARUS_INVALID&&G_SubBSPNameVisible(&e)&&JarScriptName(e.script_targetname)==key)found=n;}
    // Native duplicate-name behavior is last registration wins, within scope.
    return found;
}
