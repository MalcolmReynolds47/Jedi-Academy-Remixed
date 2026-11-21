#include "ui_jar_map_bsp.h"

// Discover installed maps independently of optional arena metadata.
enum { HOST_MAP_CARDS = 8, HOST_MAP_LIMIT = 4096 };
typedef struct {
    char name[MAX_QPATH], title[64];
    jarHostMapStatus_t status;
    qhandle_t preview;
    qboolean inspected, previewRead;
} hostMap_t;
static hostMap_t hostMaps[HOST_MAP_LIMIT];
static int hostMapRows[HOST_MAP_LIMIT], hostMapsFound, hostMapCount, hostMapPage;
static int hostMapTitleCursor;
static char hostMapSearch[64], hostMapPageText[64], hostMapTitles[HOST_MAP_CARDS][64];
static char hostMapDetails[HOST_MAP_CARDS][64];

static void HostMapShow(itemDef_t* item, qboolean visible) {
    if (!item) return;
    if (visible) item->window.flags |= WINDOW_VISIBLE;
    else item->window.flags &= ~(WINDOW_VISIBLE | WINDOW_HASFOCUS | WINDOW_MOUSEOVER);
}

static void HostMapLabel(itemDef_t* item,char* label,int capacity,const char* text,int width) {
    Q_strncpyz(label,text,capacity);
    if (!item) return;
    memset(&item->textRect,0,sizeof(item->textRect));
    if (!trap->R_Font_StrLenPixels) return;
    int size=(int)strlen(label);
    if (trap->R_Font_StrLenPixels(label,UI_JAR_Font(),item->textscale)<=width) return;
    while (size>3) {
        --size;
        memcpy(label+size-3,"...",4);
        if (trap->R_Font_StrLenPixels(label,UI_JAR_Font(),item->textscale)<=width) return;
    }
}

static int CompareHostMaps(const void* left, const void* right) {
    const hostMap_t* a=left;
    const hostMap_t* b=right;
    return Q_stricmp(a->name,b->name);
}

static void HostMapsDiscover(void) {
    static char files[HOST_MAP_LIMIT*MAX_QPATH];
    hostMapTitleCursor=hostMapsFound=0;
    if (!trap->FS_GetFileList) return;
    const int count=trap->FS_GetFileList("maps",".bsp",files,sizeof(files));
    const char* next=files;
    const char* limit=files+sizeof(files);
    for (int i=0;i<count && next<limit && hostMapsFound<HOST_MAP_LIMIT;++i) {
        const char* end=memchr(next,0,limit-next);
        if (!end) break;
        const int size=(int)(end-next);
        const char* filename=next;
        next=end+1;
        if (size<=4 || size>=MAX_QPATH || Q_stricmp(end-4,".bsp")) continue;
        char name[MAX_QPATH];
        memcpy(name,filename,size-4);
        name[size-4]=0;
        if (!UI_JAR_HostMapNameValid(name)) continue;
        hostMap_t* map=&hostMaps[hostMapsFound++];
        memset(map,0,sizeof(*map));
        Q_strncpyz(map->name,name,sizeof(map->name));
        for (int a=0;a<uiInfo.mapCount && a<MAX_MAPS;++a) {
            const mapInfo_t* arena=&uiInfo.mapList[a];
            if (!arena->mapLoadName || Q_stricmp(arena->mapLoadName,name) ||
                !arena->mapName || !arena->mapName[0]) continue;
            Q_strncpyz(map->title,arena->mapName,sizeof(map->title));
            break;
        }
    }
    qsort(hostMaps,hostMapsFound,sizeof(hostMaps[0]),CompareHostMaps);
    int unique=0;
    for (int i=0;i<hostMapsFound;++i)
        if (!unique || Q_stricmp(hostMaps[unique-1].name,hostMaps[i].name)) hostMaps[unique++]=hostMaps[i];
    hostMapsFound=unique;
}

static void HostMapsPaint(void) {
    menuDef_t* menu=Menus_FindByName("jarMPHostMaps");
    if (!menu) return;
    for (int slot=0; slot<HOST_MAP_CARDS; ++slot) {
        itemDef_t* card=Menu_FindItemByName(menu,va("map%d",slot));
        itemDef_t* shot=Menu_FindItemByName(menu,va("shot%d",slot));
        itemDef_t* detail=Menu_FindItemByName(menu,va("detail%d",slot));
        const int row=hostMapPage*HOST_MAP_CARDS+slot;
        HostMapShow(card,row<hostMapCount);
        HostMapShow(shot,qfalse);
        HostMapShow(detail,qfalse);
        if (row>=hostMapCount || !card || !shot) continue;
        hostMap_t* map=&hostMaps[hostMapRows[row]];
        HostMapLabel(card,hostMapTitles[slot],sizeof(hostMapTitles[slot]),map->title[0]?map->title:map->name,120);
        shot->window.background=map->preview;
        HostMapShow(shot,map->preview!=0);
        const char* label=!map->inspected?"":map->status==JAR_HOST_MAP_NO_START?"No FFA player start":
                          map->status!=JAR_HOST_MAP_PLAYABLE?"Map unavailable":map->title[0]?map->name:"";
        HostMapLabel(detail,hostMapDetails[slot],sizeof(hostMapDetails[slot]),label,128);
        HostMapShow(detail,label[0]!=0);
    }
    Com_sprintf(hostMapPageText,sizeof(hostMapPageText),"%d / %d   |   %d maps",hostMapPage+1,
                hostMapCount?(hostMapCount+HOST_MAP_CARDS-1)/HOST_MAP_CARDS:1,hostMapCount);
    itemDef_t* page=Menu_FindItemByName(menu,"page");
    if (page) memset(&page->textRect,0,sizeof(page->textRect));
    HostMapShow(Menu_FindItemByName(menu,"previous"),hostMapPage>0);
    HostMapShow(Menu_FindItemByName(menu,"next"),(hostMapPage+1)*HOST_MAP_CARDS<hostMapCount);
}

static void HostMapInspect(hostMap_t* map, qboolean preview) {
    if (preview ? map->previewRead : map->inspected) return;
    char title[sizeof(map->title)];
    map->status=UI_JAR_InspectHostMap(trap,map->name,title,sizeof(title),preview ? &map->preview : NULL);
    if (!map->title[0] && title[0]) Q_strncpyz(map->title,title,sizeof(map->title));
    map->inspected=qtrue;
    if (preview) map->previewRead=qtrue;
}

static void HostMapsFilter(void) {
    hostMapCount=0;
    for (int i=0; i<hostMapsFound; ++i) {
        hostMap_t* map=&hostMaps[i];
        if (*hostMapSearch && !Q_stristr(map->name,hostMapSearch) && !Q_stristr(map->title,hostMapSearch)) continue;
        hostMapRows[hostMapCount++]=i;
    }
    if (hostMapPage*HOST_MAP_CARDS >= hostMapCount) hostMapPage=0;
    HostMapsPaint();
}

static void HostMapsAction(const char* action) {
    if (!strcmp(action,"open")) {
        if (!Menu_GetFocused() || Q_stricmp(Menu_GetFocused()->window.name,"jarMPHost")) return;
        menuDef_t* menu=Page("jarMPHostMaps","Maps",NULL);
        if (!menu) return;
        UI_LoadArenas();
        HostMapsDiscover();
        if (!Menu_FindItemByName(menu,"search")) {
            itemDef_t* search=Field(menu,"search","Search: ","ui_jarHostMapSearch",52,48,qfalse);
            Rect(search,304,52,304,28);
            for (int slot=0; slot<HOST_MAP_CARDS; ++slot) {
                const float x=32+146*(slot%4), y=120+142*(slot/4);
                itemDef_t* card=Add(menu,va("map%d",slot),"",x,y,138,130,va("uiScript jarHostMaps %d ;",slot));
                if (card) {
                    card->text=hostMapTitles[slot];
                    card->textaligny=96;
                    card->textscale=.56f;
                }
                itemDef_t* shot=Add(menu,va("shot%d",slot),NULL,x+5,y+5,128,88,NULL);
                if (shot) shot->window.style=WINDOW_STYLE_SHADER;
                itemDef_t* detail=Add(menu,va("detail%d",slot),"",x+5,y+105,128,18,NULL);
                if (detail) { detail->text=hostMapDetails[slot]; detail->textscale=.46f; }
            }
            Add(menu,"back","Back",32,440,84,32,"uiScript jarHostMaps back ;");
            Add(menu,"previous","Previous",128,440,90,32,"uiScript jarHostMaps previous ;");
            Add(menu,"next","Next",226,440,76,32,"uiScript jarHostMaps next ;");
            itemDef_t* page=Add(menu,"page","",318,448,244,24,NULL);
            if (page) { page->text=hostMapPageText; page->textscale=.65f; }
            menu->onESC=String_Alloc("uiScript jarHostMaps back ;");
            Menu_PostParse(menu);
        }
        trap->Cvar_Set("ui_jarHostMapSearch","");
        hostMapSearch[0]=0; hostMapPage=0;
        HostMapsFilter();
        Open("jarMPHostMaps");
        return;
    }
    if (!Menu_GetFocused() || Q_stricmp(Menu_GetFocused()->window.name,"jarMPHostMaps")) return;
    if (!strcmp(action,"back")) {
        Open("jarMPHost");
    } else if (!strcmp(action,"previous")) {
        if (hostMapPage>0) --hostMapPage;
        HostMapsPaint();
    } else if (!strcmp(action,"next")) {
        if ((hostMapPage+1)*HOST_MAP_CARDS<hostMapCount) ++hostMapPage;
        HostMapsPaint();
    } else if (action[0]>='0' && action[0]<'0'+HOST_MAP_CARDS && !action[1]) {
        const int row=hostMapPage*HOST_MAP_CARDS+action[0]-'0';
        if (row<hostMapCount) {
            hostMap_t* map=&hostMaps[hostMapRows[row]];
            HostMapInspect(map,qtrue);
            if (map->status!=JAR_HOST_MAP_PLAYABLE) { HostMapsPaint(); return; }
            trap->Cvar_Set("ui_jarHostMap",map->name);
            Open("jarMPHost");
        }
    }
}

static void HostMapsRefresh(void) {
    menuDef_t* menu=Menu_GetFocused();
    if (!menu || Q_stricmp(menu->window.name,"jarMPHostMaps")) return;
    char search[sizeof(hostMapSearch)];
    Value("ui_jarHostMapSearch",search,sizeof(search));
    qboolean changed=strcmp(search,hostMapSearch)!=0;
    if (changed) { Q_strncpyz(hostMapSearch,search,sizeof(hostMapSearch)); hostMapPage=0; }
    // Index off-page titles independently, without registering their thumbnails
    // or opening thousands of BSPs in a single frame when the user types.
    int budget=4;
    while (hostMapTitleCursor<hostMapsFound && budget>0) {
        hostMap_t* map=&hostMaps[hostMapTitleCursor++];
        if (map->inspected || map->title[0]) continue;
        HostMapInspect(map,qfalse); --budget;
        if (*hostMapSearch && map->title[0] && Q_stristr(map->title,hostMapSearch)) changed=qtrue;
    }
    if (changed) HostMapsFilter();
    // Like the SP picker, process one visible BSP per refresh.
    const int first=hostMapPage*HOST_MAP_CARDS;
    for (int row=first;row<hostMapCount && row<first+HOST_MAP_CARDS;++row) {
        hostMap_t* map=&hostMaps[hostMapRows[row]];
        if (map->previewRead) continue;
        HostMapInspect(map,qtrue);
        HostMapsPaint();
        break;
    }
}
