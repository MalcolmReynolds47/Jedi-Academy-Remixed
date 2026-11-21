#include "../../shared/qcommon/jar_thruster.h"
// Local menu requests; all world changes are validated by the server. GPL-2.0-only.
#include "cg_local.h"
#include "fx_local.h"
#include "../../shared/qcommon/jar_tool_style.h"
#include "qcommon/jar_ui_palette.h"
#include "qcommon/jar_ui_cursor.h"
#include "ui/ui_shared.h"
#include "cg_jar_sandbox.h"
#include "../game/jar_mp_catalog.h"
#include "../game/jar_mp_context.h"
#include "../game/jar_mp_policy.h"
#include "../../shared/qcommon/jar_rope_path_mesh.h"
#include "../../shared/qcommon/jar_spawn_click.h"
#include "../../shared/qcommon/jar_sandbox_menu.h"
#include "../../shared/cgame/jar_logic_marker_sprite.h"
#include "cg_jar_props.inc"
static qboolean menu,context,menuPinned;static qhandle_t jarCursor,jarMenuFont;
static int page,pages[JAR_MENU_TABS],tab,menuKey=-1,tool=7;
static int linkOutput,thrustReverse;static qboolean searchEditing;static char searchText[96],menuStatus[240];static int menuStatusUntil,menuOwned,menuProps,menuPhysics;
static int menuActors;
static char undoStatus[sizeof(menuStatus)];
static int undoStatusUntil;
static float cursorX=320,cursorY=240,ropeSlack=1.1f,thrustForce=JAR_THRUSTER_DEFAULT;
static float rotationStep=15;
static int toolEditing;
static char toolInput[32];
static void DrawCursor(void){jarCursorBox_t b=JAR_MenuCursor(cursorX,cursorY,cgs.screenXScale,cgs.screenYScale);CG_DrawPic(b.x,b.y,b.w,b.h,jarCursor);}
static int contextEditing;
static int selectedEntity=-1,poseFrame,poseFrames;static qboolean poseEditing;static char poseInput[6];
static int cards[JAR_MENU_CARDS],cardCount,total,cardOrder[JAR_MP_CATALOG_COUNT];
static int CardCompare(const void*a,const void*b){return strcmp(JAR_MPCatalogName(&jarMPCatalog[*(const int*)a]),JAR_MPCatalogName(&jarMPCatalog[*(const int*)b]));}
static void Widget(jarMenuPart_t,int,const char*,qboolean);
static void ContextClear(void);
static qboolean ContextInputKey(int);
static void ContextClick(void);
static void DrawContext(void);
static void Text(float x,float y,const char*s,float scale){vec4_t color=JAR_UI_TEXT;CG_Text_Paint(x,y,scale,color,s,0,0,0,FONT_MEDIUM);}
static char probeStatus[80]="No active probes";
static unsigned probeEpoch;
static jarProbe_t visibleProbes[JAR_PROBE_LIMIT];
static int visibleProbeCount,probeSnapshotTime;
static qboolean probeHasSnapshot,probeOverflow;
void CG_JAR_ClearReflectionProbes(void){
 probeEpoch+=0x01000000u;
 probeHasSnapshot=probeOverflow=qfalse;visibleProbeCount=0;
 if(trap->JAR_SetReflectionProbes)trap->JAR_SetReflectionProbes(NULL,0);
 Q_strncpyz(probeStatus,"No active probes",sizeof(probeStatus));
}
void CG_JAR_ReflectionProbes(void){
 if(!cg.snap||cg.snap->snapFlags&SNAPFLAG_NOT_ACTIVE||!trap->JAR_SetReflectionProbes)return;
 const qboolean updated=!probeHasSnapshot||probeSnapshotTime!=cg.snap->serverTime;
 if(updated){
  probeHasSnapshot=qtrue;probeSnapshotTime=cg.snap->serverTime;visibleProbeCount=0;probeOverflow=qfalse;
  for(int i=0;i<cg.snap->numEntities;++i){
   const entityState_t* e=&cg.snap->entities[i];
   if(e->eType!=ET_GENERAL||e->generic1!=250||e->bolt2!=JAR_MP_PROBE_TAG||e->time<=0||!e->time2||(e->eFlags&EF_NODRAW))continue;
   jarProbe_t probe={(unsigned)e->time2^probeEpoch,(unsigned)e->time,{e->pos.trBase[0],e->pos.trBase[1],e->pos.trBase[2]},e->speed};
   if(!JAR_ProbeValid(&probe))continue;
   if(visibleProbeCount==JAR_PROBE_LIMIT){probeOverflow=qtrue;visibleProbeCount=0;break;}
   visibleProbes[visibleProbeCount++]=probe;
  }
 }
 const int count=visibleProbeCount;
 if(!count&&!updated)return;
 const int ready=trap->JAR_SetReflectionProbes(visibleProbes,count);
 const char* status=probeOverflow?"Reflection capacity exceeded":!count?"No active probes":ready==JAR_PROBE_UNSUPPORTED?"Requires Rend2 renderer":
  ready==JAR_PROBE_DISABLED?"Enable Cubemap reflections in Graphics":ready==JAR_PROBE_NO_WORLD?"Waiting for the map":
  ready<0?"Reflection submission rejected":va("Reflections ready: %d / %d",ready,count);
 Q_strncpyz(probeStatus,status,sizeof(probeStatus));
}
void CG_JAR_Init(void){for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)cardOrder[i]=(int)i;qsort(cardOrder,JAR_MP_CATALOG_COUNT,sizeof(cardOrder[0]),CardCompare);jarCursor=trap->R_RegisterShaderNoMip("jar/icons/cursor");jarMenuFont=trap->R_RegisterFont(JAR_MENU_FONT);ContextClear();PropViewsClear();poseEditing=qfalse;selectedEntity=-1;poseFrame=poseFrames=0;menu=context=menuPinned=qfalse;page=tab=linkOutput=thrustReverse=0;tool=7;memset(pages,0,sizeof(pages));searchText[0]=menuStatus[0]=0;searchEditing=qfalse;menuStatusUntil=menuOwned=menuProps=menuPhysics=0;trap->Cvar_Set("jar_mpMenuTyping","0");ropeSlack=1.1f;thrustForce=JAR_THRUSTER_DEFAULT;rotationStep=15;toolEditing=0;menuKey=-1;trap->Cvar_Set("jar_mpMenuOpen","0");
 undoStatusUntil=0;
 menuActors=0;
 trap->Cvar_Register(NULL,"jar_mpBindings","0",CVAR_ARCHIVE);
 char bindings[16];trap->Cvar_VariableStringBuffer("jar_mpBindings",bindings,sizeof(bindings));if(atoi(bindings)==0){trap->SendConsoleCommand("exec jar-mp-controls.cfg\nseta jar_mpBindings 1\n");}
}
void CG_JAR_MenuDown(void){if(!cg.snap||menu||trap->Key_GetCatcher())return;menu=qtrue;context=menuPinned=qfalse;menuKey=atoi(CG_Argv(1));trap->Cvar_Set("jar_mpMenuOpen","1");trap->Key_SetCatcher(KEYCATCH_CGAME);trap->SendClientCommand("jar_setting show");}
void CG_JAR_ContextDown(void){if(menu&&context)return;ContextClear();selectedEntity=-1;poseFrame=poseFrames=0;CG_JAR_MenuDown();if(menu){context=qtrue;trap->SendClientCommand("jar_select");}}
static void MenuClose(void){menuPinned=qfalse;searchEditing=qfalse;toolEditing=0;trap->Cvar_Set("jar_mpMenuTyping","0");if(context)trap->SendClientCommand("jar_select close");ContextClear();poseEditing=qfalse;menu=qfalse;menuKey=-1;trap->Key_SetCatcher(trap->Key_GetCatcher()&~KEYCATCH_CGAME);trap->Cvar_Set("jar_mpMenuOpen","0");}
static qboolean MenuTyping(void){return searchEditing||poseEditing||contextEditing||toolEditing;}
static void KeepEditing(void){if(MenuTyping())menuPinned=qtrue;trap->Cvar_Set("jar_mpMenuTyping",MenuTyping()?"1":"0");}
void CG_JAR_MenuUp(void){KeepEditing();if(!menuPinned)MenuClose();}
void CG_JAR_Shutdown(void){CG_JAR_ClearReflectionProbes();PropViewsClear();MenuClose();}
void CG_JAR_UndoFeedback(void){
 Q_strncpyz(undoStatus,menuStatus,sizeof(undoStatus));
 undoStatusUntil=cg.time+4500;
 trap->S_StartLocalSound(trap->S_RegisterSound("sound/interface/esc.mp3"),CHAN_LOCAL_SOUND);
}
void CG_JAR_Undo(void){trap->SendClientCommand("jar_undo");}
// Keep existing wheel aliases so current profiles receive the inverted distance action.
void CG_JAR_DistanceIn(void){if(cg.snap&&cg.snap->ps.weapon==WP_PHYSGUN)trap->SendClientCommand("jar_distance out");else CG_PrevWeapon_f();}
void CG_JAR_DistanceOut(void){if(cg.snap&&cg.snap->ps.weapon==WP_PHYSGUN)trap->SendClientCommand("jar_distance in");else CG_NextWeapon_f();}
qboolean CG_JAR_Mouse(int x,int y){if(!menu)return qfalse;cursorX=Com_Clamp(0,640,cursorX+x);cursorY=Com_Clamp(0,480,cursorY+y);return qtrue;}
static qboolean SelectionNumber(const char*s,int min,int max,int*out){int value;if(!JAR_ParseInt(s,&value)||value<min||value>max)return qfalse;*out=value;return qtrue;}
void CG_JAR_Settings(void){
 float slack,force,rotation;
 if(!JAR_ParseFloat(CG_Argv(1),&slack)||slack<=0||!JAR_ParseFloat(CG_Argv(2),&force))return;
 ropeSlack=slack;thrustForce=force;
 if(JAR_ParseFloat(CG_Argv(5),&rotation))rotationStep=rotation;
 int value;
 if(SelectionNumber(CG_Argv(3),0,JAR_DISTANCE_LINKS-1,&value))linkOutput=value;
 if(SelectionNumber(CG_Argv(4),0,1,&value))thrustReverse=value;
}
void CG_JAR_Equip(void){int weapon;if(!SelectionNumber(CG_Argv(1),WP_STUN_BATON,WP_TOOLGUN,&weapon))return;CG_RegisterWeapon(weapon);cg.weaponSelect=weapon;cg.weaponSelectTime=cg.time;}
static void ToolButton(float x,float y,float width,const char*text){vec4_t color=JAR_UI_PANEL;CG_FillRect(x,y,width,28,color);Text(x+8,y+19,text,.45f);}
#include "cg_jar_context.inc"
#include "cg_jar_menu.inc"
qboolean CG_JAR_Key(int key,qboolean down){
 if(!menu)return qfalse;if(!down){if(key==menuKey)CG_JAR_MenuUp();return qtrue;}
 if(key==A_ESCAPE){MenuClose();return qtrue;}
 if(context&&ContextInputKey(key))return qtrue;
 if(context&&poseEditing){
  if(key&K_CHAR_FLAG){int c=key&~K_CHAR_FLAG;size_t len=strlen(poseInput);if(c>='0'&&c<='9'&&len<sizeof(poseInput)-1){poseInput[len]=(char)c;poseInput[len+1]=0;}return qtrue;}
  if(key==A_BACKSPACE){size_t len=strlen(poseInput);if(len)poseInput[len-1]=0;return qtrue;}
  if(key==A_ENTER||key==A_KP_ENTER){int value;if(SelectionNumber(poseInput,0,poseFrames-1,&value)){poseEditing=qfalse;ContextProperty(va("pose_set_%d",value+1));}return qtrue;}
 }
 if(!context&&SearchKey(key))return qtrue;
 if(!context&&ToolInputKey(key))return qtrue;
 if(!context&&(key==A_MWHEELUP||key==A_MWHEELDOWN)){Cards();if(key==A_MWHEELUP&&page>0)--page;else if(key==A_MWHEELDOWN&&(page+1)*JAR_MENU_CARDS<total)++page;return qtrue;}
 if(key==A_MOUSE2){if(!context){SearchEditing(qfalse);MenuCard(qtrue);}return qtrue;}
 if(key!=A_MOUSE1)return qtrue;
 if(context){
  poseEditing=qfalse;
  ContextClick();KeepEditing();return qtrue;
 }
 MenuClick();KeepEditing();return qtrue;
}
void CG_JAR_Draw(void){
 if(!cg.snap)return;
 const qboolean undoPopup=undoStatusUntil>cg.time&&cg.snap->ps.pm_type!=PM_FREEZE;
 if(!menu){
  const jarMenuTool_t*spec=&jarMenuTools[JAR_MenuToolIndex(tool)];const char*help=NULL;
  if(cg.snap->ps.pm_type==PM_FREEZE)help="HELD / FROZEN: hold C to release yourself or block tools on you";
  else if(menuStatusUntil>cg.time)help=menuStatus;
  else if(cg.snap->ps.weapon==WP_PHYSGUN)help="PHYSGUN: left holds | use + mouse rotates | right freezes / releases | wheel distance";
  else if(cg.snap->ps.weapon==WP_TOOLGUN)help=va("%s | Left: %s | Right: %s",spec->name,spec->left,spec->right);
  if(help&&!undoPopup)Widget(JM_STATUS,0,help,qfalse);
 }else{
  KeepEditing();
  if(trap->Key_GetCatcher()!=KEYCATCH_CGAME||(!menuPinned&&menuKey>=0&&!trap->Key_IsDown(menuKey))){MenuClose();return;}
  if(context){DrawContext();DrawCursor();}else DrawSandbox();
 }
 if(undoPopup){
  vec4_t panel=JAR_UI_PANEL;
  CG_FillRect(8,8,530,46,panel);
  Text(16,28,"UNDO",.56f);
  Text(16,48,undoStatus,.5f);
 }
}
void CG_JAR_RopeTrace(trace_t*,const vec3_t,const vec3_t,const vec3_t,const vec3_t,int,int);
static void RopePathTrace(const float*a,const float*b,float radius,jarRopePathHit_t*out,void*data){
 const entityState_t*s=(const entityState_t*)data;vec3_t lo={-radius,-radius,-radius},hi={radius,radius,radius};trace_t tr;
 CG_JAR_RopeTrace(&tr,a,lo,hi,b,s->otherEntityNum,s->otherEntityNum2);
 out->fraction=tr.fraction;out->solid=tr.startsolid||tr.allsolid;VectorCopy(tr.endpos,out->point);VectorCopy(tr.plane.normal,out->normal);
}
static void RopeTrace(const float*chord,float*point,void*data){
 trace_t tr;const vec3_t low={-1.8f,-1.8f,-1.8f},high={1.8f,1.8f,1.8f};
 CG_Trace(&tr,chord,low,high,point,ENTITYNUM_NONE,MASK_SOLID);
 if(tr.startsolid||tr.allsolid)VectorCopy(chord,point);else VectorCopy(tr.endpos,point);
}
static void TriggerOutline(centity_t*cent){
 const qboolean location=contextControllerKind==JAR_LOCATION_VOLUME_CONTROLLER;
 const qboolean directional=contextControllerKind==JAR_DIRECTIONAL_CONTROLLER;
 if(!menu||!context||!contextIdentity||(!location&&!directional&&!JAR_ControllerTrigger(contextControllerKind))||!(contextCaps&JAR_CONTEXT_CONTROLLER)||cent->currentState.number!=selectedEntity||!cent->currentValid||cent->currentState.generic1!=250)return;
 vec3_t axis[3],points[8],color={.2f,1,.5f};AnglesToAxis(cent->lerpAngles,axis);
 if((!location&&!contextController.active)||contextController.inactive)VectorSet(color,.7f,.7f,.7f);
 vec3_t center;VectorCopy(cent->lerpOrigin,center);
 if(directional)VectorMA(center,contextController.speed,axis[2],center);
 vec3_t low,high;
 if(contextControllerKind==JAR_SPACE_CONTROLLER)JAR_SpaceBounds(&contextController,low,high);
 else for(int a=0;a<3;++a){low[a]=-contextController.speed;high[a]=contextController.speed;}
 for(int i=0;i<8;++i){VectorCopy(center,points[i]);for(int a=0;a<3;++a)VectorMA(points[i],i&(1<<a)?high[a]:low[a],axis[a],points[i]);}
 qhandle_t shader=trap->R_RegisterShader("gfx/effects/blueLine");
 for(int i=0;i<8;++i)for(int a=0;a<3;++a)if(!(i&(1<<a)))trap->FX_AddLine(points[i],points[i|(1<<a)],1,1,0,.7f,.7f,0,color,color,1,1,shader,0);
 if(directional){
  vec3_t start,end,tail;
  const float length=fminf(48.f,contextController.speed);
  VectorMA(center,-length,axis[0],start);VectorMA(center,length,axis[0],end);
  trap->FX_AddLine(start,end,2,2,0,1,1,0,color,color,1,1,shader,0);
  for(int side=-1;side<=1;side+=2){
   VectorMA(end,-12,axis[0],tail);VectorMA(tail,side*8,axis[1],tail);
   trap->FX_AddLine(end,tail,2,2,0,1,1,0,color,color,1,1,shader,0);
  }
 }
}
extern void CG_CalcEntityLerpPositions(centity_t *cent);
#include "cg_jar_searchlight.inc"
#include "cg_jar_portal.inc"

qboolean CG_JAR_Entity(centity_t*cent){
 TriggerOutline(cent);
 if(JAR_MP_PortalSurface(&cent->currentState)){PortalDraw(cent);return qtrue;}
 if(JAR_MP_Searchlight(&cent->currentState))SearchlightDraw(cent);
 if(cent->currentState.eType==ET_GENERAL&&cent->currentState.generic1==250&&
    (cent->currentState.bolt2==JAR_MP_LOGIC_MARKER_TAG||cent->currentState.bolt2==JAR_MP_WALL_OFF_TAG||cent->currentState.bolt2==JAR_MP_PROBE_TAG)&&cgs.media.jarLogicMarkerShader){
  if(!(cent->currentState.eFlags&EF_NODRAW)){
   refEntity_t marker;
   JAR_LogicMarkerSprite(&marker,cent->lerpOrigin,cg.refdef.vieworg,cg.refdef.viewaxis[0],cgs.media.jarLogicMarkerShader);
   trap->R_AddRefEntityToScene(&marker);
  }
  return qtrue;
 }
 if(cent->currentState.generic1==254){
  refEntity_t housing={0};housing.hModel=trap->R_RegisterModel(JAR_THRUSTER_MODEL);VectorCopy(cent->lerpOrigin,housing.origin);VectorCopy(housing.origin,housing.oldorigin);
  vectoangles(cent->currentState.origin2,housing.angles);housing.angles[PITCH]+=90;AnglesToAxis(housing.angles,housing.axis);memset(housing.shaderRGBA,255,4);trap->R_AddRefEntityToScene(&housing);
  if(cent->currentState.time2){vec3_t end,color={1,.6f,.15f};VectorMA(housing.origin,40,cent->currentState.origin2,end);trap->FX_AddLine(housing.origin,end,1,2,0,.7f,.7f,0,color,color,1,1,trap->R_RegisterShader("gfx/effects/redLine"),0);}return qtrue;
 }
 if(SkeletalProp(cent))return qtrue;
 if(cent->currentState.generic1==252||cent->currentState.generic1==253){
  const qboolean tool=cent->currentState.generic1==253;
  vec3_t start,end,color={JAR_PHYSGUN_COLOR};VectorCopy(cent->lerpOrigin,start);VectorCopy(cent->currentState.origin2,end);
  if(tool){vec3_t toolColor={JAR_TOOLGUN_COLOR};VectorCopy(toolColor,color);}
  else if(cent->interpolate&&cent->nextState.generic1==252&&
          cent->nextState.otherEntityNum==cent->currentState.otherEntityNum&&
          cent->nextState.otherEntityNum2==cent->currentState.otherEntityNum2){
   vec3_t delta;VectorSubtract(cent->nextState.origin2,end,delta);
   VectorMA(end,cg.frameInterpolation,delta,end);
  }
  const int held=cent->currentState.otherEntityNum2;
  if(!tool&&cent->currentState.time2==1&&held>=0&&held<ENTITYNUM_WORLD&&cg_entities[held].currentValid){
   centity_t *target=&cg_entities[held];
   vec3_t axis[3];
   CG_CalcEntityLerpPositions(target);
   AnglesToAxis(target->lerpAngles,axis);
   VectorCopy(target->lerpOrigin,end);
   for(int a=0;a<3;++a)VectorMA(end,cent->currentState.angles2[a],axis[a],end);
  }
  if(cent->currentState.otherEntityNum>=0&&cent->currentState.otherEntityNum<MAX_CLIENTS&&cg.snap)CG_CalcMuzzlePoint(cent->currentState.otherEntityNum,start);
  trap->FX_AddLine(start,end,1,tool?JAR_TOOLGUN_WIDTH:JAR_PHYSGUN_WIDTH,0,1,tool?0:1,0,color,color,1,tool?40:1,trap->R_RegisterShader(tool?JAR_TOOLGUN_SHADER:JAR_PHYSGUN_SHADER),tool?FX_ALPHA_LINEAR:0);
  return qtrue;
 }
 if(cent->currentState.generic1!=251)return qfalse;
 if(cent->currentState.weapon)return qtrue; // Server-only Weld/Hinge links have no rope mesh.
 jarRopeVertex_t mesh[JAR_ROPE_VERTICES];int count;const entityState_t*s=&cent->currentState;
 if(s->jarRopeState){
  if(s->jarRopeState<2||s->jarRopeState>JAR_ROPE_PATH_BENDS+2||s->otherEntityNum<0||s->otherEntityNum>ENTITYNUM_WORLD||s->otherEntityNum2<0||s->otherEntityNum2>ENTITYNUM_WORLD)return qtrue;
  jarRopePath_t path={0};path.count=s->jarRopeState-2;memcpy(path.bend,s->jarRopeBends,sizeof(path.bend));
  // Keep endpoints and topology from one snapshot; clients never choose bends.
  count=JAR_RopePathMesh(&path,s->pos.trBase,s->origin2,s->jarRopeLength,mesh,RopePathTrace,(void*)s,1);
 }else count=JAR_RopeMeshTrace(cent->lerpOrigin,s->origin2,(float)s->time2,mesh,RopeTrace,NULL);
 qhandle_t shader=trap->R_RegisterShader("white");
 for(int at=0;at<count;at+=4){polyVert_t poly[4];for(int v=0;v<4;++v){VectorCopy(mesh[at+v].xyz,poly[v].xyz);poly[v].st[0]=mesh[at+v].st[0];poly[v].st[1]=mesh[at+v].st[1];memcpy(poly[v].modulate,mesh[at+v].color,4);}trap->R_AddPolysToScene(shader,4,poly,1);}
 return qtrue;
}
