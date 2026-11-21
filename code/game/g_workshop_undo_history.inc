// Translate between live editor records and the save's stable IDs. GPL-2.0-only.
void writeUndoHistory(bool autosave,ojk::SavedGameHelper& save){
 std::vector<CreationRecord> records;
 if(!autosave){
  std::unordered_set<uint32_t> liveJoints;
  for(unsigned i=0;i<state.jointCount;++i)liveJoints.insert(state.joints[i].id);
  std::vector<const Owned*> liveObjects(ENTITYNUM_WORLD);
  for(const auto& object:state.owned)if(matches(object))liveObjects[object.entity]=&object;
  for(const auto& entry:edits.all()){
   const auto& edit=entry.second;
   if(edit.joint){
    if(liveJoints.count(edit.joint))records.push_back({CreationKind::Attachment,{edit.joint}});
   }else if(edit.spawn){
    const auto* object=liveObjects[edit.entity];
    if(object&&entityGeneration[edit.entity]==edit.generation)
     records.push_back({CreationKind::Spawn,{object->id}});
   }else if(!edit.group.empty()){
    CreationRecord record{CreationKind::Assembly,{}};
    for(const auto& member:edit.group){
     const auto* object=liveObjects[member.entity];
     if(object&&object->id==member.owner&&entityGeneration[member.entity]==member.generation)
      record.ids.push_back(member.owner);
    }
    if(!record.ids.empty())records.push_back(std::move(record));
   }
  }
 }
 WriteCreationHistory(save,records);
}

void restoreLegacySpawnHistory(){
 // Before V55 the save retained ownership IDs but no undo order.
 for(const auto& object:state.owned)
  if(removalRoot(object)&&matches(object))rememberEdit(g_entities[object.entity],true);
}

bool readUndoHistory(bool autosave,ojk::SavedGameHelper& save){
 std::vector<CreationRecord> records;
 if(!ReadCreationHistory(save,state,autosave,records))return false;
 save.ensure_all_data_read();
 std::unordered_map<uint32_t,const Owned*> objects;
 for(const auto& object:state.owned)objects.emplace(object.id,&object);
 for(const auto& record:records){
  Edit edit{};
  if(record.kind==CreationKind::Attachment)edit.joint=record.ids.front();
  else if(record.kind==CreationKind::Spawn){
   const auto& object=*objects.at(record.ids.front());
   edit.spawn=true;
   edit.entity=object.entity;
   edit.generation=entityGeneration[object.entity];
  }else {
   for(const auto id:record.ids){
    const auto& object=*objects.at(id);
    edit.group.push_back({object.entity,entityGeneration[object.entity],id});
   }
  }
  edits.push_back(std::move(edit));
 }
 return true;
}
