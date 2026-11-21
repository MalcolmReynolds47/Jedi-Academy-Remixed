// Production portal lifecycle, camera linkage and native saved fields. GPL-2.0-only.
#include "game/g_local.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include <deque>
#include <string>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::deque<std::string> strings;
char*G_NewString(const char*s){strings.emplace_back(s);return const_cast<char*>(strings.back().c_str());}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/kejim/impcam.md3"));return 7;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentAngles);VectorCopy(p,e->s.apos.trBase);}
static int nextChild=20,finds=0;static bool exhaust=false;
gentity_t*G_JAR_SpawnSetupChild(gentity_t*){if(exhaust)return nullptr;auto&e=g_entities[nextChild];e.inuse=qtrue;e.s.number=nextChild++;globals.num_entities=nextChild;return &e;}
gentity_t*G_Find(gentity_t*from,int field,const char*match){++finds;CHECK(field==FOFS(targetname));for(int n=from?from->s.number+1:0;n<globals.num_entities;++n){auto&e=g_entities[n];if(e.inuse&&e.targetname&&!strcmp(e.targetname,match))return &e;}return nullptr;}
#include "game/g_portal.inc"
static_assert(useF_portable_portal_use==85&&useF_weather_use==84,"Append-only save callbacks");
int main(){return RunTests("portable portal native lifecycle and save fields",[]{
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid save");};
 gi.trace=[](trace_t* hit,const vec3_t,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){*hit={};hit->fraction=1;VectorCopy(end,hit->endpos);};
 auto make=[](int n,bool surface)->gentity_t&{auto&e=g_entities[n];e.inuse=qtrue;e.s.number=n;e.classname=G_NewString(surface?"misc_portal_surface":"misc_portal_camera");e.model=G_NewString(surface?"@jar_portal_surface":"@jar_portal_camera");VectorSet(e.currentOrigin,100,200,300);CHECK(G_PortalSpawnPair(&e));return e;};
 auto&s=make(2,true);auto*c=s.owner;CHECK(c&&G_PortalCamera(*c)&&s.e_UseFunc==useF_portable_portal_use&&s.s.eType==ET_PORTAL&&!s.s.modelindex&&c->s.modelindex==7);
 CHECK(s.maxs[0]==2&&s.maxs[1]==64&&s.maxs[2]==96&&c->currentOrigin[0]==228&&(s.svFlags&SVF_PORTAL)&&!(s.svFlags&SVF_BROADCAST));
 jar::portal::View view;CHECK(G_PortalView(s,view)&&view.cameraOrigin[0]==228&&finds==0);
 CHECK(view.axis[0][0]==1&&view.cameraAxis[0][0]==1);
 c->currentOrigin[0]=950;VectorSet(c->currentAngles,20,30,40);G_PortalMoved(*c);G_PortalFrame();CHECK(s.s.origin2[0]==950&&G_PortalView(s,view));vec3_t axes[3];AnglesToAxis(c->currentAngles,axes);for(int i=0;i<3;++i)for(int j=0;j<3;++j)CHECK(std::fabs(view.cameraAxis[i][j]-axes[i][j])<.0001f);
 VectorSet(s.currentAngles,25,40,60);CHECK(G_PortalMoved(s));AnglesToAxis(s.currentAngles,axes);for(int a=0;a<3;++a)CHECK(std::fabs(s.maxs[a]-(2*std::fabs(axes[0][a])+64*std::fabs(axes[1][a])+96*std::fabs(axes[2][a])))<.0001f);
 auto&look=g_entities[3];look.inuse=qtrue;look.s.number=3;look.targetname=G_NewString("look_here");VectorSet(look.currentOrigin,950,500,300);c->target=G_NewString(look.targetname);G_PortalFrame();CHECK(c->owner==&look&&G_PortalView(s,view));vec3_t tracking={0,90,40};AnglesToAxis(tracking,axes);for(int i=0;i<3;++i)for(int j=0;j<3;++j)CHECK(std::fabs(view.cameraAxis[i][j]-axes[i][j])<.0001f);
 c->count=1;CHECK(G_PortalView(s,view));AnglesToAxis(c->currentAngles,axes);CHECK(std::fabs(view.cameraAxis[0][0]-axes[0][0])<.0001f);c->count=0;
 portable_portal_use(c,nullptr,nullptr);CHECK(!G_PortalView(s,view)&&jar::portal::Valid(view));portable_portal_use(c,nullptr,nullptr);CHECK(G_PortalView(s,view));
 G_PortalForget(&look);look.inuse=qfalse;CHECK(!c->owner);look.inuse=qtrue;look.targetname=G_NewString("slot_reused");level.time+=1000;G_PortalFrame();CHECK(!c->owner);
 // A broken link cannot borrow a nearby unrelated camera, and read-only render never searches.
 s.target=G_NewString("new_camera");const int before=finds;CHECK(!G_PortalView(s,view)&&jar::portal::Valid(view)&&finds==before);G_PortalFrame();CHECK(!s.owner);const int retry=finds;G_PortalFrame();CHECK(finds==retry);
 c->targetname=G_NewString("new_camera");level.time+=1000;G_PortalFrame();CHECK(s.owner==c&&G_PortalView(s,view));
 jar::portal::Settings settings;CHECK(G_PortalRead(s,settings));settings.width=256;settings.height=512;settings.mirror=true;CHECK(G_PortalSet(s,settings));G_PortalForget(c);c->inuse=qfalse;CHECK(!s.owner&&G_PortalView(s,view)&&view.mirror&&view.cameraOrigin[0]==s.currentOrigin[0]);
 portable_portal_use(&s,nullptr,nullptr);CHECK(!G_PortalView(s,view)&&jar::portal::Valid(view));portable_portal_use(&s,nullptr,nullptr);
 auto bad=settings;bad.width=std::numeric_limits<float>::quiet_NaN();CHECK(!G_PortalSet(s,bad)&&s.radius==256);bad.width=1025;CHECK(!G_PortalSet(s,bad));
 // Serialize only native value fields; native save pointer relocation is covered separately.
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t stored{},restored{};stored.radius=s.radius;stored.random=s.random;stored.count=s.count;stored.spawnflags=s.spawnflags;stored.e_UseFunc=s.e_UseFunc;stored.s.eType=s.s.eType;stored.svFlags=s.svFlags;VectorCopy(s.currentAngles,stored.currentAngles);stored.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.radius==256&&restored.random==512&&restored.count==1&&restored.e_UseFunc==useF_portable_portal_use&&restored.s.eType==ET_PORTAL&&restored.currentAngles[ROLL]==60);
 G_PortalReset();G_PortalRestore();CHECK(G_PortalView(s,view));s.e_UseFunc=useF_NULL;bool rejected=false;try{G_PortalRestore();}catch(...){rejected=true;}CHECK(rejected);s.e_UseFunc=useF_portable_portal_use;G_PortalRestore();
 auto&cameraRoot=make(4,false);auto&surfaceChild=g_entities[nextChild-1];CHECK(G_PortalSurface(surfaceChild)&&surfaceChild.owner==&cameraRoot&&G_PortalView(surfaceChild,view));
 CHECK(std::fabs(view.axis[0][0]+1)<.001f&&view.cameraAxis[0][0]==1);
 CHECK(view.origin[0]==view.cameraOrigin[0]-128);
 G_PortalForget(&cameraRoot);cameraRoot.inuse=qfalse;CHECK(!surfaceChild.owner&&!G_PortalView(surfaceChild,view));cameraRoot.inuse=qtrue;cameraRoot.classname=G_NewString("unrelated");level.time+=1000;G_PortalFrame();CHECK(!surfaceChild.owner);
 G_PortalForget(&surfaceChild);surfaceChild.inuse=qfalse;G_PortalForget(&s);s.inuse=qfalse;G_PortalFrame();CHECK(portablePortals.empty());
 auto&failed=g_entities[8];failed.inuse=qtrue;failed.classname=G_NewString("misc_portal_surface");failed.model=G_NewString("@jar_portal_surface");exhaust=true;CHECK(!G_PortalSpawnPair(&failed)&&portablePortals.empty());
 exhaust=false;
 gi.trace=[](trace_t* hit,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int,int,EG2_Collision,int){
  *hit={};hit->fraction=.5f;CHECK(end[0]-start[0]==128);
 };
 auto&nearWall=make(9,true);CHECK(G_PortalView(nearWall,view)&&view.cameraOrigin[0]==148&&view.cameraAxis[0][0]==1);
 });}
