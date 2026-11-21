// Engine-owned display trial survives UI/renderer module reloads. GPL-2.0-only.
#include "qcommon/jar_display.h"
#include <string>
extern cvar_t *r_mode,*r_customwidth,*r_customheight,*r_fullscreen,*r_noborder;
static const char*jarDisplayNames[]=JAR_DISPLAY_NAMES;
static int jarDisplayState;
static unsigned jarDisplayStarted;
static std::string jarDisplayBackup[5],jarDisplayLatched[5];
static bool jarDisplayHadLatch[5];
static cvar_t*CL_JAR_DisplayVar(int n){cvar_t*vars[]={r_mode,r_customwidth,r_customheight,r_fullscreen,r_noborder};return vars[n];}
static void CL_JAR_DisplayPublish(int seconds){Cvar_Set("ui_jarDisplayState",va("%d",jarDisplayState));Cvar_Set("ui_jarDisplaySeconds",va("%d",seconds));}
static void CL_JAR_DisplayRead(){
 if(jarDisplayState)return;
 for(int n=0;n<5;++n){cvar_t*v=CL_JAR_DisplayVar(n);if(v)Cvar_Set(va("ui_jarMP_%s",jarDisplayNames[n]),v->latchedString?v->latchedString:v->string);}
}
static void CL_JAR_DisplayBegin(){
 if(jarDisplayState)return;
 char values[5][MAX_STRING_CHARS];const char*pending[5],*original[5],*mode;
 for(int n=0;n<5;++n){cvar_t*v=CL_JAR_DisplayVar(n);if(!v)return;Cvar_VariableStringBuffer(va("ui_jarMP_%s",jarDisplayNames[n]),values[n],sizeof(values[n]));pending[n]=values[n];original[n]=v->latchedString?v->latchedString:v->string;}
 if(!JAR_DisplayValidate(pending,original,&mode))return;
 for(int n=0;n<5;++n){cvar_t*v=CL_JAR_DisplayVar(n);jarDisplayBackup[n]=v->string;jarDisplayHadLatch[n]=v->latchedString!=nullptr;jarDisplayLatched[n]=v->latchedString?v->latchedString:"";}
 jarDisplayState=1;jarDisplayStarted=(unsigned)Sys_Milliseconds();CL_JAR_DisplayPublish(20);
}
static void CL_JAR_DisplayRevert(){
 if(jarDisplayState!=1&&jarDisplayState!=2)return;
 jarDisplayState=3;
 for(int n=0;n<5;++n)Cvar_Set2(jarDisplayNames[n],jarDisplayBackup[n].c_str(),0,qtrue);
 CL_JAR_DisplayPublish(0);Cbuf_AddText("vid_restart\n");
}
static void CL_JAR_DisplayKeep(){if(jarDisplayState==2){jarDisplayState=0;CL_JAR_DisplayPublish(0);}}
static void CL_JAR_DisplayRestarted(){
 if(jarDisplayState==1){jarDisplayState=2;jarDisplayStarted=(unsigned)Sys_Milliseconds();CL_JAR_DisplayPublish(20);Key_SetCatcher(Key_GetCatcher()|KEYCATCH_UI);}
 else if(jarDisplayState==3){
  for(int n=0;n<5;++n)if(jarDisplayHadLatch[n])Cvar_Set2(jarDisplayNames[n],jarDisplayLatched[n].c_str(),0,qfalse);
  jarDisplayState=0;CL_JAR_DisplayPublish(0);
 }
}
static void CL_JAR_DisplayFrame(){
 if(jarDisplayState!=1&&jarDisplayState!=2)return;
 unsigned elapsed=(unsigned)Sys_Milliseconds()-jarDisplayStarted;
 if(elapsed>=20000){CL_JAR_DisplayRevert();return;}
 CL_JAR_DisplayPublish((20000-elapsed+999)/1000);
 if(jarDisplayState==2)Key_SetCatcher(Key_GetCatcher()|KEYCATCH_UI);
}
static void CL_JAR_DisplayInit(){
 jarDisplayState=0;
 Cvar_Get("ui_jarDisplayState","0",CVAR_ROM|CVAR_PROTECTED,"");Cvar_Get("ui_jarDisplaySeconds","0",CVAR_ROM|CVAR_PROTECTED,"");CL_JAR_DisplayPublish(0);
 Cmd_AddCommand("jar_display_read",CL_JAR_DisplayRead);Cmd_AddCommand("jar_display_begin",CL_JAR_DisplayBegin);Cmd_AddCommand("jar_display_keep",CL_JAR_DisplayKeep);Cmd_AddCommand("jar_display_revert",CL_JAR_DisplayRevert);
}
