// Validate the loaded NPC definition, including mod overrides. Stock fingerprints
// remain required for vehicle/player templates with special construction paths.
std::map<std::string,JarNpcTemplate> npcResources;bool npcResourcesRead=false;
bool actorValid(Actor&a,std::string&model,std::string&failure){
 auto reject=[&](const char*why){failure=why;return false;};
 model=a.model;
 if(!a.vehicle&&Q_stricmp(a.name,"player")){
  if(!npcResourcesRead){std::vector<JarNpcTemplate> catalog;G_JAR_ReadNpcCatalog(catalog,true);for(const auto&e:catalog)npcResources.emplace(e.name,e);npcResourcesRead=true;}
  const auto found=npcResources.find(a.name);if(found==npcResources.end())return reject("NPC definition not loaded");
  const auto&definition=found->second;if(!definition.reason.empty())return reject(definition.reason.c_str());
  a.radius=float(definition.width);a.height=float(definition.height);
  if(definition.npcClass==CLASS_GALAKMECH){a.radius=std::max(a.radius,60.0f);a.height=std::max(a.height,104.0f);}
  if(!definition.legacyModels.empty()){
   bool cached=false;for(int i=0;i<level.numKnownAnimFileSets;++i)if(!Q_stricmp(level.knownAnimFileSets[i].filename,definition.legacyAnimation.c_str())){cached=true;break;}
   if(!cached&&level.numKnownAnimFileSets>=MAX_ANIM_FILES)return reject("NPC animation cache is full");
   for(const auto&path:definition.legacyModels)if(!cgi_R_RegisterModel(path.c_str()))return reject("legacy model initialization failed");
   for(const auto&path:definition.legacySkins)if(!gi.RE_RegisterSkin(path.c_str()))return reject("legacy skin initialization failed");
   model=definition.legacyModels.front();a.model=model.c_str();return true;
  }
  model="models/players/"+definition.model+"/model.glm";
 }else{
  if(!G_JAR_NpcDefinitionMatches(a.name,a.npcHash)&&(!a.npcAlternate||!G_JAR_NpcDefinitionMatches(a.name,a.npcAlternate)))return reject("vehicle/player template differs from its supported definition");
  if(a.vehicle&&(BG_VehicleGetIndex(a.name)<=0||!vehicleDefinition(a)))return reject("vehicle definition missing or changed");
 }
 if(gi.FS_ReadFile(model.c_str(),nullptr)<=0)return reject("model file missing");
 CGhoul2Info_v probe;const bool ok=gi.G2API_InitGhoul2Model(probe,model.c_str(),0,NULL_HANDLE,NULL_HANDLE,0,0)>=0;gi.G2API_CleanGhoul2Models(probe);
 if(!ok)return reject("Ghoul2 model initialization failed");a.model=model.c_str();return true;
}
