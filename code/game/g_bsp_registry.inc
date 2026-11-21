// Persist BSP identity before native entities and remap before collision linking.
// Native entity layouts and authored model strings are unchanged. GPL-2.0-only.
#include "../../shared/qcommon/jar_bsp_model.h"
#include <array>
#include <vector>
namespace {
struct JarBSPRecord {char name[MAX_QPATH]{};int first=0,count=0;unsigned checksum=0;};
std::array<int,MAX_SUBMODELS> jarBspRestoredModels;
bool jarRestoringBsp=false;
void JarResetBSPRestore(){jarRestoringBsp=false;jarBspRestoredModels.fill(-1);}
bool JarValidBSPRecord(const JarBSPRecord&r,int first){
 if(!memchr(r.name,0,sizeof(r.name)))return false;
 char path[MAX_QPATH];int child;
 return JAR_BSPModelName(r.name,path,&child)&&!strchr(r.name,':')&&r.first==first&&first>0&&first<MAX_SUBMODELS&&r.count>0&&r.count<=MAX_SUBMODELS-first;
}
void JarWriteBSPRegistry(bool autosave,int version){
 if(version<jar::workshopSaveV41)return;
 JarBSPRecord base;std::vector<JarBSPRecord> rows;
 if(!autosave){
  if(gi.BSPModelAt(-1,base.name,sizeof(base.name),&base.first,&base.count,&base.checksum)!=1||base.first||base.name[0]||base.count<1||base.count>MAX_SUBMODELS)
   gi.Error(ERR_DROP,"JA:R: invalid main BSP registry while saving");
  int first=base.count;
  for(int slot=0;slot<MAX_SUB_BSP;++slot){JarBSPRecord row;
   const int result=gi.BSPModelAt(slot,row.name,sizeof(row.name),&row.first,&row.count,&row.checksum);if(!result)break;
   if(result!=1)gi.Error(ERR_DROP,"JA:R: unreadable sub-BSP registry while saving");
   if(!JarValidBSPRecord(row,first))gi.Error(ERR_DROP,"JA:R: invalid sub-BSP registry while saving");
   rows.push_back(row);first+=row.count;
  }
 }
 ojk::SavedGameHelper h(gi.saved_game);h.reset_buffer();
 h.write<uint32_t>(uint32_t(rows.size()));h.write<int32_t>(base.count);h.write<uint32_t>(base.checksum);
 for(const auto&r:rows){h.write<char>(r.name,MAX_QPATH);h.write<int32_t>(r.first);h.write<int32_t>(r.count);h.write<uint32_t>(r.checksum);}
 h.write_chunk(INT_ID('J','B','S','P'));
}
void JarReadBSPRegistry(bool autosave,int version){
 JarResetBSPRestore();if(version<jar::workshopSaveV41)return;
 ojk::SavedGameHelper h(gi.saved_game);h.read_chunk(INT_ID('J','B','S','P'));
 uint32_t count,checksum;int baseCount;h.read<uint32_t>(count);h.read<int32_t>(baseCount);h.read<uint32_t>(checksum);
 if(count>MAX_SUB_BSP||baseCount<0||baseCount>MAX_SUBMODELS||(!autosave&&!baseCount)||(autosave&&(count||baseCount||checksum)))
  gi.Error(ERR_DROP,"JA:R: invalid saved BSP registry header");
 std::vector<JarBSPRecord> rows;int first=baseCount;
 for(uint32_t n=0;n<count;++n){JarBSPRecord row;h.read<char>(row.name,MAX_QPATH);h.read<int32_t>(row.first);h.read<int32_t>(row.count);h.read<uint32_t>(row.checksum);
  if(!JarValidBSPRecord(row,first))gi.Error(ERR_DROP,"JA:R: invalid saved BSP model range or name");
  for(const auto&other:rows)if(!Q_stricmp(row.name,other.name))gi.Error(ERR_DROP,"JA:R: duplicate saved BSP identity");
  rows.push_back(row);first+=row.count;
 }
 h.ensure_all_data_read();if(autosave)return;
 JarBSPRecord base;
 if(gi.BSPModelAt(-1,base.name,sizeof(base.name),&base.first,&base.count,&base.checksum)!=1||base.first||base.name[0]||base.count!=baseCount||base.checksum!=checksum)
  gi.Error(ERR_DROP,"JA:R: saved main BSP no longer matches the loaded map");
 std::array<int,MAX_SUBMODELS> remap;remap.fill(-1);std::array<bool,MAX_SUBMODELS> used{};
 for(int n=0;n<baseCount;++n){remap[n]=n;used[n]=true;}
 for(const auto&r:rows){int mapped,models;unsigned hash;
  if(!gi.RegisterBSPModel(r.name,&mapped,&models,&hash)||models!=r.count||hash!=r.checksum||mapped<baseCount||mapped>=MAX_SUBMODELS||models>MAX_SUBMODELS-mapped)
   gi.Error(ERR_DROP,"JA:R: saved BSP %s is missing, changed or exceeds model capacity",r.name);
  for(int n=0;n<models;++n){if(used[mapped+n])gi.Error(ERR_DROP,"JA:R: overlapping restored BSP model ranges");used[mapped+n]=true;remap[r.first+n]=mapped+n;}
 }
 jarBspRestoredModels=remap;jarRestoringBsp=true;
}
void JarLinkRestoredEntity(gentity_t*e){
 if(jarRestoringBsp&&e->bmodel){
  const int model=e->s.modelindex;
  if(model<0||model>=MAX_SUBMODELS||jarBspRestoredModels[model]<0)gi.Error(ERR_DROP,"JA:R: saved brush entity has an unregistered BSP model");
  e->s.modelindex=jarBspRestoredModels[model];
 }
 const qboolean linked=e->linked;e->linked=qfalse;if(linked)gi.linkentity(e);
}
}
