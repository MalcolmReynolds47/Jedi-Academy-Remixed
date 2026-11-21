// Shared production item motion compiled against both native UI structures.
#ifdef JAR_TEST_SP_UI
#include "../../code/ui/ui_local.h"
#else
#include "ui/ui_shared.h"
#endif
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#ifdef JAR_TEST_SP_UI
IGhoul2InfoArray &_TheGhoul2InfoArray(){std::abort();}
static bool sandboxOpen=true,editing=false;
static unsigned pointerRehits=0;
bool UI_JAR_OverlayActive(){return sandboxOpen;}
bool UI_JAR_OverlayTyping(){return editing;}
qboolean Display_MouseMove(void* menu,int x,int y){
 assert(!menu&&x==60&&y==70);++pointerRehits;return qtrue;
}
#include "../../code/ui/ui_jar_pointer.inc"
#endif
static displayContextDef_t context;
displayContextDef_t *DC=&context;
static int updates;
void Item_UpdatePosition(itemDef_t *item){++updates;item->window.rect=item->window.rectClient;}
#include "ui/jar_item_motion.inc"
int main(){
#ifdef JAR_TEST_SP_UI
 UI_JAR_ClickFocus(A_MOUSE1,qtrue,60,70);
 UI_JAR_ClickFocus(A_MOUSE1,qfalse,60,70);
 UI_JAR_ClickFocus(A_MOUSE1,qtrue,60,70);
 assert(pointerRehits==2);
 editing=true;UI_JAR_ClickFocus(A_MOUSE2,qtrue,60,70);
 editing=false;sandboxOpen=false;UI_JAR_ClickFocus(A_MOUSE1,qtrue,60,70);
 assert(pointerRehits==2);
#endif
 itemDef_t item{};auto &w=item.window;
 w.rectClient={0,10,20,30};w.rectEffects={60,70,40,50};w.rectEffects2={4,5,2,2};w.flags=WINDOW_INTRANSITION|WINDOW_VISIBLE|WINDOW_FADINGIN;w.nextTime=10000;DC->realTime=100;
 Item_JAR_Motion(&item,0);assert(updates==1&&!(w.flags&WINDOW_INTRANSITION)&&(w.flags&WINDOW_VISIBLE)&&(w.flags&WINDOW_FADINGIN));assert(w.rectClient.x==60&&w.rectClient.y==70&&w.rectClient.w==40&&w.rectClient.h==50);assert(w.rect.x==60&&w.nextTime==10000);
 w.flags=WINDOW_ORBITING|WINDOW_VISIBLE;w.rectClient={20,10,20,20};w.rectEffects={10,10,0,0};w.nextTime=0;w.offsetTime=25;
 const auto frozen=w.rectClient;Item_JAR_Motion(&item,0);assert(updates==1&&w.rectClient.x==frozen.x&&w.rectClient.y==frozen.y&&w.nextTime==0);
 Item_JAR_Motion(&item,1);assert(updates==2&&w.nextTime==125&&fabs(w.rectClient.x-19.4492f)<.001f&&fabs(w.rectClient.y-11.0330f)<.001f);Item_JAR_Motion(&item,1);assert(updates==2);
 DC->realTime=126;Item_JAR_Motion(&item,0);assert(updates==2&&w.nextTime==125);Item_JAR_Motion(&item,1);assert(updates==3&&w.nextTime==151);
 w.flags=WINDOW_INTRANSITION|WINDOW_FORECOLORSET|WINDOW_INTRANSITIONMODEL;w.rectClient={0,20,10,40};w.rectEffects={10,0,20,20};w.rectEffects2={7,12,6,12};w.nextTime=0;DC->realTime=200;
 Item_JAR_Motion(&item,1);assert(w.rectClient.x==7&&w.rectClient.y==8&&w.rectClient.w==16&&w.rectClient.h==28&&(w.flags&WINDOW_INTRANSITION));DC->realTime=300;Item_JAR_Motion(&item,1);assert(w.rectClient.x==10&&w.rectClient.y==0&&w.rectClient.w==20&&w.rectClient.h==20&&!(w.flags&WINDOW_INTRANSITION));assert((w.flags&WINDOW_FORECOLORSET)&&(w.flags&WINDOW_INTRANSITIONMODEL));
 w.flags=WINDOW_ORBITING|WINDOW_INTRANSITION;w.rectEffects={1,2,3,4};Item_JAR_Motion(&item,0);assert(w.rectClient.x==1&&w.rectClient.y==2&&w.rectClient.w==3&&w.rectClient.h==4&&(w.flags&WINDOW_ORBITING)&&!(w.flags&WINDOW_INTRANSITION));
 puts("UI motion: shared native orbit cadence, reduced freeze, endpoint transitions, resume and unrelated flags passed.");
}
