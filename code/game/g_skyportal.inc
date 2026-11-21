// Active viewpoints stack without changing any entity's authored portal flags.
// Native sky changes preempt the stack, including identical values. GPL-2.0-only.
#include "g_skyportal.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <vector>
#include <algorithm>
bool G_SkyPortalPortable(const gentity_t&e){return e.inuse&&jar::skyPortal::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_skyportal");}
namespace {
struct SkyOwner {unsigned entity;std::string identity;};
std::vector<SkyOwner> skyOwners;
std::string skyBaseline,skyApplied;
constexpr unsigned skyOwnerLimit=4096;
std::string skyCurrent(){char text[MAX_STRING_CHARS]{};gi.GetConfigstring(CS_SKYBOXORG,text,sizeof(text));return text;}
jar::skyPortal::View skyView(const gentity_t&e){jar::skyPortal::View v;VectorCopy(e.currentOrigin,v.origin);VectorCopy(e.currentAngles,v.angles);v.entity=e.s.number;v.entities=e.count!=0;return v;}
bool skyMatches(const SkyOwner&o){if(o.entity>=ENTITYNUM_WORLD)return false;const auto&e=g_entities[o.entity];return G_SkyPortalPortable(e)&&e.targetname&&o.identity==e.targetname&&(e.count==0||e.count==1)&&jar::skyPortal::Valid(skyView(e),ENTITYNUM_WORLD);}
void skyPublish(const std::string&text){skyApplied=text;gi.SetConfigstring(CS_SKYBOXORG,text.c_str());}
void skyRestoreTop(){
 if(skyOwners.empty()){const auto baseline=skyBaseline;G_SkyPortalReset();gi.SetConfigstring(CS_SKYBOXORG,baseline.c_str());return;}
 const auto value=jar::skyPortal::Encode(skyView(g_entities[skyOwners.back().entity]));if(value!=skyApplied)skyPublish(value);
}
}
void G_SkyPortalReset(){skyOwners.clear();skyBaseline.clear();skyApplied.clear();}
void G_SetSkyPortal(const char*text){G_SkyPortalReset();gi.SetConfigstring(CS_SKYBOXORG,text?text:"");}
void G_SkyPortalFrame(){
 if(skyOwners.empty())return;
 if(skyCurrent()!=skyApplied){G_SkyPortalReset();return;}
 skyOwners.erase(std::remove_if(skyOwners.begin(),skyOwners.end(),[](const SkyOwner&o){return !skyMatches(o);}),skyOwners.end());skyRestoreTop();
}
int G_SkyPortalOrder(const gentity_t&e){G_SkyPortalFrame();for(size_t i=0;i<skyOwners.size();++i)if(skyOwners[i].entity==unsigned(e.s.number))return int(i);return -1;}
bool G_SkyPortalRead(const gentity_t&e,jar::skyPortal::Settings&s){if(!G_SkyPortalPortable(e)||(e.count!=0&&e.count!=1))return false;s.entities=e.count!=0;s.enabled=G_SkyPortalOrder(e)>=0;return true;}
bool G_SkyPortalSet(gentity_t&e,const jar::skyPortal::Settings&s,int order){
 if(!G_SkyPortalPortable(e)||!e.targetname||!e.targetname[0]||strlen(e.targetname)>63||!jar::skyPortal::Valid(skyView(e),ENTITYNUM_WORLD))return false;
 const int previous=G_SkyPortalOrder(e);const bool hadOwners=!skyOwners.empty();
 if(s.enabled&&previous<0&&skyOwners.size()>=skyOwnerLimit)return false;
 if(s.enabled&&skyOwners.empty()){const auto baseline=skyCurrent();if(!jar::skyPortal::BaselineValid(baseline.c_str(),ENTITYNUM_WORLD))return false;skyBaseline=baseline;}
 e.count=s.entities?1:0;
 if(previous>=0&&(!s.enabled||order>=0))skyOwners.erase(skyOwners.begin()+previous);
 if(s.enabled&&(previous<0||order>=0))skyOwners.insert(skyOwners.begin()+(order<0?skyOwners.size():std::min(size_t(order),skyOwners.size())),{unsigned(e.s.number),e.targetname});
 if(hadOwners||!skyOwners.empty())skyRestoreTop();return true;
}
void G_SkyPortalForget(gentity_t*e){if(!e)return;G_SkyPortalFrame();if(skyOwners.empty())return;const auto before=skyOwners.size();skyOwners.erase(std::remove_if(skyOwners.begin(),skyOwners.end(),[&](const SkyOwner&o){return o.entity==unsigned(e->s.number);}),skyOwners.end());if(before!=skyOwners.size())skyRestoreTop();}
void G_SkyPortalReleaseAll(){G_SkyPortalFrame();if(skyOwners.empty())return;const auto baseline=skyBaseline;G_SkyPortalReset();gi.SetConfigstring(CS_SKYBOXORG,baseline.c_str());}
bool G_SkyPortalMoved(gentity_t&e){if(!G_SkyPortalPortable(e))return false;VectorCopy(e.currentOrigin,e.s.origin);VectorCopy(e.currentAngles,e.s.angles);G_SkyPortalFrame();return true;}
bool G_SkyPortalSpawn(gentity_t&e){
 if(!e.inuse||!jar::skyPortal::Class(e.classname))return false;
 if(e.model&&gi.bIsFromZone(e.model,TAG_G_ALLOC))gi.Free(e.model);e.model=G_NewString("@jar_skyportal");
 e.count=1;e.takedamage=qfalse;e.s.eType=ET_GENERAL;e.contents=CONTENTS_ITEM;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;e.e_UseFunc=useF_skyportal_use;e.svFlags|=SVF_PLAYER_USABLE;
 return true; // Starts off; Activate selects this view after positioning it.
}
void skyportal_use(gentity_t*e,gentity_t*,gentity_t*){jar::skyPortal::Settings s;if(e&&G_SkyPortalRead(*e,s)){s.enabled=!s.enabled;G_SkyPortalSet(*e,s);}}
bool G_SkyPortalRestore(){for(int i=0;i<globals.num_entities;++i){const auto&e=g_entities[i];if(G_SkyPortalPortable(e)&&((e.count!=0&&e.count!=1)||e.e_UseFunc!=useF_skyportal_use||e.e_ThinkFunc!=thinkF_NULL||!jar::skyPortal::Valid(skyView(e),ENTITYNUM_WORLD)))return false;}return true;}
void G_SkyPortalExport(bool autosave,ojk::SavedGameHelper&h){
 G_SkyPortalFrame();h.write<uint32_t>(autosave?0:uint32_t(skyOwners.size()));
 char baseline[MAX_STRING_CHARS]{},applied[256]{};Q_strncpyz(baseline,skyBaseline.c_str(),sizeof(baseline));Q_strncpyz(applied,skyApplied.c_str(),sizeof(applied));h.write<char>(baseline,sizeof(baseline));h.write<char>(applied,sizeof(applied));
 if(!autosave)for(const auto&o:skyOwners){h.write<uint32_t>(o.entity);char identity[64]{};Q_strncpyz(identity,o.identity.c_str(),sizeof(identity));h.write<char>(identity,sizeof(identity));}
}
bool G_SkyPortalImport(bool autosave,ojk::SavedGameHelper&h){
 uint32_t count;h.read<uint32_t>(count);if(count>skyOwnerLimit||(autosave&&count))return false;
 char baseline[MAX_STRING_CHARS],applied[256];h.read<char>(baseline,sizeof(baseline));h.read<char>(applied,sizeof(applied));
 if(!memchr(baseline,0,sizeof(baseline))||!memchr(applied,0,sizeof(applied))||!jar::skyPortal::BaselineValid(baseline,ENTITYNUM_WORLD))return false;
 jar::skyPortal::View v;if((count&&!applied[0])||(!autosave&&!count&&applied[0])||(applied[0]&&!jar::skyPortal::Decode(applied,v,ENTITYNUM_WORLD)))return false;
 std::vector<SkyOwner> candidate;
 for(unsigned i=0;i<count;++i){unsigned entity;char identity[64];h.read<uint32_t>(entity);h.read<char>(identity,sizeof(identity));if(!memchr(identity,0,sizeof(identity))||!identity[0])return false;SkyOwner o{entity,identity};
  if(!skyMatches(o)||std::any_of(candidate.begin(),candidate.end(),[&](const SkyOwner&p){return p.entity==entity;}))return false;candidate.push_back(o);
 }
 if(count&&jar::skyPortal::Encode(skyView(g_entities[candidate.back().entity]))!=applied)return false;
 G_SkyPortalReset();if(!applied[0]||skyCurrent()!=applied)return true;
 if(autosave){gi.SetConfigstring(CS_SKYBOXORG,baseline);return true;}
 skyOwners=std::move(candidate);skyBaseline=baseline;skyApplied=applied;return true;
}
