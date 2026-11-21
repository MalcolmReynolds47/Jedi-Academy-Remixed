// Production deployment, inventory, native callbacks, geometry and save fields.
#include "cgame/cg_headers.h"
#include "game/g_items.h"
#include "qcommon/jar_inventory.h"
#include "test_support.h"
#include "memory_save.h"
#include <deque>
#include <vector>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};
gentity_t g_entities[MAX_GENTITIES];cg_t cg{};bool in_camera=false;
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
static int allocations=0,sounds=0,freed=0;static bool exhausted=false;
gentity_t*G_TrySpawn(qboolean){if(exhausted)return nullptr;auto&e=g_entities[++allocations];e.inuse=qtrue;e.s.number=allocations;globals.num_entities=allocations+1;return &e;}
void G_FreeEntity(gentity_t*e){e->inuse=qfalse;++freed;}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/mp/shield.md3"));return 7;}
int G_SoundIndex(const char*s){CHECK(strstr(s,"sound/"));return 9;}
void G_Sound(gentity_t*,int sound){CHECK(sound==9);++sounds;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);}
static int drawCount=0;static std::string shader;static polyVert_t rendered[4];
qhandle_t cgi_R_RegisterShader(const char*s){shader=s;return 4;}
void cgi_R_AddPolyToScene(qhandle_t h,int n,const polyVert_t*v){CHECK(h==4&&n==4);++drawCount;memcpy(rendered,v,sizeof(rendered));}
#include "game/g_portable_shield.inc"
#include "cgame/cg_portable_shield.inc"
qboolean INV_GoodieKeyGive(gentity_t*){throw std::runtime_error("Unexpected key pickup");}
qboolean INV_SecurityKeyGive(gentity_t*,const char*){throw std::runtime_error("Unexpected key pickup");}
qboolean G_InventorySelectable(int index,gentity_t*e){return index>=0&&index<INV_MAX&&e->client->ps.inventory[index]?qtrue:qfalse;}
#include "game/g_holdable_pickup.inc"
static int traceIndex=0,blockedTrace=-1;static bool occupied=false,noFloor=false,slope=false,moverFloor=false;
static void trace(trace_t*t,const vec3_t start,const vec3_t lo,const vec3_t hi,const vec3_t end,int pass,int mask,EG2_Collision,int){
 CHECK(lo&&hi&&mask==(MASK_SOLID|CONTENTS_BODY));*t={};t->fraction=1;VectorCopy(end,t->endpos);
 if(pass==0){
  const int index=traceIndex++;
  if(index==blockedTrace){t->startsolid=qtrue;t->allsolid=qtrue;t->fraction=0;return;}
  if(index==1&&!noFloor){CHECK(end[2]==start[2]-128);t->fraction=.5f;VectorCopy(start,t->endpos);t->endpos[2]-=64;t->plane.normal[2]=slope?.2f:1;t->entityNum=moverFloor?10:ENTITYNUM_WORLD;}
 }else {CHECK(pass>0);t->startsolid=occupied?qtrue:qfalse;}
}
static bool deploy(gentity_t&p){traceIndex=0;return G_UsePortableShield(&p);}
int main(){return RunTests("portable shield placement, collision, damage, passage, expiry, render and save",[]{
 static_assert(INV_PORTABLE_SHIELD==7&&INV_GOODIE_KEY==5&&INV_SECURITY_KEY==6&&INV_MAX<=MAX_INVENTORY,"Preserve saved inventory indices");
 int inv[MAX_INVENTORY+1];inv[MAX_INVENTORY]=12345;jar::ReadInventory("1 2 3 4 5 6 7",inv,MAX_INVENTORY);CHECK(inv[6]==7&&inv[7]==0&&inv[MAX_INVENTORY]==12345);
 jar::ReadInventory("1 2 3 4 5 6 7 3",inv,MAX_INVENTORY);CHECK(inv[7]==3&&inv[8]==0);
 jar::ReadInventory("-8 999999999999999999999 2 nope 7",inv,MAX_INVENTORY);CHECK(inv[0]==0&&inv[1]==INT_MAX&&inv[2]==2&&inv[3]==0);
 gi.trace=trace;gi.linkentity=[](gentity_t*e){e->linked=qtrue;};
 level.time=1000;auto&p=g_entities[0];gclient_t client{};p.inuse=qtrue;p.health=100;p.client=&client;p.client->playerTeam=TEAM_PLAYER;p.client->ps.inventory[INV_PORTABLE_SHIELD]=5;
 CHECK(!G_UsePortableShield(nullptr));in_camera=qtrue;CHECK(!deploy(p));in_camera=qfalse;
 p.s.m_iVehicleNum=10;CHECK(!deploy(p));p.s.m_iVehicleNum=0;
 for(int stage=0;stage<6;++stage){blockedTrace=stage;CHECK(!deploy(p)&&allocations==0&&client.ps.inventory[INV_PORTABLE_SHIELD]==5);}blockedTrace=-1;
 noFloor=true;CHECK(!deploy(p));noFloor=false;slope=true;CHECK(!deploy(p));slope=false;moverFloor=true;CHECK(!deploy(p));moverFloor=false;
 exhausted=true;CHECK(!deploy(p)&&client.ps.inventory[INV_PORTABLE_SHIELD]==5);exhausted=false;
 CHECK(deploy(p)&&traceIndex==6&&client.ps.inventory[INV_PORTABLE_SHIELD]==4&&allocations==1);auto&e=g_entities[1];
 CHECK(e.health==250&&e.delay==26000&&!e.count&&!e.takedamage&&e.pos1[1]==-254&&e.pos2[1]==254&&e.pos2[2]==253);
 CHECK(e.e_ThinkFunc==thinkF_portable_shield_think&&e.e_TouchFunc==touchF_portable_shield_touch&&e.e_DieFunc==dieF_portable_shield_die);
 level.time=1499;portable_shield_think(&e);CHECK(!e.count);
 occupied=true;level.time=1500;portable_shield_think(&e);CHECK(!e.count&&!e.takedamage);occupied=false;level.time=1600;portable_shield_think(&e);CHECK(e.count&&e.takedamage&&(e.contents&CONTENTS_SOLID)&&e.s.loopSound);
 gentity_t enemy{};gclient_t enemyClient{};enemy.client=&enemyClient;enemy.inuse=qtrue;enemy.health=100;enemyClient.playerTeam=TEAM_ENEMY;
 portable_shield_touch(&e,&enemy,nullptr);CHECK(e.count);portable_shield_touch(&e,&p,nullptr);CHECK(!e.count&&!e.s.loopSound&&!(e.contents&CONTENTS_SOLID));
 occupied=true;level.time=1800;portable_shield_think(&e);CHECK(!e.count);occupied=false;level.time=1900;portable_shield_think(&e);CHECK(e.count);
 e.health-=100;portable_shield_pain(&e,nullptr,nullptr,vec3_origin,100,0,0);CHECK(e.health==150&&e.painDebounceTime==2100);
 centity_t cent{};cent.gent=&e;VectorCopy(e.currentOrigin,cent.lerpOrigin);VectorCopy(e.currentAngles,cent.lerpAngles);cg.time=2000;CG_PortableShield(&cent);CHECK(drawCount==1&&shader=="gfx/misc/blue_dmgshield");
 for(const auto&v:rendered)for(int a=0;a<3;++a)CHECK(v.xyz[a]>=e.currentOrigin[a]+e.mins[a]-.001f&&v.xyz[a]<=e.currentOrigin[a]+e.maxs[a]+.001f);
 cg.time=2200;CG_PortableShield(&cent);CHECK(shader=="gfx/misc/blue_portashield");
 VectorSet(e.currentAngles,20,80,30);CHECK(G_PortableShieldMoved(e));VectorCopy(e.currentAngles,cent.lerpAngles);CG_PortableShield(&cent);
 for(const auto&v:rendered)for(int a=0;a<3;++a)CHECK(v.xyz[a]>=e.currentOrigin[a]+e.mins[a]-.001f&&v.xyz[a]<=e.currentOrigin[a]+e.maxs[a]+.001f);
 occupied=true;CHECK(G_PortableShieldMoved(e)&&!e.count&&!(e.contents&CONTENTS_SOLID));occupied=false;
 level.time=2200;portable_shield_think(&e);CHECK(e.count);
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},loaded{};
 stored.health=e.health;stored.delay=e.delay;stored.count=e.count;stored.nextthink=e.nextthink;stored.attackDebounceTime=e.attackDebounceTime;stored.painDebounceTime=e.painDebounceTime;
 VectorCopy(e.pos1,stored.pos1);VectorCopy(e.pos2,stored.pos2);VectorCopy(e.currentOrigin,stored.currentOrigin);VectorCopy(e.currentAngles,stored.currentAngles);
 stored.e_ThinkFunc=e.e_ThinkFunc;stored.e_TouchFunc=e.e_TouchFunc;stored.e_PainFunc=e.e_PainFunc;stored.e_DieFunc=e.e_DieFunc;
 stored.sg_export(h);save.reset_buffer_offset();loaded.sg_import(h);h.ensure_all_data_read();
 CHECK(loaded.health==150&&loaded.delay==26000&&loaded.count==1&&loaded.e_ThinkFunc==thinkF_portable_shield_think&&loaded.e_DieFunc==dieF_portable_shield_die&&loaded.pos2[2]==253);
 CHECK(G_PortableShieldRestore()&&e.health==150&&e.delay==26000&&e.s.modelindex==7);
 e.pos2[2]=std::numeric_limits<float>::quiet_NaN();CHECK(!G_PortableShieldRestore());e.pos2[2]=253;CHECK(G_PortableShieldRestore());
 level.time=26000;portable_shield_think(&e);CHECK(!e.takedamage&&e.contents==0&&(e.s.eFlags&EF_NODRAW));level.time=26100;portable_shield_think(&e);CHECK(!e.inuse&&freed==1);
 level.time=30000;CHECK(deploy(p));auto&second=g_entities[2];level.time=30500;portable_shield_think(&second);CHECK(second.count);
 portable_shield_die(&second,nullptr,nullptr,300,0,0,0);CHECK(!second.contents&&!second.s.loopSound&&second.nextthink==30600);level.time=30600;portable_shield_think(&second);CHECK(freed==2);
 client.ps.viewangles[YAW]=90;CHECK(deploy(p));auto&third=g_entities[3];CHECK(std::fabs(third.mins[0]+254)<.001f&&std::fabs(third.maxs[1]-4)<.001f);G_PortableShieldCleanup();CHECK(!third.inuse&&freed==3);
 gentity_t pickup{};gitem_t item{};item.giType=IT_HOLDABLE;item.giTag=INV_PORTABLE_SHIELD;item.quantity=100;pickup.item=&item;
 memset(client.ps.inventory,0,sizeof(client.ps.inventory));cg.inventorySelect=0;CHECK(Pickup_Holdable(&pickup,&p)==60);
 CHECK(client.ps.inventory[INV_PORTABLE_SHIELD]==1&&cg.inventorySelect==INV_PORTABLE_SHIELD&&(client.ps.stats[STAT_ITEMS]&(1<<INV_PORTABLE_SHIELD)));
 item.giTag=ITM_SHIELD_PICKUP;CHECK(Pickup_Holdable(&pickup,&p)==0&&client.ps.inventory[INV_PORTABLE_SHIELD]==1);
 CHECK(Pickup_Holdable(nullptr,&p)==0&&Pickup_Holdable(&pickup,nullptr)==0);
 });}
