// Production engine trial with controlled clock, commands and cvar services.
#include "../../codemp/qcommon/q_shared.h"
#include <map>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <climits>
struct Variable { cvar_t native{}; std::string active,latched; bool hasLatch=false; };
static std::map<std::string,Variable> vars;
static unsigned now;static int catcher,queued,registered;
static void Sync(Variable&v){v.native.string=const_cast<char*>(v.active.c_str());v.native.latchedString=v.hasLatch?const_cast<char*>(v.latched.c_str()):nullptr;}
static cvar_t* Cvar_Set2(const char*n,const char*s,uint32_t,qboolean force){
 auto&v=vars[n];
 // Native cvar.cpp compares pending and active strings before force/latch.
 if(v.hasLatch){if(v.active==s){v.hasLatch=false;Sync(v);return &v.native;}if(v.latched==s)return &v.native;}
 else if(v.active==s)return &v.native;
 if(!force&&(v.native.flags&CVAR_LATCH)){v.latched=s;v.hasLatch=true;}else{v.active=s;v.hasLatch=false;}
 Sync(v);return &v.native;
}
static cvar_t*Cvar_Set(const char*n,const char*s){return Cvar_Set2(n,s,0,qtrue);}
static cvar_t*Cvar_Get(const char*n,const char*s,uint32_t flags,const char*){assert(!(flags&CVAR_LATCH));assert(flags==(CVAR_ROM|CVAR_PROTECTED));return Cvar_Set(n,s);}
static void Cvar_VariableStringBuffer(const char*n,char*out,int size){snprintf(out,size,"%s",vars[n].active.c_str());}
static int Sys_Milliseconds(){return (int)now;}
static void Cbuf_AddText(const char*s){assert(!strcmp(s,"vid_restart\n"));++queued;}
static int Key_GetCatcher(){return catcher;}static void Key_SetCatcher(int v){catcher=v;}
static void Cmd_AddCommand(const char*,void(*)()){++registered;}
char*QDECL va(const char*fmt,...){static char buffers[8][4096];static unsigned next;char*b=buffers[next++%8];va_list args;va_start(args,fmt);vsnprintf(b,4096,fmt,args);va_end(args);return b;}
cvar_t *r_mode,*r_customwidth,*r_customheight,*r_fullscreen,*r_noborder;
#include "../../codemp/client/cl_jar_display.inc"
static void Defaults(){
 vars.clear();now=100;queued=0;catcher=4;registered=0;
 const char*values[]={"4","1600","1024","0","0"};
 for(int n=0;n<5;++n){Cvar_Set(jarDisplayNames[n],values[n]);vars[jarDisplayNames[n]].native.flags=CVAR_ARCHIVE|CVAR_LATCH;}
 r_mode=&vars["r_mode"].native;r_customwidth=&vars["r_customwidth"].native;r_customheight=&vars["r_customheight"].native;r_fullscreen=&vars["r_fullscreen"].native;r_noborder=&vars["r_noborder"].native;
 CL_JAR_DisplayInit();assert(registered==4);CL_JAR_DisplayRead();
}
static void Restart(){for(int n=0;n<5;++n){auto&v=vars[jarDisplayNames[n]];if(v.hasLatch){v.active=v.latched;v.hasLatch=false;Sync(v);}}CL_JAR_DisplayRestarted();}
static void Trial(const char*mode){Cvar_Set("ui_jarMP_r_mode",mode);CL_JAR_DisplayBegin();assert(jarDisplayState==1);Cvar_Set2("r_mode",mode,0,qfalse);}
int main(){
 Defaults();Cvar_Set2("r_customwidth","1920",0,qfalse);CL_JAR_DisplayRead();assert(vars["ui_jarMP_r_customwidth"].active=="1920"&&r_customwidth->latchedString&&!strcmp(r_customwidth->string,"1600"));
 Trial("6");assert(!strcmp(r_mode->string,"4")&&!strcmp(r_mode->latchedString,"6")&&jarDisplayBackup[1]=="1600"&&jarDisplayLatched[1]=="1920");
 CL_JAR_DisplayBegin();CL_JAR_DisplayKeep();assert(jarDisplayState==1);now=17000;Restart();assert(jarDisplayState==2&&!strcmp(r_mode->string,"6"));
 now=36999;catcher=0;CL_JAR_DisplayFrame();assert(jarDisplayState==2&&vars["ui_jarDisplaySeconds"].active=="1"&&(catcher&KEYCATCH_UI));
 now=37000;CL_JAR_DisplayFrame();assert(jarDisplayState==3&&queued==1&&!strcmp(r_mode->string,"4")&&!strcmp(r_customwidth->string,"1600"));CL_JAR_DisplayFrame();CL_JAR_DisplayRevert();assert(queued==1);
 Restart();assert(jarDisplayState==0&&!strcmp(r_customwidth->string,"1600")&&!strcmp(r_customwidth->latchedString,"1920"));
 // Keep and an unrelated restart must not extend or reactivate a trial.
 Defaults();Trial("9");Restart();now+=1234;CL_JAR_DisplayRestarted();assert(jarDisplayStarted==100);CL_JAR_DisplayKeep();now+=30000;CL_JAR_DisplayFrame();assert(!jarDisplayState&&!queued&&!strcmp(r_mode->string,"9"));
 Defaults();Trial("6");CL_JAR_DisplayRevert();assert(jarDisplayState==3&&queued==1&&!strcmp(r_mode->string,"4")&&!r_mode->latchedString);Restart();assert(!jarDisplayState);
 Defaults();Trial("6");now+=20000;CL_JAR_DisplayFrame();assert(jarDisplayState==3&&queued==1);Restart();assert(!jarDisplayState&&!strcmp(r_mode->string,"4"));
 Defaults();now=UINT_MAX-5000;Trial("6");Restart();now+=19999;CL_JAR_DisplayFrame();assert(jarDisplayState==2);++now;CL_JAR_DisplayFrame();assert(jarDisplayState==3&&queued==1);
 Defaults();r_mode=nullptr;CL_JAR_DisplayBegin();assert(!jarDisplayState);r_mode=&vars["r_mode"].native;
 for(const char*s:{"0","319","16385","9999999999999999999999","1e3","800;quit",""}){Cvar_Set("ui_jarMP_r_customwidth",s);CL_JAR_DisplayBegin();assert(!jarDisplayState&&!queued&&!strcmp(r_customwidth->string,"1600"));}
 CL_JAR_DisplayRead();Cvar_Set("ui_jarMP_r_customwidth","16384");Cvar_Set("ui_jarMP_r_customheight","16384");CL_JAR_DisplayBegin();assert(!jarDisplayState);
 // Shared policy limits and normalization used by both UI and engine.
 const char*old[]={"4","1600","1024","0","0"},*pending[]={"4","320","200","0","0"},*mode;
 assert(JAR_DisplayValidate(pending,old,&mode)&&!strcmp(mode,"-1"));pending[1]="8192";pending[2]="8192";assert(JAR_DisplayValidate(pending,old,&mode));pending[1]="8193";assert(!JAR_DisplayValidate(pending,old,&mode));
 pending[3]="2";assert(JAR_DisplayValidate(pending,old,&mode)&&!strcmp(mode,"-2"));pending[4]="2";assert(!JAR_DisplayValidate(pending,old,&mode));
 puts("MP display trial: active/latch preservation, fresh post-restart deadline, Keep/manual/timeout restore, armed timeout, clock wrap, input capture, validation and one rollback passed.");
}
