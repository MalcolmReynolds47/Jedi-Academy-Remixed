// Ordered Workshop history with an index for local entity removal. GPL-2.0-only.
class WorkshopEditHistory {
 using Key=uint64_t;
 using Records=std::map<Key,Edit>;
 using Index=std::map<uint32_t,std::vector<Key>>;
 Records records;
 Index entities,joints;
 std::set<Key> properties;
 Key next=0;

 static void index(Index& entries,uint32_t slot,Key key){
  auto& keys=entries[slot];
  const auto at=std::lower_bound(keys.begin(),keys.end(),key);
  if(at==keys.end()||*at!=key)keys.insert(at,key);
 }
 static void unindex(Index& entries,uint32_t slot,Key key){
  const auto entry=entries.find(slot);
  if(entry==entries.end())return;
  auto& keys=entry->second;
  const auto at=std::lower_bound(keys.begin(),keys.end(),key);
  if(at!=keys.end()&&*at==key)keys.erase(at);
  if(keys.empty())entries.erase(entry);
 }

public:
 const Records& all() const{return records;}
 bool empty() const{return records.empty();}
 size_t size() const{return records.size();}
 const Edit& front() const{return records.begin()->second;}
 Edit& back(){return records.rbegin()->second;}
 void clear(){records.clear();entities.clear();joints.clear();properties.clear();}
 void pop_back(){erase(records.rbegin()->first);}

 void push_back(Edit edit){
  if(edit.context&&properties.size()==64)erase(*properties.begin());
  const Key key=++next;
  auto& stored=records.emplace(key,std::move(edit)).first->second;
  if(stored.context)properties.insert(key);
  if(stored.joint)index(joints,stored.joint,key);
  else if(stored.group.empty())index(entities,stored.entity,key);
  else for(const auto& member:stored.group)index(entities,member.entity,key);
 }

 void erase(Key key){
  const auto record=records.find(key);
  if(record==records.end())return;
  const auto& edit=record->second;
  if(edit.context)properties.erase(key);
  if(edit.joint)unindex(joints,edit.joint,key);
  else if(edit.group.empty())unindex(entities,edit.entity,key);
  else for(const auto& member:edit.group)unindex(entities,member.entity,key);
  records.erase(record);
 }

 template<class Predicate>
 Records::const_iterator lastFor(uint32_t slot,Predicate accepts) const {
  const auto entry=entities.find(slot);
  if(entry!=entities.end())for(auto it=entry->second.rbegin();it!=entry->second.rend();++it){
   const auto record=records.find(*it);
   if(accepts(record->second))return record;
  }
  return records.end();
 }

 void detachChild(Key key,const SpawnedEdit& root,const SpawnedEdit& child){
  auto& edit=records.at(key);
  if(edit.spawn){
   edit.group.push_back(root);
   edit.spawn=false;
  }
  if(std::none_of(edit.group.begin(),edit.group.end(),[&](const auto& member){return member.owner==child.owner;})){
   edit.group.push_back(child);
   index(entities,child.entity,key);
  }
 }

 void forgetEntity(uint32_t slot){
  const auto entry=entities.find(slot);
  if(entry==entities.end())return;
  const auto keys=std::move(entry->second);
  entities.erase(entry);
  for(const Key key:keys){
   auto& group=records.at(key).group;
   group.erase(std::remove_if(group.begin(),group.end(),[slot](const auto& member){return member.entity==slot;}),group.end());
   if(group.empty())erase(key);
  }
 }

 void forgetAttachment(uint32_t id){
  const auto entry=joints.find(id);
  if(entry!=joints.end()){
   const auto keys=entry->second;
   for(const Key key:keys)erase(key);
  }
  for(auto it=properties.begin();it!=properties.end();){
   const Key key=*it++;
   const auto& edit=records.at(key);
   if(edit.thrusterId==id||edit.ropeLengthId==id)erase(key);
  }
 }
};
