// Production sky controller, ownership restoration and native save fields.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <deque>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;static std::string current;static unsigned writes=0;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
#include "game/g_skyportal.inc"
static gentity_t&Make(int n,float x){auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString("misc_skyportal");e.targetname=G_NewString(va("sky_%d",n));VectorSet(e.currentOrigin,x,0,32);CHECK(G_SkyPortalSpawn(e));return e;}
static void Use(gentity_t&e){skyportal_use(&e,nullptr,nullptr);}
static jar::skyPortal::View Current(){jar::skyPortal::View v;CHECK(jar::skyPortal::Decode(current.c_str(),v,ENTITYNUM_WORLD));return v;}
int main(){return RunTests("portable sky lifecycle, native precedence, edits, save ownership and teardown",[]{
 static_assert(useF_skyportal_use==86&&useF_portable_portal_use==85,"Append-only native save callback IDs");
 gi.bIsFromZone=[](const void*,memtag_t){return qfalse;};globals.num_entities=64;
 gi.GetConfigstring=[](int n,char*out,int size){CHECK(n==CS_SKYBOXORG);Q_strncpyz(out,current.c_str(),size);};
 gi.SetConfigstring=[](int n,const char*s){CHECK(n==CS_SKYBOXORG);current=s;++writes;};
 const std::string baseline="128.00 -32.00 512.00 3 0.20 0.30 0.40 0 300";G_SetSkyPortal(baseline.c_str());
 auto&a=Make(10,100);auto&b=Make(11,200);jar::skyPortal::Settings settings;
 CHECK(G_SkyPortalRead(a,settings)&&!settings.enabled&&settings.entities&&current==baseline);
 CHECK(a.e_UseFunc==useF_skyportal_use&&a.e_ThinkFunc==thinkF_NULL&&!a.nextthink&&!a.takedamage&&a.contents==CONTENTS_ITEM);
 Use(a);CHECK(Current().entity==10);Use(b);CHECK(Current().entity==11);CHECK(G_SkyPortalOrder(a)==0&&G_SkyPortalOrder(b)==1);
 a.currentOrigin[0]=350;a.currentAngles[YAW]=90;G_SkyPortalMoved(a);CHECK(Current().entity==11);
 Use(b);CHECK(Current().entity==10&&Current().origin[0]==350&&Current().angles[YAW]==90);
 const auto before=writes;G_SkyPortalFrame();G_SkyPortalFrame();CHECK(writes==before); // no stationary config spam
 CHECK(G_SkyPortalSet(a,{true,false}));CHECK(!Current().entities);CHECK(G_SkyPortalSet(a,{true,true}));
 Use(b);CHECK(G_SkyPortalSet(a,{false,true}));CHECK(Current().entity==11);CHECK(G_SkyPortalSet(a,{true,true},0));CHECK(Current().entity==11&&G_SkyPortalOrder(a)==0);
 G_SkyPortalForget(&b);b.inuse=qfalse;CHECK(Current().entity==10);G_SkyPortalForget(&a);a.inuse=qfalse;CHECK(current==baseline);
 a.inuse=b.inuse=qtrue;Use(a);const auto identical=current;G_SetSkyPortal(identical.c_str());CHECK(G_SkyPortalOrder(a)==-1);G_SkyPortalForget(&a);CHECK(current==identical);
 G_SetSkyPortal(baseline.c_str());Use(a);current="1 2 3 0 0 0 0 0 300";G_SkyPortalFrame();CHECK(G_SkyPortalOrder(a)==-1&&current=="1 2 3 0 0 0 0 0 300");
 // Exact authored config and controller ordering survive full saves.
 G_SetSkyPortal(baseline.c_str());Use(a);Use(b);MemorySave saved;ojk::SavedGameHelper h(&saved);G_SkyPortalExport(false,h);G_SkyPortalReset();saved.reset_buffer_offset();CHECK(G_SkyPortalImport(false,h));h.ensure_all_data_read();
 CHECK(G_SkyPortalOrder(a)==0&&G_SkyPortalOrder(b)==1);Use(b);CHECK(Current().entity==10);Use(a);CHECK(current==baseline);
 // Reused slots, duplicate owners, invalid baselines and malformed records reject.
 current=jar::skyPortal::Encode(skyView(b));b.targetname=G_NewString("reused");saved.reset_buffer_offset();CHECK(!G_SkyPortalImport(false,h)&&skyOwners.empty());b.targetname=G_NewString("sky_11");
 auto*bytes=static_cast<byte*>(const_cast<void*>(saved.get_buffer_data()));const size_t header=4+MAX_STRING_CHARS+256;std::memcpy(bytes+header+68,bytes+header,68);saved.reset_buffer_offset();CHECK(!G_SkyPortalImport(false,h));
 G_SetSkyPortal(baseline.c_str());Use(a);MemorySave automatic;ojk::SavedGameHelper ah(&automatic);G_SkyPortalExport(true,ah);G_SkyPortalReset();a.inuse=b.inuse=qfalse;automatic.reset_buffer_offset();CHECK(G_SkyPortalImport(true,ah));ah.ensure_all_data_read();CHECK(current==baseline);
 current="9 8 7 0 0 0 0 0 300";automatic.reset_buffer_offset();CHECK(G_SkyPortalImport(true,ah)&&current=="9 8 7 0 0 0 0 0 300");
 a.inuse=b.inuse=qtrue;Use(a);Use(b);G_SkyPortalReleaseAll();CHECK(current=="9 8 7 0 0 0 0 0 300"&&skyOwners.empty());
 Use(a);a.targetname=G_NewString("different_lifetime");G_SkyPortalFrame();CHECK(skyOwners.empty());
 a.currentOrigin[0]=std::numeric_limits<float>::quiet_NaN();CHECK(!G_SkyPortalSet(a,{true,true}));a.currentOrigin[0]=10;
 current="not a sky config";CHECK(!G_SkyPortalSet(a,{true,true})&&current=="not a sky config");
 // Render option, transformed pose and callback persist through real native fields.
 G_SetSkyPortal("");CHECK(G_SkyPortalSet(a,{true,false}));MemorySave native;ojk::SavedGameHelper nh(&native);gentity_t stored{},loaded{};stored.count=a.count;stored.e_UseFunc=a.e_UseFunc;stored.e_ThinkFunc=a.e_ThinkFunc;VectorCopy(a.currentOrigin,stored.currentOrigin);VectorCopy(a.currentAngles,stored.currentAngles);stored.sg_export(nh);native.reset_buffer_offset();loaded.sg_import(nh);nh.ensure_all_data_read();CHECK(!loaded.count&&loaded.e_UseFunc==useF_skyportal_use&&loaded.currentAngles[YAW]==90&&loaded.currentOrigin[0]==10);
 CHECK(G_SkyPortalRestore());a.e_UseFunc=useF_NULL;CHECK(!G_SkyPortalRestore());a.e_UseFunc=useF_skyportal_use;
 MemorySave invalid;ojk::SavedGameHelper bad(&invalid);bad.write<uint32_t>(4097);invalid.reset_buffer_offset();CHECK(!G_SkyPortalImport(false,bad));
 G_SkyPortalReleaseAll();CHECK(current.empty());
 });}
