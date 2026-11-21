// JA:R MP menus use native widgets, input, server browser and video/audio flows.
#include "ui_local.h"
#include "ui_jar_frontend.h"
#include "qcommon/jar_host_port.h"
#include "qcommon/jar_ui_palette.h"
#include "qcommon/jar_ui_cursor.h"
#include "qcommon/jar_console_style.h"
#include "qcommon/jar_interface_choices.h"
#include "qcommon/jar_classic_graphics.h"
#include "qcommon/jar_display.h"
#include "qcommon/jar_effect_choices.h"
#include "qcommon/jar_combat.h"
#include "game/jar_saber_rgb.h"
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
static const vec4_t jarBackground=JAR_UI_BACKGROUND,jarPanel=JAR_UI_PANEL,jarEdge=JAR_UI_EDGE,jarText=JAR_UI_TEXT,jarMuted=JAR_UI_MUTED,jarAccent=JAR_UI_ACCENT;
static char jarReturn[64]="main",jarStatus[192],controlsReturn[64];
static int jarFont;static qhandle_t jarCursor;
// Only replace known frame artwork. Informative images (Siege objectives,
// Force/weapon icons, gamma ramps and map previews) keep their native shader.
static const char*chromeShaders[]={
 "gfx/menus/main_background","gfx/menus/main_centerblue","gfx/menus/main_centerwindow","gfx/menus/main_leftwindow","gfx/menus/main_rightwindow","gfx/menus/main_ring",
 "gfx/menus/menu_side_text","gfx/menus/menu_side_text_right","gfx/menus/menu_boxes_left","gfx/menus/menu_boxes_right","gfx/menus/menu_top_mp","gfx/menus/menu_box_ingame","gfx/menus/menu_boxred",
 "gfx/menus/menu_blendbox","gfx/menus/menu_blendbox2","gfx/menus/menu_blendbox3","gfx/menus/menu_blendbox_extended","gfx/menus/menu_blendboxr","gfx/menus/menu_buttonback",
 "gfx/menus/charmenu_bottom","gfx/menus/sabermenu_box","gfx/menus/sabermenu_box_bottom","gfx/menus/sabermenu_box_middle","gfx/menus/sabermenu_box_top"
};
static qhandle_t chromeHandles[ARRAY_LEN(chromeShaders)];
static qboolean Chrome(qhandle_t shader){if(shader)for(int n=0;n<ARRAY_LEN(chromeHandles);++n)if(shader==chromeHandles[n])return qtrue;return qfalse;}
extern void UI_LoadNonIngame(void);
static void Color(const float*from,float*to){memcpy(to,from,sizeof(vec4_t));}
int UI_JAR_Font(void){return jarFont?jarFont:uiInfo.uiDC.Assets.qhMediumFont;}
static void Rect(itemDef_t*i,float x,float y,float w,float h){if(!i)return;i->window.rectClient=(rectDef_t){x,y,w,h};memset(&i->textRect,0,sizeof(i->textRect));}
static void Button(itemDef_t*i){if(!i)return;i->window.style=WINDOW_STYLE_FILLED;i->window.flags|=WINDOW_BACKCOLORSET;Color(jarPanel,i->window.backColor);Color(jarEdge,i->window.borderColor);i->window.border=WINDOW_BORDER_FULL;i->window.borderSize=1;i->textalignment=ITEM_ALIGN_LEFT;i->textalignx=10;i->textaligny=7;}
static itemDef_t*Add(menuDef_t*m,const char*name,const char*text,float x,float y,float w,float h,const char*action){
 if(!m)return NULL;itemDef_t*i=Menu_FindItemByName(m,name);if(i)return i;if(m->itemCount>=MAX_MENUITEMS)return NULL;
 i=UI_Alloc(sizeof(*i));if(!i)return NULL;Item_Init(i);i->parent=m;i->window.name=String_Alloc(name);i->window.flags=WINDOW_VISIBLE|WINDOW_FORECOLORSET;i->type=action?ITEM_TYPE_BUTTON:ITEM_TYPE_TEXT;i->text=text?String_Alloc(text):NULL;i->iMenuFont=5;i->textscale=.8f;i->textStyle=ITEM_TEXTSTYLE_SHADOWED;i->textaligny=2;i->action=action?String_Alloc(action):NULL;Color(jarText,i->window.foreColor);Rect(i,x,y,w,h);if(action)Button(i);else i->window.flags|=WINDOW_DECORATION;m->items[m->itemCount++]=i;return i;
}
static void Scale(itemDef_t*i,float scale){if(i)i->textscale=scale;}
static void Line(menuDef_t*m,const char*name,float y){itemDef_t*i=Add(m,name,NULL,32,y,576,1,NULL);if(i){i->window.style=WINDOW_STYLE_FILLED;i->window.flags|=WINDOW_BACKCOLORSET;Color(jarEdge,i->window.backColor);}}
static void Base(menuDef_t*m){
 m->window.style=WINDOW_STYLE_FILLED;m->window.background=0;m->window.flags|=WINDOW_BACKCOLORSET;Color(jarBackground,m->window.backColor);Color(jarAccent,m->focusColor);Color(jarMuted,m->descColor);m->appearanceIncrement=0;
 for(int n=0;n<m->itemCount;++n){itemDef_t*i=m->items[n];if(i->type==ITEM_TYPE_MODEL)continue;
  if((i->window.flags&WINDOW_DECORATION)&&i->window.name&&strstr(i->window.name,"_undertext"))i->text=NULL;
  if(!i->window.ownerDraw&&Chrome(i->window.background)){i->window.background=0;i->window.style=WINDOW_STYLE_FILLED;i->window.flags|=WINDOW_BACKCOLORSET;Color(jarPanel,i->window.backColor);i->window.border=WINDOW_BORDER_NONE;}
  if(i->text||i->cvar||i->type==ITEM_TYPE_MULTI){i->iMenuFont=5;Color(jarText,i->window.foreColor);if(i->textscale>.8f)i->textscale=.8f;i->window.flags|=WINDOW_FORECOLORSET;i->textStyle=ITEM_TEXTSTYLE_SHADOWED;}
  Color(jarEdge,i->window.borderColor);Color(jarAccent,i->window.outlineColor);
 }
}
static menuDef_t*Page(const char*name,const char*title,const char*subtitle){
 menuDef_t*m=Menu_CreateJAR(name);if(!m||m->itemCount)return m;m->fullScreen=qtrue;Base(m);m->window.rect=(rectDef_t){0,0,640,480};
 itemDef_t*i=Add(m,"jarTitle",title,32,30,576,36,NULL);if(i)i->textscale=1.5f;i=Add(m,"jarSubtitle",subtitle,34,74,572,22,NULL);if(i){i->textscale=.75f;Color(jarMuted,i->window.foreColor);}Line(m,"jarHeader",108);Line(m,"jarFooter",430);
 m->descX=32;m->descY=412;m->descScale=.65f;m->descAlignment=ITEM_ALIGN_LEFT;m->onESC=String_Alloc("uiScript jarBack ;");return m;
}
static itemDef_t*Field(menuDef_t*m,const char*id,const char*label,const char*cvar,float y,int maxChars,qboolean number){
 itemDef_t*i=Add(m,id,label,32,y,330,28," ");if(!i)return NULL;i->type=number?ITEM_TYPE_NUMERICFIELD:ITEM_TYPE_EDITFIELD;i->cvar=String_Alloc(cvar);i->typeData.edit=UI_Alloc(sizeof(editFieldDef_t));if(!i->typeData.edit)return NULL;memset(i->typeData.edit,0,sizeof(editFieldDef_t));i->typeData.edit->maxChars=maxChars;i->typeData.edit->maxPaintChars=maxChars>26?26:maxChars;i->textscale=.7f;return i;
}
static void Toggle(menuDef_t*m,const char*id,const char*label,const char*cvar,float x,float y,float w){itemDef_t*i=Add(m,id,label,x,y,w,30," ");if(i){i->type=ITEM_TYPE_YESNO;i->cvar=String_Alloc(cvar);i->textscale=.7f;}}
typedef struct {const char*label,*value;} jarMenuChoice_t;
static void Choice(menuDef_t*m,const char*id,const char*label,const char*cvar,float y,const jarMenuChoice_t*choices,int count){
 if(count<1||count>MAX_MULTI_CVARS)return;multiDef_t*multi=UI_Alloc(sizeof(*multi));if(!multi)return;itemDef_t*i=Add(m,id,label,32,y,480,30," ");if(!i)return;
 memset(multi,0,sizeof(*multi));multi->count=count;multi->strDef=qtrue;for(int n=0;n<count;++n){multi->cvarList[n]=String_Alloc(choices[n].label);multi->cvarStr[n]=String_Alloc(choices[n].value);}
 i->type=ITEM_TYPE_MULTI;i->cvar=String_Alloc(cvar);i->typeData.multi=multi;i->textscale=.7f;
}
static void SettingsTabs(menuDef_t*m){
 itemDef_t*subtitle=Menu_FindItemByName(m,"jarSubtitle");if(subtitle)subtitle->text=NULL;
 const char*ids[]={"camera","console","interface","graphics","textures","detail","display","effects","combat"};const char*labels[]={"Camera","Console","Interface","Graphics","Textures","Detail","Display","Effects","Combat"};
 for(int n=0;n<ARRAY_LEN(ids);++n)Scale(Add(m,va("%sTab",ids[n]),labels[n],32+n*64,74,60,24,va("uiScript jarSettingsPage %s ;",ids[n])),.5f);
}
static qboolean Active(void){uiClientState_t s={0};trap->GetClientState(&s);return s.connState==CA_ACTIVE;}
static void Open(const char*name){Menus_CloseAll();Menus_ActivateByName(name);trap->Key_SetCatcher(trap->Key_GetCatcher()|KEYCATCH_UI);}
static void Remember(void){menuDef_t*m=Menu_GetFocused();if(m&&m->window.name&&(!Q_stricmp(m->window.name,"jarMPPause")||Q_stricmpn(m->window.name,"jarMP",5)))Q_strncpyz(jarReturn,m->window.name,sizeof(jarReturn));else if(!m)Q_strncpyz(jarReturn,Active()?"jarMPPause":"main",sizeof(jarReturn));}
static void Value(const char*name,char*out,int size){trap->Cvar_VariableStringBuffer(name,out,size);}
static qboolean Number(const char*name,float lo,float hi,qboolean integer,float*out){char b[64],*end;Value(name,b,sizeof(b));float n=strtof(b,&end);if(!b[0]||*end||!isfinite(n)||n<lo||n>hi||(integer&&n!=floorf(n)))return qfalse;*out=n;return qtrue;}

static void LayoutControls(menuDef_t *menu)
{
 if(!menu||Menu_FindItemByName(menu,"jarControlsBack"))return;
 // Preserve the retail binding widgets' screen positions when expanding the
 // in-game popup into a standalone page.
 if(!menu->fullScreen){
  const float border=menu->window.border?menu->window.borderSize:0;
  for(int i=0;i<menu->itemCount;++i){
   menu->items[i]->window.rectClient.x+=menu->window.rect.x+border;
   menu->items[i]->window.rectClient.y+=menu->window.rect.y+border;
  }
  menu->fullScreen=qtrue;
  menu->window.border=WINDOW_BORDER_NONE;
  menu->window.flags&=~(WINDOW_OOB_CLICK|WINDOW_POPUP);
 }
 menu->onESC=String_Alloc("uiScript jarControlsBack ;");
 Add(menu,"jarControlsBack","Back",32,440,156,32,menu->onESC);
 Menu_PostParse(menu);
}

static void OpenControls(void)
{
 const qboolean active=Active();
 const char *dest=active?"ingame_controls":"controlsMenu";
 menuDef_t *parent=Menu_GetFocused();
 Q_strncpyz(controlsReturn,parent&&parent->window.name?parent->window.name:active?"jarMPPause":"main",sizeof(controlsReturn));
 // Custom menu lists can omit the standard Controls page.
 if(!Menus_FindByName(dest)){
  UI_LoadJARControls(active);
  menuDef_t *menu=Menus_FindByName(dest);
  if(!menu)return;
  Base(menu);
  LayoutControls(menu);
 }
 Open(dest);
}

#define JAR_DEFINE_GFX_CHOICES(id,name,label,initial,restart,page,choices) static const jarMenuChoice_t gfx_##id[]=choices;
JAR_CLASSIC_GRAPHICS_ROWS(JAR_DEFINE_GFX_CHOICES)
#undef JAR_DEFINE_GFX_CHOICES
static const struct{const char*name;const char*value;float low,high;int integer;const char*asset;const jarMenuChoice_t*choices;int count;int restart;} clientSettings[]={
 {"jar_cameraPreset","0",0,2,1},{"cg_fov","80",40,140,0},{"cg_thirdPersonRange","80",0,512,0},{"cg_thirdPersonVertOffset","16",-128,256,0},{"cg_thirdPerson","0",0,1,1},{"cg_fovAspectAdjust","0",0,1,1},
 {"jar_consoleStyle","0",0,3,1},{"con_opacity","1",.25f,1,0},{"con_scale","1",1,4,0},{"con_height","0.5",.25f,1,0},
 {"jar_saberExtension","1",0,1,1},{"jar_continuousFists","1",0,1,1},
 {"jar_facePreset","0",0,3,1},{"jar_faceSensitivity","1",.5f,2,0},
 {"r_cubeMapping","0",0,1,1,NULL,NULL,0,1},
 {"jar_saberPack","stock",0,0,0,"saber"},{"jar_soundPack","stock",0,0,0,"sound"},{"jar_fxPack","stock",0,0,0,"fx"},
 {"jar_fontPack","sans",0,0,0,"font"},{"jar_uiPack","up2",0,0,0,"ui"},{"r_fontSharpness","1",1,4,0},{"jar_uiMotion","1",0,1,1},{"cg_hudFiles","ui/jahud.txt",0,0,0,"hud"},
#define JAR_GFX_SETTING(id,name,label,initial,restart,page,choices) {name,initial,0,0,0,NULL,gfx_##id,ARRAY_LEN(gfx_##id),restart},
 JAR_CLASSIC_GRAPHICS_ROWS(JAR_GFX_SETTING)
#undef JAR_GFX_SETTING
};
static char originals[ARRAY_LEN(clientSettings)][64];
static const char*displayNames[]=JAR_DISPLAY_NAMES;
static char displayOriginals[5][MAX_STRING_CHARS],displayPrompt[192];
static int DisplayState(void){char b[16];Value("ui_jarDisplayState",b,sizeof(b));return atoi(b);}
static void ReadSettings(void){
 for(int n=0;n<ARRAY_LEN(clientSettings);++n){Value(clientSettings[n].name,originals[n],sizeof(originals[n]));trap->Cvar_Set(va("ui_jarMP_%s",clientSettings[n].name),originals[n]);}
 // Engine readback preserves existing native latches; registering a display
 // cvar here would consume that pending value before a video restart.
 trap->Cmd_ExecuteText(EXEC_NOW,"jar_display_read\n");
 for(int n=0;n<5;++n)Value(va("ui_jarMP_%s",displayNames[n]),displayOriginals[n],sizeof(displayOriginals[n]));
 jarStatus[0]=0;
}
static void DisplayPages(void){
 menuDef_t*m=Menus_FindByName("jarMPDisplay");if(!m){m=Page("jarMPDisplay","JA:R SETTINGS","MULTIPLAYER");if(m){SettingsTabs(m);
  static const jarMenuChoice_t resolution[]=JAR_RESOLUTION_CHOICES,window[]=JAR_WINDOW_CHOICES,border[]=JAR_BORDER_CHOICES;
  Choice(m,"mode","Resolution: ","ui_jarMP_r_mode",130,resolution,ARRAY_LEN(resolution));
  Field(m,"width","Custom width: ","ui_jarMP_r_customwidth",170,5,qtrue);Field(m,"height","Custom height: ","ui_jarMP_r_customheight",210,5,qtrue);
  Choice(m,"window","Window mode: ","ui_jarMP_r_fullscreen",250,window,ARRAY_LEN(window));Choice(m,"border","Window border: ","ui_jarMP_r_noborder",290,border,ARRAY_LEN(border));
  Scale(Add(m,"displayHelp","Size edits choose Custom. Borderless fullscreen uses the desktop size.",32,336,576,20,NULL),.6f);
  Scale(Add(m,"displayTrial","Display changes revert after 20 seconds unless you choose Keep.",32,358,576,20,NULL),.6f);
  itemDef_t*i=Add(m,"status","",32,386,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}
  Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");Menu_PostParse(m);
 }}
 m=Menus_FindByName("jarMPDisplayConfirm");if(!m){m=Page("jarMPDisplayConfirm","KEEP THIS DISPLAY?","DISPLAY SETTINGS");if(m){
  itemDef_t*i=Add(m,"countdown","",32,172,576,32,NULL);if(i){i->text=displayPrompt;i->textscale=.75f;}
  Scale(Add(m,"hint","Choose Keep to accept, or Restore to return to your previous display.",32,226,576,24,NULL),.65f);
  Add(m,"restore","Restore",32,440,180,32,"uiScript jarDisplayRevert ;");Add(m,"keep","Keep",428,440,180,32,"uiScript jarDisplayKeep ;");m->onESC=String_Alloc("uiScript jarDisplayRevert ;");Menu_PostParse(m);
 }}
}

static qboolean SelectionAvailable(const char*category,const char*pack){
 if(!strcmp(category,"saber")||!strcmp(category,"sound")||!strcmp(category,"fx")){
  if(!JAR_BuiltinEffectPack(category,pack))return qfalse;if(!strcmp(pack,"stock"))return qtrue;
  fileHandle_t file=0;int bytes=trap->FS_Open(va("jar/%s/%s/pack.txt",category,pack),&file,FS_READ);if(file)trap->FS_Close(file);return bytes>0&&file;
 }
 if(!strcmp(category,"hud")){
  static const jarMenuChoice_t choices[]=JAR_HUD_CHOICES;qboolean valid=qfalse;
  for(int n=0;n<ARRAY_LEN(choices);++n)if(!strcmp(pack,choices[n].value))valid=qtrue;
  if(!valid)return qfalse;
  fileHandle_t file=0;int bytes=trap->FS_Open("ui/jahud.txt",&file,FS_READ);if(file)trap->FS_Close(file);return bytes>0&&file;
 }
 if(!JAR_BuiltinInterfacePack(category,pack))return qfalse;
 if(!strcmp(pack,"stock"))return qtrue;
 const char*probes[]={"fonts/arialnb.fontdat","fonts/arialnb.tga","ui/assets/gradientbar2.tga"};
 const int font=!strcmp(category,"font");
 for(int n=font?0:2;n<(font?2:3);++n){fileHandle_t file=0;int bytes=trap->FS_Open(va("jar/%s/%s/%s",category,pack,probes[n]),&file,FS_READ);if(file)trap->FS_Close(file);if(bytes<=0||!file)return qfalse;}
 return qtrue;
}
static void CombatPage(void){
 if(Menus_FindByName("jarMPCombat"))return;menuDef_t*m=Page("jarMPCombat","JA:R SETTINGS","MULTIPLAYER");if(!m)return;SettingsTabs(m);
 static const jarMenuChoice_t extension[]=JAR_SABER_EXTENSION_CHOICES,fists[]=JAR_FIST_INPUT_CHOICES;
 Choice(m,"saberExtension","Saber extension: ","ui_jarMP_jar_saberExtension",140,extension,ARRAY_LEN(extension));
 Choice(m,"fistInput","Fist input: ","ui_jarMP_jar_continuousFists",190,fists,ARRAY_LEN(fists));
 Scale(Add(m,"extensionHelp","Quick ignites your held blades in 250 ms. Reach stays unchanged.",32,252,576,24,NULL),.6f);
 Scale(Add(m,"fistHelp","Continuous punches follow each animation. Release ends the sequence.",32,288,576,24,NULL),.6f);
 Scale(Add(m,"legacyHelp","Legacy keeps native MP timing. Kicks and grabs keep server rules.",32,324,576,24,NULL),.6f);
 itemDef_t*i=Add(m,"status","",32,382,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");Menu_PostParse(m);
}
static void EffectsPage(void){
 if(Menus_FindByName("jarMPEffects"))return;menuDef_t*m=Page("jarMPEffects","JA:R SETTINGS","MULTIPLAYER");if(!m)return;SettingsTabs(m);
 static const jarMenuChoice_t face[]=JAR_FACE_PRESET_CHOICES,sensitivity[]=JAR_FACE_SENSITIVITY_CHOICES;
 Choice(m,"facePreset","Facial expressions: ","ui_jarMP_jar_facePreset",130,face,ARRAY_LEN(face));Choice(m,"faceSensitivity","Speech sensitivity: ","ui_jarMP_jar_faceSensitivity",170,sensitivity,ARRAY_LEN(sensitivity));
 static const jarMenuChoice_t sabers[]=JAR_SABER_PACK_CHOICES,sounds[]=JAR_SOUND_PACK_CHOICES,effects[]=JAR_FX_PACK_CHOICES;
 Choice(m,"saberPack","Saber blades: ","ui_jarMP_jar_saberPack",210,sabers,ARRAY_LEN(sabers));
 Choice(m,"soundPack","Saber sounds: ","ui_jarMP_jar_soundPack",250,sounds,ARRAY_LEN(sounds));
 Choice(m,"fxPack","Combat effects: ","ui_jarMP_jar_fxPack",290,effects,ARRAY_LEN(effects));
 Scale(Add(m,"effectsHelp","Face presets apply live. Stock restores native facial movement.",32,336,576,20,NULL),.65f);
 Scale(Add(m,"effectsReload","Packs reload visuals; changing saber sounds also restarts audio.",32,358,576,20,NULL),.6f);
 itemDef_t*i=Add(m,"status","",32,390,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");Menu_PostParse(m);
}
static void GraphicsPages(void){
 const char*names[]={"jarMPGraphics","jarMPTextures","jarMPDetail"};const char*help[]={"MSAA and glow changes reload video on Apply.","Texture detail, anisotropy and curve changes reload video on Apply.","These choices apply live. Existing impact marks fade normally."};
 static const struct{const char*name,*label;int page;const jarMenuChoice_t*choices;int count;} entries[]={
#define JAR_GFX_ENTRY(id,name,label,initial,restart,page,choices) {name,label,page,gfx_##id,ARRAY_LEN(gfx_##id)},
 JAR_CLASSIC_GRAPHICS_ROWS(JAR_GFX_ENTRY)
#undef JAR_GFX_ENTRY
 };
 for(int page=0;page<3;++page){if(Menus_FindByName(names[page]))continue;menuDef_t*m=Page(names[page],"JA:R SETTINGS","MULTIPLAYER");if(!m)continue;SettingsTabs(m);int row=0;
  for(int n=0;n<ARRAY_LEN(entries);++n)if(entries[n].page==page){char label[80],key[96];Com_sprintf(label,sizeof(label),"%s: ",entries[n].label);Com_sprintf(key,sizeof(key),"ui_jarMP_%s",entries[n].name);Choice(m,entries[n].name,label,key,130+row++*40,entries[n].choices,entries[n].count);}
  char renderer[MAX_QPATH];Value("ui_jarRendererActive",renderer,sizeof(renderer));
  const qboolean cubemaps=!page&&!strcmp(renderer,"rdjar-mp-rend2");
  if(cubemaps){static const jarMenuChoice_t choices[]=JAR_GFX_R_DYNAMICGLOW_CHOICES;
   Choice(m,"cubemapReflections","Cubemap reflections: ","ui_jarMP_r_cubeMapping",326,choices,ARRAY_LEN(choices));
  }
  Scale(Add(m,"graphicsHelp",cubemaps?"MSAA, glow and reflection changes reload video on Apply.":help[page],32,cubemaps?362:344,576,20,NULL),.6f);
  itemDef_t*i=Add(m,"status","",32,380,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}
  Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");Menu_PostParse(m);
 }
}
static const struct{const char*pending;const char*name;const char*value;float low,high;} hostSettings[]={
 {"ui_jarHostPlayers","sv_maxclients","8",2,32},{"ui_jarHostShared","jar_sharedEditing","1",0,1},{"ui_jarHostCleanup","jar_cleanupDisconnect","0",0,1},{"ui_jarHostPlayerTools","jar_playerTools","1",0,1},{"ui_jarHostMapEditing","jar_mapActorEditing","1",0,1},{"ui_jarHostWorldEditing","jar_worldEditing","1",0,1},{"ui_jarHostAutoLoad","jar_worldAutoLoad","1",0,1},{"ui_jarHostAutoSave","jar_worldAutoSave","120",0,3600}
};
#include "ui_jar_host_maps.inc"

static void StartHost(void){
 char map[MAX_QPATH],host[128],portText[16],portStatus[8];
 float persistent,cheats,values[ARRAY_LEN(hostSettings)];
 if(Active())return;
 Value("ui_jarHostMap",map,sizeof(map));Value("ui_jarHostName",host,sizeof(host));
 Value("ui_jarHostPort",portText,sizeof(portText));
 const int port=JAR_HostPort(portText);
 if(!port){Q_strncpyz(jarStatus,"Enter a UDP port from 1 to 65535.",sizeof(jarStatus));return;}
 if(!Number("ui_jarHostCheats",0,1,qtrue,&cheats)){Q_strncpyz(jarStatus,"Choose whether to allow cheats.",sizeof(jarStatus));return;}
 if(!UI_JAR_HostMapNameValid(map)){Q_strncpyz(jarStatus,"Enter a map name such as mp/ffa3, without .bsp.",sizeof(jarStatus));return;}
 if(!host[0]||strlen(host)>48){Q_strncpyz(jarStatus,"Server name must contain 1-48 characters.",sizeof(jarStatus));return;}
 for(const char*p=host;*p;++p)if((unsigned char)*p<32||*p=='"'||*p==';'||*p=='\\'){Q_strncpyz(jarStatus,"Use a server name without quotes or command punctuation.",sizeof(jarStatus));return;}
 for(int n=0;n<ARRAY_LEN(hostSettings);++n)if(!Number(hostSettings[n].pending,hostSettings[n].low,hostSettings[n].high,qtrue,&values[n])){Q_strncpyz(jarStatus,"Check players (2-32), autosave (0-3600) and toggles.",sizeof(jarStatus));return;}
 if(!Number("ui_jarHostPersistent",0,1,qtrue,&persistent)){Q_strncpyz(jarStatus,"Choose Keep world on or off.",sizeof(jarStatus));return;}
 const jarHostMapStatus_t status=UI_JAR_InspectHostMap(trap,map,NULL,0,NULL);
 if(status!=JAR_HOST_MAP_PLAYABLE){
  const char* message=status==JAR_HOST_MAP_MISSING?"That map is not installed.":
                      status==JAR_HOST_MAP_NO_START?"That map has no FFA player start.":"That map is not a readable Jedi Academy BSP.";
  Q_strncpyz(jarStatus,message,sizeof(jarStatus));return;
 }
 trap->Cvar_Set("ui_jarHostPortStatus","0");
 trap->Cmd_ExecuteText(EXEC_NOW,va("jar_host_port %d",port));
 Value("ui_jarHostPortStatus",portStatus,sizeof(portStatus));
 if(strcmp(portStatus,"1")){
  Com_sprintf(jarStatus,sizeof(jarStatus),"Could not open UDP port %d. Choose another port or check network settings.",port);
  return;
 }
 for(int n=0;n<ARRAY_LEN(hostSettings);++n)trap->Cvar_SetValue(hostSettings[n].name,values[n]);
 trap->Cvar_SetValue("jar_hostCheats",cheats);
 trap->Cvar_SetValue("ui_jarHostPort",port);
 if(values[1]==0)trap->Cvar_Set("jar_worldEditing","0");
 if(!persistent){trap->Cvar_Set("jar_cleanupDisconnect","1");trap->Cvar_Set("jar_worldAutoSave","0");trap->Cvar_Set("jar_worldAutoLoad","0");trap->Cvar_Set("jar_worldLoad","");}
 // This menu runs in the graphical client; standalone dedicated modes belong
 // to jar_server. Old archived server-type choices are deliberately ignored.
 trap->Cvar_Set("dedicated","0");
 trap->Cvar_Set("sv_hostname",host);trap->Cvar_Set("jar_sandbox","1");trap->Cvar_Set("g_gametype","0");trap->Cvar_Set("fraglimit","0");trap->Cvar_Set("timelimit","0");trap->Cvar_Set("g_doWarmup","0");trap->Cvar_Set("ui_singlePlayerActive","0");
 Menus_CloseAll();trap->Key_SetCatcher(trap->Key_GetCatcher()&~KEYCATCH_UI);trap->Cvar_Set("cl_paused","0");trap->Cmd_ExecuteText(EXEC_APPEND,va("map %s\n",map));
}
static void Hub(menuDef_t*m){
 if(!m||Menu_FindItemByName(m,"jarMainTitle"))return;
 itemDef_t*play=Menu_FindItemByName(m,"playbutton"),*controls=Menu_FindItemByName(m,"controlsbutton"),*setup=Menu_FindItemByName(m,"setupbutton"),*profile=Menu_FindItemByName(m,"profilebutton");
 if(!play||!controls||!setup||!profile){
  Add(m,"jarSettings","JA:R Settings",32,390,272,30,"uiScript jarOpen ;");
  if(!controls)controls=Add(m,"controlsbutton","Controls",336,390,272,30,"uiScript jarNative controls ;");
  if(controls)controls->action=String_Alloc("uiScript jarNative controls ;");
  Menu_PostParse(m);return;
 }
 Base(m);for(int n=0;n<m->itemCount;++n){itemDef_t*i=m->items[n];if(i->window.flags&WINDOW_DECORATION){i->text=NULL;i->window.background=0;i->window.cinematicName=NULL;i->window.style=WINDOW_STYLE_EMPTY;i->window.border=WINDOW_BORDER_NONE;}}
 itemDef_t*i=Add(m,"jarMainTitle","JEDI ACADEMY",32,32,576,36,NULL);if(i)i->textscale=1.5f;i=Add(m,"jarMainSubtitle","REMIXED  /  MULTIPLAYER",34,72,572,22,NULL);if(i)i->textscale=.75f;
 Line(m,"jarHeader",108);Add(m,"jarPlayGroup","PLAY TOGETHER",32,126,272,22,NULL);Add(m,"jarSettingsGroup","CONFIGURATION",336,126,272,22,NULL);
 Rect(play,32,156,272,44);play->text=String_Alloc("Play multiplayer");Button(play);play->textscale=.8f;play->mouseEnter=play->mouseExit=NULL;
 Add(m,"jarHost","Host JA:R Sandbox",32,214,272,44,"uiScript jarHost ;");Rect(profile,32,272,272,44);Button(profile);profile->textscale=.8f;profile->mouseEnter=profile->mouseExit=NULL;
 Add(m,"jarSettings","JA:R Settings",336,156,272,44,"uiScript jarOpen ;");Rect(controls,336,214,272,44);Button(controls);controls->action=String_Alloc("uiScript jarNative controls ;");controls->textscale=.8f;controls->mouseEnter=controls->mouseExit=NULL;
 Rect(setup,336,272,272,44);Button(setup);setup->textscale=.8f;setup->mouseEnter=setup->mouseExit=NULL;
 i=Add(m,"jarHint","Build together. JA:R Settings contains appearance and Sandbox controls.",32,352,576,22,NULL);if(i)i->textscale=.65f;Line(m,"jarFooter",430);
 i=Menu_FindItemByName(m,"exitgamebutton");Rect(i,452,440,156,32);Button(i);if(i){i->textscale=.8f;i->mouseEnter=i->mouseExit=NULL;}
 const char*order[]={"playbutton","jarHost","profilebutton","jarSettings","controlsbutton","setupbutton","exitgamebutton"};
 for(int n=0;n<ARRAY_LEN(order);++n){itemDef_t*item=Menu_FindItemByName(m,order[n]);if(!item)continue;int at=0;while(at<m->itemCount&&m->items[at]!=item)++at;for(int k=at;k<m->itemCount-1;++k)m->items[k]=m->items[k+1];m->items[m->itemCount-1]=item;}
 m->cursorItem=-1;Menu_PostParse(m);
}
void UI_JAR_LoadMenus(void){
 jarFont=trap->R_RegisterFont("arialnb");jarCursor=trap->R_RegisterShaderNoMip("jar/icons/cursor");
 for(int n=0;n<ARRAY_LEN(chromeShaders);++n)chromeHandles[n]=trap->R_RegisterShaderNoMip(chromeShaders[n]);
 for(int n=0;n<ARRAY_LEN(clientSettings);++n)trap->Cvar_Register(NULL,clientSettings[n].name,clientSettings[n].value,CVAR_ARCHIVE|(!strcmp(clientSettings[n].name,"jar_continuousFists")?CVAR_USERINFO:0));
 trap->Cvar_Register(NULL,"ui_jarHostName","JA:R Sandbox",CVAR_ARCHIVE);trap->Cvar_Register(NULL,"ui_jarHostMap","mp/ffa3",CVAR_ARCHIVE);
 trap->Cvar_Register(NULL,"ui_jarHostMapSearch","",0);
 for(int n=0;n<ARRAY_LEN(hostSettings);++n)trap->Cvar_Register(NULL,hostSettings[n].pending,hostSettings[n].value,CVAR_ARCHIVE);
 trap->Cvar_Register(NULL,"ui_jarHostPersistent","1",CVAR_ARCHIVE);
 char port[16];Value("net_port",port,sizeof(port));
 trap->Cvar_Register(NULL,"ui_jarHostPort",JAR_HostPort(port)?port:XSTRING(JAR_DEFAULT_HOST_PORT),CVAR_ARCHIVE);
 trap->Cvar_Register(NULL,"ui_jarHostCheats","0",CVAR_ARCHIVE);
 trap->Cvar_Register(NULL,"ui_jarHostDefaultsVersion","0",CVAR_ARCHIVE);
 char defaults[16];Value("ui_jarHostDefaultsVersion",defaults,sizeof(defaults));if(atoi(defaults)<1){trap->Cvar_Set("ui_jarHostShared","1");trap->Cvar_Set("ui_jarHostCleanup","0");trap->Cvar_Set("ui_jarHostWorldEditing","1");trap->Cvar_Set("ui_jarHostDefaultsVersion","1");}
 // Retain native widget positions, actions, feeders and conditional visibility.
 static const char*names[]={"advancedcreateserver","Connect","controlsMenu","createfavorite_popmenu","createserver","demo","error_popmenu","findplayer_popmenu","ingame","ingame_about","ingame_addbot","ingame_callvote","ingame_controls","ingame_join","ingame_leave","ingame_objectives","ingame_orders","ingame_player","ingame_player2","ingame_playerforce","ingame_saber","ingame_setup","ingame_siegeobjectives","ingame_voicechat","ingame_vote","joinserver","multiplayermenu","password_popmenu","password_request","playerMenu","playerMenu2","quickbots","quickgame","quickgame2","quitMenu","rulesMenu","rulesMenu_force","rulesMenu_games","rulesMenu_items","rulesMenu_moves","rulesMenu_weapons","saberMenu","serverinfo_popmenu","setup_menu","ingame_siegeclass","siege_popmenu","videodriverMenu","videowarningMenu"};
 for(int n=0;n<ARRAY_LEN(names);++n){menuDef_t*m=Menus_FindByName(names[n]);if(m&&!Menu_FindItemByName(m,"jarStyleMarker")){Base(m);itemDef_t*i=Add(m,"jarStyleMarker",NULL,0,0,0,0,NULL);if(i)i->window.flags=WINDOW_DECORATION;Menu_PostParse(m);}}
 LayoutControls(Menus_FindByName("controlsMenu"));
 LayoutControls(Menus_FindByName("ingame_controls"));
 menuDef_t *ingame=Menus_FindByName("ingame");
 itemDef_t *nativeControls=ingame?Menu_FindItemByName(ingame,"controls"):NULL;
 if(nativeControls)nativeControls->action=String_Alloc("uiScript jarNative controls ;");
 Hub(Menus_FindByName("main"));
 GraphicsPages();DisplayPages();EffectsPage();CombatPage();
 menuDef_t *blades=Menus_FindByName("jarMPBladeColors");
 if(!blades){
  blades=Page("jarMPBladeColors","SABER BLADE COLORS","MULTIPLAYER");
  if(blades){
   Field(blades,"primary","Right-hand RGB  ", "ui_jarBladeRGB1",158,7,qfalse);
   Field(blades,"secondary","Left-hand RGB  ", "ui_jarBladeRGB2",206,7,qfalse);
   Scale(Add(blades,"example","Enter #RRGGBB, or leave blank to use a stock color.",32,262,576,24,NULL),.65f);
   itemDef_t *status=Add(blades,"status","",32,390,576,28,NULL);
   if(status)status->text=jarStatus;
   Add(blades,"back","Back",32,440,156,32,"uiScript jarBladeBack ;");
   Add(blades,"apply","Apply",452,440,156,32,"uiScript jarApplyBladeRGB ;");
   blades->onESC=String_Alloc("uiScript jarBladeBack ;");
   Menu_PostParse(blades);
  }
 }
 menuDef_t*m=Menus_FindByName("jarMPSettings");if(!m){m=Page("jarMPSettings","JA:R SETTINGS","MULTIPLAYER  /  CAMERA AND CONTROLS");if(m){SettingsTabs(m);
 itemDef_t*mode=Add(m,"cameraPreset","Gameplay camera: ",32,128,330,28," ");if(mode){mode->type=ITEM_TYPE_MULTI;mode->cvar=String_Alloc("ui_jarMP_jar_cameraPreset");mode->textscale=.7f;multiDef_t*choices=UI_Alloc(sizeof(*choices));if(choices){memset(choices,0,sizeof(*choices));mode->typeData.multi=choices;choices->count=3;choices->strDef=qtrue;const char*labels[]={"Legacy / custom","Comfort","Crisp"};const char*values[]={"0","1","2"};for(int n=0;n<3;++n){choices->cvarList[n]=String_Alloc(labels[n]);choices->cvarStr[n]=String_Alloc(values[n]);}}}
 Field(m,"fov","Field of view: ","ui_jarMP_cg_fov",166,6,qfalse);Field(m,"range","Camera distance: ","ui_jarMP_cg_thirdPersonRange",204,6,qfalse);Field(m,"height","Camera height: ","ui_jarMP_cg_thirdPersonVertOffset",242,6,qfalse);
 Toggle(m,"third","Third person: ","ui_jarMP_cg_thirdPerson",32,280,330);Toggle(m,"aspect","Widescreen FOV: ","ui_jarMP_cg_fovAspectAdjust",32,318,330);
 Scale(Add(m,"cameraHelp","Comfort reduces bob/chase lag. Crisp removes them.",32,354,576,20,NULL),.6f);
 Add(m,"video","Video and audio",382,146,226,32,"uiScript jarNative setup ;");Add(m,"controls","Controls",382,190,226,32,"uiScript jarNative controls ;");Add(m,"bindings","Sandbox bindings...",382,234,226,32,"uiScript jarBindings ;");Add(m,"allow","Allow tools on me",382,278,226,32,"uiScript jarConsent 1 ;");Add(m,"block","Block tools on me",382,322,226,32,"uiScript jarConsent 0 ;");
 Add(m,"bladeColors","Saber blade colors",382,360,226,30,"uiScript jarBladeColors ;");
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarCameraRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");itemDef_t*i=Add(m,"status","",32,380,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}Menu_PostParse(m);}}
 m=Menus_FindByName("jarMPConsole");if(!m){m=Page("jarMPConsole","JA:R SETTINGS","MULTIPLAYER  /  CONSOLE");if(m){SettingsTabs(m);
 static const jarMenuChoice_t styles[]=JAR_CONSOLE_STYLE_CHOICES,fractions[]=JAR_CONSOLE_FRACTION_CHOICES,scales[]=JAR_CONSOLE_SCALE_CHOICES;
 Choice(m,"style","Background: ","ui_jarMP_jar_consoleStyle",140,styles,ARRAY_LEN(styles));
 Choice(m,"opacity","Background opacity: ","ui_jarMP_con_opacity",184,fractions,ARRAY_LEN(fractions));
 Choice(m,"scale","Console text size: ","ui_jarMP_con_scale",228,scales,ARRAY_LEN(scales));
 Choice(m,"height","Console height: ","ui_jarMP_con_height",272,fractions,ARRAY_LEN(fractions));
 Scale(Add(m,"consoleHelp","Text size affects the console. Full-screen backgrounds stay opaque.",32,334,576,20,NULL),.6f);
 Scale(Add(m,"consolePending","Changes on all settings pages apply together. Back discards pending edits.",32,356,576,20,NULL),.6f);
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");itemDef_t*i=Add(m,"status","",32,380,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}Menu_PostParse(m);}}
 m=Menus_FindByName("jarMPInterface");if(!m){m=Page("jarMPInterface","JA:R SETTINGS","MULTIPLAYER  /  INTERFACE");if(m){SettingsTabs(m);
 static const jarMenuChoice_t fonts[]=JAR_FONT_PACK_CHOICES,art[]=JAR_UI_PACK_CHOICES,detail[]=JAR_FONT_DETAIL_CHOICES,huds[]=JAR_HUD_CHOICES,motion[]=JAR_UI_MOTION_CHOICES;
 Choice(m,"fontPack","Text appearance: ","ui_jarMP_jar_fontPack",130,fonts,ARRAY_LEN(fonts));
 Choice(m,"uiPack","UI artwork: ","ui_jarMP_jar_uiPack",170,art,ARRAY_LEN(art));
 Choice(m,"fontDetail","Font variant detail: ","ui_jarMP_r_fontSharpness",210,detail,ARRAY_LEN(detail));
 Choice(m,"hud","HUD layout: ","ui_jarMP_cg_hudFiles",250,huds,ARRAY_LEN(huds));
 Choice(m,"motion","UI motion: ","ui_jarMP_jar_uiMotion",290,motion,ARRAY_LEN(motion));
 Scale(Add(m,"interfaceHelp","HUD, text and artwork changes reload video on Apply.",32,334,576,20,NULL),.6f);
 Scale(Add(m,"interfaceMemory","Reduced freezes 2D UI animation. World effects and movies keep playing.",32,356,576,20,NULL),.6f);
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"revert","Revert",208,440,156,32,"uiScript jarSettingsRevert ;");Add(m,"apply","Apply",452,440,156,32,"uiScript jarApply ;");itemDef_t*i=Add(m,"status","",32,380,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}Menu_PostParse(m);}}
 m=Menus_FindByName("jarMPHost");if(!m){m=Page("jarMPHost","HOST JA:R SANDBOX","HOST AND PLAY  /  LAN SERVER");if(m){
 Field(m,"name","Name: ","ui_jarHostName",136,48,qfalse);Field(m,"map","Map: ","ui_jarHostMap",174,MAX_QPATH-10,qfalse);Field(m,"players","Players: ","ui_jarHostPlayers",212,2,qtrue);
 Field(m,"port","UDP port: ","ui_jarHostPort",250,5,qtrue);
 Field(m,"autosave","Autosave seconds: ","ui_jarHostAutoSave",288,4,qtrue);
 Rect(Menu_FindItemByName(m,"map"),32,174,232,28);
 Scale(Add(m,"browseMaps","Browse maps",269,174,93,28,"uiScript jarHostMaps open ;"),.57f);
 Toggle(m,"persist","Keep world across sessions: ","ui_jarHostPersistent",32,326,330);
 Scale(Add(m,"modeHint","Dedicated servers: Host-JA-R-Server.cmd",32,364,330,20,NULL),.6f);
 Toggle(m,"shared","Edit everyone's objects: ","ui_jarHostShared",390,136,218);Toggle(m,"cleanup","Clean on disconnect: ","ui_jarHostCleanup",390,174,218);Toggle(m,"playersTools","Player tools: ","ui_jarHostPlayerTools",390,212,218);Toggle(m,"mapEditing","Map actor tools: ","ui_jarHostMapEditing",390,250,218);Toggle(m,"worldEditing","Saved world tools: ","ui_jarHostWorldEditing",390,288,218);Toggle(m,"restore","Restore saved world: ","ui_jarHostAutoLoad",390,326,218);
 Toggle(m,"cheats","Allow cheats: ","ui_jarHostCheats",390,364,218);
 Add(m,"back","Back",32,440,156,32,"uiScript jarBack ;");Add(m,"start","Start Sandbox",452,440,156,32,"uiScript jarStartHost ;");itemDef_t*i=Add(m,"status","",32,396,576,24,NULL);if(i){i->text=jarStatus;i->textscale=.6f;}Menu_PostParse(m);}}
 m=Menus_FindByName("jarMPBindings");if(!m){m=Page("jarMPBindings","SANDBOX CONTROLS","INSTALL DEFAULT BINDINGS");if(m){
 Scale(Add(m,"line1","Q: spawn/tools menu    C: entity properties    Z: Undo",32,150,576,24,NULL),.7f);
 Scale(Add(m,"line2","Mouse wheel: move a held object nearer or farther",32,192,576,24,NULL),.7f);
 Scale(Add(m,"line3","Physgun: left hold / right freeze / USE + mouse rotate",32,234,576,24,NULL),.7f);
 Scale(Add(m,"line4","Install replaces Q, C, Z and wheel bindings. Other keys stay as set.",32,296,576,24,NULL),.6f);
 Add(m,"back","Back",32,440,156,32,"uiScript jarReturnSettings ;");Add(m,"install","Install bindings",452,440,156,32,"uiScript jarInstallBindings ;");m->onESC=String_Alloc("uiScript jarReturnSettings ;");Menu_PostParse(m);}}
 m=Menus_FindByName("jarMPPause");if(!m){m=Page("jarMPPause","JEDI ACADEMY","REMIXED  /  MULTIPLAYER SESSION");if(m){
 Add(m,"resume","Resume",32,156,272,44,"uiScript jarResume ;");Add(m,"player","Player and team",32,214,272,44,"uiScript jarNative player ;");Add(m,"session","Session / vote / bots",32,272,272,44,"close all ; open ingame ; open ingame_about ;");
 Add(m,"settings","JA:R Settings",336,156,272,44,"uiScript jarOpen ;");Add(m,"controls","Controls",336,214,272,44,"uiScript jarNative controls ;");Add(m,"setup","Video and audio",336,272,272,44,"uiScript jarNative setup ;");
 Scale(Add(m,"hint","Multiplayer keeps running while menus are open.",32,352,576,22,NULL),.7f);Add(m,"leave","Disconnect...",452,440,156,32,"close all ; open ingame ; open ingame_leave ;");m->onESC=String_Alloc("uiScript jarResume ;");Menu_PostParse(m);}}
}
qboolean UI_JAR_Script(const char*name,char**args){
 if(!Q_stricmp(name,"jarDisplayKeep")){if(DisplayState()==2){trap->Cmd_ExecuteText(EXEC_NOW,"jar_display_keep\n");Value("ui_jarDisplayParent",jarReturn,sizeof(jarReturn));ReadSettings();Open(Menus_FindByName(jarReturn)?jarReturn:Active()?"jarMPPause":"main");}return qtrue;}
 if(!Q_stricmp(name,"jarDisplayRevert")){trap->Cmd_ExecuteText(EXEC_NOW,"jar_display_revert\n");return qtrue;}
 if(DisplayState()&&!Q_stricmpn(name,"jar",3))return qtrue;
 if(!Q_stricmp(name,"jarOpen")){Remember();ReadSettings();Open("jarMPSettings");return qtrue;}
 if(!Q_stricmp(name,"jarSettingsPage")){const char*page;if(String_Parse(args,&page)){if(!strcmp(page,"camera"))Open("jarMPSettings");else if(!strcmp(page,"console"))Open("jarMPConsole");else if(!strcmp(page,"interface"))Open("jarMPInterface");else if(!strcmp(page,"graphics"))Open("jarMPGraphics");else if(!strcmp(page,"textures"))Open("jarMPTextures");else if(!strcmp(page,"detail"))Open("jarMPDetail");else if(!strcmp(page,"display"))Open("jarMPDisplay");else if(!strcmp(page,"effects"))Open("jarMPEffects");else if(!strcmp(page,"combat"))Open("jarMPCombat");}return qtrue;}
 if(!Q_stricmp(name,"jarHost")){Remember();jarStatus[0]=0;Open("jarMPHost");return qtrue;}
 if(!Q_stricmp(name,"jarHostMaps")){const char*action;if(String_Parse(args,&action))HostMapsAction(action);return qtrue;}
 if(!Q_stricmp(name,"jarBack")){Open(Menus_FindByName(jarReturn)?jarReturn:Active()?"jarMPPause":"main");return qtrue;}
 if(!Q_stricmp(name,"jarBindings")){Open("jarMPBindings");return qtrue;}
 if(!Q_stricmp(name,"jarReturnSettings")){Open("jarMPSettings");return qtrue;}
 if(!Q_stricmp(name,"jarInstallBindings")){trap->Cmd_ExecuteText(EXEC_APPEND,"exec jar-mp-controls.cfg\nseta jar_mpBindings 1\n");Q_strncpyz(jarStatus,"Sandbox keys installed. Controls edits other bindings.",sizeof(jarStatus));Open("jarMPSettings");return qtrue;}
 if(!Q_stricmp(name,"jarCameraRevert")||!Q_stricmp(name,"jarSettingsRevert")){ReadSettings();Q_strncpyz(jarStatus,"Pending settings discarded.",sizeof(jarStatus));return qtrue;}
 if(!Q_stricmp(name,"jarApply")){
  char values[ARRAY_LEN(clientSettings)][64];float parsed;qboolean reload=qfalse,soundRestart=qfalse;
  for(int n=0;n<ARRAY_LEN(clientSettings);++n){
   const char*key=va("ui_jarMP_%s",clientSettings[n].name);Value(key,values[n],sizeof(values[n]));if(!strcmp(values[n],originals[n]))continue;
   if(clientSettings[n].choices){qboolean valid=qfalse;for(int k=0;k<clientSettings[n].count;++k)if(!strcmp(values[n],clientSettings[n].choices[k].value))valid=qtrue;if(!valid){Q_strncpyz(jarStatus,"Choose a supported graphics option. Nothing applied.",sizeof(jarStatus));return qtrue;}if(clientSettings[n].restart)reload=qtrue;}
   else if(clientSettings[n].asset){if(!SelectionAvailable(clientSettings[n].asset,values[n])){Q_strncpyz(jarStatus,"HUD or appearance selection unavailable. Nothing applied.",sizeof(jarStatus));return qtrue;}reload=qtrue;if(!strcmp(clientSettings[n].asset,"sound"))soundRestart=qtrue;}
   else if(!Number(key,clientSettings[n].low,clientSettings[n].high,clientSettings[n].integer,&parsed)){Q_strncpyz(jarStatus,"Setting is outside its allowed range. Nothing applied.",sizeof(jarStatus));return qtrue;}
  }
  char display[5][MAX_STRING_CHARS];const char*pending[5],*old[5],*mode;qboolean displayChanged=qfalse;
  for(int n=0;n<5;++n){Value(va("ui_jarMP_%s",displayNames[n]),display[n],sizeof(display[n]));pending[n]=display[n];old[n]=displayOriginals[n];if(strcmp(pending[n],old[n]))displayChanged=qtrue;}
  if(!JAR_DisplayValidate(pending,old,&mode)){Q_strncpyz(jarStatus,"Invalid display size or option. Nothing applied.",sizeof(jarStatus));return qtrue;}
  if(displayChanged){
   if(mode!=display[0])Q_strncpyz(display[0],mode,sizeof(display[0]));
   trap->Cvar_Set("ui_jarMP_r_mode",display[0]);trap->Cvar_Set("ui_jarDisplayParent",jarReturn);
   trap->Cmd_ExecuteText(EXEC_NOW,"jar_display_begin\n");
   if(DisplayState()!=1){Q_strncpyz(jarStatus,"Display trial unavailable. Nothing applied.",sizeof(jarStatus));return qtrue;}
   for(int n=0;n<5;++n)if(strcmp(display[n],displayOriginals[n]))trap->Cvar_Set(displayNames[n],display[n]);
   reload=qtrue;
  }
  for(int n=0;n<ARRAY_LEN(clientSettings);++n)if(strcmp(values[n],originals[n]))trap->Cvar_Set(clientSettings[n].name,values[n]);
  if(!displayChanged)ReadSettings();Q_strncpyz(jarStatus,reload?"Settings applied. Reloading video...":"Settings applied.",sizeof(jarStatus));
  if(reload)trap->Cmd_ExecuteText(EXEC_APPEND,soundRestart?"snd_restart\nvid_restart\n":"vid_restart\n");return qtrue;
 }
 if(!Q_stricmp(name,"jarStartHost")){StartHost();return qtrue;}
 if(!Q_stricmp(name,"jarBladeColors")){
  char color[16];byte rgb[3];
  Value("color1",color,sizeof(color));trap->Cvar_Set("ui_jarBladeRGB1",JAR_ParseSaberRGB(color,rgb)?color:"");
  Value("color2",color,sizeof(color));trap->Cvar_Set("ui_jarBladeRGB2",JAR_ParseSaberRGB(color,rgb)?color:"");
  jarStatus[0]=0;Open("jarMPBladeColors");return qtrue;
 }
 if(!Q_stricmp(name,"jarApplyBladeRGB")){
  static const char *fields[]={"ui_jarBladeRGB1","ui_jarBladeRGB2"};
  static const char *colors[]={"color1","color2"};
  char pending[2][16],old[16];byte rgb[3];
  for(int i=0;i<2;++i){
   Value(fields[i],pending[i],sizeof(pending[i]));
   if(pending[i][0]&&!JAR_ParseSaberRGB(pending[i],rgb)){
    Q_strncpyz(jarStatus,"Enter six hexadecimal digits after #.",sizeof(jarStatus));
    return qtrue;
   }
  }
  for(int i=0;i<2;++i){
   Value(colors[i],old,sizeof(old));
   if(pending[i][0])trap->Cvar_Set(colors[i],pending[i]);
   else if(JAR_ParseSaberRGB(old,rgb))trap->Cvar_Set(colors[i],"4");
  }
  Q_strncpyz(jarStatus,"Blade colors applied.",sizeof(jarStatus));return qtrue;
 }
 if(!Q_stricmp(name,"jarBladeBack")){jarStatus[0]=0;Open("jarMPSettings");return qtrue;}
 if(!Q_stricmp(name,"jarResume")){Menus_CloseAll();trap->Key_SetCatcher(trap->Key_GetCatcher()&~KEYCATCH_UI);trap->Key_ClearStates();trap->Cvar_Set("cl_paused","0");return qtrue;}
 if(!Q_stricmp(name,"jarConsent")){const char*value;if(String_Parse(args,&value)&&(!strcmp(value,"0")||!strcmp(value,"1"))){if(Active()){trap->Cmd_ExecuteText(EXEC_APPEND,va("cmd jar_allowTools %s\n",value));Q_strncpyz(jarStatus,"Tool permission request sent to the server.",sizeof(jarStatus));}else Q_strncpyz(jarStatus,"Join a Sandbox server to change player tool permission.",sizeof(jarStatus));}return qtrue;}
 if(!Q_stricmp(name,"jarControlsBack")){
  const qboolean active=Active();
  const char *parent=active?"jarMPPause":"main";
  if(!strcmp(controlsReturn,"jarMPSettings")||(active&&!strcmp(controlsReturn,"ingame")))parent=controlsReturn;
  Open(parent);
  return qtrue;
 }
 if(!Q_stricmp(name,"jarNative")){const char*part;if(!String_Parse(args,&part))return qtrue;const char*dest=NULL;if(!strcmp(part,"setup"))dest=Active()?"ingame_setup":"setup_menu";else if(!strcmp(part,"controls")){OpenControls();return qtrue;}else if(!strcmp(part,"player"))dest=Active()?"ingame_player":"playerMenu";if(dest){if(!Active()&&!Menus_FindByName(dest))UI_LoadNonIngame();Menus_CloseAll();if(Active())Menus_ActivateByName("ingame");Menus_ActivateByName(dest);}return qtrue;}
 return qfalse;
}

void UI_JAR_Refresh(void){
 HostMapsRefresh();
 if(DisplayState()==2){char seconds[16];Value("ui_jarDisplaySeconds",seconds,sizeof(seconds));Com_sprintf(displayPrompt,sizeof(displayPrompt),"%d x %d  /  Restoring in %d seconds",uiInfo.uiDC.glconfig.vidWidth,uiInfo.uiDC.glconfig.vidHeight,atoi(seconds));menuDef_t*m=Menu_GetFocused();if(!m||Q_stricmp(m->window.name,"jarMPDisplayConfirm"))Open("jarMPDisplayConfirm");}
 for(int n=0;n<Menu_Count();++n){menuDef_t*m=Menu_JARAt(n);if(!m)continue;for(int k=0;k<m->itemCount;++k){itemDef_t*i=m->items[k];if(i->iMenuFont!=5||!i->text)continue;float*c=i->window.foreColor;if(c[0]>.6f&&c[1]>.4f&&c[2]<.4f)Color(jarMuted,c);else if(c[0]>.98f&&c[1]>.98f&&c[2]>.98f)Color(jarAccent,c);}}
}

void UI_JAR_DrawCursor(void){jarCursorBox_t b=JAR_MenuCursor(uiInfo.uiDC.cursorx,uiInfo.uiDC.cursory,uiInfo.uiDC.xscale,uiInfo.uiDC.yscale);UI_DrawHandlePic(b.x,b.y,b.w,b.h,jarCursor?jarCursor:uiInfo.uiDC.Assets.cursor);}
