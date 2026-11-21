// Q ownership survives config writes and never overwrites a later user edit.
// Included in cl_main.cpp after client headers. GPL-2.0-only.
static void CL_JAR_SandboxBinding(bool enabled,int key,const char* command,const char* owns,const char* previousKey){
 auto*owned=Cvar_Get(owns,"0",CVAR_ARCHIVE);
 auto*backup=Cvar_Get(previousKey,"",CVAR_ARCHIVE);
 const char*current=Key_GetBinding(key);if(!current)current="";
 if(enabled&&!owned->integer){
  if(std::strlen(current)>200)return;
  Cvar_Set(previousKey,jar::workshop::Hex(current).c_str());Cvar_Set(owns,"1");Key_SetBinding(key,command);
 }else if(!enabled&&owned->integer){
  std::string previous;const bool decoded=jar::workshop::Unhex(backup->string,previous,200)&&previous.find('\0')==std::string::npos;
  if(decoded&&!std::strcmp(current,command))Key_SetBinding(key,previous.c_str());
  Cvar_Set(owns,"0");Cvar_Set(previousKey,"");
 }
}
static void CL_JAR_WorkshopBinding(){
 const bool enabled=Cvar_Get("jar_sandbox","0",CVAR_ARCHIVE)->integer==1;
 CL_JAR_SandboxBinding(enabled,'C',"+jar_contextmenu","jar_sandboxOwnsC","jar_sandboxPreviousC");
 CL_JAR_SandboxBinding(enabled,'Z',"jar_sb undo","jar_sandboxOwnsZ","jar_sandboxPreviousZ");
 auto*owned=Cvar_Get("jar_sandboxOwnsQ","0",CVAR_ARCHIVE);
 auto*backup=Cvar_Get("jar_sandboxPreviousQ","",CVAR_ARCHIVE);
 const char*current=Key_GetBinding('Q');if(!current)current="";
 if(enabled){
  if(owned->integer && !std::strcmp(current,"jar_spawnmenu")){Key_SetBinding('Q',"+jar_spawnmenu");current=Key_GetBinding('Q');}
  if(!owned->integer){
   if(std::strlen(current)>200){Cvar_Set("jar_sandboxQStatus","Q unchanged: binding exceeds backup limit. Open Sandbox through JA:R Settings.");return;}
   Cvar_Set("jar_sandboxPreviousQ",jar::workshop::Hex(current).c_str());Cvar_Set("jar_sandboxOwnsQ","1");Key_SetBinding('Q',"+jar_spawnmenu");
   Cvar_Set("jar_sandboxQStatus","Hold Q opens Sandbox without pausing. The prior binding will be restored when Sandbox is off.");
  }else Cvar_Set("jar_sandboxQStatus",jar::workshop::RestoreQ(true,current)?"Hold Q opens Sandbox without pausing. Off restores the prior binding.":"Q was changed by you; your current binding is preserved. Use JA:R Settings to open Sandbox.");
 }else if(owned->integer){
  std::string previous;const bool decoded=jar::workshop::Unhex(backup->string,previous,200)&&previous.find('\0')==std::string::npos;
  if(decoded && jar::workshop::RestoreQ(true,current))Key_SetBinding('Q',previous.c_str());
  Cvar_Set("jar_sandboxQStatus",decoded?"Sandbox off. Prior Q restored when still owned; user edits kept.":"Sandbox off. Q backup is invalid; review the Q binding in Controls.");
  Cvar_Set("jar_sandboxOwnsQ","0");Cvar_Set("jar_sandboxPreviousQ","");
 }else Cvar_Set("jar_sandboxQStatus","Sandbox off. Q binding is unchanged.");
}
