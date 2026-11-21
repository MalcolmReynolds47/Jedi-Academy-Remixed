// Production input and layout with captured draw/engine imports; no window.
#include "cgame/cg_jar_sandbox.c"
#include <assert.h>
#include <stdarg.h>
cg_t cg;cgs_t cgs;centity_t cg_entities[MAX_GENTITIES];static cgameImport_t imports;cgameImport_t*trap=&imports;
static snapshot_t snapshot;static int requests,keyHeld=1,previous,next;static char request[192];static const char*args[22]={"","81","","","99","1","0","1023","0","0","80","0","0","0","0","0","0","0","0","","",""};
static int leftList,rightProperties,help;static char hoverText[128];
static int undoTitles;
static char undoText[128];
const char*CG_Argv(int n){return args[n];}
static void Send(const char*s){++requests;Q_strncpyz(request,s,sizeof(request));}
#include "mp_native_keys.inc"
static int Catcher(void){return Key_GetCatcher();}static void Catch(int n){Key_SetCatcher(n);}
static qboolean Down(int n){return nativeKeys?kg.keys[n].down:(keyHeld?qtrue:qfalse);}
static void Set(const char*n,const char*v){if(!strcmp(n,"jar_mpMenuOpen"))overlayOpen=atoi(v);}static void Register(vmCvar_t*c,const char*n,const char*v,uint32_t f){}
static void Cvar(const char*n,char*out,int size){Q_strncpyz(out,"1",size);}
static char lastShader[96];static qhandle_t Shader(const char*n){Q_strncpyz(lastShader,n,sizeof(lastShader));return 1;}
void CG_Trace(trace_t*t,const vec3_t a,const vec3_t lo,const vec3_t hi,const vec3_t b,int pass,int mask){memset(t,0,sizeof(*t));t->fraction=1;VectorCopy(b,t->endpos);}
void CG_JAR_RopeTrace(trace_t*t,const vec3_t a,const vec3_t lo,const vec3_t hi,const vec3_t b,int first,int second){CG_Trace(t,a,lo,hi,b,first,MASK_SOLID|CONTENTS_BODY);}
qboolean CG_CalcMuzzlePoint(int n,vec3_t point){VectorSet(point,11,22,33);return qtrue;}
void CG_CalcEntityLerpPositions(centity_t *target){assert(target==&cg_entities[41]);}
static float beamWidth,beamColor[3],beamEnd[3];static int beamLifetime,beamFlags;
static void Line(vec3_t start,vec3_t end,float a,float b,float c,float d,float e,float f,vec3_t color,vec3_t color2,float g,int life,qhandle_t shader,int flags){beamWidth=b;VectorCopy(color,beamColor);VectorCopy(end,beamEnd);beamLifetime=life;beamFlags=flags;assert(start[0]==11);}

static int Font(const char*name){assert(!strcmp(name,"arialnb"));return 7;}
static void FontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){assert(font==(7|STYLE_DROPSHADOW));CG_Text_Paint((float)x,(float)y,scale,(float*)c,s,0,0,0,0);}
static int registeredWeapon;void CG_RegisterWeapon(int weapon){registeredWeapon=weapon;}
void CG_PrevWeapon_f(void){++previous;}void CG_NextWeapon_f(void){++next;}
void CG_FillRect(float x,float y,float w,float h,const float*c){}
void CG_DrawPic(float x,float y,float w,float h,qhandle_t s){}
void CG_Text_Paint(float x,float y,float scale,vec4_t c,const char*s,float a,int l,int style,int font){
 if(x==278&&y>=108&&y<=360&&scale==.53f)++leftList;
 if(x==390&&(strstr(s,"Thrust (N):")||strstr(s,"Thrust:")))++rightProperties;
 if(x==16&&y==12)++help;
 if(x==16&&y==12)Q_strncpyz(hoverText,s,sizeof(hoverText));
 if(x==16&&y==28&&!strcmp(s,"UNDO"))++undoTitles;
 if(x==16&&y==48)Q_strncpyz(undoText,s,sizeof(undoText));
}
char*QDECL va(const char*fmt,...){static char b[4096];va_list a;va_start(a,fmt);vsnprintf(b,sizeof(b),fmt,a);va_end(a);return b;}
int QDECL Com_sprintf(char*b,int size,const char*fmt,...){va_list a;va_start(a,fmt);int n=vsnprintf(b,size,fmt,a);va_end(a);return n;}
static void Error(int n,const char*s,...){abort();}static void Print(const char*s,...){}
void(*Com_Error)(int,const char*,...)=Error;void(*Com_Printf)(const char*,...)=Print;
static void Click(int x,int y){CG_JAR_Mouse(x-(int)cursorX,y-(int)cursorY);assert(CG_JAR_Key(A_MOUSE1,qtrue));}
#include "mp_context_menu.inc"
#include "mp_context_panel.inc"
#include "mp_transport_menu.inc"
#include "mp_media_menu.inc"
#include "mp_laser_client.inc"
#include "mp_controller_switch_menu.inc"
#include "mp_logic_menu.inc"
#include "mp_action_menu.inc"
#include "mp_trigger_menu.inc"
#include "mp_trigger_transport_menu.inc"
#include "mp_station_menu.inc"
#include "mp_usable_menu.inc"
#include "mp_breakable_menu.inc"
#include "mp_small_cargo_menu.inc"
#include "mp_gun_rack_menu.inc"
#include "mp_shooter_menu.inc"
#include "mp_animated_menu.inc"
#include "mp_door_camera_menu.inc"
#include "mp_welder_menu.inc"
#include "mp_crystal_crate_menu.inc"
#include "mp_search_menu.inc"
#include "mp_feedback_menu.inc"
static const char*propName="models/props/test.glm";
const char*CG_ConfigString(int index){assert(index>CS_MODELS&&index<CS_MODELS+MAX_MODELS);return propName;}
static int propPool[8],propInits,propCleans,propPoses,propRenders;static refEntity_t propRendered;
static int markerRenders;
static float markerRadius;
static void MarkerRender(const refEntity_t*entity){
 ++markerRenders;
 assert(entity->reType==RT_SPRITE&&entity->customShader==123&&entity->radius==markerRadius&&entity->origin[2]==118&&entity->renderfx==0);
 assert(entity->shaderRGBA[0]==153&&entity->shaderRGBA[1]==229&&entity->shaderRGBA[2]==255&&entity->shaderRGBA[3]==255);
}
static void LogicMarkerCases(void){
 centity_t marker={0};marker.currentValid=qtrue;marker.currentState.eType=ET_GENERAL;marker.currentState.generic1=250;
 marker.currentState.bolt2=JAR_MP_LOGIC_MARKER_TAG;VectorSet(marker.lerpOrigin,24,60,100);
 cgs.media.jarLogicMarkerShader=123;
 void (*saved)(const refEntity_t*)=imports.R_AddRefEntityToScene;imports.R_AddRefEntityToScene=MarkerRender;
 markerRadius=11;
 assert(CG_JAR_Entity(&marker)&&markerRenders==1);
 cg.refdef.viewaxis[0][0]=1;
 cg.refdef.vieworg[0]=-966;
 markerRadius=30;
 assert(CG_JAR_Entity(&marker)&&markerRenders==2);
 cg.refdef.viewaxis[0][0]=0;
 cg.refdef.vieworg[0]=0;
 marker.currentState.eFlags=EF_NODRAW;assert(CG_JAR_Entity(&marker)&&markerRenders==2);
 marker.currentState.eFlags=0;marker.currentState.bolt2=0;assert(!CG_JAR_Entity(&marker)&&markerRenders==2);
 imports.R_AddRefEntityToScene=saved;cgs.media.jarLogicMarkerShader=0;
}
static int PropInit(void**instance,const char*path,int index,qhandle_t skin,qhandle_t shader,int flags,int lod){assert(!strcmp(path,propName));*instance=&propPool[propInits++%8];return 0;}
static void PropClean(void**instance){assert(*instance);++propCleans;*instance=NULL;}
static qboolean PropSkinless(void*g,int n){return qtrue;}
static qhandle_t PropSkin(const char*name){assert(strstr(name,"model_default.skin"));return 2;}
static qboolean PropSetSkin(void*g,int n,qhandle_t a,qhandle_t b){assert(a==2&&b==2);return qtrue;}
static qboolean PropPose(void*g,int frame,int*frames,vec3_t low,vec3_t high){++propPoses;*frames=4;if(frame>3)return qfalse;*(int*)g=frame;VectorSet(low,-1,-2,-3);VectorSet(high,20+frame,30+frame,10);return qtrue;}
static void PropRender(const refEntity_t*ent){++propRenders;propRendered=*ent;assert(ent->ghoul2&&*(int*)ent->ghoul2==ent->frame&&ent->frame==ent->oldframe&&ent->backlerp==0);}
static void PropClientCases(void){
 imports.G2API_InitGhoul2Model=PropInit;imports.G2API_CleanGhoul2Models=PropClean;imports.G2API_SkinlessModel=PropSkinless;imports.R_RegisterSkin=PropSkin;imports.G2API_SetSkin=PropSetSkin;imports.JAR_GhoulPoseBounds=PropPose;imports.R_AddRefEntityToScene=PropRender;
 centity_t*a=&cg_entities[70],*b=&cg_entities[71];a->currentValid=b->currentValid=qtrue;
 a->currentState.eType=ET_GENERAL;a->currentState.number=70;a->currentState.modelindex=1;a->currentState.modelGhoul2=1;a->currentState.generic1=250;a->currentState.time2=70;a->currentState.frame=1;
 b->currentState=a->currentState;b->currentState.number=71;b->currentState.time2=71;b->currentState.frame=2;VectorSet(a->lerpOrigin,100,200,300);VectorSet(a->lerpAngles,15,30,45);
 assert(CG_JAR_Entity(a)&&propRenders==1&&propInits==1&&propPoses==1&&VectorCompare(propRendered.origin,a->lerpOrigin)&&VectorCompare(propRendered.angles,a->lerpAngles)&&propRendered.radius>30);
 assert(CG_JAR_Entity(b)&&propRenders==2&&propInits==2&&propPoses==2&&propViews[70].ghoul!=propViews[71].ghoul);
 for(int i=0;i<10;++i){cg.time+=25;CG_JAR_PropFrame();CG_JAR_Entity(a);CG_JAR_Entity(b);}assert(propPoses==2&&propInits==2);
 a->currentState.frame=3;CG_JAR_Entity(a);assert(propPoses==3&&*(int*)propViews[71].ghoul==2);
 a->currentState.frame=4;int rendered=propRenders;CG_JAR_Entity(a);CG_JAR_Entity(a);assert(propRenders==rendered&&propPoses==4); // invalid pose never shows stale geometry
 a->currentState.frame=1;a->currentState.time2+=1024;CG_JAR_Entity(a);assert(propCleans==1&&propInits==3&&propPoses==5);
 propName="models/props/replacement.glm";CG_JAR_Entity(a);assert(propCleans==2&&propInits==4&&propPoses==6);
 b->currentValid=qfalse;CG_JAR_PropFrame();assert(!propViews[71].ghoul&&propCleans==3);
 a->currentState.eType=ET_NPC;assert(!CG_JAR_Entity(a));CG_JAR_PropFrame();assert(!propViews[70].ghoul&&propCleans==4);
 a->currentState.eType=ET_GENERAL;CG_JAR_Entity(a);CG_JAR_Shutdown();assert(propCleans==propInits);
 puts("MP skeletal rendering: first-frame pose, independent instances, stable cached poses, invalid-frame refusal, slot/model reuse, PVS and shutdown cleanup passed.");
}
static void MusicMenuCases(void){
 static const char pair[]="music/artus_topside/impbased_action.mp3 music/bespin_streets/bespina_action.mp3";
 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 char hex[127];JAR_ControllerHex(jarMusicDefault,hex);
 args[1]="99";args[2]="34";for(int i=3;i<=9;++i)args[i]="0";args[10]=hex;args[11]="0";args[12]="1";CG_JAR_Controller();
 assert(contextControllerKind==JAR_MUSIC_CONTROLLER&&contextController.active);
 contextPage=1;CG_JAR_Draw();Click(450,154);
 assert(contextEditing==200&&!strcmp(contextInput,"music/hoth2/hoth2_explore.mp3"));
 contextInput[0]=0;ContextType(pair);CG_JAR_Key(A_ENTER,qtrue);
 char compact[64],expected[192];assert(JAR_MusicPack(pair,compact));
 JAR_ControllerHex(compact,hex);Com_sprintf(expected,sizeof(expected),"jar_property controller_text_%s 99",hex);
 assert(!strcmp(request,expected)&&!contextEditing);
 Click(550,232);assert(!strcmp(request,"jar_property controller_asset_next 99"));
 Click(450,266);assert(!strcmp(request,"jar_property activate 99"));
 CG_JAR_MenuUp();
}
static void LightMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="33";for(int i=3;i<=9;++i)args[i]="0";args[10]="-";args[11]="160";args[12]="1";CG_JAR_Controller();assert(contextControllerKind==33&&contextController.speed==160&&contextController.active==1);
 args[11]="161";CG_JAR_Controller();assert(contextController.speed==160);
 contextPage=1;CG_JAR_Draw();Click(450,154);assert(contextEditing==212);
 Q_strncpyz(contextInput,"512",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_radius_512 99"));
 Click(450,182);assert(!strcmp(request,"jar_property controller_playing 99"));
 Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);
}
static int ropePolys;
static void RopePoly(qhandle_t shader,int count,const polyVert_t*verts,int polys){assert(count==4&&polys==1);++ropePolys;for(int i=0;i<4;++i)for(int a=0;a<3;++a)assert(isfinite(verts[i].xyz[a]));}
static void RopeClientCases(void){
 imports.R_AddPolysToScene=RopePoly;centity_t c={0};c.currentState.generic1=251;c.currentState.jarRopeState=4;c.currentState.jarRopeLength=120;
 c.currentState.otherEntityNum=20;c.currentState.otherEntityNum2=ENTITYNUM_WORLD;
 VectorSet(c.currentState.pos.trBase,0,0,100);VectorSet(c.currentState.origin2,100,0,100);VectorSet(c.currentState.jarRopeBends[0],40,20,100);VectorSet(c.currentState.jarRopeBends[1],60,20,100);
 ropePolys=0;assert(CG_JAR_Entity(&c)&&ropePolys==JAR_ROPE_VERTICES/4);
 for(int invalid=0;invalid<4;++invalid){ropePolys=0;c.currentState.jarRopeState=invalid==0?1:invalid==1?11:4;c.currentState.jarRopeLength=invalid==2?-1:120;if(invalid==3)c.currentState.jarRopeBends[0][1]=NAN;assert(CG_JAR_Entity(&c)&&!ropePolys);}
 c.currentState.jarRopeState=2;c.currentState.jarRopeLength=100;memset(c.currentState.jarRopeBends,0,sizeof(c.currentState.jarRopeBends));ropePolys=0;assert(CG_JAR_Entity(&c)&&ropePolys==96);
 puts("MP rope client: server-chosen bends, full mesh, unresolved and malformed suppression, and unwrapping passed.");
}
int main(void){
 imports.SendClientCommand=imports.SendConsoleCommand=Send;imports.Key_GetCatcher=Catcher;imports.Key_SetCatcher=Catch;imports.Key_IsDown=Down;imports.Cvar_Set=Set;imports.Cvar_Register=Register;imports.Cvar_VariableStringBuffer=Cvar;imports.R_RegisterShaderNoMip=Shader;
 imports.R_RegisterFont=Font;imports.R_Font_DrawString=FontDraw;imports.FX_AddLine=Line;imports.R_RegisterShader=Shader;
 cg.snap=&snapshot;centity_t beam={0};beam.currentState.generic1=252;CG_JAR_Entity(&beam);assert(beamWidth==1.4f&&beamLifetime==1&&!strcmp(lastShader,"gfx/effects/blueLine")&&beamColor[0]==.35f);
 beam.interpolate=qtrue;beam.nextState=beam.currentState;VectorSet(beam.nextState.origin2,8,12,16);cg.frameInterpolation=.25f;
 assert(CG_JAR_Entity(&beam)&&beamEnd[0]==2&&beamEnd[1]==3&&beamEnd[2]==4);
 beam.nextState.otherEntityNum2=42;assert(CG_JAR_Entity(&beam)&&VectorCompare(beamEnd,vec3_origin));
 beam.currentState.otherEntityNum2=41;beam.currentState.time2=1;
 VectorSet(beam.currentState.angles2,10,0,3);
 cg_entities[41].currentValid=qtrue;
 VectorSet(cg_entities[41].lerpOrigin,100,200,300);VectorSet(cg_entities[41].lerpAngles,0,90,0);
 assert(CG_JAR_Entity(&beam)&&fabsf(beamEnd[0]-100)<.01f&&fabsf(beamEnd[1]-210)<.01f&&fabsf(beamEnd[2]-303)<.01f);
 cg_entities[41].currentValid=qfalse;
 assert(CG_JAR_Entity(&beam)&&VectorCompare(beamEnd,vec3_origin));
 beam.currentState.generic1=253;CG_JAR_Entity(&beam);assert(beamWidth==1.1f&&beamLifetime==40&&!strcmp(lastShader,"gfx/effects/redLine")&&beamColor[0]==1&&beamFlags==FX_ALPHA_LINEAR);
 cg.time=1000;snapshot.ps.weapon=WP_TOOLGUN;CG_JAR_Init();CG_JAR_MenuDown();assert(menu&&catcher==KEYCATCH_CGAME&&!strcmp(request,"jar_setting show"));
 args[1]="12";args[2]="8";args[3]="1";args[4]="3";CG_JAR_Stats();
 assert(menuOwned==12&&menuProps==8&&menuPhysics==1&&menuActors==3);
 args[4]="13";CG_JAR_Stats();assert(menuActors==3);
 args[4]="";CG_JAR_Stats();assert(menuOwned==12&&menuActors==0);
 Click(100,85);assert(tab==2);Cards();assert(cardCount>0);
 Click(30,140);assert(!strncmp(request,"jar_equip ",10));
 int beforeRelease=requests;CG_JAR_Key(A_MOUSE1,qfalse);assert(requests==beforeRelease);
 CG_JAR_Key(A_MOUSE2,qtrue);assert(!strncmp(request,"jar_spawn ",10));
 beforeRelease=requests;CG_JAR_Key(A_MOUSE2,qfalse);assert(requests==beforeRelease);
 args[1]="5";CG_JAR_Equip();assert(cg.weaponSelect==5&&registeredWeapon==5&&cg.weaponSelectTime==cg.time);
 args[1]="999";CG_JAR_Equip();assert(cg.weaponSelect==5);args[1]="5;quit";CG_JAR_Equip();assert(cg.weaponSelect==5);
 // Every shared tool row uses server acknowledgement before changing active tool.
 for(int mode=0;mode<JAR_MENU_TOOLS;++mode){int old=tool,sent=requests;Click(280,108+(mode+2)*18);
  assert(tool==old);if(jarMenuTools[mode].mp<0){assert(requests==sent&&strstr(menuStatus,"being ported"));continue;}
  assert(!strcmp(request,va("jar_tool %d",jarMenuTools[mode].mp)));char value[8];snprintf(value,sizeof(value),"%d",jarMenuTools[mode].mp);args[1]=value;args[2]="1";CG_JAR_Tool();assert(tool==jarMenuTools[mode].mp&&cg.weaponSelect==WP_TOOLGUN);
 }
 assert(tool==12);args[1]="2";args[2]="1";CG_JAR_Tool();Click(450,285);assert(!strcmp(request,"jar_setting force_more")&&thrustForce==5000);
 args[1]="1.4";args[2]="6000";CG_JAR_Settings();assert(ropeSlack==1.4f&&thrustForce==6000);
 Click(400,312);assert(!strcmp(request,"jar_setting thrust_out")&&!thrustReverse);
 args[4]="1";CG_JAR_Settings();Click(400,312);assert(!strcmp(request,"jar_setting thrust_in")&&thrustReverse);
 args[4]="invalid";CG_JAR_Settings();assert(thrustReverse);
 args[4]="0";CG_JAR_Settings();assert(!thrustReverse);args[4]="99";
 args[1]="nan";args[2]="4500";CG_JAR_Settings();assert(thrustForce==6000);args[1]="3.1";CG_JAR_Settings();assert(ropeSlack==3.1f);
 int sent=requests;Click(438,285);assert(requests==sent);
 Click(420,261);assert(toolEditing==2&&menuPinned);ContextType("+-2e+Z;");CG_JAR_Key(A_ENTER,qtrue);
 assert(requests==sent&&toolEditing==2&&!strcmp(toolInput,"+2e+"));
 ContextType("5");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_setting force_200000")&&!toolEditing);
 menuPinned=qfalse;CG_JAR_Draw();assert(leftList==15&&rightProperties==2&&help==1);
 args[1]="4";args[2]="1";CG_JAR_Tool();Click(395,285);assert(!strcmp(request,"jar_setting slack_less"));
 Click(280,390);assert(!strcmp(request,"jar_undo"));Click(400,390);assert(!strcmp(request,"jar_cleanup"));
 Click(280,110);assert(!strncmp(request,"jar_equip ",10));
 assert(CG_JAR_Key(81,qfalse)&&!menu&&catcher==0);
 args[1]="81";CG_JAR_MenuDown();keyHeld=0;CG_JAR_Draw();assert(!menu&&catcher==0);keyHeld=1;
 catcher=KEYCATCH_CONSOLE;CG_JAR_MenuDown();assert(!menu&&catcher==KEYCATCH_CONSOLE);catcher=0;
 CG_JAR_ContextDown();assert(context&&!strcmp(request,"jar_select"));Click(100,425);assert(!strcmp(request,"jar_release"));
 args[1]="64";args[2]="1";args[3]="3";CG_JAR_Selection();assert(selectedEntity==64&&poseFrame==1&&poseFrames==3);
 Click(560,55);Click(550,220);assert(!strcmp(request,"jar_property pose_next 99"));Click(450,252);assert(!strcmp(request,"jar_property pose_undo 99"));
 Click(450,155);assert(poseEditing);int poseSent=requests;CG_JAR_Key('9'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);assert(requests==poseSent&&poseEditing);
 CG_JAR_Key(A_BACKSPACE,qtrue);CG_JAR_Key('2'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);assert(!poseEditing&&!strcmp(request,"jar_property pose_set_3 99"));
 Click(450,155);CG_JAR_Key('1'|K_CHAR_FLAG,qtrue);Click(100,425);assert(!poseEditing);
 args[2]="3";CG_JAR_Selection();assert(poseFrame==1);args[2]="garbage";CG_JAR_Selection();assert(poseFrame==1);
 CG_JAR_Draw();CG_JAR_Key(A_ESCAPE,qtrue);args[1]="81";CG_JAR_ContextDown();assert(selectedEntity==-1&&poseFrames==0);CG_JAR_MenuUp();
 // Traverse every actual menu page and click each card: newly exposed large
 // props use stable server catalog IDs and remain reachable beyond page 20.
 args[1]="81";CG_JAR_MenuDown();byte seen[JAR_MP_CATALOG_COUNT]={0};int visited=0;
 for(int t=0;t<JAR_MENU_TABS;++t){jarMenuRect_t bounds=JAR_MenuRect(JM_TAB,t);pages[t]=0;page=0;Click(bounds.x+bounds.w/2,bounds.y+bounds.h/2);assert(tab==t&&page==0);
  for(;;){CG_JAR_Draw();for(int i=0;i<cardCount;++i){int entry=cards[i];assert(!seen[entry]);seen[entry]=1;++visited;Click(30+(i%3)*80,140+(i/3)*57);assert(!strcmp(request,va("%s %u",t==2?"jar_equip":"jar_spawn",jarMPCatalog[entry].id)));if(t==2){CG_JAR_Key(A_MOUSE2,qtrue);assert(!strcmp(request,va("jar_spawn %u",jarMPCatalog[entry].id)));}CG_JAR_Draw();assert(!strncmp(hoverText,JAR_MPCatalogName(&jarMPCatalog[entry]),strlen(JAR_MPCatalogName(&jarMPCatalog[entry]))));}
   int last=page;CG_JAR_Key(A_MWHEELDOWN,qtrue);if(page==last)break;
  }
 }
 assert(JAR_MenuRect(JM_TAB,3).x<JAR_MenuRect(JM_TAB,JAR_MENU_MAP_SECTIONS).x&&JAR_MenuRect(JM_TAB,JAR_MENU_MAP_SECTIONS).x<JAR_MenuRect(JM_TAB,4).x);
 int menuEntries=0;
 for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i){
  const jarMPEntry_t*entry=&jarMPCatalog[i];
  const qboolean visible=JAR_MPCatalogVisible(entry);
  assert((seen[i]!=0)==visible);
  if(visible)++menuEntries;
 }
 assert(visited==menuEntries);CG_JAR_MenuUp();
 snapshot.ps.weapon=WP_PHYSGUN;CG_JAR_DistanceIn();assert(!strcmp(request,"jar_distance out"));
 CG_JAR_DistanceOut();assert(!strcmp(request,"jar_distance in"));
 snapshot.ps.weapon=WP_BLASTER;CG_JAR_DistanceOut();assert(next==1);CG_JAR_DistanceIn();assert(previous==1);
 ContextMenuCases();MovementMenuCases();DirectionalMenuCases();SearchlightMenuCases();PortalMenuCases();TransformMenuCases();ControllerMenuCases();TransportMenuCases();MediaMenuCases();ControllerSwitchMenuCases();LaserClientCases();PropClientCases();NativeKeyCases();SearchMenuCases();FeedbackMenuCases();LogicControllerMenuCases();ActionMenuCases();
 TriggerMenuCases();LocationMenuCases();LogicMarkerCases();
 TriggerTransportMenuCases();
 StationMenuCases();UsableMenuCases();BreakableMenuCases();SmallCargoMenuCases();RackMenuCases();ShooterMenuCases();AnimatedMenuCases();DoorCameraMenuCases();WelderMenuCases();CrystalCrateMenuCases();LightMenuCases();MusicMenuCases();RopeClientCases();ReflectionMenuCases();
 puts("MP production menu: compact tool/properties layout, settings echo, commands, gaps, held-key lifecycle, context and wheel routing passed.");return 0;
}
