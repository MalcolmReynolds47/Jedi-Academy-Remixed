// Execute the complete SP vehicle loader, with headless resource services.
#include "../../code/game/bg_vehicleLoad.cpp"
#include "test_support.h"
#include <fstream>
#include <iterator>
game_import_t gi{};
stringID_table_t animTable[MAX_ANIMATIONS+1]={{"BOTH_VS_IDLE",0},{"",-1}};
int G_ModelIndex(const char*){return 1;}int G_SoundIndex(const char*){return 1;}int G_EffectIndex(const char*){return 1;}int G_SkinIndex(const char*){return 1;}
char *G_NewString(const char *s){char*out=static_cast<char*>(malloc(strlen(s)+1));memcpy(out,s,strlen(s)+1);return out;}
void G_SetSharedVehicleFunctions(vehicleInfo_t*){}void G_SetAnimalVehicleFunctions(vehicleInfo_t*){}void G_SetSpeederVehicleFunctions(vehicleInfo_t*){}void G_SetWalkerVehicleFunctions(vehicleInfo_t*){}void G_SetFighterVehicleFunctions(vehicleInfo_t*){}
#include "vehicle_variants_cases.inc"
#include "qcommon/jar_savegame.h"
#include "chunk_save.h"
#include "game/g_vehicle_registry.inc"
int main(int argc,char**argv){return RunTests("native SP vehicle definitions, color skins, precedence and capacity",[&]{
 gi.RE_RegisterSkin=[](const char*){return qhandle_t(1);};
 COM_BeginParseSession();VehicleCases(argc,argv);COM_EndParseSession();
 // Exercise old and current save versions with all 63 definitions in a different order.
 for(int version:{jar::workshopSaveV34,jar::workshopSaveV37,jar::workshopSaveCurrent}){
 std::vector<std::string> names;std::string definitions;
 for(int i=1;i<numVehicles;++i){names.push_back(g_vehicleInfo[i].name);definitions+=names.back()+"\n{\nname "+names.back()+"\ntype VH_SPEEDER\nmodel swoop\nskin default\n}\n";}
 ChunkSave save;gi.saved_game=&save;gi.Error=Com_Error;JarWriteVehicleRegistry(version);
 CHECK(save.chunks.size()==2&&save.chunks[1].second.size()==63*MAX_TOKEN_CHARS);
 Q_strncpyz(VehicleParms,definitions.c_str(),sizeof(VehicleParms));numVehicles=1;memset(g_vehicleInfo,0,sizeof(g_vehicleInfo));
 CHECK(BG_VehicleGetIndex(names.back().c_str())==1);JarReadVehicleRegistry(version);
 CHECK(jarRestoringVehicles&&jarRestoredVehicleCount==63&&numVehicles==64&&jarRestoredVehicles[63]==1);
 for(int i=1;i<=63;++i)CHECK(names[i-1]==g_vehicleInfo[jarRestoredVehicles[i]].name);
 ChunkSave invalid;invalid.chunks=save.chunks;uint32_t overflow=64;memcpy(invalid.chunks[0].second.data(),&overflow,4);gi.saved_game=&invalid;
 bool rejected=false;try{JarReadVehicleRegistry(version);}catch(const std::runtime_error&){rejected=true;}CHECK(rejected);
 }
 });}
