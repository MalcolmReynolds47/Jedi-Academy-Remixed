// Native assembly child catalog and ownership. GPL-2.0-only.
void keepDetachedChildCreation(const Owned& parent,const Owned& child);
void detachChild(gentity_t*source,gentity_t*child){
 const auto*parent=source?owned(source->s.number):nullptr;auto*member=child?owned(child->s.number):nullptr;
 if(parent&&member&&matches(*parent)&&matches(*member)&&member->parent==parent->id){
  keepDetachedChildCreation(*parent,*member);
  SetOwnershipParent(state,*member,0);
  spawnCountDirty=true;
 }
}
uint32_t childCatalog(const gentity_t&e){
 if(e.item){
  if(e.item->giType==IT_WEAPON&&CatalogValid(2000+e.item->giTag))return 2000+e.item->giTag;
  for(const auto&s:supplies)if(e.item==supplyItem(s))return s.id;
 }
 if(e.classname)for(const auto&c:jar::catalog::entries)if(c.kind==5&&!c.reason[0]&&!strcmp(c.name,e.classname))return c.id;
 return 0;
}
bool trackChild(gentity_t*source,gentity_t*drop){
 auto*owner=source?owned(source->s.number):nullptr;
 if(!owner||!matches(*owner))return true;
 if(!drop||!drop->inuse)return false;
 const auto parent=owned(source->s.number)->id;
 const auto catalog=childCatalog(*drop);if(!catalog||!Own(state,drop->s.number,catalog,parent))return false;
 const auto& record=state.owned.back();
 drop->targetname=G_NewString(va("jar_sb_%u",record.id));return true;
}

