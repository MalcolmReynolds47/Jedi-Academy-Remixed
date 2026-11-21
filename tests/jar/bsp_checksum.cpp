// Execute the real map loader/cache and MD4 checksum. Lump decoders and file
// services use synthetic map data; no collision world or renderer is launched.
#include "qcommon/cm_local.h"
#include "test_support.h"
#include <map>
#include <vector>
clipMap_t cmg{},SubBSP[MAX_SUB_BSP]{};int NumSubBSP=0,TotalSubModels=0;
byte*cmod_base=nullptr;void*gpvCachedMapDiskImage=nullptr;char gsCachedMapDiskImage[MAX_QPATH]{};
cvar_t *cm_noAreas=nullptr,*cm_noCurves=nullptr,*cm_playerCurveClip=nullptr;
static std::map<std::string,std::vector<byte>> disk;static std::string openFile;static int reads=0,decodes=0;
static bool lowMemory=false;static cArea_t areas[3]{};static int portals[3]{};
long FS_FOpenFileRead(const char*name,fileHandle_t*file,qboolean){openFile=name;*file=disk.count(name)?1:0;return *file?long(disk.at(name).size()):-1;}
int FS_Read(void*bytes,int count,fileHandle_t file){CHECK(file==1&&count==int(disk.at(openFile).size()));memcpy(bytes,disk.at(openFile).data(),count);++reads;return count;}
void FS_FCloseFile(fileHandle_t file){CHECK(file==1);}
void Z_TagFree(memtag_t){}
void CM_ClearLevelPatches(){}
void CM_ClearMap(){cmg={};for(auto&m:SubBSP)m={};NumSubBSP=TotalSubModels=0;}
void CM_InitBoxHull(){}
void CM_FloodAreaConnections(clipMap_t&){}
void CM_CleanLeafCache(){}
qboolean Sys_LowPhysicalMemory(){return lowMemory?qtrue:qfalse;}
void CMod_LoadShaders(lump_t*,clipMap_t&){}
void CMod_LoadLeafs(lump_t*,clipMap_t&map){int n=&map==&cmg?0:int(&map-SubBSP)+1;CHECK(n<3);map.numAreas=1;map.areas=&areas[n];map.areaPortals=&portals[n];}
void CMod_LoadLeafBrushes(lump_t*,clipMap_t&){}
void CMod_LoadLeafSurfaces(lump_t*,clipMap_t&){}
void CMod_LoadPlanes(lump_t*,clipMap_t&){}
void CMod_LoadBrushSides(lump_t*,clipMap_t&){}
void CMod_LoadBrushes(lump_t*,clipMap_t&){}
void CMod_LoadSubmodels(lump_t*l,clipMap_t&map){map.numSubModels=l->filelen/int(sizeof(dmodel_t));++decodes;}
void CMod_LoadNodes(lump_t*,clipMap_t&){}
void CMod_LoadEntityString(lump_t*,clipMap_t&,const char*){}
void CMod_LoadVisibility(lump_t*,clipMap_t&){}
void CMod_LoadPatches(lump_t*,lump_t*,clipMap_t&){}
#include "qcommon/cm_map_load.inc"
#include "qcommon/cm_subbsp_load.inc"
int CM_PointLeafnum_r(const vec3_t point,int node,clipMap_t* map){
 CHECK(node==0&&map==&SubBSP[0]);return point[0]<0?0:1;
}
#include "qcommon/cm_section_pvs.inc"
static std::vector<byte> Bytes(int models,byte salt){
 std::vector<byte> bytes(sizeof(dheader_t)+models*sizeof(dmodel_t)+8,salt);dheader_t h{};h.version=BSP_VERSION;
 h.lumps[LUMP_MODELS].fileofs=sizeof(h);h.lumps[LUMP_MODELS].filelen=models*sizeof(dmodel_t);memcpy(bytes.data(),&h,sizeof(h));return bytes;
}
static unsigned Hash(const char*name){const auto&b=disk.at(name);return LittleLong(Com_BlockChecksum(b.data(),int(b.size())));}
int main(){return RunTests("native BSP source checksums survive interleaved sub-map loads and cached reloads",[]{
 disk={{"maps/main.bsp",Bytes(3,11)},{"maps/first.bsp",Bytes(2,22)},{"maps/second.bsp",Bytes(4,33)}};
 for(bool low:{false,true}){
  lowMemory=low;if(gpvCachedMapDiskImage)Z_Free(gpvCachedMapDiskImage);gpvCachedMapDiskImage=nullptr;gsCachedMapDiskImage[0]=0;CM_ClearMap();reads=decodes=0;
  int checksum;CM_LoadMap_Actual("maps/main.bsp",qfalse,&checksum,cmg);CHECK(unsigned(checksum)==Hash("maps/main.bsp")&&cmg.checksum==unsigned(checksum));
  CHECK(CM_LoadSubBSP("maps/first.bsp",qfalse)==3);CHECK(CM_LoadSubBSP("maps/second.bsp",qfalse)==5);
  CHECK(cmg.checksum!=SubBSP[0].checksum&&cmg.checksum!=SubBSP[1].checksum&&SubBSP[0].checksum==Hash("maps/first.bsp")&&SubBSP[1].checksum==Hash("maps/second.bsp"));
  CHECK(reads==3&&decodes==3&&TotalSubModels==9);
  CM_LoadMap_Actual("maps/main.bsp",qtrue,&checksum,cmg);CHECK(unsigned(checksum)==Hash("maps/main.bsp"));
  CM_LoadMap_Actual("maps/first.bsp",qtrue,&checksum,SubBSP[0]);CHECK(unsigned(checksum)==Hash("maps/first.bsp"));
  cmg.checkcount=15;portals[0]=1;CM_LoadMap_Actual("maps/main.bsp",qfalse,&checksum,cmg);
  CHECK(unsigned(checksum)==Hash("maps/main.bsp")&&cmg.checkcount==16&&!portals[0]);
  CHECK(reads==3&&decodes==3&&TotalSubModels==9&&CM_LoadSubBSP("maps/FIRST.bsp",qfalse)==3);
  CHECK(lowMemory?!gpvCachedMapDiskImage:gpvCachedMapDiskImage!=nullptr);
 }
 cmodel_t world{{-128,-128,-128},{128,128,128},{}};
 cLeaf_t leaves[2]{};leaves[1].cluster=1;
 byte pvs[]={1,2};
 auto& map=SubBSP[0];map.numNodes=1;map.numLeafs=2;map.leafs=leaves;map.cmodels=&world;
 map.numClusters=2;map.clusterBytes=1;map.visibility=pvs;map.vised=qtrue;
 const vec3_t left={-16,8,32},right={16,8,32},outside={9000,0,0};
 CHECK(CM_SectionPVS(3,left,left)&&!CM_SectionPVS(3,left,right)&&!CM_SectionPVS(3,right,left));
 CHECK(CM_SectionPVS(3,outside,left)&&CM_SectionPVS(3,left,outside));
 pvs[0]=3;CHECK(CM_SectionPVS(3,left,right));
 leaves[1].cluster=-1;CHECK(CM_SectionPVS(3,left,right));
 if(gpvCachedMapDiskImage)Z_Free(gpvCachedMapDiskImage);gpvCachedMapDiskImage=nullptr;
 });}
