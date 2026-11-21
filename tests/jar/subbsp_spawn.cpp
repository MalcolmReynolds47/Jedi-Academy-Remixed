#include "subbsp_fixture.h"

bool G_SiegeKorribanRoom(const gentity_t*){return false;}
bool G_SiegeKorribanFinishMedia(gentity_t&){return false;}
int main(){return RunTests("Native sub-BSP nested parser/BSP state and full transform composition",[]{
 gi.Printf=print;gi.SetBrushModel=brush;gi.linkentity=link;gi.SetActiveSubBSP=activate;
 char previous[]="parent-";level.mTargetAdjust=previous;level.mRotationAdjust=17;VectorSet(level.mOriginAdjust,7,8,9);
 parse("{ \"classname\" \"misc_bsp\" \"bspmodel\" \"first\" \"spacing\" \"4\" \"_jar_namespace\" \"1\" }");
 char before[sizeof(spawnVarChars)];memcpy(before,spawnVarChars,sizeof(before));const int count=numSpawnVars,chars=numSpawnVarChars;char *saved=spawnVars[1][1];
 auto&root=g_entities[10];root.inuse=qtrue;root.s.number=10;root.classname="misc_bsp";G_SubBSPAllocated(&root);VectorSet(root.s.origin,100,200,300);VectorSet(root.s.angles,0,90,0);root.s.eFlags=EF_NODRAW;SP_misc_bsp(&root);
 CHECK(root.s.time2==4&&(root.s.eFlags&EF_NODRAW)&&(root.s.eFlags&EF_PERMANENT));CHECK(level.hasBspInstances&&level.mNumBSPInstances==2);CHECK(G_SubBSPNamespace(&root)!=0&&G_SubBSPNamespace(&g_entities[11])==G_SubBSPNamespace(&root));
 CHECK(switches==std::vector<int>({1,2,1,-1}));CHECK(registrations==std::vector<std::string>({"#first","#second"}));
 CHECK(level.mTargetAdjust==previous&&level.mRotationAdjust==17&&level.mBSPInstanceDepth==0&&active==-1);CHECK(level.mOriginAdjust[0]==7&&level.mOriginAdjust[1]==8&&level.mOriginAdjust[2]==9);
 CHECK(!spawning&&NPCsPrecached&&numSpawnVars==count&&numSpawnVarChars==chars&&spawnVars[1][1]==saved&&!memcmp(before,spawnVarChars,sizeof(before)));
 CHECK(records.size()==3&&records[0].depth==1&&records[1].depth==2&&records[2].depth==1&&records[1].bsp==2&&records[2].bsp==1);
 CHECK(closeEnough(records[0].origin[0],100)&&closeEnough(records[0].origin[1],210));
 CHECK(closeEnough(records[1].origin[0],96)&&closeEnough(records[1].origin[1],210)&&closeEnough(records[1].angles[1],-160));
 CHECK(closeEnough(records[2].origin[0],90)&&closeEnough(records[2].origin[1],200)&&records[2].target=="door");
 // Native target/script strings remain intact until the owned-controller namespace exists.
 CHECK(records[0].prefix=="1-"&&records[1].prefix=="2-"&&records[2].prefix=="1-");
 vec3_t offset{10,20,30},rotation{90,0,0},out;
 parse("{ \"classname\" \"misc_model\" \"origin\" \"2 3 4\" \"angles\" \"0 0 0\" \"modelAngles\" \"0 0 90\" }");G_AdjustSubBSPSpawnVars(offset,rotation);point("origin",out);CHECK(closeEnough(out[0],14)&&closeEnough(out[1],23)&&closeEnough(out[2],28));
 vec3_t expected[3],actual[3],local[3],parent[3];AnglesToAxis(rotation,parent);vec3_t modelAngles{0,0,90};AnglesToAxis(modelAngles,local);point("modelAngles",out);AnglesToAxis(out,actual);
 for(int i=0;i<3;++i){VectorClear(expected[i]);for(int j=0;j<3;++j)VectorMA(expected[i],local[i][j],parent[j],expected[i]);for(int j=0;j<3;++j)CHECK(closeEnough(actual[i][j],expected[i][j]));}
 parse("{ \"classname\" \"func_door\" \"angle\" \"-1\" }");G_AdjustSubBSPSpawnVars(offset,rotation);point("angles",out);vec3_t forward;AngleVectors(out,forward,nullptr,nullptr);CHECK(closeEnough(forward[0],1)&&closeEnough(forward[2],0));CHECK(!field("angle"));
 VectorClear(rotation);parse("{ \"classname\" \"NPC_stormtrooper\" \"angle\" \"-1\" }");G_AdjustSubBSPSpawnVars(offset,rotation);point("angles",out);CHECK(closeEnough(out[1],-1)&&closeEnough(out[0],0));
 // Bad input and capacity failure cannot publish a partially replaced record.
 parse("{ \"origin\" \"nan 0 0\" }");memcpy(before,spawnVarChars,sizeof(before));rejects([&]{G_AdjustSubBSPSpawnVars(offset,rotation);});CHECK(!memcmp(before,spawnVarChars,sizeof(before)));
 std::string many="{ ";for(int i=0;i<MAX_SPAWN_VARS;++i)many+="\"k"+std::to_string(i)+"\" \"v\" ";many+="}";parse(many.c_str());memcpy(before,spawnVarChars,sizeof(before));rejects([&]{G_AdjustSubBSPSpawnVars(offset,rotation);});CHECK(numSpawnVars==MAX_SPAWN_VARS&&!memcmp(before,spawnVarChars,sizeof(before)));
 parse("{ \"bspmodel\" \"first\" }");worldAllowed=false;const int priorCount=numSpawnVars;G_SubBSPSpawnEntitiesFromString(firstMap,offset,rotation);CHECK(!spawning&&NPCsPrecached&&numSpawnVars==priorCount&&!strcmp(field("bspmodel"),"first"));worldAllowed=true;
 rejects([&]{G_SubBSPSpawnEntitiesFromString("",offset,rotation);});CHECK(!spawning&&NPCsPrecached&&!strcmp(field("bspmodel"),"first"));
 {G_SpawnContext scope;parse("{ \"other\" \"child\" }");NPCsPrecached=qfalse;}CHECK(!spawning&&NPCsPrecached&&!strcmp(field("bspmodel"),"first"));
 parse("{ \"bspmodel\" \"\" }");SP_misc_bsp(&root);CHECK(freed==2&&active==-1&&!g_entities[11].inuse);
 level.mBSPInstanceDepth=MAX_SUB_BSP;rejects([&]{SP_misc_bsp(&root);});level.mBSPInstanceDepth=0;
 level.mNumBSPInstances=INT_MAX;const size_t registrationsBefore=registrations.size();rejects([&]{SP_misc_bsp(&root);});CHECK(registrations.size()==registrationsBefore);
 subBSPActiveModel=2;active=2;level.mBSPInstanceDepth=2;G_ResetSubBSPSpawnContext();CHECK(active==-1&&subBSPActiveModel==-1&&!level.mTargetAdjust&&!level.mBSPInstanceDepth&&level.mRotationAdjust==0);
 // Admission reaches no registration, capture or child callbacks on failure.
 gi.FS_FOpenFile=openBsp;gi.FS_Read=readBsp;gi.FS_FCloseFile=closeBsp;gi.FS_ReadFile=modelFile;gi.BSPModelAt=registry;
 gi.GetConfigstring=[](int,char*out,int){*out=0;};
 jar::bspAdmission::Report admission;
 candidate("{ classname worldspawn }\n{ classname func_door model *1 model2 models/test.md3 }\n");
 CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&fileReads==0);
 optionalSpawn=true;
 CHECK(G_AdmitPortableSubBSP("#candidate",admission)&&fileReads==1&&fileCloses==1&&admission.allocationBudget==3);
 CHECK(!strcmp(field("bspmodel"),""));
 gi.GetConfigstring=[](int key,char*out,int size){if(key>CS_MODELS&&key<CS_MODELS+MAX_MODELS)Q_strncpyz(out,"occupied.md3",size);else *out=0;};
 CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&admission.reason.find("registration capacity")!=std::string::npos);
 gi.GetConfigstring=[](int key,char*out,int size){if(key>CS_MODELS&&key<CS_MODELS+MAX_MODELS)Q_strncpyz(out,key==CS_MODELS+1?"models/test.md3":"occupied.md3",size);else *out=0;};
 CHECK(G_AdmitPortableSubBSP("#candidate",admission));gi.GetConfigstring=[](int,char*out,int){*out=0;};
 const int readBefore=fileReads;CHECK(!G_AdmitPortableSubBSP("#../bad",admission)&&fileReads==readBefore);
 shortRead=true;const int registryBefore=registryCalls;CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&registryCalls==registryBefore);shortRead=false;
 mainModels=MAX_SUBMODELS-1;CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&admission.reason.find("capacity")!=std::string::npos);mainModels=3;
 cachedResource=true;CHECK(G_AdmitPortableSubBSP("#candidate",admission));
 changedChecksum=true;CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&admission.reason.find("cached collision")!=std::string::npos);changedChecksum=false;cachedResource=false;
 missingModel=true;CHECK(!G_AdmitPortableSubBSP("#candidate",admission)&&admission.reason.find("missing")!=std::string::npos);missingModel=false;
 candidate("{ classname worldspawn }\n{ classname target_level_change mapname next }\n");
 level.mNumBSPInstances=0;root.inuse=qtrue;root.classname="misc_bsp";G_SubBSPAllocated(&root);
 parse("{ bspmodel candidate _jar_portable 1 }");const auto priorRegistrations=registrations.size();const auto priorRecords=records.size();
 SP_misc_bsp(&root);CHECK(!root.inuse&&!G_SubBSPOwner(&root)&&registrations.size()==priorRegistrations&&records.size()==priorRecords&&level.mNumBSPInstances==0&&active==-1);
 CHECK(fileReads==fileCloses&&!strcmp(field("_jar_portable"),"1"));
 // A content-qualified instance explicitly forces a separate namespace.
 candidate("{ classname worldspawn }\n");root.inuse=qtrue;G_SubBSPAllocated(&root);SP_misc_bsp(&root);
 CHECK(root.inuse&&G_SubBSPNamespace(&root)!=0&&level.mNumBSPInstances==1&&registrations.size()==priorRegistrations+1&&active==-1);
 CHECK(fileReads==fileCloses);G_FreeEntity(&root);optionalSpawn=false;
 });}
