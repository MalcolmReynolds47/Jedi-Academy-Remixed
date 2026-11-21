// Set the native Use/trigger gate on owned destinations. GPL-2.0-only.
bool useGateMarker(gentity_t& source,gentity_t* other,gentity_t* activator){
 const auto kind=G_LogicKind(source);
 const auto* owner=owned(source.s.number);
 if(!jar::logic::UseGate(kind)||!owner||!matches(*owner))return false;
 jar::logic::State settings;
 if(!G_LogicRead(source,settings)||!settings.enabled)return true;

 const bool direct=other==&g_entities[0]&&activator==other;
 const auto lifetime=G_EntityLifetime(&source);
 G_ActivateBehavior(&source,BSET_USE);
 if(!source.inuse||G_EntityLifetime(&source)!=lifetime)return true;

 const bool enabled=kind==jar::logic::Enable;
 unsigned changed=0;
 for(int slot=0;slot<jar::logic::outputCount;++slot){
  const char* name=G_LogicOutput(source,slot);
  auto* target=name?G_Find(nullptr,FOFS(targetname),name):nullptr;
  if(!target||target==&source||!G_LogicCanSetUse(*target)||G_LogicUseEnabled(*target)==enabled)continue;
  const auto* targetOwner=owned(target->s.number);
  if(!targetOwner||!matches(*targetOwner))continue;
  G_LogicSetUseEnabled(*target,enabled);++changed;
 }
 if(direct){
  if(changed)message(va(enabled?"Use enabled on %u linked entities.":"Use disabled on %u linked entities.",changed));
  else message("No linked use state changed.");
 }
 return true;
}
