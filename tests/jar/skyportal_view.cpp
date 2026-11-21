// Production server dispatch and client entity submission with deterministic PVS.
#include "qcommon/q_shared.h"
#include "qcommon/jar_skyportal.h"
#include "game/bg_public.h"
#include "rd-common/tr_types.h"
#include "../../shared/qcommon/jar_yavin_sky.h"
#include "game/g_bsp_section_rift.h"
#include "test_support.h"
#include <vector>
struct SectionFixture { int skyZ; const char* model; bool portable=true; };
struct centity_t {entityState_t currentState{};SectionFixture*gent=nullptr;qboolean currentValid=qfalse;vec3_t lerpOrigin{},lerpAngles{};};
struct Snapshot {int serverTime=0,numEntities=0;entityState_t entities[8]{};};
struct {Snapshot*snap=nullptr,*nextSnap=nullptr;int time=0;float frameInterpolation=0;vec3_t autoAngles{},autoAnglesFast{},autoAxis[3]{},autoAxisFast[3]{};bool hyperspace=false;refdef_t refdef{};} cg;
static struct {qhandle_t inlineDrawModel[8]{};} cgs;
centity_t cg_entities[8];centity_t*cg_permanents[4];int cg_numpermanents=0;
static std::vector<int> submitted;
void CG_CalcEntityLerpPositions(centity_t*c){VectorCopy(c->currentState.pos.trBase,c->lerpOrigin);VectorCopy(c->currentState.apos.trBase,c->lerpAngles);}
qboolean cgi_R_inPVS(vec3_t from,vec3_t to){CHECK(from[0]==100);return to[0]>=90&&to[0]<=110?qtrue:qfalse;}
void CG_AddCEntity(centity_t*c){submitted.push_back(c->currentState.number);}
#include "cgame/cg_skyportal.inc"
static int sectionPasses;
static refEntity_t scenery;
static refdef_t sectionView;
static bool CG_SectionSky(const centity_t& cent) { return cent.gent && cent.currentState.eType == ET_MOVER; }
static qboolean cgi_CM_SectionSky(int model, const vec3_t localEye) {
    if(model!=5&&model!=7)return qfalse;
    return Distance(localEye, vec3_origin) < 32 ? qtrue : qfalse;
}
static bool G_BSPSectionSkyOrigin(const SectionFixture& instance, vec3_t local) {
    VectorSet(local, 4352, 1024, instance.skyZ);
    return true;
}
static void cgi_R_AddRefEntityToScene(const refEntity_t* entity) { scenery = *entity; }
static void cgi_R_RenderScene(const refdef_t* view) { sectionView = *view; ++sectionPasses; }
#include "cgame/cg_section_sky.inc"

static void SectionSkyViews() {
    Snapshot snapshot;
    snapshot.numEntities = 2;
    snapshot.entities[0].number = 1;
    snapshot.entities[1].number = 2;
    cg.snap = &snapshot;
    cgs.inlineDrawModel[5] = 19;
    SectionFixture first{640,jar::hothSky::resource}, second{700,jar::hothSky::resource};
    for (int index = 1; index <= 2; ++index) {
        auto& cent = cg_entities[index];
        cent.gent = index == 1 ? &first : &second;
        cent.currentState.eType = ET_MOVER;
        cent.currentState.modelindex = 5;
        VectorSet(cent.currentState.pos.trBase, 300 * index, -200, 64);
        VectorSet(cent.currentState.apos.trBase, 0, index * 90, 0);
    }
    const vec3_t eyeAngles = {0, 90, 0};
    AnglesToAxis(eyeAngles, cg.refdef.viewaxis);
    VectorSet(cg.refdef.vieworg, 600, -200, 64);
    cg.refdef.fov_x = 97;
    CHECK(CG_DrawSectionSky() && sectionPasses == 1);
    CHECK(sectionView.vieworg[0] == 4352 && sectionView.vieworg[2] == second.skyZ);
    CHECK(sectionView.fov_x == 97 && cg.refdef.vieworg[0] == 600);
    CHECK(std::fabs(sectionView.viewaxis[0][0]) < .001f && sectionView.viewaxis[0][1] < -.999f);
    CHECK(scenery.hModel == 19 && VectorCompare(scenery.origin, vec3_origin) && scenery.axis[0][0] == 1);
    CHECK(scenery.renderfx == (RF_NOSHADOW | RF_JAR_SECTION_BACKDROP));
    CHECK(sectionView.rdflags == (RDF_NOWORLDMODEL | RDF_SKYBOXPORTAL | RDF_DRAWSKYBOX | RDF_JAR_SECTION_SKY));
    CHECK(cg.refdef.rdflags == RDF_JAR_SECTION_SKY);
    cg_entities[2].currentState.eFlags |= EF_NODRAW;
    CHECK(!CG_DrawSectionSky() && sectionPasses == 1);
    cg.refdef.vieworg[0] = 300;
    CHECK(CG_DrawSectionSky() && sectionPasses == 2 && sectionView.vieworg[2] == first.skyZ);
    CHECK(sectionView.viewaxis[0][0] > .999f);
    cg.refdef.vieworg[0] = 900;
    CHECK(!CG_DrawSectionSky() && sectionPasses == 2);
    SectionFixture yavin{-832,jar::yavinSky::resource};
    cg_entities[2].gent=&yavin;
    cg_entities[2].currentState.eFlags=0;
    cg_entities[2].currentState.modelindex=7;
    cgs.inlineDrawModel[7]=23;
    cg.refdef.vieworg[0]=600;
    CHECK(CG_DrawSectionSky()&&sectionPasses==3&&sectionView.vieworg[2]==-832&&scenery.hModel==23);
    CHECK(jar::sectionSky::SubmitSurface(3449,4,true)&&!jar::sectionSky::SubmitSurface(3449,7,true));
    CHECK(jar::sectionSky::SubmitSurface(3449,7,false)&&!jar::sectionSky::SubmitSurface(3449,3443,false));
    cg.snap = nullptr;
}
namespace jar { namespace bspSection {
bool Portable(const SectionFixture& entity){return entity.portable;}
} }
static bool fixtureRiftMap=true;
static qboolean FixtureSectionModelInfo(const char* name,int* first,int* count){
    CHECK(!strcmp(name,jar::rift::resource));*first=6;*count=3;return qtrue;
}
static qboolean cgi_CM_RiftSectionAir(int model,const vec3_t point){
    CHECK(model==6);
    const vec3_t interior={32,16,0};
    return fixtureRiftMap&&(!point||Distance(point,interior)<.01f)?qtrue:qfalse;
}
#define cgi_CM_BSPModelInfo FixtureSectionModelInfo
#include "cgame/cg_section_air.inc"
#undef cgi_CM_BSPModelInfo

static void SectionAirViews(){
    Snapshot snapshot;
    snapshot.numEntities=2;
    cg.snap=&snapshot;
    SectionFixture room{0,jar::rift::resource};
    for(int index=1;index<=2;++index){
        snapshot.entities[index-1].number=index;
        auto& cent=cg_entities[index];
        cent={};cent.gent=&room;
        cent.currentState.modelindex=6;
        cent.currentState.eType=ET_MOVER;
        cent.currentState.solid=SOLID_BMODEL;
        VectorSet(cent.currentState.pos.trBase,200*index,0,20);
        cent.currentState.apos.trBase[YAW]=index==1?0:90;
    }
    const vec3_t first={232,16,20},second={384,32,20},outside={350,25,20};
    CG_CollectRiftAirSpaces();
    CHECK(CG_RiftSectionAir(first)&&CG_RiftSectionAir(second)&&!CG_RiftSectionAir(outside));
    cg.refdef.rdflags|=RDF_SKYBOXPORTAL;
    CHECK(!CG_RiftSectionAir(second));
    cg.refdef.rdflags&=~RDF_SKYBOXPORTAL;
    cg_entities[1].currentState.eFlags=EF_NODRAW;
    CG_CollectRiftAirSpaces();
    CHECK(!CG_RiftSectionAir(first)&&CG_RiftSectionAir(second));
    fixtureRiftMap=false;CG_CollectRiftAirSpaces();CHECK(!CG_RiftSectionAir(second));
    fixtureRiftMap=true;cg.snap=nullptr;CG_CollectRiftAirSpaces();CHECK(!CG_RiftSectionAir(second));
}
struct clientSnapshot_t {int skyVisits=0;};struct snapshotEntityNumbers_t {int calls=0;};
void SV_AddEntitiesVisibleFromPoint(vec3_t origin,clientSnapshot_t*frame,snapshotEntityNumbers_t*numbers,qboolean portal){CHECK(origin[0]==100&&origin[1]==0&&origin[2]==24&&portal);++frame->skyVisits;++numbers->calls;}
#include "server/sv_skyportal.inc"
int main(){return RunTests("sky view parsing, rotation, PVS routing and client submission",[]{
 using namespace jar::skyPortal;View v;v.entity=1;v.origin[0]=100;v.origin[2]=24;v.angles[YAW]=90;
 const auto text=Encode(v);View restored;CHECK(Decode(text.c_str(),restored,ENTITYNUM_WORLD)&&restored.angles[YAW]==90&&restored.entity==1);
 for(const char*bad:{"","1 2 3 0 0 0 0 0 300","nan 0 0 0 0 0 0 0 300 jar_sky1 1 0 0 0 1","1 2 3 0 0 0 0 0 300 jar_sky1 -1 0 0 0 1","1 2 3 0 0 0 0 0 300 jar_sky1 1 0 0 0 2"})CHECK(!Decode(bad,restored,ENTITYNUM_WORLD));
 CHECK(!Decode((text+" extra").c_str(),restored,ENTITYNUM_WORLD));CHECK(!Decode(text.c_str(),restored,1));
 CHECK(BaselineValid("",ENTITYNUM_WORLD)&&BaselineValid("1 2 3 3 .1 .2 .3 0 300",ENTITYNUM_WORLD));CHECK(!BaselineValid("bad",ENTITYNUM_WORLD)&&!BaselineValid("1 2 3 9 0 0 0 0 300",ENTITYNUM_WORLD));
 vec3_t offset[3],view[3],result[3],angles={0,0,0};AnglesToAxis(v.angles,offset);AnglesToAxis(angles,view);Rotate(offset,view,result);CHECK(std::fabs(result[0][0])<.001f&&std::fabs(result[0][1]-1)<.001f);Rotate(offset,view,view);CHECK(!std::memcmp(view,result,sizeof(view)));
 clientSnapshot_t frame;snapshotEntityNumbers_t numbers;SV_AddPortableSkyEntities(text.c_str(),&frame,&numbers);CHECK(frame.skyVisits==1&&numbers.calls==1);
 v.entities=false;SV_AddPortableSkyEntities(Encode(v).c_str(),&frame,&numbers);SV_AddPortableSkyEntities("",&frame,&numbers);CHECK(numbers.calls==1);v.entities=true;
 Snapshot snap,next;snap.serverTime=100;next.serverTime=200;cg.snap=&snap;cg.nextSnap=&next;cg.time=150;snap.numEntities=4;SectionFixture live{1,nullptr};
 for(int i=0;i<6;++i){cg_entities[i].currentState.number=i;cg_entities[i].gent=&live;cg_entities[i].currentState.pos.trBase[0]=100;}
 for(int i=0;i<4;++i)snap.entities[i].number=i;cg_entities[2].currentState.pos.trBase[0]=500;cg_entities[3].gent=nullptr;
 cg_numpermanents=2;cg_permanents[0]=&cg_entities[4];cg_entities[4].currentValid=qtrue;cg_permanents[1]=&cg_entities[5];
 CG_AddPortableSkyEntities(v);CHECK((submitted==std::vector<int>{0,4}));CHECK(cg.frameInterpolation==.5f); // camera, out-of-PVS and invalid entities excluded
 submitted.clear();v.entities=false;CG_AddPortableSkyEntities(v);CHECK(submitted.empty());v.entities=true;cg.nextSnap=nullptr;CG_AddPortableSkyEntities(v);CHECK(cg.frameInterpolation==0);
 CHECK(!cg_entities[0].currentState.isPortalEnt&&!cg_entities[4].currentState.isPortalEnt);
 SectionSkyViews();
 SectionAirViews();
 });}
