// Shared-world overlays keep their held key and movement during input handoff.
// Ordinary menus, console and focus loss still clear all keys. GPL-2.0-only.
static qboolean CL_JAR_OverlayBinding(const char *binding) {
 static const char *const allowed[]={"+jar_menu","+jar_context","+forward","+back","+moveleft","+moveright","+moveup","+movedown","+speed"};
 size_t i;
 if(!binding)return qfalse;
 for(i=0;i<ARRAY_LEN(allowed);++i)if(!Q_stricmp(binding,allowed[i]))return qtrue;
 return qfalse;
}
static void CL_ClearKeyStates(qboolean overlay) {
 int i;
 for(i=0;i<MAX_KEYS;++i){
  if(overlay&&CL_JAR_OverlayBinding(kg.keys[i].binding))continue;
  if(kg.keys[i].down)CL_KeyEvent(i,qfalse,0);
  kg.keys[i].down=qfalse;kg.keys[i].repeats=0;
 }
 kg.keyDownCount=0;
 for(i=0;i<MAX_KEYS;++i)if(kg.keys[i].down)++kg.keyDownCount;
 kg.anykeydown=kg.keyDownCount?qtrue:qfalse;
}
void Key_ClearStates(void){CL_ClearKeyStates(qfalse);}
static int keyCatchers=0;
int Key_GetCatcher(void){return keyCatchers;}
void Key_SetCatcher(int catcher){
 if(catcher!=keyCatchers){
  const qboolean overlay=(qboolean)(cls.cgameStarted&&cls.state==CA_ACTIVE&&Cvar_VariableIntegerValue("jar_mpMenuOpen")&&
   ((keyCatchers==0&&catcher==KEYCATCH_CGAME)||(keyCatchers==KEYCATCH_CGAME&&catcher==0)));
  // Release attack/use and other actions, retaining only overlay and movement keys.
  CL_ClearKeyStates(overlay);
 }
 keyCatchers=catcher;
}
