// LaunchItem supplies dropped-item lifetime/touch behavior; skeletal world
// pickups also need the Ghoul2 instance normally supplied by FinishSpawningItem.
bool workshopPickupModel(gentity_t&e){
 if(!e.item||!e.item->world_model)return false;
 const char* model=e.item->world_model;
 if(const auto* art=jar::carriedWeapon::StockPickupArt(*e.item))
  if(gi.FS_ReadFile(art->pickup,nullptr)>0)model=art->pickup;
 if(Q_stricmp(COM_GetExtension(model),"glm"))return true;
 return gi.G2API_InitGhoul2Model(e.ghoul2,model,G_ModelIndex(model),NULL_HANDLE,NULL_HANDLE,0,0)>=0;
}

void workshopRestoreCarriedPickup(gentity_t& entity){
 if(entity.s.eType==ET_ITEM&&entity.item&&jar::carriedWeapon::StockPickupArt(*entity.item)&&
    (!entity.ghoul2.DiagnosticHandle()||!entity.ghoul2.size()))
  workshopPickupModel(entity);
}
