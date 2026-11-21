// Persistent Sandbox tags supplement the authored registry. Included by g_ref.
// Saved native fields: message=name, ownername=scope, radius, SVF_INACTIVE.
namespace {
using ReferenceKey=std::pair<std::string,std::string>;
struct PortableReference {int entity;reference_tag_t tag{};};
std::map<ReferenceKey,PortableReference> portableReferences;
std::map<int,ReferenceKey> referenceEntities;
ReferenceKey referenceKey(const jar::reference::Settings&s){return {s.owner,jar::reference::Name(s.name)};}
bool referenceAvailable(const gentity_t&e,const jar::reference::Settings&s){
 // Native fallback to world tags remains authoritative, including inactive
 // portable name reservations. Edits cannot replace a mission's authored tag.
 if(TAG_FindAuthored(s.owner.c_str(),s.name.c_str()))return false;
 const auto key=referenceKey(s);auto found=portableReferences.find(key);
 if(found!=portableReferences.end()&&found->second.entity!=e.s.number)return false;
 if(s.owner.empty())for(const auto&entry:portableReferences)if(entry.first.second==key.second&&entry.second.entity!=e.s.number)return false;
 if(!s.owner.empty()){found=portableReferences.find({"",key.second});if(found!=portableReferences.end()&&found->second.entity!=e.s.number)return false;}
 return true;
}
void referenceString(char*&field,const std::string&text){
 char*next=text.empty()?nullptr:G_NewString(text.c_str());
 if(field&&gi.bIsFromZone(field,TAG_G_ALLOC))gi.Free(field);field=next;
}
}
bool G_ReferencePortable(const gentity_t&e){return e.inuse&&e.s.number>=0&&e.s.number<ENTITYNUM_WORLD&&jar::reference::Class(e.classname)&&e.model&&!strcmp(e.model,"@jar_reference");}
bool G_ReferenceRead(const gentity_t&e,jar::reference::Settings&out){
 if(!G_ReferencePortable(e)||!std::isfinite(e.radius)||e.radius!=std::floor(e.radius)||e.radius<0||e.radius>1024)return false;
 jar::reference::Settings s;s.name=e.message?e.message:"";s.owner=e.ownername?e.ownername:"";s.radius=int(e.radius);s.enabled=!(e.svFlags&SVF_INACTIVE);
 if(!jar::reference::Valid(s)||(!strcmp(e.classname,"waypoint_navgoal")&&!s.owner.empty()))return false;out=s;return true;
}
void G_ReferenceForget(gentity_t*e){
 if(!e)return;const auto found=referenceEntities.find(e->s.number);if(found==referenceEntities.end())return;
 const auto tag=portableReferences.find(found->second);if(tag!=portableReferences.end()&&tag->second.entity==e->s.number)portableReferences.erase(tag);
 referenceEntities.erase(found);
}
void G_ReferenceReset(){portableReferences.clear();referenceEntities.clear();}
bool G_ReferenceSet(gentity_t&e,const jar::reference::Settings&value){
 if(!G_ReferencePortable(e)||!jar::reference::Valid(value)||(!strcmp(e.classname,"waypoint_navgoal")&&!value.owner.empty()))return false;
 auto s=value;s.name=jar::reference::Name(s.name);if(!referenceAvailable(e,s))return false;
 referenceString(e.message,s.name);referenceString(e.ownername,s.owner);e.radius=float(s.radius);
 e.svFlags=s.enabled?e.svFlags&~SVF_INACTIVE:e.svFlags|SVF_INACTIVE;
 G_ReferenceForget(&e);const auto key=referenceKey(s);referenceEntities.emplace(e.s.number,key);portableReferences.emplace(key,PortableReference{e.s.number});return true;
}
bool G_ReferenceSpawn(gentity_t&e){
 if(!jar::reference::Class(e.classname))return false;
 referenceString(e.model,"@jar_reference");jar::reference::Settings s;
 s.name=e.targetname?e.targetname:va("jar_ref_%d",e.s.number);s.radius=!strcmp(e.classname,"waypoint_navgoal")?12:16;
 if(!G_ReferenceSet(e,s)&&!G_ReferenceCopy(e,s))return false;
 e.s.eType=ET_GENERAL;e.contents=CONTENTS_ITEM;e.takedamage=qfalse;e.e_ThinkFunc=thinkF_NULL;e.nextthink=0;e.e_UseFunc=useF_marker_use;
 return true;
}
bool G_ReferenceCopy(gentity_t&e,const jar::reference::Settings&source){
 if(!jar::reference::Valid(source))return false;auto copy=source;
 const auto prefix=source.name.substr(0,20)+"_copy";
 for(int i=1;i<=65535;++i){copy.name=prefix+std::to_string(i);if(referenceAvailable(e,copy))return G_ReferenceSet(e,copy);}return false;
}
bool G_ReferenceRestore(){
 G_ReferenceReset();
 for(int i=0;i<globals.num_entities;++i){auto&e=g_entities[i];if(!G_ReferencePortable(e))continue;
  jar::reference::Settings s;if(!G_ReferenceRead(e,s)||!G_ReferenceSet(e,s)){G_ReferenceReset();return false;}
 }return true;
}
reference_tag_t*G_ReferenceFind(const char*owner,const char*name){
 if(!name)return nullptr;const std::string keyName=jar::reference::Name(name);
 auto found=portableReferences.find({owner?owner:"",keyName});
 if(found==portableReferences.end()&&owner&&*owner)found=portableReferences.find({"",keyName});
 if(found==portableReferences.end())return nullptr;auto&record=found->second;auto&e=g_entities[record.entity];jar::reference::Settings s;
 if(!G_ReferenceRead(e,s)||!s.enabled||referenceKey(s)!=found->first)return nullptr;
 auto&tag=record.tag;Q_strncpyz(tag.name,s.name.c_str(),sizeof(tag.name));tag.radius=s.radius;tag.flags=!strcmp(e.classname,"waypoint_navgoal")?RTF_NAVGOAL:RTF_NONE;
 VectorCopy(e.currentOrigin,tag.origin);VectorCopy(e.currentAngles,tag.angles);
 if(e.target&&*e.target){if(auto*target=G_Find(nullptr,FOFS(targetname),e.target)){vec3_t direction;VectorSubtract(target->currentOrigin,e.currentOrigin,direction);if(VectorNormalize(direction)>0)vectoangles(direction,tag.angles);}}
 return &tag;
}
