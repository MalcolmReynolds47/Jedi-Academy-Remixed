// Production engine key-catcher handoff joined to the actual cgame menu.
// CL_KeyEvent is the engine-service boundary: release state, queue minus
// bindings, then deliver the cgame event in the same order as CL_KeyUpEvent.
static struct {struct {qboolean down;int repeats;const char*binding;}keys[MAX_KEYS];int keyDownCount;qboolean anykeydown;}kg;
static struct {qboolean cgameStarted;int state;}cls;
static int overlayOpen,nativeKeys,queuedMenuRelease,queuedContextRelease,releasedAttack,releasedUse;
static int Cvar_VariableIntegerValue(const char*name){assert(!strcmp(name,"jar_mpMenuOpen"));return overlayOpen;}
static void CL_KeyEvent(int key,qboolean down,unsigned time);
#include "client/cl_jar_keycatcher.inc"
#define catcher keyCatchers
static void CL_KeyEvent(int key,qboolean down,unsigned time){
 assert(!down);kg.keys[key].down=qfalse;kg.keys[key].repeats=0;
 if(kg.keyDownCount>0)--kg.keyDownCount;kg.anykeydown=kg.keyDownCount?qtrue:qfalse;
 const char*binding=kg.keys[key].binding;
 if(binding){
  if(!strcmp(binding,"+jar_menu"))++queuedMenuRelease;
  if(!strcmp(binding,"+jar_context"))++queuedContextRelease;
  if(!strcmp(binding,"+attack"))++releasedAttack;
  if(!strcmp(binding,"+use"))++releasedUse;
 }
 if((Key_GetCatcher()&KEYCATCH_CGAME)&&cls.cgameStarted)CG_JAR_Key(key,qfalse);
}
static void Held(int key,const char*binding){assert(!kg.keys[key].down);kg.keys[key].binding=binding;kg.keys[key].down=qtrue;kg.keys[key].repeats=1;++kg.keyDownCount;kg.anykeydown=qtrue;}
static void PumpReleases(void){while(queuedMenuRelease){--queuedMenuRelease;CG_JAR_MenuUp();}while(queuedContextRelease){--queuedContextRelease;CG_JAR_MenuUp();}}
static void NativeKeyCases(void){
 nativeKeys=1;cls.cgameStarted=qtrue;cls.state=CA_ACTIVE;keyCatchers=0;overlayOpen=0;
 // Opening used to synthesize Q-up here, immediately closing the overlay.
 args[1]="81";Held('Q',"+jar_menu");Held('W',"+forward");Held(A_MOUSE1,"+attack");Held('E',"+use");
 CG_JAR_MenuDown();assert(menu&&overlayOpen&&Key_GetCatcher()==KEYCATCH_CGAME);
 assert(kg.keys['Q'].down&&kg.keys['W'].down&&kg.keyDownCount==2&&kg.anykeydown);
 assert(!kg.keys[A_MOUSE1].down&&!kg.keys['E'].down&&releasedAttack==1&&releasedUse==1&&!queuedMenuRelease);
 PumpReleases();CG_JAR_Draw();assert(menu);
 Key_SetCatcher(KEYCATCH_CGAME);assert(kg.keyDownCount==2); // unchanged catcher
 CL_KeyEvent('Q',qfalse,1);PumpReleases();assert(!menu&&!overlayOpen&&Key_GetCatcher()==0&&kg.keys['W'].down&&kg.keyDownCount==1);
 // The C context overlay uses the same native transition and real release.
 args[1]="67";Held('C',"+jar_context");CG_JAR_ContextDown();PumpReleases();CG_JAR_Draw();assert(menu&&context&&kg.keys['C'].down);
 CL_KeyEvent('C',qfalse,2);PumpReleases();assert(!menu&&kg.keys['W'].down);
 // Console takeover, focus loss and disconnect still clear every held key.
 args[1]="81";Held('Q',"+jar_menu");CG_JAR_MenuDown();Key_SetCatcher(KEYCATCH_CONSOLE);PumpReleases();CG_JAR_Draw();
 assert(!menu&&Key_GetCatcher()==KEYCATCH_CONSOLE&&!kg.keyDownCount&&!kg.anykeydown);
 Key_SetCatcher(0);Held('Q',"+jar_menu");Held('W',"+forward");CG_JAR_MenuDown();Key_ClearStates();PumpReleases();assert(!menu&&!kg.keyDownCount&&!kg.anykeydown);
 Held('Q',"+jar_menu");Held('W',"+forward");CG_JAR_MenuDown();cls.state=CA_DISCONNECTED;Key_SetCatcher(0);PumpReleases();assert(!menu&&!kg.keyDownCount);
 // An ordinary cgame catcher never gains the held-key exception.
 cls.state=CA_ACTIVE;Held('W',"+forward");Key_SetCatcher(KEYCATCH_CGAME);assert(!kg.keyDownCount);Key_SetCatcher(0);
 // Escape removes the catcher first; draw then retires the menu state.
 Held('Q',"+jar_menu");Held('W',"+forward");CG_JAR_MenuDown();Key_SetCatcher(0);CG_JAR_Draw();assert(!menu&&!overlayOpen&&kg.keys['W'].down);
 Key_ClearStates();PumpReleases();assert(!kg.keyDownCount&&!kg.anykeydown);nativeKeys=0;
 puts("MP native key handoff: held Q/C survive open, release closes, movement persists, attacks release, console/focus/disconnect fully clear.");
}
