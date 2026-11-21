// Actual shared renderer submission/selection/fallback with bounded scene fixtures.
#include "qcommon/q_shared.h"
#include "rd-common/tr_types.h"
#include "qcommon/jar_portal.h"
#include "test_support.h"
#include <limits>
#include <algorithm>
#include <vector>
enum surfaceType_t{SF_BAD,SF_POLY};
struct srfPoly_t{surfaceType_t surfaceType=SF_POLY;qhandle_t hShader=0;polyVert_t verts[4]{};const jar::portal::View*jarPortal=nullptr;qhandle_t jarFallback=0;bool jarSelected=false;qhandle_t jarMonitorShader=0;};
struct Backend{jar::portal::View jarPortals[jar::portal::frameLimit]{};unsigned numJarPortals=0;srfPoly_t polys[132]{};}storage;
Backend*backEndData=&storage;int r_numpolys=0;bool refusePoly=false;
struct{bool registered=true;}tr;
qhandle_t RE_RegisterShader(const char*s){
 if(!strcmp(s,JAR_PORTAL_PANEL_SHADER))return 1;
 for(int slot=0;slot<JAR_PORTAL_CAMERA_FEEDS;++slot)if(!strcmp(s,jar::portal::monitorMaterials[slot]))return 3+slot;
 CHECK(!strcmp(s,"gfx/colors/black"));return 2;
}
void RE_AddPolyToScene(qhandle_t shader,int count,const polyVert_t*v,int polys){CHECK(count==4&&polys==1);if(refusePoly||r_numpolys==132)return;auto&p=storage.polys[r_numpolys++];p={};p.hShader=shader;std::copy(v,v+4,p.verts);}
#include "rd-common/tr_portal_submit.inc"
enum { SS_BAD, SS_PORTAL, SS_OPAQUE };
struct shader_t{unsigned sortedIndex;int sort;const char* name;};
shader_t shaders[]={{0,SS_PORTAL,"authored"},{11,SS_PORTAL,"mirror"},{200,SS_OPAQUE,"black"},
                    {100,SS_OPAQUE,"monitor0"},{101,SS_OPAQUE,"monitor1"},
                    {102,SS_OPAQUE,"monitor2"},{103,SS_OPAQUE,"monitor3"}};
shader_t*R_GetShaderByHandle(qhandle_t h){CHECK(h>=0&&h<int(std::size(shaders)));return &shaders[h];}
struct msurface_t{shader_t*shader=nullptr;surfaceType_t*data=nullptr;};
struct trRefdef_t{int numPolys=0;srfPoly_t*polys=nullptr;};
int rend2Resets=0;
std::vector<int> rend2Slots;
void R_SetupViewParms(const trRefdef_t*){++rend2Resets;}
bool R_PortablePanelVisible(const jar::portal::View& panel){return panel.origin[0]!=-500;}
float R_PortablePanelCoverage(const jar::portal::View& panel){
 return R_PortablePanelVisible(panel)?panel.origin[2]/1000.f:0;
}
bool MonitorCanRender(const jar::portal::View& panel){
 return R_PortablePanelVisible(panel)&&panel.origin[0]!=-600;
}
qboolean R_MirrorViewBySurface(msurface_t*s,int entity,int slot=0){
 CHECK(entity==REFENTITYNUM_WORLD&&s->data);
 const auto* poly=reinterpret_cast<const srfPoly_t*>(s->data);
 if(poly->jarPortal->mirror)return qtrue;
 CHECK(s->shader==&shaders[3]&&slot>=0&&slot<JAR_PORTAL_CAMERA_FEEDS);
 rend2Slots.push_back(slot);
 return MonitorCanRender(*poly->jarPortal)?qtrue:qfalse;
}
#include "rd-common/tr_portal_views.inc"
constexpr int SHADERNUM_BITS=13,QSORT_SHADERNUM_SHIFT=23;
struct drawSurf_t{uint64_t sort=0;surfaceType_t*surface=nullptr;};
int sorts=0;void R_RadixSort(drawSurf_t*d,int n){++sorts;std::sort(d,d+n,[](const drawSurf_t&a,const drawSurf_t&b){return a.sort<b.sort;});}
void R_DecomposeSort(uint64_t sort,int*entity,shader_t**shader,int*,int*){
 *entity=int(sort&15);
 const auto index=(sort>>QSORT_SHADERNUM_SHIFT)&((1u<<SHADERNUM_BITS)-1);
 *shader=std::find_if(std::begin(shaders),std::end(shaders),[&](const auto& entry){return entry.sortedIndex==index;});
 CHECK(*shader!=std::end(shaders));
}
std::vector<const surfaceType_t*> classicVisits;
std::vector<int> classicSlots;
std::vector<const surfaceType_t*> classicVisible;
qboolean R_MirrorViewBySurface(drawSurf_t* draw,int entity,int slot=0){
 const auto* poly=*draw->surface==SF_POLY?reinterpret_cast<const srfPoly_t*>(draw->surface):nullptr;
 CHECK(entity==(poly&&poly->jarPortal?REFENTITYNUM_WORLD:1));
 classicVisits.push_back(draw->surface);
 if(poly&&poly->jarPortal&&!poly->jarPortal->mirror){
  classicSlots.push_back(slot);
  return MonitorCanRender(*poly->jarPortal)?qtrue:qfalse;
 }
 return std::find(classicVisible.begin(),classicVisible.end(),draw->surface)!=classicVisible.end()?qtrue:qfalse;
}
#include "rd-common/tr_portal_sort.inc"
int main(){return RunTests("portable portal renderer geometry and scene ownership",[]{
 jar::portal::View v;vec3_t a={25,40,60},b={-10,120,15};AnglesToAxis(a,v.axis);AnglesToAxis(b,v.cameraAxis);VectorSet(v.origin,100,200,300);VectorSet(v.cameraOrigin,900,800,700);CHECK(jar::portal::Valid(v));
 float points[4][3];jar::portal::Corners(v,points);vec4_t plane;CHECK(PlaneFromPoints(plane,points[0],points[1],points[2]));for(int j=0;j<3;++j)CHECK(std::fabs(plane[j]-v.axis[0][j])<.0001f);CHECK(std::fabs(plane[3]-DotProduct(v.origin,v.axis[0]))<.001f);
 vec3_t edge;VectorSubtract(points[1],points[0],edge);CHECK(std::fabs(VectorLength(edge)-v.height)<.001f);VectorSubtract(points[2],points[1],edge);CHECK(std::fabs(VectorLength(edge)-v.width)<.001f);
 vec3_t front,behind;VectorMA(v.origin,128,v.axis[0],front);VectorMA(v.origin,-128,v.axis[0],behind);
 // A tilted panel spanning the viewport stays eligible even if every corner
 // is individually outside it. One-sided or entirely clipped panels do not.
 auto project=[&](const float* point,float* clip){
  vec3_t offset;VectorSubtract(point,v.origin,offset);
  VectorSet4(clip,DotProduct(offset,v.axis[1])/32,DotProduct(offset,v.axis[2])/32,0,1);
 };
 CHECK(jar::portal::PanelVisible(v,front,project));
 CHECK(!jar::portal::PanelVisible(v,behind,project));
 CHECK(!jar::portal::PanelVisible(v,front,[&](const float* point,float* clip){project(point,clip);clip[0]+=8;}));
 CHECK(!jar::portal::PanelVisible(v,front,[&](const float* point,float* clip){project(point,clip);clip[2]=-2;}));
 CHECK(std::fabs(jar::portal::PanelCoverage(v,front,project)-1)<.001f);
 CHECK(jar::portal::PanelCoverage(v,behind,project)==0);
 CHECK(jar::portal::PanelCoverage(v,front,[&](const float* point,float* clip){project(point,clip);clip[0]+=8;})==0);
 CHECK(std::fabs(jar::portal::PanelCoverage(v,front,[&](const float* point,float* clip){
  project(point,clip);clip[0]+=2.5f;
 })-.25f)<.001f);
 CHECK(std::fabs(jar::portal::PanelCoverage(v,front,[&](const float* point,float* clip){
  project(point,clip);clip[2]=-1+clip[0];
 })-.5f)<.001f);
 orientation_t source{},camera{};vec3_t pvs;jar::portal::Orientations(v,source,camera,pvs);for(int i=0;i<3;++i){CHECK(source.origin[i]==v.origin[i]&&camera.origin[i]==v.cameraOrigin[i]&&pvs[i]==v.cameraOrigin[i]);for(int j=0;j<3;++j)CHECK(camera.axis[i][j]==(i<2?-v.cameraAxis[i][j]:v.cameraAxis[i][j]));}
 float fovX=0,fovY=0;jar::portal::MonitorView(v,camera,pvs,fovX,fovY);
 for(int i=0;i<3;++i){CHECK(camera.origin[i]==v.cameraOrigin[i]&&pvs[i]==v.cameraOrigin[i]);for(int j=0;j<3;++j)CHECK(camera.axis[i][j]==v.cameraAxis[i][j]);}
 CHECK(fovX>0&&fovX<fovY&&std::fabs(fovY-75)<.001f);
 v.mirror=1;jar::portal::Orientations(v,source,camera,pvs);for(int i=0;i<3;++i){CHECK(camera.origin[i]==v.origin[i]);for(int j=0;j<3;++j)CHECK(camera.axis[i][j]==(i==0?-v.axis[i][j]:v.axis[i][j]));}v.mirror=0;
 auto invalid=v;invalid.axis[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!jar::portal::Valid(invalid));RE_AddPortalToScene(&invalid);CHECK(!r_numpolys&&!storage.numJarPortals);invalid=v;VectorScale(invalid.axis[2],-1,invalid.axis[2]);CHECK(!jar::portal::Valid(invalid));invalid=v;invalid.width=15;CHECK(!jar::portal::Valid(invalid));
 RE_AddPortalToScene(&v);CHECK(r_numpolys==1&&storage.numJarPortals==1&&storage.polys[0].jarPortal!=&v&&storage.polys[0].jarFallback==2&&storage.polys[0].hShader==3);v.cameraOrigin[0]=123;CHECK(storage.polys[0].jarPortal->cameraOrigin[0]==900);for(int i=0;i<4;++i){for(int j=0;j<3;++j)CHECK(storage.polys[0].verts[i].xyz[j]==points[i][j]);CHECK(storage.polys[0].verts[i].st[0]==float(i>=2)&&storage.polys[0].verts[i].st[1]==float(i==1||i==2));}
 refusePoly=true;RE_AddPortalToScene(&v);CHECK(storage.numJarPortals==1);refusePoly=false;
 for(unsigned i=1;i<jar::portal::frameLimit;++i)RE_AddPortalToScene(&v);RE_AddPortalToScene(&v);CHECK(r_numpolys==129&&storage.numJarPortals==128&&!storage.polys[128].jarPortal&&storage.polys[128].hShader==2);
 tr.registered=false;RE_AddPortalToScene(&v);CHECK(r_numpolys==129);tr.registered=true;
 storage.numJarPortals=0;v.mirror=1;RE_AddPortalToScene(&v);CHECK(storage.polys[129].hShader==1);v.mirror=0;
 CHECK(jar::portal::MonitorImageSlot("$jarCamera")==0&&jar::portal::MonitorImageSlot("$JARCAMERA3")==3);
 CHECK(jar::portal::MonitorImageSlot("$jarCamera4")==-1&&jar::portal::MonitorImageSlot("$jarCamera12")==-1);
 enum { Unknown, Offscreen, Unavailable, Original, OtherPosition, OtherAngle,
        OtherAspect, Overflow, Shared, SharedOffscreen, SharedOtherAspect, Mirror, Count };
 srfPoly_t polys[Count]{};
 jar::portal::View panels[Count];
 for(int i=Offscreen;i<Mirror;++i){
  panels[i]=v;
  polys[i]=storage.polys[i];
  polys[i].jarPortal=&panels[i];
 }
 panels[Offscreen].origin[0]=-500;
 panels[Unavailable].origin[0]=-600;
 panels[OtherPosition].cameraOrigin[0]+=100;
 const vec3_t otherAngles={0,90,0};
 AnglesToAxis(otherAngles,panels[OtherAngle].cameraAxis);
 panels[OtherAspect].width*=2;
 panels[Overflow].cameraOrigin[1]+=100;
 panels[Shared].origin[1]+=100;
 AnglesToAxis(otherAngles,panels[Shared].axis);
 panels[Shared].width*=2;
 panels[Shared].height*=2;
 panels[SharedOffscreen].origin[0]=-500;
 panels[SharedOtherAspect]=panels[OtherAspect];
 panels[SharedOtherAspect].origin[1]-=100;
 panels[Mirror]=v;
 panels[Mirror].mirror=1;
 polys[Mirror]=storage.polys[129];
 polys[Mirror].jarPortal=&panels[Mirror];

 const qhandle_t expectedShaders[Count]={0,0,0,3,4,5,6,0,3,0,6,0};
 const std::vector<int> expectedRenders={0,0,1,2,3};
 trRefdef_t ref{Count,polys};
 CHECK(R_AddPortablePortalView(&ref));
 CHECK(rend2Resets==5);
 CHECK(rend2Slots==expectedRenders);
 for(int i=Offscreen;i<Count;++i){
  CHECK(polys[i].jarMonitorShader==expectedShaders[i]);
  CHECK(polys[i].jarSelected==(i==Mirror||expectedShaders[i]!=0));
 }
 ref.numPolys=Mirror;
 rend2Slots.clear();
 CHECK(!R_AddPortablePortalView(&ref)); // Fixed feeds leave the authored portal budget available.
 CHECK(rend2Slots==expectedRenders);

 surfaceType_t mapMirror=SF_BAD;
 const uint64_t mask=((uint64_t(1)<<SHADERNUM_BITS)-1)<<QSORT_SHADERNUM_SHIFT;
 const uint64_t other=1;
 drawSurf_t draws[Count]={{other,&mapMirror}};
 for(int i=Offscreen;i<Mirror;++i)
  draws[i]={other|(uint64_t(100)<<QSORT_SHADERNUM_SHIFT),&polys[i].surfaceType};
 draws[Mirror]={other|(uint64_t(11)<<QSORT_SHADERNUM_SHIFT),&polys[Mirror].surfaceType};
 classicVisible.push_back(&mapMirror);
 CHECK(R_SelectPortal(draws,Count)==&mapMirror&&classicSlots.empty());
 R_AddPortableMonitors(draws,Count);
 CHECK(classicSlots==expectedRenders);
 for(int i=Offscreen;i<Count;++i)CHECK(polys[i].jarMonitorShader==expectedShaders[i]);
 R_PortablePortalShaders(draws,Count,&mapMirror,true);
 for(const auto& draw:draws){
  CHECK((draw.sort&~mask)==other);
  unsigned expected=draw.surface==&mapMirror?0:200;
  for(int i=Offscreen;i<Mirror;++i)
   if(expectedShaders[i]&&draw.surface==&polys[i].surfaceType)expected=shaders[expectedShaders[i]].sortedIndex;
  CHECK(((draw.sort&mask)>>QSORT_SHADERNUM_SHIFT)==expected);
 }
 R_PortablePortalShaders(draws,Count,nullptr,false);
 for(const auto& draw:draws)CHECK(((draw.sort&mask)>>QSORT_SHADERNUM_SHIFT)==(draw.surface==&mapMirror?0:200));

 // The late large panel earns a feed. Screens sharing a view use one slot,
 // including when the largest screen is the last instance of that view.
 panels[Offscreen].origin[2]=990;
 panels[Unavailable].origin[2]=950;
 panels[Shared].origin[2]=900;
 panels[Overflow].origin[2]=700;
 const qhandle_t prioritized[Count]={0,0,0,3,5,6,0,4,3,0,0,0};
 CHECK(!R_AddPortablePortalView(&ref));
 for(int i=Offscreen;i<Mirror;++i){
  CHECK(polys[i].jarMonitorShader==prioritized[i]);
  CHECK(polys[i].jarSelected==(prioritized[i]!=0));
  draws[i]={other|(uint64_t(100)<<QSORT_SHADERNUM_SHIFT),&polys[i].surfaceType};
 }
 R_AddPortableMonitors(draws,Count);
 for(int i=Offscreen;i<Mirror;++i)
  CHECK(polys[i].jarMonitorShader==prioritized[i]);
 });}
