// Creation Undo stores stable ownership and attachment IDs. GPL-2.0-only.
#pragma once

#include "qcommon/jar_workshop_state.h"
#include "qcommon/ojk_saved_game_helper.h"
#include <unordered_map>
#include <unordered_set>

namespace jar { namespace workshop {
enum class CreationKind : uint32_t { Spawn = 1, Assembly, Attachment };
struct CreationRecord {
 CreationKind kind;
 std::vector<uint32_t> ids;
};

inline void WriteCreationHistory(ojk::SavedGameHelper& save,const std::vector<CreationRecord>& records){
 save.write<uint32_t>(uint32_t(records.size()));
 for(const auto& record:records){
  save.write<uint32_t>(uint32_t(record.kind));
  save.write<uint32_t>(uint32_t(record.ids.size()));
  save.write<uint32_t>(record.ids.data(),int(record.ids.size()));
 }
}

inline bool ReadCreationHistory(ojk::SavedGameHelper& save,const State& state,bool autosave,
                                std::vector<CreationRecord>& records){
 uint32_t count;
 save.read<uint32_t>(count);
 if(count>state.ownedCount+state.jointCount||(autosave&&count))return false;
 std::unordered_set<uint32_t> objects,joints;
 for(const auto& object:state.owned)objects.insert(object.id);
 for(unsigned i=0;i<state.jointCount;++i)joints.insert(state.joints[i].id);
 std::vector<CreationRecord> restored;
 restored.reserve(count);
 for(unsigned i=0;i<count;++i){
  uint32_t kind,length;
  save.read<uint32_t>(kind);
  save.read<uint32_t>(length);
  if(kind<uint32_t(CreationKind::Spawn)||kind>uint32_t(CreationKind::Attachment)||!length)return false;
  const auto type=CreationKind(kind);
  if(type!=CreationKind::Assembly&&length!=1)return false;
  auto& remaining=type==CreationKind::Attachment?joints:objects;
  if(length>remaining.size())return false;
  CreationRecord record{type,{}};
  record.ids.reserve(length);
  for(unsigned member=0;member<length;++member){
   uint32_t id;
   save.read<uint32_t>(id);
   if(!remaining.erase(id))return false;
   record.ids.push_back(id);
  }
  restored.push_back(std::move(record));
 }
 records=std::move(restored);
 return true;
}
} }
