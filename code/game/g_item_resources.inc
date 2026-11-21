// Resolve only known missing retail paths, after loading native item definitions.
// Existing models and mod-specific paths/type/tag overrides retain precedence.
#include "qcommon/jar_item_resources.h"
static void JAR_RepairItemResources(){
 for(int i=1;i<bg_numItems;++i){auto&item=bg_itemlist[i];
  if(item.classname&&!strcmp(item.classname,"item_datapad")&&item.giType==IT_HOLDABLE&&item.giTag==INV_DATAPAD&&(!item.icon||!*item.icon)&&gi.FS_ReadFile("models/items/datapad_screen.jpg",nullptr)>0)
   item.icon=G_NewString("models/items/datapad_screen");
 }
 for(int i=1;i<bg_numItems;++i){auto&item=bg_itemlist[i];const auto*r=jar::itemResources::Find(item.classname);
  if(!r||!item.world_model||strcmp(item.world_model,r->original)||item.giType!=r->type||item.giTag!=r->tag)continue;
  if(gi.FS_ReadFile(item.world_model,nullptr)>0||gi.FS_ReadFile(r->model,nullptr)<=0)continue;
  const char*old=item.world_model;item.world_model=G_NewString(r->model);
  if(gi.bIsFromZone(old,TAG_G_ALLOC))gi.Free(const_cast<char*>(old));
 }
}
