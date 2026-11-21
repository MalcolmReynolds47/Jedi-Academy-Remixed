#include "qcommon/jar_workshop_state.h"
#include "test_support.h"
#include <map>
#include <cstdlib>
constexpr int CVAR_ARCHIVE=1;
struct MockCvar{std::string value;const char*string="";int integer=0;};
static std::map<std::string,MockCvar> vars;static std::string binding,contextBinding,undoBinding;
static MockCvar*Cvar_Get(const char*n,const char*d,int){auto it=vars.find(n);if(it==vars.end())it=vars.emplace(n,MockCvar{}).first;
 if(!it->second.string[0]&&it->second.value.empty())it->second.value=d;it->second.string=it->second.value.c_str();it->second.integer=atoi(it->second.string);return &it->second;}
static void Cvar_Set(const char*n,const char*v){auto&x=vars[n];x.value=v;x.string=x.value.c_str();x.integer=atoi(v);}
static std::string& bindingFor(int key){CHECK(key=='Q'||key=='C'||key=='Z');return key=='Q'?binding:key=='C'?contextBinding:undoBinding;}
static const char*Key_GetBinding(int key){return bindingFor(key).c_str();}
static void Key_SetBinding(int key,const char*b){bindingFor(key)=b;}
#include "client/cl_jar_workshop.inc"
int main(){return RunTests("production Sandbox Q lifecycle",[]{
 contextBinding="+movedown";binding="+force_grip";undoBinding="+zoom";CL_JAR_WorkshopBinding();CHECK(binding=="+force_grip");
 Cvar_Set("jar_sandbox","1");CL_JAR_WorkshopBinding();CHECK(binding=="+jar_spawnmenu");
 CHECK(contextBinding=="+jar_contextmenu"&&undoBinding=="jar_sb undo");
 CL_JAR_WorkshopBinding();Cvar_Set("jar_sandbox","0");CL_JAR_WorkshopBinding();CHECK(binding=="+force_grip"&&contextBinding=="+movedown"&&undoBinding=="+zoom");
 binding="say \"custom; quoted\"";Cvar_Set("jar_sandbox","1");CL_JAR_WorkshopBinding();
 Cvar_Set("jar_sandbox","0");CL_JAR_WorkshopBinding();CHECK(binding=="say \"custom; quoted\"");
 Cvar_Set("jar_sandbox","1");CL_JAR_WorkshopBinding();binding="+use";CL_JAR_WorkshopBinding();CHECK(binding=="+use");
 contextBinding="custom_C";undoBinding="custom_Z";Cvar_Set("jar_sandbox","0");CL_JAR_WorkshopBinding();CHECK(binding=="+use"&&contextBinding=="custom_C"&&undoBinding=="custom_Z");
 // Startup with archived ownership recovers the old binding on Off.
 Cvar_Set("jar_sandboxOwnsQ","1");Cvar_Set("jar_sandboxPreviousQ",jar::workshop::Hex("+speed").c_str());binding="jar_spawnmenu";
 CL_JAR_WorkshopBinding();CHECK(binding=="+speed");
 binding=std::string(201,'x');Cvar_Set("jar_sandbox","1");CL_JAR_WorkshopBinding();CHECK(binding==std::string(201,'x'));CHECK(vars["jar_sandboxOwnsQ"].integer==0);
 std::string decoded="kept";CHECK(!jar::workshop::Unhex("0z",decoded,10));CHECK(decoded=="kept");
 CHECK(!jar::workshop::Unhex("0",decoded,10));CHECK(!jar::workshop::Unhex("0011",decoded,1));
 });}
