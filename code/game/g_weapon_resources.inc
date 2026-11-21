// The retail Jawa gun uses Bryar firing behavior but its model never shipped.
// Use the matching stock pistol art only when the authored model is missing.
#include "qcommon/jar_item_resources.h"
static void JAR_RepairWeaponResources(){
 auto&w=weaponData[WP_JAWA];const auto*r=jar::itemResources::Find("weapon_jawa");
 if(strcmp(w.classname,r->name)||strcmp(w.weaponMdl,r->original)||gi.FS_ReadFile(w.weaponMdl,nullptr)>0)return;
 const char*view="models/weapons2/briar_pistol/briar_pistol.md3";
 if(gi.FS_ReadFile(view,nullptr)<=0||gi.FS_ReadFile(r->model,nullptr)<=0||
    gi.FS_ReadFile("models/weapons2/briar_pistol/briar_pistol_hand.md3",nullptr)<=0)return;
 Q_strncpyz(w.weaponMdl,view,sizeof(w.weaponMdl));
 if(!w.weaponIcon[0])Q_strncpyz(w.weaponIcon,"gfx/hud/w_icon_briar",sizeof(w.weaponIcon));
 if(!w.selectSnd[0])Q_strncpyz(w.selectSnd,"sound/weapons/bryar/select.wav",sizeof(w.selectSnd));
}
