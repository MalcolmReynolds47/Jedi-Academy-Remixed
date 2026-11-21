// The client executable uses _dedicated as its authoritative mode. A legacy
// UI/config "dedicated" value must not disable image loading or Ghoul2 skins.
static int CL_RendererCvarInteger(const char*name){
 if(!Q_stricmp(name,"dedicated"))return com_dedicated?com_dedicated->integer:0;
 return Cvar_VariableIntegerValue(name);
}
