#ifndef JAR_NPC_RESOURCES_INC
#define JAR_NPC_RESOURCES_INC
#include "qcommon/jar_npc_resources.h"
// Used before native animation/model registration, including precaching, and by
// the loaded-definition audit. Existing MD3/MDR and custom names always win.
static const char* JAR_NpcLegacyLegs(const char* name,const char* value){
 if(!name||!value)return value;
 for(const auto&r:jar::npcResources::legacyLegs){
  if(Q_stricmp(name,r.name)||Q_stricmp(value,r.original))continue;
  char path[MAX_QPATH];
  for(const char*ext:{"md3","mdr"}){
   Com_sprintf(path,sizeof(path),"models/players/%s/lower.%s",r.original,ext);
   if(gi.FS_ReadFile(path,nullptr)>0)return value;
  }
  Com_sprintf(path,sizeof(path),"models/players/%s/lower.md3",r.replacement);
  if(gi.FS_ReadFile(path,nullptr)<=0)return value;
  Com_sprintf(path,sizeof(path),"models/players/%s/animation.cfg",r.replacement);
  if(gi.FS_ReadFile(path,nullptr)<=0)return value;
  return r.replacement;
 }
 return value;
}
// Some unused definitions request a Ghoul2 body which only shipped as MD3.
// Preserve the definition's class/team/weapon; replace only an absent stock body.
static const char* JAR_NpcLegacyBody(const char* name,const char* model,const char* skin){
 if(!name||!model||!skin)return nullptr;
 for(const auto&r:jar::npcResources::legacyBodies){
  if(Q_stricmp(name,r.name)||Q_stricmp(model,r.original)||Q_stricmp(skin,r.skin))continue;
  char path[MAX_QPATH];
  Com_sprintf(path,sizeof(path),"models/players/%s/model.glm",r.original);
  if(gi.FS_ReadFile(path,nullptr)>0)return nullptr;
  for(const char*file:{"lower.md3","animation.cfg"}){
   Com_sprintf(path,sizeof(path),"models/players/%s/%s",r.replacement,file);
   if(gi.FS_ReadFile(path,nullptr)<=0)return nullptr;
  }
  return r.replacement;
 }
 return nullptr;
}
static bool JAR_NpcApplyLegacyBody(const char*name,const char*model,const char*skin,renderInfo_t&ri){
 const char*legs=JAR_NpcLegacyBody(name,model,skin);if(!legs)return false;
 ri.headModelName[0]=ri.torsoModelName[0]=0;
 Q_strncpyz(ri.legsModelName,legs,sizeof(ri.legsModelName));
 ri.headYawRangeLeft=ri.headYawRangeRight=ri.headPitchRangeUp=ri.headPitchRangeDown=0;
 ri.torsoYawRangeLeft=ri.torsoYawRangeRight=ri.torsoPitchRangeUp=ri.torsoPitchRangeDown=0;
 return true;
}
#endif
