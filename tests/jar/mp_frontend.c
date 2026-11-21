// Production MP front-end builder/actions with engine services captured, no window.
#include "ui/ui_jar_frontend.c"
#include "qcommon/qfiles.h"
#include <assert.h>
#include <stdarg.h>
uiInfo_t uiInfo;static uiImport_t imports;uiImport_t*trap=&imports;
static menuDef_t testMenus[64];static int count,catcher=KEYCATCH_UI,commands,openedFiles;static menuDef_t*focused;static connstate_t connection=CA_DISCONNECTED;static char command[512];static qboolean mapExists=qtrue,packExists=qtrue,hudExists=qtrue;
static int arenaLoads;
static struct {char name[96],value[256];} vars[256];static int varCount;
static int Find(const char*n){for(int i=0;i<varCount;++i)if(!strcmp(vars[i].name,n))return i;assert(varCount<256);Q_strncpyz(vars[varCount].name,n,sizeof(vars[0].name));return varCount++;}
static void Set(const char*n,const char*v){Q_strncpyz(vars[Find(n)].value,v,sizeof(vars[0].value));}
static void SetValue(const char*n,float v){char b[64];snprintf(b,sizeof(b),"%.6g",v);Set(n,b);}
static uint32_t fistFlags;
static void Register(vmCvar_t*c,const char*n,const char*v,uint32_t flags){if(!strcmp(n,"jar_continuousFists"))fistFlags|=flags;for(int i=0;i<varCount;++i)if(!strcmp(vars[i].name,n))return;Set(n,v);}
static void Get(const char*n,char*b,int size){Q_strncpyz(b,vars[Find(n)].value,size);}
static const char*V(const char*n){return vars[Find(n)].value;}
static int Catcher(void){return catcher;}static void Catch(int n){catcher=n;}static void Clear(void){}
static qboolean refuseDisplay,refusePort;
static int binds;
static void Execute(int when,const char*s){
 if(when==EXEC_NOW){
  if(!strncmp(s,"jar_host_port ",14)){
   ++binds;
   assert(JAR_HostPort(s+14));
   if(!refusePort){Set("net_port",s+14);Set("ui_jarHostPortStatus","1");}
  }
  else if(!strcmp(s,"jar_display_read\n")){if(!atoi(V("ui_jarDisplayState")))for(int n=0;n<5;++n)Set(va("ui_jarMP_%s",displayNames[n]),V(displayNames[n]));}
  else if(!strcmp(s,"jar_display_begin\n")){if(!refuseDisplay&&!atoi(V("ui_jarDisplayState")))Set("ui_jarDisplayState","1");}
  else if(!strcmp(s,"jar_display_keep\n"))Set("ui_jarDisplayState","0");
  else if(!strcmp(s,"jar_display_revert\n")){Set("ui_jarDisplayState","3");++commands;Q_strncpyz(command,"vid_restart\n",sizeof(command));}
  else assert(0);return;
 }
 assert(when==EXEC_APPEND);++commands;Q_strncpyz(command,s,sizeof(command));
}
static void State(uiClientState_t*s){memset(s,0,sizeof(*s));s->connState=connection;}
static char hostBsp[4096],blockedBsp[4096];
static int hostBspSize,blockedBspSize,fileOffset,thumbnailWrites;
static char thumbnailPaths[2][MAX_QPATH];
static void HostFixture(void){
 const char entities[]="{\"classname\" \"worldspawn\" \"message\" \"Brush Courtyard\"}"
  "{\"classname\" \"info_player_start\" \"targetname\" \"named\" \"origin\" \"0 0 24\"}";
 const char blocked[]="{\"classname\" \"worldspawn\"}{\"classname\" \"info_player_start\" \"notfree\" \"1\"}"
  "{\"classname\" \"info_player_deathmatch\" \"nohumans\" \"1\"}"
  "{\"classname\" \"info_player_deathmatch\" \"gametype\" \"ctf\"}";
 dheader_t header={.ident=BSP_IDENT,.version=BSP_VERSION};
 dshader_t shader={"textures/test/floor",0,0};
 dmodel_t model={{-128,-128,0},{128,128,80},0,1,0,0};
 drawVert_t verts[3]={0};
 verts[0].xyz[0]=-128;verts[1].xyz[0]=128;verts[2].xyz[1]=128;
 const int indices[]={0,1,2};
 const dsurface_t surface={.surfaceType=MST_PLANAR,.numVerts=3,.numIndexes=3};
 struct {int index;const void* data;size_t size;} chunks[]={
  {LUMP_SHADERS,&shader,sizeof(shader)},{LUMP_MODELS,&model,sizeof(model)},
  {LUMP_DRAWVERTS,verts,sizeof(verts)},{LUMP_DRAWINDEXES,indices,sizeof(indices)},
  {LUMP_SURFACES,&surface,sizeof(surface)},{LUMP_ENTITIES,entities,sizeof(entities)}
 };
 hostBspSize=sizeof(header);
 for(int i=0;i<ARRAY_LEN(chunks);++i){
  header.lumps[chunks[i].index]=(lump_t){hostBspSize,(int)chunks[i].size};
  memcpy(hostBsp+hostBspSize,chunks[i].data,chunks[i].size);hostBspSize+=(int)chunks[i].size;
 }
 memcpy(hostBsp,&header,sizeof(header));
 memcpy(blockedBsp,hostBsp,hostBspSize);
 header.lumps[LUMP_ENTITIES].filelen=sizeof(blocked);
 memcpy(blockedBsp,&header,sizeof(header));
 memcpy(blockedBsp+header.lumps[LUMP_ENTITIES].fileofs,blocked,sizeof(blocked));
 blockedBspSize=header.lumps[LUMP_ENTITIES].fileofs+sizeof(blocked);
}
static int MapList(const char*path,const char*extension,char*list,int size){
 static const char names[]="gm_flatgrass.bsp\0gm_construct.bsp\0custom/brush.bsp\0blocked.bsp\0custom/brush.bsp\0../escape.bsp\0wrong.txt\0bad.bsp\0";
 assert(!strcmp(path,"maps")&&!strcmp(extension,".bsp")&&size>=sizeof(names));
 memcpy(list,names,sizeof(names));return 8;
}
static int File(const char*name,fileHandle_t*f,fsMode_t mode){
 fileOffset=0;
 if(!strncmp(name,"jar/mapshots/",13)){
  if(mode==FS_WRITE){
   assert(thumbnailWrites<ARRAY_LEN(thumbnailPaths));
   Q_strncpyz(thumbnailPaths[thumbnailWrites],name,sizeof(thumbnailPaths[0]));*f=5;return 0;
  }
  *f=0;
  for(int i=0;i<thumbnailWrites;++i)if(!strcmp(name,thumbnailPaths[i]))*f=5;
  return *f?18+144*96*3:-1;
 }
 assert(mode==FS_READ);++openedFiles;qboolean available=mapExists;
 if(!strncmp(name,"maps/",5)){
  if(!mapExists){*f=0;return -1;}
  if(!strcmp(name,"maps/blocked.bsp")){*f=3;return blockedBspSize;}
  if(!strcmp(name,"maps/bad.bsp")){*f=4;return 12;}
  if(!strcmp(name,"maps/mp/ffa3.bsp")||!strcmp(name,"maps/gm_construct.bsp")||
     !strcmp(name,"maps/gm_flatgrass.bsp")||!strcmp(name,"maps/custom/brush.bsp")){*f=2;return hostBspSize;}
  *f=0;return -1;
 }
 if(!strncmp(name,"levelshots/",11)){
  available=!strcmp(name,"levelshots/gm_construct.png")||!strcmp(name,"levelshots/gm_flatgrass.png");
 }else if(!strcmp(name,"ui/jahud.txt"))available=hudExists;
 else {available=packExists;assert(!strncmp(name,"jar/",4));assert(strstr(name,"/fonts/arialnb.fontdat")||strstr(name,"/fonts/arialnb.tga")||strstr(name,"/ui/assets/gradientbar2.tga")||strstr(name,"/pack.txt"));}
 *f=available?1:0;return available?123:-1;
}
static int ReadMap(void* dst,int length,fileHandle_t f){
 assert(f==2||f==3);const int size=f==2?hostBspSize:blockedBspSize;
 assert(length>=0&&fileOffset<=size-length);
 memcpy(dst,(f==2?hostBsp:blockedBsp)+fileOffset,length);fileOffset+=length;return length;
}
static int WriteThumbnail(const void* data,int length,fileHandle_t f){
 const byte* tga=data;assert(f==5&&length==18+144*96*3&&tga[2]==2&&tga[12]==144&&tga[14]==96);
 ++thumbnailWrites;return length;
}
static void Close(fileHandle_t f){assert(f>0&&f<=5);}
static qhandle_t Shader(const char*n){if(!strcmp(n,"jar/icons/cursor"))return 9;if(!strcmp(n,"levelshots/gm_construct"))return 201;if(!strcmp(n,"levelshots/gm_flatgrass"))return 202;if(!strncmp(n,"jar/mapshots/",13))return 203;for(int k=0;k<ARRAY_LEN(chromeShaders);++k)if(!strcmp(n,chromeShaders[k]))return 100+k;assert(0);return 0;}
void UI_DrawHandlePic(float x,float y,float w,float h,qhandle_t shader){assert(shader==9&&fabsf(w*3-h*2)<.001f);}
static qhandle_t Font(const char*n){assert(!strcmp(n,"arialnb"));return 42;}
void*UI_Alloc(int n){void*p=calloc(1,n);assert(p);return p;}
const char*String_Alloc(const char*s){char*p=UI_Alloc((int)strlen(s)+1);strcpy(p,s);return p;}
void Item_Init(itemDef_t*i){memset(i,0,sizeof(*i));}
menuDef_t*Menus_FindByName(const char*n){for(int i=0;i<count;++i)if(!Q_stricmp(testMenus[i].window.name,n))return &testMenus[i];return NULL;}
menuDef_t*Menu_CreateJAR(const char*n){menuDef_t*m=Menus_FindByName(n);if(m)return m;assert(count<64);m=&testMenus[count++];m->window.name=String_Alloc(n);return m;}
menuDef_t*Menu_JARAt(int n){return n>=0&&n<count?&testMenus[n]:NULL;}int Menu_Count(void){return count;}
itemDef_t*Menu_FindItemByName(menuDef_t*m,const char*n){for(int i=0;i<m->itemCount;++i)if(!Q_stricmp(m->items[i]->window.name,n))return m->items[i];return NULL;}
void Menu_PostParse(menuDef_t*m){for(int i=0;i<m->itemCount;++i)m->items[i]->window.rect=m->items[i]->window.rectClient;}
menuDef_t*Menu_GetFocused(void){return focused;}void Menus_CloseAll(void){focused=NULL;}
menuDef_t*Menus_ActivateByName(const char*n){return focused=Menus_FindByName(n);}
void UI_LoadNonIngame(void){assert(0);}
void UI_LoadJARControls(qboolean inGame){
 assert(inGame);
 menuDef_t *menu=Menu_CreateJAR("ingame_controls");
 menu->window.rect=(rectDef_t){45,35,550,335};
 menu->window.flags=WINDOW_OOB_CLICK;
 menu->onClose=String_Alloc("uiScript saveControls ;");
 Add(menu,"bind","Jump",220,85,300,20," ");
}
void UI_LoadArenas(void){++arenaLoads;}
qboolean String_Parse(char**p,const char**out){*out=*p;return **p?qtrue:qfalse;}
char*QDECL va(const char*fmt,...){static char b[8][4096];static unsigned at;char*out=b[at++%8];va_list a;va_start(a,fmt);vsnprintf(out,4096,fmt,a);va_end(a);return out;}
int QDECL Com_sprintf(char*b,int size,const char*fmt,...){va_list a;va_start(a,fmt);int n=vsnprintf(b,size,fmt,a);va_end(a);return n;}
static void Error(int n,const char*s,...){abort();}static void Print(const char*s,...){}
void(*Com_Error)(int,const char*,...)=Error;void(*Com_Printf)(const char*,...)=Print;
static void Run(const char*n,const char*arg){char*p=(char*)arg;assert(UI_JAR_Script(n,&p));}
int main(void){
 HostFixture();
 imports.Cvar_Set=Set;imports.Cvar_SetValue=SetValue;imports.Cvar_Register=Register;imports.Cvar_VariableStringBuffer=Get;imports.GetClientState=State;imports.Key_GetCatcher=Catcher;imports.Key_SetCatcher=Catch;imports.Key_ClearStates=Clear;imports.Cmd_ExecuteText=Execute;imports.FS_Open=File;imports.FS_Close=Close;imports.R_RegisterFont=Font;imports.R_RegisterShaderNoMip=Shader;
 imports.FS_GetFileList=MapList;imports.FS_Read=ReadMap;imports.FS_Write=WriteThumbnail;
 Set("r_mode","4");Set("r_customwidth","1600");Set("r_customheight","1024");Set("r_fullscreen","0");Set("r_noborder","0");
 menuDef_t*m=Menu_CreateJAR("main");const char*buttons[]={"playbutton","profilebutton","controlsbutton","setupbutton","exitgamebutton"};
 for(int i=0;i<5;++i)Add(m,buttons[i],buttons[i],36,100+i*30,150,24,"native action unchanged ;");
 itemDef_t*art=Add(m,"starwars",NULL,100,8,428,112,NULL);art->window.background=123;
 menuDef_t*browser=Menu_CreateJAR("joinserver");itemDef_t*list=Add(browser,"servers","Servers",12,100,440,280,"native sort ;");list->type=ITEM_TYPE_LISTBOX;list->special=123;list->cvarTest=String_Alloc("native_visibility");const char*action=list->action;rectDef_t rect=list->window.rectClient;
 itemDef_t*icon=Add(browser,"nativeIcon","icon label",12,12,40,40,NULL);icon->window.background=77;icon->window.style=WINDOW_STYLE_SHADER;
 itemDef_t*preview=Add(Menu_CreateJAR("Connect"),"miniFrame",NULL,0,0,640,480,NULL);preview->window.background=78;preview->window.style=WINDOW_STYLE_SHADER;
 itemDef_t*frame=Add(browser,"frame",NULL,0,0,640,480,NULL);frame->window.background=Shader(chromeShaders[0]);frame->window.style=WINDOW_STYLE_SHADER;
 Menu_CreateJAR("setup_menu");Menu_CreateJAR("controlsMenu")->fullScreen=qtrue;Menu_CreateJAR("ingame");Menu_CreateJAR("ingame_setup");Menu_CreateJAR("ingame_player");
 Set("cg_fov","110");Set("jar_sharedEditing","1");UI_JAR_LoadMenus();assert(!strcmp(V("cg_fov"),"110")&&!strcmp(V("jar_sharedEditing"),"1")&&UI_JAR_Font()==42&&!art->window.background);
 assert(list->action==action&&list->special==123&&!memcmp(&rect,&list->window.rectClient,sizeof(rect))&&!strcmp(list->cvarTest,"native_visibility")&&list->iMenuFont==5);
 assert(icon->window.background==77&&icon->window.style==WINDOW_STYLE_SHADER&&preview->window.background==78&&preview->window.style==WINDOW_STYLE_SHADER&&!frame->window.background&&frame->window.style==WINDOW_STYLE_FILLED);
 assert(!strcmp(Menu_FindItemByName(m,"jarSettings")->action,"uiScript jarOpen ;")&&Menu_FindItemByName(m,"jarHost")->window.rectClient.x==32&&Menu_FindItemByName(m,"jarSettings")->window.rectClient.x==336);
 assert(!strcmp(Menu_FindItemByName(m,"controlsbutton")->action,"uiScript jarNative controls ;"));
 focused=m;Run("jarNative","controls");assert(focused->fullScreen&&!strcmp(focused->window.name,"controlsMenu"));Run("jarControlsBack","");assert(focused==m);
 uiInfo.uiDC.xscale=3;uiInfo.uiDC.yscale=2;UI_JAR_DrawCursor();
 const int menus=count,items=m->itemCount;UI_JAR_LoadMenus();assert(count==menus&&m->itemCount==items);
 focused=m;Run("jarOpen","");assert(!strcmp(focused->window.name,"jarMPSettings")&&!strcmp(V("ui_jarMP_cg_fov"),"110"));Set("ui_jarMP_cg_fov","120");Set("ui_jarMP_cg_thirdPersonRange","nan");Run("jarApply","");assert(!strcmp(V("cg_fov"),"110"));Set("ui_jarMP_cg_thirdPersonRange","96");Set("ui_jarMP_cg_thirdPersonVertOffset","-12");Run("jarApply","");assert(!strcmp(V("cg_fov"),"120")&&!strcmp(V("cg_thirdPersonRange"),"96")&&!strcmp(V("cg_thirdPersonVertOffset"),"-12"));
 Run("jarBack","");assert(focused==m);Run("jarOpen","");Run("jarConsent","1");assert(!commands);connection=CA_ACTIVE;Run("jarConsent","1;quit");assert(!commands);Run("jarConsent","1");assert(commands==1&&!strcmp(command,"cmd jar_allowTools 1\n"));
 menuDef_t *pause=Menus_FindByName("jarMPPause");focused=pause;
 Run("jarOpen","");menuDef_t *settingsPage=focused;
 Set("ui_jarMP_cg_fov","123");
 Run("jarNative","controls");
 assert(focused->fullScreen&&!(focused->window.flags&WINDOW_OOB_CLICK)&&!strcmp(focused->window.name,"ingame_controls"));
 assert(!strcmp(focused->onClose,"uiScript saveControls ;")&&!strcmp(focused->onESC,"uiScript jarControlsBack ;"));
 assert(Menu_FindItemByName(focused,"bind")->window.rectClient.x==265&&Menu_FindItemByName(focused,"bind")->window.rectClient.y==120);
 assert(Menu_FindItemByName(focused,"jarControlsBack"));
 Run("jarControlsBack","");assert(focused==settingsPage&&!strcmp(V("ui_jarMP_cg_fov"),"123"));
 Run("jarBack","");assert(focused==pause);Run("jarNative","controls");Run("jarControlsBack","");assert(focused==pause);
 Run("jarResume","");assert(!focused&&!(catcher&KEYCATCH_UI));
 connection=CA_DISCONNECTED;focused=m;Run("jarHost","");assert(!strcmp(focused->window.name,"jarMPHost")&&!strcmp(V("jar_sharedEditing"),"1"));int before=commands;
 Set("ui_jarHostMap","mp/ffa3;quit");Run("jarStartHost","");assert(commands==before&&!openedFiles);
 Set("ui_jarHostMap","../mp/ffa3");Run("jarStartHost","");assert(commands==before&&!openedFiles);
 Set("ui_jarHostMap","mp/ffa3");Set("ui_jarHostPlayers","33");Run("jarStartHost","");assert(commands==before&&!openedFiles);
 Set("ui_jarHostPlayers","8");Set("ui_jarHostName","bad;quit");Run("jarStartHost","");assert(commands==before&&!openedFiles);
 Set("ui_jarHostName","Test Sandbox");mapExists=qfalse;Run("jarStartHost","");assert(commands==before&&!strcmp(V("jar_sharedEditing"),"1"));
 assert(!binds&&!strcmp(V("ui_jarHostPort"),"29070")&&!strcmp(V("ui_jarHostCheats"),"0"));
 mapExists=qtrue;
 const char *invalidPorts[]={"","0","-1","65536","29070;quit","2e3","290.0","10000000000000000"};
 for(int i=0;i<ARRAY_LEN(invalidPorts);++i){
  Set("ui_jarHostPort",invalidPorts[i]);Run("jarStartHost","");
  assert(!binds&&commands==before&&strstr(jarStatus,"1 to 65535"));
 }
 Set("ui_jarHostPort","65535");Set("ui_jarHostCheats","2");Run("jarStartHost","");
 assert(!binds&&commands==before&&strstr(jarStatus,"cheats"));
 Set("ui_jarHostCheats","1");refusePort=qtrue;Run("jarStartHost","");
 assert(binds==1&&commands==before&&!V("jar_hostCheats")[0]&&strstr(jarStatus,"65535")&&!strcmp(focused->window.name,"jarMPHost"));
 refusePort=qfalse;
 Run("jarStartHost","");assert(binds==2&&commands==before+1&&!strcmp(command,"map mp/ffa3\n")&&!strcmp(V("sv_maxclients"),"8")&&!strcmp(V("dedicated"),"0")&&!strcmp(V("g_gametype"),"0")&&!strcmp(V("jar_sandbox"),"1")&&!strcmp(V("jar_sharedEditing"),"1")&&!strcmp(V("sv_hostname"),"Test Sandbox")&&!focused);
 assert(!strcmp(V("net_port"),"65535")&&!strcmp(V("jar_hostCheats"),"1"));
 focused=m;Run("jarOpen","");Run("jarBindings","");assert(commands==before+1);Run("jarInstallBindings","");assert(commands==before+2&&strstr(command,"exec jar-mp-controls.cfg")&&!strcmp(focused->window.name,"jarMPSettings"));
 list->window.foreColor[0]=1;list->window.foreColor[1]=.682f;list->window.foreColor[2]=0;UI_JAR_Refresh();assert(!memcmp(list->window.foreColor,jarMuted,sizeof(vec4_t)));
 assert(!Menu_FindItemByName(Menus_FindByName("jarMPHost"),"dedicated"));
 menuDef_t *hostPage=Menus_FindByName("jarMPHost");
 assert(Menu_FindItemByName(hostPage,"port")->type==ITEM_TYPE_NUMERICFIELD&&Menu_FindItemByName(hostPage,"port")->typeData.edit->maxChars==5);
 assert(Menu_FindItemByName(hostPage,"cheats")->type==ITEM_TYPE_YESNO);
 Set("ui_jarHostCheats","0");Set("ui_jarHostPort","29072");
 for(int oldMode=0;oldMode<=2;++oldMode){SetValue("ui_jarHostDedicated",oldMode);SetValue("dedicated",oldMode);focused=m;Run("jarHost","");Run("jarStartHost","");assert(!strcmp(V("dedicated"),"0")&&!strcmp(command,"map mp/ffa3\n")&&!focused);}
 assert(!strcmp(V("jar_hostCheats"),"0")&&!strcmp(V("net_port"),"29072"));
 // Persistence and owned-only choices apply only when Start validates fully.
 Set("ui_jarHostPersistent","0");Set("ui_jarHostShared","0");Run("jarHost","");Run("jarStartHost","");
 assert(!strcmp(V("jar_sharedEditing"),"0")&&!strcmp(V("jar_cleanupDisconnect"),"1")&&!strcmp(V("jar_worldAutoSave"),"0")&&!strcmp(V("jar_worldAutoLoad"),"0"));
 Set("ui_jarHostPersistent","1");Set("ui_jarHostShared","1");Run("jarHost","");Run("jarStartHost","");
 assert(!strcmp(V("jar_cleanupDisconnect"),"0")&&!strcmp(V("jar_worldAutoSave"),"120")&&!strcmp(V("jar_worldAutoLoad"),"1"));
 // Both converted maps are selectable with previews and use the normal host path.
 uiInfo.mapCount=3;
 uiInfo.mapList[0]=(mapInfo_t){.mapName="Flatgrass",.mapLoadName="gm_flatgrass",.imageName="levelshots/gm_flatgrass",.typeBits=1<<GT_FFA,.levelShot=-1};
 uiInfo.mapList[1]=(mapInfo_t){.mapName="Construct",.mapLoadName="gm_construct",.imageName="levelshots/gm_construct",.typeBits=1<<GT_FFA,.levelShot=-1};
 uiInfo.mapList[2]=(mapInfo_t){.mapName="Duel",.mapLoadName="mp/duel1",.typeBits=1<<GT_DUEL,.levelShot=-1};
 focused=m;Run("jarHost","");Run("jarHostMaps","open");
 assert(!thumbnailWrites);
 for(int i=0;i<5;++i)UI_JAR_Refresh();
 assert(arenaLoads==1&&hostMapCount==5&&!strcmp(hostMapTitles[2],"Brush Courtyard")&&
        !strcmp(hostMapTitles[3],"Construct")&&!strcmp(hostMapTitles[4],"Flatgrass"));
 assert(!strcmp(hostMapDetails[0],"Map unavailable")&&!strcmp(hostMapDetails[1],"No FFA player start"));
 assert(Menu_FindItemByName(focused,"shot2")->window.background==203&&thumbnailWrites==2);
 assert(Menu_FindItemByName(focused,"shot3")->window.background==201&&Menu_FindItemByName(focused,"shot4")->window.background==202);
 Run("jarHostMaps","1");assert(!strcmp(focused->window.name,"jarMPHostMaps"));
 Set("ui_jarHostMapSearch","FLAT");UI_JAR_Refresh();
 assert(hostMapCount==1&&!strcmp(hostMapTitles[0],"Flatgrass"));
 int beforeMap=commands;Run("jarHostMaps","1");assert(commands==beforeMap);
 Run("jarHostMaps","0");assert(!strcmp(focused->window.name,"jarMPHost")&&!strcmp(V("ui_jarHostMap"),"gm_flatgrass"));
 Run("jarStartHost","");assert(commands==beforeMap+1&&!strcmp(command,"map gm_flatgrass\n"));
 focused=m;Run("jarHost","");Run("jarHostMaps","open");Run("jarHostMaps","3");Run("jarStartHost","");
 assert(commands==beforeMap+2&&!strcmp(command,"map gm_construct\n"));
 focused=m;Run("jarHost","");Run("jarHostMaps","open");
 for(int i=0;i<5;++i)UI_JAR_Refresh();
 Set("ui_jarHostMapSearch","BRUSH COURT");UI_JAR_Refresh();
 assert(hostMapCount==1&&!strcmp(hostMapTitles[0],"Brush Courtyard")&&thumbnailWrites==2);
 Run("jarHostMaps","0");Run("jarStartHost","");
 assert(commands==beforeMap+3&&!strcmp(command,"map custom/brush\n"));
 focused=m;Run("jarHost","");Set("ui_jarHostMap","blocked");Run("jarStartHost","");
 assert(commands==beforeMap+3&&strstr(jarStatus,"no FFA player start"));
 Set("ui_jarHostMap","bad");Run("jarStartHost","");
 assert(commands==beforeMap+3&&strstr(jarStatus,"readable Jedi Academy BSP"));
 Set("ui_jarHostMap","mp/ffa3");
 // Preset selection is a pending transaction, just like SP. Reject invalid
 // edits atomically, preserve untouched custom values and cancel on Back.
 focused=m;Set("jar_cameraPreset","0");Set("cg_thirdPersonRange","900");Run("jarOpen","");
 itemDef_t*mode=Menu_FindItemByName(focused,"cameraPreset");assert(mode&&mode->type==ITEM_TYPE_MULTI&&mode->typeData.multi->count==3&&!strcmp(mode->typeData.multi->cvarList[1],"Comfort")&&!strcmp(mode->typeData.multi->cvarStr[2],"2"));
 Set("ui_jarMP_jar_cameraPreset","1");assert(!strcmp(V("jar_cameraPreset"),"0"));Run("jarApply","");assert(!strcmp(V("jar_cameraPreset"),"1")&&!strcmp(V("cg_thirdPersonRange"),"900"));
 const char*invalid[]={"3","-1","1.5","nan","2;quit"};
 for(int i=0;i<ARRAY_LEN(invalid);++i){Run("jarOpen","");Set("ui_jarMP_cg_fov","125");Set("ui_jarMP_jar_cameraPreset",invalid[i]);const int oldCommands=commands;Run("jarApply","");assert(!strcmp(V("jar_cameraPreset"),"1")&&strcmp(V("cg_fov"),"125")&&commands==oldCommands);}
 Run("jarOpen","");Set("ui_jarMP_jar_cameraPreset","2");Set("ui_jarMP_cg_fov","130");Run("jarCameraRevert","");assert(!strcmp(V("ui_jarMP_jar_cameraPreset"),"1")&&!strcmp(V("ui_jarMP_cg_fov"),V("cg_fov")));
 Set("ui_jarMP_jar_cameraPreset","2");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("jar_cameraPreset"),"1")&&!strcmp(V("ui_jarMP_jar_cameraPreset"),"1"));
 Set("ui_jarMP_jar_cameraPreset","2");Run("jarApply","");assert(!strcmp(V("jar_cameraPreset"),"2"));Run("jarOpen","");assert(!strcmp(V("ui_jarMP_jar_cameraPreset"),"2"));
 puts("MP camera menu: Legacy/Comfort/Crisp, atomic validation, pending Apply/Revert/Back and custom value preservation passed.");
 // Console and camera pages share one pending transaction. Navigation does
 // not write runtime settings; an invalid value on either page rejects all.
 focused=m;Set("con_opacity","0.83");Set("con_scale","1.23");Set("con_height","0.61");Run("jarOpen","");Set("ui_jarMP_jar_cameraPreset","1");Run("jarSettingsPage","console");assert(!strcmp(focused->window.name,"jarMPConsole")&&!strcmp(V("jar_cameraPreset"),"2")&&!strcmp(V("con_opacity"),"0.83"));
 const char*choiceIds[]={"style","opacity","scale","height"};const int choiceCounts[]={4,4,5,4};for(int i=0;i<4;++i){itemDef_t*c=Menu_FindItemByName(focused,choiceIds[i]);assert(c&&c->type==ITEM_TYPE_MULTI&&c->typeData.multi->count==choiceCounts[i]);}
 assert(!strcmp(Menu_FindItemByName(focused,"style")->typeData.multi->cvarList[2],"Amber")&&!strcmp(Menu_FindItemByName(focused,"scale")->typeData.multi->cvarStr[4],"4"));
 Set("ui_jarMP_jar_consoleStyle","2");Set("ui_jarMP_con_scale","nan");Run("jarApply","");assert(!strcmp(V("jar_consoleStyle"),"0")&&!strcmp(V("jar_cameraPreset"),"2")&&!strcmp(V("con_scale"),"1.23"));
 Run("jarSettingsPage","camera");assert(!strcmp(focused->window.name,"jarMPSettings")&&!strcmp(V("ui_jarMP_jar_consoleStyle"),"2"));Set("ui_jarMP_con_scale","1.23");Run("jarApply","");assert(!strcmp(V("jar_consoleStyle"),"2")&&!strcmp(V("jar_cameraPreset"),"1")&&!strcmp(V("con_opacity"),"0.83")&&!strcmp(V("con_height"),"0.61"));
 const char*consoleKeys[]={"jar_consoleStyle","con_opacity","con_scale","con_height"};const char*invalidConsole[]={"4","1.01","0.5","0.1"};
 for(int i=0;i<4;++i){Run("jarOpen","");Run("jarSettingsPage","console");Set("ui_jarMP_jar_cameraPreset","2");Set(va("ui_jarMP_%s",consoleKeys[i]),invalidConsole[i]);Run("jarApply","");assert(!strcmp(V("jar_cameraPreset"),"1")&&!strcmp(V("jar_consoleStyle"),"2"));}
 Run("jarSettingsRevert","");assert(!strcmp(V("ui_jarMP_jar_cameraPreset"),"1")&&!strcmp(V("ui_jarMP_con_height"),"0.61"));
 const char*low[]={"0","0.25","1","0.25"};const char*high[]={"3","1","4","1"};
 for(int edge=0;edge<2;++edge){for(int i=0;i<4;++i)Set(va("ui_jarMP_%s",consoleKeys[i]),edge?high[i]:low[i]);Run("jarApply","");for(int i=0;i<4;++i)assert(!strcmp(V(consoleKeys[i]),edge?high[i]:low[i]));}
 menuDef_t*console=focused;const int oldCommands=commands;Run("jarSettingsPage","console;quit");Run("jarSettingsPage","unknown");assert(focused==console&&commands==oldCommands);
 Set("ui_jarMP_con_scale","2");Run("jarBack","");assert(focused==m&&!strcmp(V("con_scale"),"4"));Run("jarOpen","");Run("jarSettingsPage","console");assert(!strcmp(V("ui_jarMP_con_scale"),"4"));
 focused=Menus_FindByName("jarMPPause");Run("jarOpen","");Run("jarSettingsPage","console");Run("jarBack","");assert(!strcmp(focused->window.name,"jarMPPause"));
 puts("MP console menu: shared stock choices, cross-page pending state, atomic validation, custom preservation, bounds, Revert/Back and parent navigation passed.");
 // Interface settings share native assets with SP and one atomic transaction
 // with the other pages. Reload only after valid applied pack changes.
 focused=m;Run("jarOpen","");Run("jarSettingsPage","interface");assert(!strcmp(focused->window.name,"jarMPInterface"));
 const char*interfaceIds[]={"fontPack","uiPack","fontDetail"};for(int i=0;i<3;++i){itemDef_t*c=Menu_FindItemByName(focused,interfaceIds[i]);assert(c&&c->type==ITEM_TYPE_MULTI&&c->typeData.multi->count==3);}
 assert(!strcmp(Menu_FindItemByName(focused,"fontPack")->typeData.multi->cvarList[1],"Crisp Sans"));
 assert(!strcmp(V("jar_fontPack"),"sans")&&!strcmp(V("jar_uiPack"),"up2"));
 int reloads=commands;Set("ui_jarMP_jar_fontPack","serif");Set("ui_jarMP_jar_uiPack","up4");Run("jarSettingsPage","camera");assert(commands==reloads&&!strcmp(V("jar_fontPack"),"sans"));
 Set("ui_jarMP_cg_fov","nan");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_fontPack"),"sans"));
 Set("ui_jarMP_cg_fov","100");packExists=qfalse;Run("jarApply","");assert(commands==reloads&&strcmp(V("cg_fov"),"100")&&!strcmp(V("jar_uiPack"),"up2"));
 packExists=qtrue;Run("jarApply","");assert(commands==reloads+1&&!strcmp(command,"vid_restart\n")&&!strcmp(V("cg_fov"),"100")&&!strcmp(V("jar_fontPack"),"serif")&&!strcmp(V("jar_uiPack"),"up4"));reloads=commands;
 Run("jarApply","");assert(commands==reloads);
 Set("ui_jarMP_r_fontSharpness","4");Run("jarApply","");assert(commands==reloads&&!strcmp(V("r_fontSharpness"),"4"));
 const char*badPacks[]={"","../base","up4;quit","SANS","custom"};for(int i=0;i<ARRAY_LEN(badPacks);++i){Run("jarOpen","");Set("ui_jarMP_jar_fontPack",badPacks[i]);Set("ui_jarMP_con_height","0.5");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_fontPack"),"serif")&&!strcmp(V("con_height"),"1"));}
 Run("jarSettingsRevert","");Set("ui_jarMP_jar_uiPack","sans");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_uiPack"),"up4"));Run("jarSettingsRevert","");
 Set("ui_jarMP_jar_fontPack","stock");Set("ui_jarMP_jar_uiPack","stock");packExists=qfalse;int probes=openedFiles;Run("jarApply","");assert(commands==reloads+1&&openedFiles==probes&&!strcmp(V("jar_fontPack"),"stock"));packExists=qtrue;reloads=commands;
 Set("jar_fontPack","customfont");Set("jar_uiPack","customui");Set("r_fontSharpness","8");Run("jarOpen","");Set("ui_jarMP_jar_cameraPreset","2");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_fontPack"),"customfont")&&!strcmp(V("jar_uiPack"),"customui")&&!strcmp(V("r_fontSharpness"),"8"));
 Set("ui_jarMP_jar_fontPack","sans");Run("jarSettingsRevert","");assert(!strcmp(V("ui_jarMP_jar_fontPack"),"customfont"));
 Set("ui_jarMP_jar_uiPack","up2");Run("jarSettingsPage","interface");Run("jarBack","");assert(focused==m);Run("jarOpen","");assert(!strcmp(V("ui_jarMP_jar_uiPack"),"customui")&&commands==reloads);
 Set("ui_jarMP_r_fontSharpness","0");Set("ui_jarMP_jar_fontPack","sans");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_fontPack"),"customfont"));
 focused=Menus_FindByName("jarMPPause");Run("jarOpen","");Run("jarSettingsPage","interface");Run("jarBack","");assert(!strcmp(focused->window.name,"jarMPPause"));
 puts("MP interface menu: shared SP choices, pack probes, atomic validation, one video reload, custom preservation and Revert/Back passed.");
 // HUD uses native Academy/simple modes; apply reloads once and rejects
 // invalid choices/missing retail definitions before any setting is written.
 focused=m;Set("cg_hudFiles","ui/custom_hud.txt");Set("jar_uiMotion","1");Run("jarOpen","");Run("jarSettingsPage","interface");reloads=commands;
 itemDef_t*h=Menu_FindItemByName(focused,"hud"),*u=Menu_FindItemByName(focused,"motion");assert(h&&u&&h->typeData.multi->count==2&&u->typeData.multi->count==2);assert(!strcmp(h->typeData.multi->cvarStr[1],"1")&&!strcmp(u->typeData.multi->cvarStr[0],"0"));
 Set("ui_jarMP_jar_uiMotion","0");Run("jarApply","");assert(!strcmp(V("jar_uiMotion"),"0")&&!strcmp(V("cg_hudFiles"),"ui/custom_hud.txt")&&commands==reloads);
 Set("ui_jarMP_cg_hudFiles","1");hudExists=qfalse;Set("ui_jarMP_jar_uiMotion","1");Run("jarApply","");assert(!strcmp(V("jar_uiMotion"),"0")&&!strcmp(V("cg_hudFiles"),"ui/custom_hud.txt")&&commands==reloads);hudExists=qtrue;
 Run("jarApply","");assert(!strcmp(V("cg_hudFiles"),"1")&&!strcmp(V("jar_uiMotion"),"1")&&commands==reloads+1&&!strcmp(command,"vid_restart\n"));reloads=commands;
 Set("ui_jarMP_cg_hudFiles","ui/jahud.txt");Set("ui_jarMP_jar_fontPack","sans");Run("jarApply","");assert(commands==reloads+1&&!strcmp(V("cg_hudFiles"),"ui/jahud.txt"));reloads=commands;
 const char*badHud[]={"2","../hud.txt","ui/custom_hud.txt","1;quit"};for(int n=0;n<ARRAY_LEN(badHud);++n){Run("jarOpen","");Set("ui_jarMP_cg_hudFiles",badHud[n]);Set("ui_jarMP_jar_uiMotion","0");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_uiMotion"),"1")&&!strcmp(V("cg_hudFiles"),"ui/jahud.txt"));}
 Run("jarSettingsRevert","");Set("ui_jarMP_jar_uiMotion","0.5");Run("jarApply","");assert(!strcmp(V("jar_uiMotion"),"1")&&commands==reloads);Run("jarSettingsRevert","");
 Set("ui_jarMP_cg_hudFiles","1");Set("ui_jarMP_jar_uiMotion","0");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("ui_jarMP_cg_hudFiles"),"ui/jahud.txt")&&!strcmp(V("ui_jarMP_jar_uiMotion"),"1")&&commands==reloads);
 puts("MP HUD/motion menu: native choices, custom preservation, resource validation, atomic Apply, one reload and cancellation passed.");
 // Classic graphics pages reuse SP's choices and native consumers. A failed
 // graphics choice blocks all pages; only restart-sensitive changes reload.
 const char*gfxKeys[]={"r_ext_multisample","r_dynamicGlow","r_swapInterval","com_maxfps","cg_drawFPS","r_picmip","r_textureMode","r_ext_texture_filter_anisotropic","r_lodbias","r_subdivisions","cg_shadows","r_dynamiclight","r_surfaceSprites","cg_marks","r_fastsky"};
 const int gfxRestarts[]={1,1,0,0,0,1,0,1,0,1,0,0,0,0,0};
 const char*gfxPages[]={"graphics","textures","detail"},*gfxMenus[]={"jarMPGraphics","jarMPTextures","jarMPDetail"};
 focused=m;Run("jarOpen","");reloads=commands;
 for(int page=0;page<3;++page){Run("jarSettingsPage",gfxPages[page]);assert(!strcmp(focused->window.name,gfxMenus[page]));for(int row=0;row<5;++row){int n=page*5+row;itemDef_t*control=Menu_FindItemByName(focused,gfxKeys[n]);assert(control&&control->type==ITEM_TYPE_MULTI);multiDef_t*options=control->typeData.multi;const char*value=strcmp(V(gfxKeys[n]),options->cvarStr[options->count-1])?options->cvarStr[options->count-1]:options->cvarStr[0];Run("jarSettingsRevert","");Set(va("ui_jarMP_%s",gfxKeys[n]),value);Run("jarApply","");assert(!strcmp(V(gfxKeys[n]),value));reloads+=gfxRestarts[n];assert(commands==reloads);if(gfxRestarts[n])assert(!strcmp(command,"vid_restart\n"));}}
 assert(!strcmp(Menu_FindItemByName(Menus_FindByName("jarMPGraphics"),"com_maxfps")->typeData.multi->cvarList[5],"Unlimited"));
 assert(!strcmp(Menu_FindItemByName(Menus_FindByName("jarMPDetail"),"cg_shadows")->typeData.multi->cvarList[3],"Projected"));
 for(int n=0;n<15;++n){Run("jarOpen","");char oldValue[64];Q_strncpyz(oldValue,V(gfxKeys[n]),sizeof(oldValue));Set(va("ui_jarMP_%s",gfxKeys[n]),"invalid;quit");Set("ui_jarMP_cg_fov","99");Run("jarApply","");assert(!strcmp(V(gfxKeys[n]),oldValue)&&strcmp(V("cg_fov"),"99")&&commands==reloads);}
 Set("r_picmip","7");Set("r_textureMode","GL_NEAREST");Run("jarOpen","");Set("ui_jarMP_jar_consoleStyle","1");Run("jarApply","");assert(!strcmp(V("r_picmip"),"7")&&!strcmp(V("r_textureMode"),"GL_NEAREST")&&commands==reloads);
 Run("jarSettingsPage","textures");Set("ui_jarMP_r_picmip","0");Run("jarSettingsPage","graphics");Set("ui_jarMP_r_ext_multisample","2");Run("jarSettingsPage","interface");Set("ui_jarMP_cg_hudFiles","1");Run("jarApply","");assert(commands==reloads+1&&!strcmp(V("r_picmip"),"0")&&!strcmp(V("r_ext_multisample"),"2")&&!strcmp(V("cg_hudFiles"),"1"));reloads=commands;
 Run("jarApply","");assert(commands==reloads);Set("ui_jarMP_r_picmip","3");Run("jarSettingsRevert","");assert(!strcmp(V("ui_jarMP_r_picmip"),"0"));Set("ui_jarMP_r_picmip","3");Run("jarBack","");assert(focused==m);Run("jarOpen","");assert(!strcmp(V("ui_jarMP_r_picmip"),"0"));
 focused=Menus_FindByName("jarMPPause");Run("jarOpen","");Run("jarSettingsPage","detail");Run("jarBack","");assert(!strcmp(focused->window.name,"jarMPPause"));
 puts("MP classic graphics menu: fifteen SP controls, native choices, live/restart routing, cross-page atomic validation, custom preservation and cancellation passed.");
 // Display transactions validate every page before writes, survive UI state
 // reset via engine cvars, and require explicit Keep after the actual restart.
 focused=m;Run("jarOpen","");Run("jarSettingsPage","display");assert(!strcmp(focused->window.name,"jarMPDisplay"));reloads=commands;
 assert(Menu_FindItemByName(focused,"width")->typeData.edit->maxChars==5);
 const char*badSizes[]={"0","319","16385","999999999999999999999","1e3","800;quit","800.0","-800",""};
 for(int n=0;n<ARRAY_LEN(badSizes);++n){Run("jarSettingsRevert","");Set("ui_jarMP_r_customwidth",badSizes[n]);Set("ui_jarMP_cg_fov","97");Run("jarApply","");assert(strcmp(V("cg_fov"),"97")&&!strcmp(V("r_mode"),"4")&&commands==reloads&&!DisplayState());}
 Run("jarSettingsRevert","");Set("ui_jarMP_r_customwidth","16384");Set("ui_jarMP_r_customheight","16384");Run("jarApply","");assert(!DisplayState()&&commands==reloads);
 Run("jarSettingsRevert","");Set("ui_jarMP_r_customwidth","1280");Set("ui_jarMP_r_customheight","720");Set("ui_jarMP_cg_fov","97");refuseDisplay=qtrue;Run("jarApply","");assert(!DisplayState()&&commands==reloads&&strcmp(V("cg_fov"),"97")&&!strcmp(V("r_mode"),"4"));refuseDisplay=qfalse;
 Set("ui_jarMP_r_ext_multisample","4");Run("jarApply","");assert(DisplayState()==1&&!strcmp(V("r_mode"),"-1")&&!strcmp(V("r_customwidth"),"1280")&&!strcmp(V("cg_fov"),"97")&&!strcmp(V("r_ext_multisample"),"4")&&commands==++reloads);
 Run("jarApply","");Run("jarStartHost","");Run("jarResume","");assert(commands==reloads&&DisplayState()==1);
 // Simulate the DLL losing its statics on restart. The parent and trial
 // countdown are engine-owned; refresh always restores confirmation focus.
 strcpy(jarReturn,"bad");focused=NULL;Set("ui_jarDisplayState","2");Set("ui_jarDisplaySeconds","19");uiInfo.uiDC.glconfig.vidWidth=1280;uiInfo.uiDC.glconfig.vidHeight=720;UI_JAR_Refresh();assert(!strcmp(focused->window.name,"jarMPDisplayConfirm")&&strstr(displayPrompt,"1280 x 720")&&strstr(displayPrompt,"19 seconds"));
 assert(!strcmp(focused->onESC,"uiScript jarDisplayRevert ;"));focused=m;UI_JAR_Refresh();assert(!strcmp(focused->window.name,"jarMPDisplayConfirm"));Run("jarDisplayKeep","");assert(!DisplayState()&&focused==m&&commands==reloads);
 Run("jarOpen","");Set("ui_jarMP_r_fullscreen","2");Run("jarApply","");assert(DisplayState()==1&&!strcmp(V("r_mode"),"-2")&&!strcmp(V("r_fullscreen"),"2")&&commands==++reloads);Set("ui_jarDisplayState","2");Run("jarDisplayRevert","");assert(DisplayState()==3&&commands==++reloads);Set("ui_jarDisplayState","0");
 Set("r_mode","13");Set("r_customwidth","1234");Set("r_customheight","777");Run("jarOpen","");Run("jarApply","");assert(!DisplayState()&&!strcmp(V("r_mode"),"13")&&commands==reloads);
 Set("ui_jarMP_r_customwidth","900");Run("jarSettingsRevert","");assert(!strcmp(V("ui_jarMP_r_customwidth"),"1234"));Set("ui_jarMP_r_customwidth","900");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("ui_jarMP_r_customwidth"),"1234"));
 puts("MP display menu: atomic validation, custom mode, desktop borderless, trial refusal, one restart, confirmation after reload, Keep/Restore and cancellation passed.");
 // Cosmetic selectors expose SP's exact choices and one complete pending
 // transaction. Native MP needs sound restart before its one video restart.
 focused=m;Run("jarOpen","");Run("jarSettingsPage","effects");assert(!strcmp(focused->window.name,"jarMPEffects"));reloads=commands;
 const char*effectKeys[]={"jar_saberPack","jar_soundPack","jar_fxPack"},*effectIds[]={"saberPack","soundPack","fxPack"};const int effectCounts[]={4,6,2};
 for(int n=0;n<3;++n){itemDef_t*i=Menu_FindItemByName(focused,effectIds[n]);assert(i&&i->type==ITEM_TYPE_MULTI&&i->typeData.multi->count==effectCounts[n]);for(int k=1;k<effectCounts[n];++k){const char*value=i->typeData.multi->cvarStr[k];Set(va("ui_jarMP_%s",effectKeys[n]),value);Run("jarApply","");assert(!strcmp(V(effectKeys[n]),value)&&commands==++reloads);assert(!strcmp(command,n==1?"snd_restart\nvid_restart\n":"vid_restart\n"));}}
 for(int n=0;n<3;++n){Run("jarSettingsRevert","");char prior[64];Q_strncpyz(prior,V(effectKeys[n]),sizeof(prior));Set(va("ui_jarMP_%s",effectKeys[n]),"../bad;quit");Set("ui_jarMP_cg_fov","101");Run("jarApply","");assert(!strcmp(V(effectKeys[n]),prior)&&strcmp(V("cg_fov"),"101")&&commands==reloads);}
 Run("jarSettingsRevert","");packExists=qfalse;Set("ui_jarMP_jar_saberPack","cinematic");Set("ui_jarMP_cg_fov","101");Set("ui_jarMP_r_customwidth","1440");Run("jarApply","");assert(commands==reloads&&!DisplayState()&&strcmp(V("cg_fov"),"101"));packExists=qtrue;
 Run("jarSettingsRevert","");Set("ui_jarMP_jar_soundPack","stock");Set("ui_jarMP_jar_saberPack","stock");Set("ui_jarMP_jar_fxPack","stock");Set("ui_jarMP_r_ext_multisample","0");Set("ui_jarMP_r_customwidth","1440");Run("jarApply","");assert(commands==++reloads&&DisplayState()==1&&!strcmp(command,"snd_restart\nvid_restart\n"));Set("ui_jarDisplayState","2");Run("jarDisplayKeep","");
 Set("jar_fxPack","custom");Run("jarOpen","");Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_fxPack"),"custom"));Set("ui_jarMP_jar_fxPack","mbii");Run("jarSettingsRevert","");assert(!strcmp(V("ui_jarMP_jar_fxPack"),"custom"));Set("ui_jarMP_jar_fxPack","mbii");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("ui_jarMP_jar_fxPack"),"custom")&&commands==reloads);
 puts("MP effects menu: complete shared choices, native reload ordering, cross-page atomic validation, unavailable packs, one combined display restart, custom preservation and cancellation passed.");
 // Facial controls apply live through the same transaction as every page.
 focused=m;Run("jarOpen","");Run("jarSettingsPage","effects");reloads=commands;
 itemDef_t*faceControl=Menu_FindItemByName(focused,"facePreset"),*speechControl=Menu_FindItemByName(focused,"faceSensitivity");assert(faceControl&&speechControl&&faceControl->typeData.multi->count==4&&speechControl->typeData.multi->count==4);
 for(int n=0;n<4;++n){Set("ui_jarMP_jar_facePreset",faceControl->typeData.multi->cvarStr[n]);Set("ui_jarMP_jar_faceSensitivity",speechControl->typeData.multi->cvarStr[n]);Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_facePreset"),faceControl->typeData.multi->cvarStr[n])&&!strcmp(V("jar_faceSensitivity"),speechControl->typeData.multi->cvarStr[n]));}
 Set("ui_jarMP_jar_faceSensitivity","nan");Set("ui_jarMP_cg_fov","102");Run("jarApply","");assert(!strcmp(V("jar_faceSensitivity"),"2")&&strcmp(V("cg_fov"),"102")&&commands==reloads);
 Run("jarSettingsRevert","");Set("ui_jarMP_jar_facePreset","4");Run("jarApply","");assert(!strcmp(V("jar_facePreset"),"3"));Run("jarSettingsRevert","");Set("ui_jarMP_jar_facePreset","0");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("ui_jarMP_jar_facePreset"),"3"));
 puts("MP facial menu: four shared presets and sensitivities, live Apply, atomic rejection and cancellation passed.");
 // Combat choices are live, transactional, and advertised through userinfo.
 Run("jarOpen","");Run("jarSettingsPage","combat");reloads=commands;assert(!strcmp(focused->window.name,"jarMPCombat")&&(fistFlags&CVAR_USERINFO));
 itemDef_t*extensionControl=Menu_FindItemByName(focused,"saberExtension"),*fistControl=Menu_FindItemByName(focused,"fistInput");assert(extensionControl&&fistControl&&extensionControl->typeData.multi->count==2&&fistControl->typeData.multi->count==2);
 for(int n=0;n<2;++n){Set("ui_jarMP_jar_saberExtension",extensionControl->typeData.multi->cvarStr[n]);Set("ui_jarMP_jar_continuousFists",fistControl->typeData.multi->cvarStr[n]);Run("jarApply","");assert(commands==reloads&&!strcmp(V("jar_saberExtension"),extensionControl->typeData.multi->cvarStr[n])&&!strcmp(V("jar_continuousFists"),fistControl->typeData.multi->cvarStr[n]));}
 Set("ui_jarMP_jar_continuousFists","1.5");Set("ui_jarMP_jar_saberExtension","1");Run("jarApply","");assert(!strcmp(V("jar_continuousFists"),"0")&&!strcmp(V("jar_saberExtension"),"0")&&commands==reloads);Run("jarSettingsRevert","");Set("ui_jarMP_jar_saberExtension","1");Run("jarBack","");Run("jarOpen","");assert(!strcmp(V("ui_jarMP_jar_saberExtension"),"0"));
 puts("MP combat menu: shared choices, userinfo preference, live Apply, atomic refusal and cancellation passed.");
 // Every new field/button is inside the virtual canvas; host text scrolls within its column.
 for(int n=0;n<count;++n){menuDef_t*p=&testMenus[n];if(Q_stricmpn(p->window.name,"jarMP",5))continue;for(int k=0;k<p->itemCount;++k){rectDef_t*r=&p->items[k]->window.rectClient;assert(r->x>=0&&r->y>=0&&r->x+r->w<=640&&r->y+r->h<=480);} }
 puts("PASS: native MP front end, actions, state preservation, validation, host commands and menu return");return 0;
}
