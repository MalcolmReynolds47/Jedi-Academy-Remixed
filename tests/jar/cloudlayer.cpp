// Native cloud setup/use/save and actual cgame submission, without a renderer.
#include "cgame/cg_headers.h"
#include "game/g_functions.h"
#include "memory_save.h"
#include "rd-common/jar_cloud_geometry.h"
#include <vector>
#include <string>
#include <limits>
game_import_t gi{};game_export_t globals{};level_locals_t level{};gentity_t g_entities[MAX_GENTITIES];
IGhoul2InfoArray&_TheGhoul2InfoArray(){throw std::runtime_error("Unexpected Ghoul2 allocation");}
static std::string effect,shader;static int markers=0;static std::vector<refEntity_t> submitted;
int G_EffectIndex(const char*s){effect=s;return 1;}
int G_ModelIndex(const char*s){CHECK(!strcmp(s,"models/map_objects/imperial/crate1.md3"));return 7;}
qboolean G_SpawnFloat(const char*,const char*d,float*out){*out=float(atof(d));return qfalse;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);}
void G_SetAngles(gentity_t*e,const vec3_t a){VectorCopy(a,e->currentAngles);VectorCopy(a,e->s.apos.trBase);}
#include "game/g_cloud.inc"
static void CG_General(centity_t*){++markers;}
qhandle_t cgi_R_RegisterShader(const char*s){shader=s;return 10;}
void cgi_R_AddRefEntityToScene(const refEntity_t*e){submitted.push_back(*e);}
#include "cgame/cg_cloud.inc"
int main(){return RunTests("cloud setup, properties, cgame submission, stock geometry and saves",[]{
 static_assert(useF_space_use==79&&useF_bidirectional_use==80&&useF_marker_use==81&&useF_shipboundary_use==82&&useF_cloudlayer_use==83,"Saved use callbacks append only");
 gi.linkentity=[](gentity_t*e){e->linked=qtrue;};gi.Error=[](int,const char*,...){throw std::runtime_error("Invalid saved cloud");};globals.num_entities=20;
 auto&e=g_entities[10];e.inuse=qtrue;e.s.number=10;e.classname=const_cast<char*>("fx_cloudlayer");e.model=const_cast<char*>("@jar_cloud");VectorSet(e.s.origin,10,20,30);SP_fx_cloudlayer(&e);
 CHECK(effect.empty()&&e.radius==2048&&e.random==128&&e.wait==0&&e.s.eType==ET_CLOUD&&e.s.modelindex==7&&e.e_UseFunc==useF_cloudlayer_use&&G_CloudHasLayers()&&e.linked&&e.maxs[2]==22&&(e.svFlags&SVF_BROADCAST));
 jar::cloud::Settings s;CHECK(G_CloudRead(e,s));s.radius=1024;s.inner=256;s.curve=2;s.tube=s.alternate=true;CHECK(G_CloudSet(e,s));
 centity_t cent{};cent.gent=&e;VectorCopy(e.currentOrigin,cent.lerpOrigin);VectorSet(cent.lerpAngles,90,0,0);CG_Clouds(&cent);
 CHECK(markers==1&&submitted.size()==1&&shader=="jar/world/cloud_alt");auto entity=submitted.back();CHECK(entity.reType==RT_CLOUDS&&entity.radius==1024&&entity.rotation==256&&entity.backlerp==2&&(entity.renderfx&RF_GROW)&&std::fabs(entity.axis[0][2]+1)<.001f);
 cloudlayer_use(&e,nullptr,nullptr);CG_Clouds(&cent);CHECK(markers==2&&submitted.size()==1);cloudlayer_use(&e,nullptr,nullptr);e.svFlags|=SVF_INACTIVE;cloudlayer_use(&e,nullptr,nullptr);CG_Clouds(&cent);CHECK(!(e.spawnflags&4)&&submitted.size()==1);e.svFlags&=~SVF_INACTIVE;
 e.model=nullptr;CG_Clouds(&cent);CHECK(markers==3&&submitted.size()==2&&shader=="gfx/world/haze2");e.model=const_cast<char*>("@jar_cloud");
 s.tube=s.alternate=false;CHECK(G_CloudSet(e,s));CG_Clouds(&cent);CHECK(shader=="jar/world/cloud"&&submitted.back().rotation==0&&!(submitted.back().renderfx&RF_GROW));
 for(float radius:{0.f,15.f,32769.f,std::numeric_limits<float>::quiet_NaN()}){auto bad=s;bad.radius=radius;CHECK(!G_CloudSet(e,bad)&&e.radius==1024);}
 auto bad=s;bad.inner=1017;CHECK(!G_CloudSet(e,bad));bad=s;bad.curve=21;CHECK(!G_CloudSet(e,bad));s.radius=16;s.inner=8;s.curve=-20;CHECK(G_CloudSet(e,s));s.radius=32768;s.inner=32760;s.curve=20;s.enabled=false;CHECK(G_CloudSet(e,s));
 MemorySave save;ojk::SavedGameHelper h(&save);gentity_t persisted{},restored{};persisted.radius=e.radius;persisted.random=e.random;persisted.wait=e.wait;persisted.spawnflags=e.spawnflags;persisted.s.eType=e.s.eType;persisted.e_UseFunc=e.e_UseFunc;persisted.sg_export(h);save.reset_buffer_offset();restored.sg_import(h);h.ensure_all_data_read();CHECK(restored.radius==32768&&restored.random==32760&&restored.wait==20&&restored.spawnflags==4&&restored.e_UseFunc==useF_cloudlayer_use&&restored.s.eType==ET_CLOUD);
 e.e_UseFunc=useF_NULL;G_CloudRestore();CHECK(e.e_UseFunc==useF_cloudlayer_use);e.inuse=qfalse;CHECK(!G_CloudHasLayers());e.inuse=qtrue;e.classname=const_cast<char*>("target_position");CHECK(!G_CloudRead(e,s)&&!G_CloudSet(e,s));
 // Renderers share the exact bounded mesh and preserve their input across views.
 float origin[3]={10,20,30},axis[3][3]={{1,0,0},{0,1,0},{0,0,1}};std::vector<jar::cloud::Vertex> vertices;
 auto collect=[&](const jar::cloud::Vertex*v){vertices.insert(vertices.end(),v,v+4);};
 CHECK(jar::cloud::Geometry(512,128,2,false,origin,axis,collect)&&vertices.size()==144);CHECK(vertices[0].point[0]==10&&vertices[0].point[2]==30&&vertices[0].shade==1);CHECK(std::fabs(vertices[143].point[2]-50.48f)<.001f&&vertices[143].shade==0);
 vertices.clear();CHECK(jar::cloud::Geometry(512,128,2,true,origin,axis,collect)&&vertices.size()==240);const auto first=vertices;CHECK(vertices[0].point[0]==138&&vertices[0].shade==0);vertices.clear();CHECK(jar::cloud::Geometry(512,128,2,true,origin,axis,collect));CHECK(!memcmp(first.data(),vertices.data(),first.size()*sizeof(first[0])));
 vec3_t angle={90,0,0};AnglesToAxis(angle,axis);vertices.clear();CHECK(jar::cloud::Geometry(512,0,0,false,origin,axis,collect));for(const auto&v:vertices)CHECK(std::fabs(v.point[0]-10)<.001f);
 vertices.clear();CHECK(!jar::cloud::Geometry(512,512,0,true,origin,axis,collect)&&vertices.empty());CHECK(!jar::cloud::Geometry(512,0,std::numeric_limits<float>::infinity(),false,origin,axis,collect)&&vertices.empty());
 });}
