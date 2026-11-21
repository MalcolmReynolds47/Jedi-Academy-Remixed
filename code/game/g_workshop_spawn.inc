// The menu and duplicator share this synchronous allocation boundary.
// Helpers allocated through native G_Spawn are included. GPL-2.0-only.
gentity_t*spawn(uint32_t catalog,const float*requestedOrigin,const float*requestedAngles,int poseFrame,
 bool wallStartsOff,bool pastePortal,WorkshopSpawnFeedback feedback){
 G_NPCContext npcContext;
 bool exhausted=false;
 {
  G_SpawnTransaction transaction;
  try{
   auto*result=spawnImpl(catalog,requestedOrigin,requestedAngles,poseFrame,wallStartsOff,pastePortal,feedback);
   if(result&&transaction.Commit())return result;
   exhausted=transaction.Exhausted();
  }catch(const G_SpawnCapacity&){exhausted=true;}
 }
 if(exhausted&&feedback==WorkshopSpawnFeedback::Individual){
  message("Entity capacity reached; incomplete spawn removed. Critical reserve preserved.");
  list();
 }
 return nullptr;
}
