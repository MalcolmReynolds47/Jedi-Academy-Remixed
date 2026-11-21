// Actual native holdable pickup/use: reusable reader, message and save fields.
#include "cgame/cg_headers.h"
#include "game/g_items.h"
#include "qcommon/jar_inventory.h"
#include "test_support.h"
#include "memory_save.h"
#include <cstdarg>
game_import_t gi{};cg_t cg{};bool in_camera=false;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
qboolean INV_GoodieKeyGive(gentity_t*){throw std::runtime_error("Unexpected key pickup");}
qboolean INV_SecurityKeyGive(gentity_t*,const char*){throw std::runtime_error("Unexpected key pickup");}
qboolean G_InventorySelectable(int index,gentity_t*e){return index>=0&&index<INV_MAX&&e->client->ps.inventory[index]?qtrue:qfalse;}
#include "game/g_holdable_pickup.inc"
#include "game/g_datapad.inc"
static int opened=0,notes=0;static std::string lastNote;
static void SendNote(int client,const char*format,...){CHECK(client==0);char text[256];va_list args;va_start(args,format);vsnprintf(text,sizeof(text),format,args);va_end(args);lastNote=text;++notes;}
int main(){return RunTests("datapad pickup, reuse, notes, native UI request and inventory persistence",[]{
 static_assert(INV_PORTABLE_SHIELD==7&&INV_DATAPAD==8&&INV_MAX<=MAX_INVENTORY,"Append inventory slots");
 gi.SendConsoleCommand=[](const char*cmd){CHECK(!strcmp(cmd,"datapad\n"));++opened;};
 gi.SendServerCommand=SendNote;
 gentity_t player{},pickup{};gclient_t client{};gitem_t item{};
 player.inuse=qtrue;player.health=100;player.client=&client;pickup.item=&item;item.giType=IT_HOLDABLE;item.giTag=INV_DATAPAD;item.quantity=1;
 CHECK(!G_UseDatapad(&player)&&!opened);cg.inventorySelect=0;
 CHECK(Pickup_Holdable(&pickup,&player)==60&&client.ps.inventory[INV_DATAPAD]==1&&cg.inventorySelect==INV_DATAPAD&&notes==0);
 CHECK(client.ps.stats[STAT_ITEMS]&(1<<INV_DATAPAD));
 const auto before=client.ps;for(int i=0;i<3;++i)CHECK(G_UseDatapad(&player));CHECK(opened==3&&!memcmp(&before,&client.ps,sizeof(before)));
 pickup.message=const_cast<char*>("Access code: 7342");CHECK(Pickup_Holdable(&pickup,&player)==60&&client.ps.inventory[INV_DATAPAD]==1&&notes==1&&lastNote=="cp \"Access code: 7342\"");
 for(const char*text:{"bad\"quote","cmd;quit","bad\\escape","bad\nline","   "}){pickup.message=const_cast<char*>(text);CHECK(Pickup_Holdable(&pickup,&player)==60&&notes==1);}
 std::string tooLong(64,'a');pickup.message=&tooLong[0];CHECK(Pickup_Holdable(&pickup,&player)==60&&notes==1);
 pickup.message=const_cast<char*>("100% charged %s %n");CHECK(Pickup_Holdable(&pickup,&player)==60&&notes==2&&lastNote=="cp \"100% charged %s %n\"");
 CHECK(!G_UseDatapad(nullptr));player.s.number=1;CHECK(!G_UseDatapad(&player)&&Pickup_Holdable(&pickup,&player)==0);player.s.number=0;
 player.inuse=qfalse;CHECK(!G_UseDatapad(&player));player.inuse=qtrue;player.health=0;CHECK(!G_UseDatapad(&player));player.health=100;
 in_camera=true;CHECK(!G_UseDatapad(&player));in_camera=false;client.ps.viewEntity=17;CHECK(!G_UseDatapad(&player));client.ps.viewEntity=0;
 player.client=nullptr;CHECK(!G_UseDatapad(&player));player.client=&client;CHECK(opened==3);
 // Native player state retains the appended slot without widening its array.
 MemorySave save;ojk::SavedGameHelper h(&save);playerState_t loaded{};client.ps.sg_export(h);save.reset_buffer_offset();loaded.sg_import(h);h.ensure_all_data_read();
 CHECK(loaded.inventory[INV_DATAPAD]==1&&loaded.stats[STAT_ITEMS]==client.ps.stats[STAT_ITEMS]);
 int inventory[MAX_INVENTORY];jar::ReadInventory("1 2 3 4 5 6 7 4",inventory,MAX_INVENTORY);CHECK(inventory[7]==4&&inventory[8]==0);
 jar::ReadInventory("1 2 3 4 5 6 7 4 1",inventory,MAX_INVENTORY);CHECK(inventory[7]==4&&inventory[8]==1);
 });}
