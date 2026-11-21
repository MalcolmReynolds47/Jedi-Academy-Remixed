// Original unpaused Sandbox overlay. Included after AddItem. GPL-2.0-only.
#include "qcommon/jar_catalog_generated.h"
#include "qcommon/jar_force_boon.h"
#include "qcommon/jar_force_enlightenment.h"
#include "../../shared/qcommon/jar_spawn_click.h"
#include "../../shared/qcommon/jar_sandbox_menu.h"
#include "../../shared/qcommon/jar_thruster.h"
#include "../../shared/qcommon/jar_bobbing_motion.h"
#include "qcommon/jar_movement.h"
#include "qcommon/jar_logic_settings.h"
#include "qcommon/jar_contact_trigger.h"
#include "qcommon/jar_push.h"
#include "qcommon/jar_teleport.h"
#include "qcommon/jar_portal.h"
#include "qcommon/jar_skyportal.h"
static bool sandboxOverlayOpen,sandboxContextOpen,sandboxPinned;
static char sandboxSearch[96];
static int sandboxTab,sandboxPage[JAR_MENU_TABS];
static constexpr int sandboxPageSize=JAR_MENU_CARDS;
static int sandboxSlots[JAR_MENU_TABS][sandboxPageSize];
static char sandboxCardLabels[JAR_MENU_TABS][sandboxPageSize][64],sandboxPageLabels[JAR_MENU_TABS][64],sandboxDescriptions[JAR_MENU_TABS][sandboxPageSize][256];

struct SandboxCard {int tab,id;const char* name;const char* icon;const char* grant;};
static const std::vector<SandboxCard> sandboxCards=[](){std::vector<SandboxCard> out={
 {0,1001,"Supply crate","jar/icons/crate",nullptr},
 {0,1002,"Banded crate","jar/icons/banded",nullptr},
 {0,1003,"Tall crate","jar/icons/crate",nullptr},
 {0,1004,"Cargo crate","jar/icons/banded",nullptr},
 {0,1005,"Square crate","jar/icons/crate",nullptr},
 {0,1006,"Imperial cargo","jar/icons/banded",nullptr},
 {0,1007,"Barrel pair","jar/icons/barrels",nullptr},
 {0,1008,"Factory crate","jar/icons/crate",nullptr},
 {0,1009,"Factory case","jar/icons/banded",nullptr},
 {0,1010,"Snow crate","jar/icons/crate",nullptr},
 {0,1011,"Tall snow crate","jar/icons/banded",nullptr},
 {0,1012,"Small snow crate","jar/icons/crate",nullptr},
 {0,1013,"Kejim crate","jar/icons/crate",nullptr},
 {0,1014,"Mine cargo","jar/icons/banded",nullptr},
 {0,1015,"Rooftop crate","jar/icons/crate",nullptr},
 {0,1016,"Red cargo box","jar/icons/banded",nullptr},
 {0,1017,"Large factory crate","jar/icons/crate",nullptr},
 {0,1018,"Large factory case","jar/icons/banded",nullptr},
 {0,1019,"Outdoor snow crate","jar/icons/crate",nullptr},
 {0,1020,"Outdoor small crate","jar/icons/banded",nullptr},
 {1,3004,"Imperial officer","models/players/imperial/icon_default",nullptr},
 {1,3005,"Rodian sniper","models/players/rodian/icon_default",nullptr},
 {1,3006,"Gran grenadier","models/players/gran/icon_default",nullptr},
 {1,3007,"Trandoshan","models/players/trandoshan/icon_default",nullptr},
 {1,3008,"Weequay","models/players/weequay/icon_default",nullptr},
 {1,3009,"R5 unit","jar/icons/r2",nullptr},
 {1,3010,"Protocol droid","jar/icons/r2",nullptr},
 {1,3011,"Jedi ally","models/players/jedi/icon_default",nullptr},
 {1,3012,"Mercenary","models/players/human_merc/icon_default",nullptr},
 {1,3001,"Stormtrooper","models/players/stormtrooper/icon_default",nullptr},
 {1,3002,"Rebel","models/players/rebel/icon_default",nullptr},
 {1,3003,"R2 unit","jar/icons/r2",nullptr},
 {2,2002,"Blaster pistol","gfx/hud/w_icon_blaster_pistol",nullptr},
 {2,2003,"E-11 blaster","gfx/hud/w_icon_blaster",nullptr},
 {2,2004,"Disruptor","gfx/hud/w_icon_disruptor",nullptr},
 {2,2005,"Bowcaster","gfx/hud/w_icon_bowcaster",nullptr},
 {2,2006,"Repeater","gfx/hud/w_icon_repeater",nullptr},
 {2,2007,"DEMP2","gfx/hud/w_icon_demp2",nullptr},
 {2,2008,"Flechette","gfx/hud/w_icon_flechette",nullptr},
 {2,2009,"Rocket launcher","gfx/hud/w_icon_merrsonn",nullptr},
 {2,2010,"Thermal","gfx/hud/w_icon_thermal",nullptr},
 {2,2011,"Trip mine","gfx/hud/w_icon_tripmine",nullptr},
 {2,2012,"Det pack","gfx/hud/w_icon_detpack",nullptr},
 {2,2013,"Concussion","gfx/hud/w_icon_c_rifle",nullptr},
 {2,2101,"Shortblade","jar/icons/shortblade","jar_shortblade"},
 {2,2102,"Energy pike","jar/icons/pike","jar_pike"},
 {2,2103,"Sith sword","jar/icons/sword","sith_sword"},
 {3,2501,"Health pack","jar/icons/health",nullptr},
 {3,2502,"Small shield","jar/icons/shield",nullptr},
 {3,2503,"Battery","jar/icons/battery",nullptr},
 {3,2504,"Large shield","jar/icons/shield_large",nullptr},
 {4,4001,"Swoop","jar/icons/swoop",nullptr},
 {4,4002,"Tauntaun","jar/icons/tauntaun",nullptr}
};std::vector<SandboxCard> added;
 for(const auto&e:jar::catalog::entries){
  // NPC spawners resolve to the exact same ID as the direct NPC card.
  if(e.tab==3&&e.alias){
   const auto* actor=jar::catalog::Find(e.alias);
   if((actor&&actor->kind==2&&actor->tab==1)||(e.alias>=3001&&e.alias<=3012))continue;
  }
  const auto* enlightenment=jar::enlightenment::Definition(e.name);
  const char* label=e.tab==JAR_MENU_MAP_SECTIONS&&!strcmp(e.name,"misc_bsp")?"Vjun Fuel Processing":
   jar::portal::SurfaceClass(e.name)?"Portal surface":jar::portal::CameraClass(e.name)?"Portal camera":
   jar::skyPortal::Class(e.name)?"Map Sky View":
   !strcmp(e.name,jar::forceBoon::classname)?jar::forceBoon::title:
   enlightenment?enlightenment->title:
   e.id==JAR_BOB_ID?"Bobbing platform":e.id==jar::movement::gravityId?"Gravity change":
   e.id==jar::movement::frictionId?"Friction change":e.id==jar::logic::delayId?"Delay":
   e.id==jar::logic::counterId?"Counter":e.id==jar::logic::timerId?"Timer":
   e.id==jar::logic::randomId?"Random":e.id==jar::logic::relayId?"Relay":
   e.id==jar::logic::scoreId?"Score":e.id==jar::logic::killId?"Kill":e.id==jar::logic::giveId?"Give":
   e.id==jar::logic::enableId?"Enable entity":e.id==jar::logic::disableId?"Disable entity":
   e.id==jar::contactTrigger::repeatId?"Repeat trigger":e.id==jar::contactTrigger::onceId?"Once trigger":
   e.id==jar::contactTrigger::hurtId?"Hurt trigger":e.id==jar::push::volumeId?"Push volume":
   e.id==jar::push::markerId?"Push marker":e.id==jar::teleport::volumeId?"Teleport volume":
   e.id==jar::teleport::discId?"Teleporter":e.id==jar::teleport::markerId?"Teleport marker":
   e.id==jar::teleport::destinationId?"Teleport destination":e.name;
  added.push_back({e.tab,int(e.id),label,e.icon,nullptr});
 }
 std::sort(added.begin(),added.end(),[](const SandboxCard&a,const SandboxCard&b){return std::strcmp(a.name,b.name)<0;});out.insert(out.end(),added.begin(),added.end());return out;}();
static char sandboxToolTitle[64],sandboxToolDetails[5][80],sandboxToolValue[80];
static char sandboxOverlayStatus[240],sandboxOverlayCount[128],sandboxPhysicsLabel[64];
bool UI_JAR_OverlayActive(){return sandboxOverlayOpen;}
bool UI_JAR_OverlayTyping(){return sandboxOverlayOpen && UI_JAR_TextEntryActive();}
static bool SandboxOverlayFocused(){auto*m=Menu_GetFocused();return m && m->window.name && (!Q_strncmp(m->window.name,"jarSpawnOverlay",15)||!strcmp(m->window.name,"jarEntityContext"));}
void UI_JAR_OverlayClose(){
 if(!sandboxOverlayOpen)return;
 UI_JAR_EndTextEntry();
 if(SandboxOverlayFocused() || !Menu_GetFocused()){Menus_CloseAll();ui.Key_SetCatcher(ui.Key_GetCatcher()&~KEYCATCH_UI);}
 if(sandboxContextOpen){Cvar_Set("jar_contextOpen","0");ui.Cmd_ExecuteText(EXEC_APPEND,"jar_sb context clear\n");}
 sandboxOverlayOpen=sandboxContextOpen=sandboxPinned=false;
 JAR_Log(1,"sandbox","overlay_close","paused=%s",Cvar_VariableString("cl_paused"));
}
#include "ui_jar_context.inc"
static bool SandboxMatches(const SandboxCard&c,int tab){
 if(c.tab!=tab)return false;
 if(JAR_MenuMatches(c.name,sandboxSearch))return true;
 const char* alias=c.id==JAR_BOB_ID?"func_bobbing":c.id==jar::movement::gravityId?"target_gravity_change":
  c.id==jar::movement::frictionId?"target_friction_change":c.id==jar::logic::delayId?"target_delay":
  c.id==jar::logic::counterId?"target_counter":c.id==jar::logic::timerId?"func_timer":
  c.id==jar::logic::randomId?"target_random":c.id==jar::logic::relayId?"target_relay":
  c.id==jar::logic::scoreId?"target_score":c.id==jar::logic::killId?"target_kill":c.id==jar::logic::giveId?"target_give":
  c.id==jar::logic::enableId?"target_activate":c.id==jar::logic::disableId?"target_deactivate":
  c.id==jar::contactTrigger::repeatId?"trigger_multiple":c.id==jar::contactTrigger::onceId?"trigger_once":
  c.id==jar::contactTrigger::hurtId?"trigger_hurt":c.id==jar::push::volumeId?"trigger_push":
  c.id==jar::push::markerId?"target_push":c.id==jar::teleport::volumeId?"trigger_teleport":
  c.id==jar::teleport::discId?"misc_teleporter":c.id==jar::teleport::markerId?"target_teleporter":
  c.id==jar::teleport::destinationId?"misc_teleporter_dest":nullptr;
 if(alias)return JAR_MenuMatches(alias,sandboxSearch);
 const auto* detail=jar::catalog::Find(c.id);
 return detail&&JAR_MenuMatches(detail->name,sandboxSearch);
}
static int SandboxTabCount(int tab){int count=0;for(const auto&c:sandboxCards)if(SandboxMatches(c,tab))++count;return count;}
static bool SandboxIndex(const char* value,int maximum,int& out){
 if(!value||!*value)return false;out=0;
 for(const char*p=value;*p;++p){if(*p<'0'||*p>'9'||out>maximum/10)return false;out=out*10+(*p-'0');if(out>maximum)return false;}return true;
}
static void SandboxOverlayPage(int tab){
 auto*m=Menus_FindByName(va("jarSpawnOverlay%d",tab));if(!m)return;
 const int total=SandboxTabCount(tab),last=std::max(0,(total-1)/sandboxPageSize);
 sandboxPage[tab]=std::max(0,std::min(last,sandboxPage[tab]));m->cursorItem=-1;
 Com_sprintf(sandboxPageLabels[tab],sizeof(sandboxPageLabels[tab]),"%d / %d (%d)",sandboxPage[tab]+1,last+1,total);
 for(int slot=0;slot<sandboxPageSize;++slot){
  int index=-1,at=0;for(unsigned c=0;c<sandboxCards.size();++c)if(SandboxMatches(sandboxCards[c],tab) && at++==sandboxPage[tab]*sandboxPageSize+slot){index=int(c);break;}
  sandboxSlots[tab][slot]=index;
  auto*b=Menu_FindItemByName(m,va("card%d",slot));auto*icon=Menu_FindItemByName(m,va("icon%d",slot));
  if(!b||!icon)continue;
  for(auto*item:{b,icon}){item->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);item->textRect.w=item->textRect.h=0;}
  sandboxCardLabels[tab][slot][0]=0;icon->window.background=0;
  if(index>=0){const auto&entry=sandboxCards[index];Q_strncpyz(sandboxCardLabels[tab][slot],entry.name,sizeof(sandboxCardLabels[tab][slot]));
   const auto*detail=jar::catalog::Find(uint32_t(entry.id));
   Com_sprintf(sandboxDescriptions[tab][slot],256,"%s%s%s",entry.name,detail&&detail->reason[0]?" | ":"",detail?detail->reason:"");b->descText=sandboxDescriptions[tab][slot];
   if(detail&&jar::enlightenment::Definition(detail->name))
    Com_sprintf(sandboxDescriptions[tab][slot],256,"%s | Temporarily boosts matching and neutral Force powers. Match your learned alignment. Either pickup works if the sides are tied.",entry.name);
   if(entry.tab==2)Com_sprintf(sandboxDescriptions[tab][slot],256,"%s | Left click: equip in hands | Right click: spawn on ground",entry.name);
   if(entry.tab==JAR_MENU_MAP_SECTIONS)Com_sprintf(sandboxDescriptions[tab][slot],256,"%s | Spawn a movable section. Use Activate to enter.",entry.name);
   if(detail&&jar::portal::SurfaceClass(detail->name))b->descText="Live screen for a linked Portal Camera. Both spawn together. Look at the screen from the front. C / Portal changes its size, mode and power.";
   if(detail&&jar::portal::CameraClass(detail->name))b->descText="A movable camera linked to a Portal Surface. Both spawn together. Move and aim the camera to change what the screen shows.";
   if(detail&&jar::skyPortal::Class(detail->name))b->descText="Changes the view through the map's sky surfaces. Turn it on with Activate or C / Sky. For a camera screen, spawn Portal Surface.";
   if(detail&&!strcmp(detail->name,"target_interest"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Interest marker: attracts the nearby player head-look when no enemy is selected. Move with Physgun. Link sets its feedback output; Activate toggles. Owned marker and target survive full saves.");
   if(detail&&!strcmp(detail->name,"target_location"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Location marker: supplies the nearest visible location name outside authored location volumes. C edits its label. Hover the context status to see the selected entity location. Activate toggles.");
   if(detail&&!strcmp(detail->name,"trigger_always"))Com_sprintf(sandboxDescriptions[tab][slot],256,"One-shot trigger: fires after 300 ms when spawned and stays editable. Link sets its output. Activate arms another shot. C edits the delay for future shots.");
   if(detail&&!strcmp(detail->name,"trigger_visible"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Visibility trigger: fires its Primary output once when seen. Activate rearms it. C / Visibility edits range, angle, walls and Force Sight.");
   if(detail&&!strcmp(detail->name,"trigger_entdist"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Distance trigger: Activate checks range once and fires Inside or Outside. C / Distance sets the range, subjects and Link role, and rearms the marker.");
   if(detail&&!strcmp(detail->name,"func_wall"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Wall: Activate toggles a solid stock panel. When off, pick its gizmo. C edits visibility and activation.");
   if(entry.id==JAR_BOB_ID)Com_sprintf(sandboxDescriptions[tab][slot],256,"Bobbing platform: carries riders and cargo. Use pauses or resumes it. C / Platform edits amplitude and cycle length.");
   if(detail&&JAR_StartCard(detail->name))Com_sprintf(sandboxDescriptions[tab][slot],256,"Start marker: Activate visits a clear marker on foot. Link a teleporter to it. C edits its label and pose.");
   if(detail&&!strcmp(detail->name,"trigger_bidirectional"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Directional volume: cross along the arrow for Forward, against it for Reverse. Includes two feedback targets. Q Link properties choose the output. C edits size; Physgun USE rotates. Activate toggles. One-second cooldown.");
   if(detail&&!strcmp(detail->name,"misc_crystal_crate"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Crystal crate: stock crate and blue crystal. Hits emit a nearby damage pulse; destruction or Activate detonates it. C / Blast edits both damage/radius settings and solidity. Health is under Properties.");
   if(detail&&!strcmp(detail->name,"misc_model_jabba_cam"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Door camera: Activate extends/retracts the stock camera head and telescoping mount. C / Camera edits travel and speed. Select any part to move the whole assembly. Extension stops at obstacles.");
   if(detail&&!strcmp(detail->name,"misc_model_welder"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Welder: stock DEMP2 housing with pulsed sparks, electrical loop and radial damage. C / Welder edits reach, radius, interval, damage and On/Off. Zero damage makes it decorative. Activate toggles.");
   if(detail&&!strcmp(detail->name,"misc_spotlight"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Spotlight: move its aim marker or choose Angles in C / Light. Edit range, cone and color. Primary Link aims; Secondary Link connects detection. Includes a feedback target. Activate toggles.");
   if(detail&&!strcmp(detail->name,"trigger_space"))Com_sprintf(sandboxDescriptions[tab][slot],256,"Space volume: default 128 units. C edits dimensions/angles; hold Physgun left click + USE to rotate. Zero gravity; breathing actors suffocate after 5 seconds. Activate toggles it. Enclosed vehicles protect riders.");
   JAR_MenuLabel(entry.name,sandboxCardLabels[tab][slot],sizeof(sandboxCardLabels[tab][slot]));
   if(detail&&detail->reason[0])std::strcat(sandboxCardLabels[tab][slot]," !");
   b->window.flags|=WINDOW_VISIBLE;icon->window.flags|=WINDOW_VISIBLE;icon->window.background=ui.R_RegisterShaderNoMip?ui.R_RegisterShaderNoMip(entry.icon):0;}
 }
 for(int direction=0;direction<2;++direction)if(auto*b=Menu_FindItemByName(m,direction?"pageNext":"pagePrevious")){
  if(direction?sandboxPage[tab]<last:sandboxPage[tab]>0)b->window.flags|=WINDOW_VISIBLE;
  else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
 }
}
static void SandboxCardAction(int slot,bool alternate){
 if(slot<0||slot>=sandboxPageSize)return;const int card=sandboxSlots[sandboxTab][slot];if(card<0)return;
 const auto&entry=sandboxCards[card];
 ui.Cmd_ExecuteText(EXEC_APPEND,va("jar_sb %s %d\n",JAR_CardEquip(entry.tab,alternate)?"weapon":"spawn",entry.id));
}
qboolean UI_JAR_OverlayKey(int key,qboolean down){
 if(!sandboxOverlayOpen||sandboxContextOpen||UI_JAR_OverlayTyping()||!SandboxOverlayFocused())return qfalse;
 if(key=='z'||key=='Z'){
  if(down)ui.Cmd_ExecuteText(EXEC_APPEND,"jar_sb undo\n");
  return qtrue;
 }
 if(sandboxTab!=2||(key!=A_MOUSE1&&key!=A_MOUSE2))return qfalse;
 auto*m=Menus_FindByName("jarSpawnOverlay2");if(!m)return qfalse;
 for(int slot=0;slot<sandboxPageSize;++slot){auto*b=Menu_FindItemByName(m,va("card%d",slot));if(!b||!(b->window.flags&WINDOW_VISIBLE))continue;
  const auto&r=b->window.rect;
  if(uiInfo.uiDC.cursorx>=r.x&&uiInfo.uiDC.cursorx<r.x+r.w&&uiInfo.uiDC.cursory>=r.y&&uiInfo.uiDC.cursory<r.y+r.h){if(down)SandboxCardAction(slot,key==A_MOUSE2);return qtrue;}
 }
 return qfalse;
}
static itemDef_t* SandboxItem(menuDef_t*m,jarMenuPart_t part,int index,const char*id,const char*text,const char*action){
 const auto r=JAR_MenuRect(part,index);auto*b=AddItem(m,id,text,r.x,r.y,r.w,action);b->window.rectClient.h=r.h;b->textscale=r.scale;b->textalignx=r.tx;b->textaligny=r.ty;b->font=UI_RegisterFont(JAR_MENU_FONT);
 if(r.fill){const vec4_t panel={JAR_MENU_PANEL_COLOR},card={JAR_MENU_CARD_COLOR},status={JAR_MENU_STATUS_COLOR};b->window.style=WINDOW_STYLE_FILLED;b->window.flags|=WINDOW_BACKCOLORSET;memcpy(b->window.backColor,r.fill==1?panel:r.fill==2?card:status,sizeof(vec4_t));}return b;
}
static void SandboxOverlayMenus(){
 for(int tab=0;tab<JAR_MENU_TABS;++tab){
  const char*name=va("jarSpawnOverlay%d",tab);if(Menus_FindByName(name))continue;
  auto*m=Menu_CreateJAR(String_Alloc(name));if(!m)return;m->fullScreen=qfalse;m->window.style=WINDOW_STYLE_EMPTY;m->window.rect={0,0,640,480};
  const vec4_t focus={JAR_MENU_FOCUS_COLOR};memcpy(m->focusColor,focus,sizeof(vec4_t));m->onESC="uiScript jarOverlayClose ;";
  SandboxItem(m,JM_PANEL,0,"panel",nullptr,nullptr);SandboxItem(m,JM_TITLE,0,"title","SANDBOX",nullptr);
  SandboxItem(m,JM_HOLD,0,"hold","Hold Q to browse / release to play",nullptr);
  for(int t=0;t<JAR_MENU_TABS;++t){auto*b=SandboxItem(m,JM_TAB,t,va("tab%d",t),jarMenuTabs[t],String_Alloc(va("uiScript jarOverlayTab %d ;",t)));if(t==tab)memcpy(b->window.foreColor,focus,sizeof(vec4_t));}
  auto*search=SandboxItem(m,JM_SEARCH,0,"search","Search: ","");search->type=ITEM_TYPE_EDITFIELD;search->cvar="jar_spawnSearch";search->descText="Search this tab. Click to type, then release Q. Esc closes the menu.";
  auto*edit=static_cast<editFieldDef_t*>(UI_Alloc(sizeof(editFieldDef_t)));if(edit){memset(edit,0,sizeof(*edit));edit->maxChars=95;edit->maxPaintChars=23;search->typeData=edit;}
  SandboxItem(m,JM_CLEAR,0,"clearSearch","X","uiScript jarOverlaySearch clear ;");SandboxItem(m,JM_TOOLS,0,"toolsTitle","TOOLS",nullptr);
  for(int slot=0;slot<sandboxPageSize;++slot){SandboxItem(m,JM_CARD,slot,va("card%d",slot),sandboxCardLabels[tab][slot],String_Alloc(va("uiScript jarOverlaySpawn %d ;",slot)));SandboxItem(m,JM_ICON,slot,va("icon%d",slot),nullptr,nullptr)->window.style=WINDOW_STYLE_SHADER;}
  SandboxItem(m,JM_PREVIOUS,0,"pagePrevious","< Previous","uiScript jarOverlayPage previous ;");SandboxItem(m,JM_PAGE,0,"pageLabel",sandboxPageLabels[tab],nullptr);SandboxItem(m,JM_NEXT,0,"pageNext","Next >","uiScript jarOverlayPage next ;");
  for(int i=0;i<JAR_MENU_TOOLS+2;++i){const char*label=i<2?(i?"Toolgun":"Physgun"):jarMenuTools[i-2].name;
   const char*action=i<2?String_Alloc(va("uiScript jarOverlayGrant %s ;",i?"toolgun":"physics_gun")):String_Alloc(va("uiScript jarOverlayMode %s ;",jarMenuTools[i-2].command));
   SandboxItem(m,JM_TOOL,i,va("tool%d",i),label,action)->descText=label;
  }
  SandboxItem(m,JM_PROPERTIES,0,"toolProperties",sandboxToolTitle,nullptr);
  for(int i=0;i<5;++i)SandboxItem(m,JM_DETAIL,i,va("toolDetail%d",i),sandboxToolDetails[i],nullptr);
  SandboxItem(m,JM_VALUE,0,"toolValue",sandboxToolValue,nullptr);SandboxItem(m,JM_LESS,0,"propertyLess","-","uiScript jarOverlayProperty down ;");SandboxItem(m,JM_MORE,0,"propertyMore","+","uiScript jarOverlayProperty up ;");
  auto*direction=SandboxItem(m,JM_THRUST_DIRECTION,0,"thrustDirection",JAR_THRUSTER_DIRECTION_IN,"uiScript jarOverlayProperty reverse ;");
  direction->descText="Click to change the direction of new thrusters.";
  direction->window.flags&=~WINDOW_VISIBLE;
  SandboxItem(m,JM_UNDO,0,"undo","Undo (Z)","uiScript jarOverlayCommand undo ;");SandboxItem(m,JM_CLEANUP,0,"cleanup","Despawn All","uiScript jarOverlayCommand cleanup ;");SandboxItem(m,JM_PHYSICS,0,"physics",sandboxPhysicsLabel,"uiScript jarOverlayPhysics toggle ;");
  SandboxItem(m,JM_COUNT,0,"count",sandboxOverlayCount,nullptr);SandboxItem(m,JM_STATUS,0,"status",sandboxOverlayStatus,nullptr);
  Menu_PostParse(m);SandboxOverlayPage(tab);
 }
}
void UI_JAR_OverlayHeld(bool down){
 if(!down){if(UI_JAR_OverlayTyping())sandboxPinned=true;if(!sandboxContextOpen&&!sandboxPinned)UI_JAR_OverlayClose();return;}
 if(sandboxOverlayOpen || std::strcmp(Cvar_VariableString("sv_running"),"1") || std::strcmp(Cvar_VariableString("jar_sandbox"),"1") || UI_JAR_DisplayTrialActive())return;
 SandboxOverlayMenus();Menus_CloseAll();sandboxOverlayOpen=true;
 uiInfo.uiDC.cursorShow=qtrue;
 Cvar_Set("cl_paused","0");ui.Key_SetCatcher(KEYCATCH_UI);
 SandboxOverlayPage(sandboxTab);Menus_ActivateByName(va("jarSpawnOverlay%d",sandboxTab));
 JAR_Log(1,"sandbox","overlay_open","tab=%d paused=%s",sandboxTab,Cvar_VariableString("cl_paused"));
}
static void SandboxOverlayUpdate(){
 if(!sandboxOverlayOpen)return;
 if(UI_JAR_OverlayTyping())sandboxPinned=true;
 if(std::strcmp(Cvar_VariableString("sv_running"),"1") || std::strcmp(Cvar_VariableString("jar_sandbox"),"1") || !SandboxOverlayFocused() || !(ui.Key_GetCatcher()&KEYCATCH_UI)) {UI_JAR_OverlayClose();return;}
 if(sandboxContextOpen){ContextUpdate();return;}
 if(auto*m=Menus_FindByName(va("jarSpawnOverlay%d",sandboxTab)))if(auto*b=Menu_FindItemByName(m,"hold"))b->text=sandboxPinned?"Typing unlocked / Esc to close":"Hold Q to browse / release to play";
 const char*search=Cvar_VariableString("jar_spawnSearch");if(std::strcmp(search,sandboxSearch)){Q_strncpyz(sandboxSearch,search,sizeof(sandboxSearch));for(int&i:sandboxPage)i=0;SandboxOverlayPage(sandboxTab);}
 const int mode=std::max(0,std::min(12,atoi(Cvar_VariableString("jar_toolMode"))));
 if(auto*m=Menus_FindByName(va("jarSpawnOverlay%d",sandboxTab)))for(int i=2;i<15;++i)if(auto*b=Menu_FindItemByName(m,va("tool%d",i))){
  VectorSet4(b->window.foreColor,i==mode+2?.35f:.9f,.9f,i==mode+2?1.f:.9f,1);}
 Q_strncpyz(sandboxToolTitle,jarMenuTools[mode].name,sizeof(sandboxToolTitle));
 Com_sprintf(sandboxToolDetails[0],80,"Left: %s",jarMenuTools[mode].left);Com_sprintf(sandboxToolDetails[1],80,"Right: %s",jarMenuTools[mode].right);
 Q_strncpyz(sandboxToolDetails[2],"Physgun: hold left to move",80);Q_strncpyz(sandboxToolDetails[3],"Wheel: distance / Right: freeze",80);Q_strncpyz(sandboxToolDetails[4],"Release left to fling",80);
 if(mode==10){Q_strncpyz(sandboxToolDetails[2],"Aim at a face to mount",80);Q_strncpyz(sandboxToolDetails[3],"Direction applies to new mounts",80);Q_strncpyz(sandboxToolDetails[4],"Visible jet points opposite thrust",80);}
 if(mode==5||mode==6){Q_strncpyz(sandboxToolDetails[2],mode==6?"Pivot / axis: first aimed face":"Keeps the current relative pose",80);Q_strncpyz(sandboxToolDetails[3],"Frozen / movers / world: anchors",80);Q_strncpyz(sandboxToolDetails[4],"C: select player, then Weld / Hinge",80);}
 if(mode==9)Com_sprintf(sandboxToolValue,80,"Rope slack: %.1fx",atof(Cvar_VariableString("jar_ropeSlack")));
 else if(mode==10)Com_sprintf(sandboxToolValue,80,"Thrust: %.0f N",atof(Cvar_VariableString("jar_thrusterForce")));
 else if(mode==12){
  const int role=atoi(Cvar_VariableString("jar_linkOutput"));
  Q_strncpyz(sandboxToolValue,role==JAR_DISTANCE_SUBJECT?"Distance input: NPC":role==JAR_DISTANCE_REFERENCE?"Distance input: Reference":role==JAR_DISTANCE_OUTSIDE?"Secondary: Reverse / Outside / Detection / Focus":"Primary: Forward / Inside / Aim",80);
 }
 else if(mode==8)Q_strncpyz(sandboxToolValue,Cvar_VariableString("jar_duplicateStatus"),80);
 else Q_strncpyz(sandboxToolValue,mode==1?"Rotation: 15 degrees":"",80);
 if(auto*m=Menus_FindByName(va("jarSpawnOverlay%d",sandboxTab)))for(const char*name:{"propertyLess","propertyMore"})if(auto*b=Menu_FindItemByName(m,name)){
  if(mode==9||mode==10||mode==12)b->window.flags|=WINDOW_VISIBLE;else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);}
 if(auto*m=Menus_FindByName(va("jarSpawnOverlay%d",sandboxTab)))if(auto*b=Menu_FindItemByName(m,"thrustDirection")){
  if(mode==10){b->window.flags|=WINDOW_VISIBLE;b->text=atoi(Cvar_VariableString("jar_thrusterReverse"))?JAR_THRUSTER_DIRECTION_OUT:JAR_THRUSTER_DIRECTION_IN;}
  else b->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
 }
 Q_strncpyz(sandboxPhysicsLabel,atoi(Cvar_VariableString("jar_box3d"))==1?"Box3D: On":"Box3D: Off",sizeof(sandboxPhysicsLabel));
  Q_strncpyz(sandboxOverlayStatus,Cvar_VariableString("jar_workshopStatus"),sizeof(sandboxOverlayStatus));
 Q_strncpyz(sandboxOverlayCount,Cvar_VariableString("jar_sandboxCount"),sizeof(sandboxOverlayCount));
}
static qboolean SandboxOverlayScript(const char*name,const char**args){
 if(ContextScript(name,args))return qtrue;
 if(!Q_stricmp(name,"jarOverlayClose")){UI_JAR_OverlayClose();return qtrue;}
 if(Q_strncmp(name,"jarOverlay",10))return qfalse;
 const char*value;if(!String_Parse(args,&value) || !sandboxOverlayOpen)return qtrue;
 if(!Q_stricmp(name,"jarOverlaySearch")){if(!std::strcmp(value,"clear")){Cvar_Set("jar_spawnSearch","");sandboxSearch[0]=0;for(int&i:sandboxPage)i=0;SandboxOverlayPage(sandboxTab);}return qtrue;}
 if(!Q_stricmp(name,"jarOverlayTab")){int tab;if(SandboxIndex(value,JAR_MENU_TABS-1,tab)){sandboxTab=tab;SandboxOverlayPage(tab);Menus_CloseAll();Menus_ActivateByName(va("jarSpawnOverlay%d",sandboxTab));}return qtrue;}
 if(!Q_stricmp(name,"jarOverlayPage")){
  if(!std::strcmp(value,"next"))++sandboxPage[sandboxTab];else if(!std::strcmp(value,"previous"))--sandboxPage[sandboxTab];else return qtrue;
  SandboxOverlayPage(sandboxTab);return qtrue;
 }
 char command[128]={};
 if(!Q_stricmp(name,"jarOverlayPhysics")){
   if(std::strcmp(value,"toggle"))return qtrue;
   Com_sprintf(command,sizeof(command),"jar_sb physics %s\n",atoi(Cvar_VariableString("jar_box3d"))==1?"off":"on");
  }else if(!Q_stricmp(name,"jarOverlaySpawn")){int slot;if(!SandboxIndex(value,sandboxPageSize-1,slot))return qtrue;const int card=sandboxSlots[sandboxTab][slot];if(card<0)return qtrue;
  const auto&entry=sandboxCards[card];
  SandboxCardAction(slot,false);return qtrue;
 }else if(!Q_stricmp(name,"jarOverlayGrant")){
  if(std::strcmp(value,"physics_gun") && std::strcmp(value,"toolgun"))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb give %s\njar_sb equip %s\n",value,value);
 }else if(!Q_stricmp(name,"jarOverlayMode")){
  if(std::strcmp(value,"select") && std::strcmp(value,"rotate") && std::strcmp(value,"freeze") && std::strcmp(value,"release") && std::strcmp(value,"remove") && std::strcmp(value,"weld") && std::strcmp(value,"hinge") && std::strcmp(value,"unlink") && std::strcmp(value,"duplicate") && std::strcmp(value,"rope") && std::strcmp(value,"thruster") && std::strcmp(value,"activate") && std::strcmp(value,"link"))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb give toolgun\njar_sb mode %s\njar_sb equip toolgun\n",value);
 }else if(!Q_stricmp(name,"jarOverlayProperty")){
  const int mode=atoi(Cvar_VariableString("jar_toolMode"));
  if(!std::strcmp(value,"reverse")){
   if(mode==10)Cvar_Set("jar_thrusterReverse",atoi(Cvar_VariableString("jar_thrusterReverse"))?"0":"1");
   return qtrue;
  }
  const int direction=!std::strcmp(value,"up")?1:!std::strcmp(value,"down")?-1:0;if(!direction)return qtrue;
  if(mode==9)Cvar_Set("jar_ropeSlack",va("%.1f",std::max(1.,std::min(3.,atof(Cvar_VariableString("jar_ropeSlack"))+.1*direction))));
  if(mode==10)Cvar_Set("jar_thrusterForce",va("%.0f",std::max(100.,std::min(100000.,atof(Cvar_VariableString("jar_thrusterForce"))+1000*direction))));
  if(mode==12)Cvar_Set("jar_linkOutput",direction>0?"1":"0");
  return qtrue;
 }else if(!Q_stricmp(name,"jarOverlayCommand")){
  if(std::strcmp(value,"undo") && std::strcmp(value,"cleanup"))return qtrue;
  Com_sprintf(command,sizeof(command),"jar_sb %s\n",value);
 }else return qfalse;
 ui.Cmd_ExecuteText(EXEC_APPEND,command);return qtrue;
}
