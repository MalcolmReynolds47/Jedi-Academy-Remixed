// Single-player map grid and lazy thumbnail generation. GPL-2.0-only.
#include "ui/jar_map_bsp.h"

namespace {
constexpr const char* mapPickerMenu="jarMapPicker";
constexpr int mapCards=8;
struct PickerMap {
    std::string name;
    jar::mapPicker::MapInfo info;
    qhandle_t preview=0;
    bool infoRead=false, previewRead=false;
};
std::vector<PickerMap> pickerMaps;
std::vector<int> pickerMatches;
std::string pickerSearch;
std::string pickerParent;
int pickerPage=0, pickerSelected=-1;
int pickerSkill=0;
bool pickerBuildScript=false;
size_t pickerSearchCursor=0;
char pickerNames[mapCards][64]{}, pickerIds[mapCards][64]{};
char pickerPageText[64]{}, pickerDetails[144]{};

class PickerBsp {
public:
    explicit PickerBsp(const char* name) {
        length=ui.FS_FOpenFile(name,&file,FS_READ);
        if (!file || length<int(sizeof(header)) ||
            ui.FS_Read(&header,sizeof(header),file)!=sizeof(header)) return;
        offset=sizeof(header);
        valid=jar::mapPicker::ValidHeader(header,length);
    }
    ~PickerBsp() { if (file) ui.FS_FCloseFile(file); }
    PickerBsp(const PickerBsp&)=delete;
    PickerBsp& operator=(const PickerBsp&)=delete;
    explicit operator bool() const { return valid; }
    int Offset(int index) const { return header.lumps[index].fileofs; }

    template<class T>
    bool Read(int index,int limit,std::vector<T>& result) {
        const auto lump=header.lumps[index];
        if (!valid || lump.filelen>limit || size_t(lump.filelen)%sizeof(T)) return false;
        result.resize(lump.filelen/sizeof(T));
        if (result.empty()) return true;
        if (lump.fileofs!=offset) {
            const bool forward=lump.fileofs>offset;
            if (FS_Seek(file,forward ? lump.fileofs-offset : lump.fileofs,
                        forward ? FS_SEEK_CUR : FS_SEEK_SET)<0) return false;
        }
        offset=lump.fileofs+lump.filelen;
        return ui.FS_Read(result.data(),lump.filelen,file)==lump.filelen;
    }

private:
    dheader_t header{};
    fileHandle_t file=0;
    int length=0, offset=0;
    bool valid=false;
};

bool PickerFileExists(const char* path) {
    fileHandle_t file=0;
    const int size=ui.FS_FOpenFile(path,&file,FS_READ);
    if (file) ui.FS_FCloseFile(file);
    return size>0;
}

qhandle_t PickerLevelshot(const std::string& name) {
    const std::string path="levelshots/"+name;
    if (path.size()>=MAX_QPATH) return 0;
    for (const char* extension:{".jpg",".tga",".png"}) {
        const auto file=path+extension;
        if (file.size()<MAX_QPATH && PickerFileExists(file.c_str()))
            if (const auto preview=ui.R_RegisterShaderNoMip(path.c_str())) return preview;
    }
    return 0;
}

qhandle_t PickerGeneratedShot(PickerBsp& bsp,const jar::mapPicker::Point& focus) {
    jar::mapPicker::Image image;
    if (!jar::mapPicker::RasterizeBsp(bsp,focus,image)) return 0;
    char path[MAX_QPATH];
    jar::mapPicker::ThumbnailPath(image,path);
    if (!PickerFileExists(path)) {
        fileHandle_t file=0;
        ui.FS_FOpenFile(path,&file,FS_WRITE);
        if (!file) return 0;
        const int written=ui.FS_Write(image.data(),int(image.size()),file);
        ui.FS_FCloseFile(file);
        if (written!=int(image.size())) return 0;
    }
    return ui.R_RegisterShaderNoMip(path);
}

void InspectPickerInfo(PickerMap& map,PickerBsp& bsp) {
    map.infoRead=true;
    map.info={};
    std::vector<char> entities;
    if (bsp && bsp.Read(LUMP_ENTITIES,4*1024*1024,entities) && !entities.empty())
        map.info=jar::mapPicker::ReadEntities({entities.data(),entities.size()},
            jar::mapPicker::StartMode::SinglePlayer,pickerSkill,pickerBuildScript);
}

void InspectPickerMap(PickerMap& map) {
    map.previewRead=true;
    PickerBsp bsp(("maps/"+map.name+".bsp").c_str());
    if (!map.infoRead) InspectPickerInfo(map,bsp);
    if (!bsp) return;
    map.preview=PickerLevelshot(map.name);
    if (!map.preview) map.preview=PickerGeneratedShot(bsp,map.info.start);
}

void DiscoverPickerMaps() {
    pickerMaps.clear();
    if (!ui.FS_GetFileList || !ui.FS_FOpenFile || !ui.FS_Read || !ui.FS_Write ||
        !ui.FS_FCloseFile || !ui.R_RegisterShaderNoMip) return;
    std::vector<char> filenames(256*1024);
    const int count=ui.FS_GetFileList("maps",".bsp",filenames.data(),int(filenames.size()));
    const char* next=filenames.data();
    const char* limit=next+filenames.size();
    for (int i=0;i<count && next<limit;++i) {
        const auto* end=static_cast<const char*>(std::memchr(next,0,limit-next));
        if (!end) break;
        const std::string filename(next,end);
        next=end+1;
        if (filename.size()<=4 || Q_stricmp(end-4,".bsp")) continue;
        const auto name=filename.substr(0,filename.size()-4);
        if (jar::mapPicker::SafeName(name)) pickerMaps.push_back({name});
    }
    std::sort(pickerMaps.begin(),pickerMaps.end(),[](const auto& a,const auto& b) {
        return Q_stricmp(a.name.c_str(),b.name.c_str())<0;
    });
}

int PickerLastPage() {
    return pickerMatches.empty() ? 0 : int((pickerMatches.size()-1)/mapCards);
}
int PickerCardIndex(int slot) {
    const size_t index=pickerPage*mapCards+slot;
    return index<pickerMatches.size() ? pickerMatches[index] : -1;
}
void ShowPickerItem(itemDef_t& item,bool show) {
    if (show) item.window.flags|=WINDOW_VISIBLE;
    else item.window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS|WINDOW_MOUSEOVER);
}

void SetPickerLabel(char (&label)[64],const std::string& value,const itemDef_t& item) {
    Q_strncpyz(label,value.c_str(),sizeof(label));
    if (!ui.R_Font_StrLenPixels) return;
    size_t size=std::strlen(label);
    if (ui.R_Font_StrLenPixels(label,item.font,item.textscale)<=126) return;
    while (size>3) {
        --size;
        std::memcpy(label+size-3,"...",4);
        if (ui.R_Font_StrLenPixels(label,item.font,item.textscale)<=126) return;
    }
}

void UpdatePickerPage() {
    auto* menu=Menus_FindByName(mapPickerMenu);
    if (!menu) return;
    for (int slot=0;slot<mapCards;++slot) {
        auto* card=Menu_FindItemByName(menu,va("card%d",slot));
        auto* shot=Menu_FindItemByName(menu,va("shot%d",slot));
        auto* id=Menu_FindItemByName(menu,va("id%d",slot));
        if (!card || !shot || !id) continue;
        const int index=PickerCardIndex(slot);
        ShowPickerItem(*card,index>=0);
        if (index<0) {
            ShowPickerItem(*shot,false);
            ShowPickerItem(*id,false);
            continue;
        }
        const auto& map=pickerMaps[index];
        SetPickerLabel(pickerNames[slot],map.info.title.empty()?map.name:map.info.title,*card);
        SetPickerLabel(pickerIds[slot],map.name,*id);
        card->textRect={};
        id->textRect={};
        NativeColor(index==pickerSelected ? jar::ui::accent : jar::ui::edge,card->window.borderColor);
        shot->window.background=map.preview;
        ShowPickerItem(*shot,map.preview!=0);
        ShowPickerItem(*id,!map.info.title.empty() && Q_stricmp(map.info.title.c_str(),map.name.c_str()));
    }
    Com_sprintf(pickerPageText,sizeof(pickerPageText),"%d / %d   |   %d maps",pickerPage+1,
                PickerLastPage()+1,int(pickerMatches.size()));
    if (auto* item=Menu_FindItemByName(menu,"page")) item->textRect={};
    if (auto* item=Menu_FindItemByName(menu,"previous")) ShowPickerItem(*item,pickerPage>0);
    if (auto* item=Menu_FindItemByName(menu,"next")) ShowPickerItem(*item,pickerPage<PickerLastPage());
    const auto* selection=pickerSelected>=0 ? &pickerMaps[pickerSelected] : nullptr;
    if (auto* item=Menu_FindItemByName(menu,"launch"))
        ShowPickerItem(*item,selection && selection->info.playable);
    if (pickerMatches.empty()) Q_strncpyz(pickerDetails,pickerSearchCursor<pickerMaps.size()
        ? "Searching map titles..." : "No maps match this search.",sizeof(pickerDetails));
    else if (!selection) Q_strncpyz(pickerDetails,"Select a map.",sizeof(pickerDetails));
    else if (!selection->info.playable)
        Com_sprintf(pickerDetails,sizeof(pickerDetails),"%s has no usable start for this single-player difficulty.",selection->name.c_str());
    else Com_sprintf(pickerDetails,sizeof(pickerDetails),"%s  |  Starting replaces the autosave.",selection->name.c_str());
    if (auto* item=Menu_FindItemByName(menu,"details")) item->textRect={};
}

bool PickerMatchesQuery(const PickerMap& map) {
    return JAR_MenuMatches(map.name.c_str(),pickerSearch.c_str()) ||
        (map.infoRead && !map.info.title.empty() && JAR_MenuMatches(map.info.title.c_str(),pickerSearch.c_str()));
}

void AdvancePickerSearch() {
    while (pickerSearchCursor<pickerMaps.size() && pickerMaps[pickerSearchCursor].infoRead)
        ++pickerSearchCursor;
}

void FilterPickerMaps() {
    pickerSearch=Cvar_VariableString("ui_jar_mapSearch");
    pickerMatches.clear();
    for (size_t i=0;i<pickerMaps.size();++i)
        if (PickerMatchesQuery(pickerMaps[i]))
            pickerMatches.push_back(int(i));
    pickerSearchCursor=pickerSearch.empty()?pickerMaps.size():0;
    AdvancePickerSearch();
    pickerPage=0;
    pickerSelected=-1;
    UpdatePickerPage();
}

bool RefreshPickerFilter() {
    const int skill=std::atoi(Cvar_VariableString("g_spskill"));
    const bool buildScript=std::atoi(Cvar_VariableString("com_buildScript"))!=0;
    const bool policyChanged=skill!=pickerSkill || buildScript!=pickerBuildScript;
    if (!policyChanged && pickerSearch==Cvar_VariableString("ui_jar_mapSearch")) return false;
    if (policyChanged) {
        pickerSkill=skill;
        pickerBuildScript=buildScript;
        for (auto& map:pickerMaps) {
            map.infoRead=map.previewRead=false;
            map.preview=0;
        }
    }
    FilterPickerMaps();
    return true;
}

void BuildPickerMenu() {
    if (Menus_FindByName(mapPickerMenu)) return;
    auto* menu=Menu_CreateJAR(mapPickerMenu);
    if (!menu) return;
    menu->fullScreen=qtrue;
    menu->window.rect={0,0,640,480};
    menu->onESC="uiScript jarMapPicker close ;";
    NativeBase(menu);
    NativeLabel(menu,"title","Maps",32,20,180,1.25f);
    if (auto* search=AddItem(menu,"search","Search: ",300,27,308,"")) {
        auto* field=static_cast<editFieldDef_t*>(UI_Alloc(sizeof(editFieldDef_t)));
        if (field) {
            *field={};
            field->maxChars=54; field->maxPaintChars=34;
            search->type=ITEM_TYPE_EDITFIELD; search->cvar="ui_jar_mapSearch"; search->typeData=field;
        }
    }
    NativePanel(menu,"headerLine",32,62,576,1,true);
    for (int slot=0;slot<mapCards;++slot) {
        const float x=32+146.0f*(slot%4), y=75+155.0f*(slot/4);
        char name[16];
        Com_sprintf(name,sizeof(name),"card%d",slot);
        if (auto* card=AddItem(menu,name,pickerNames[slot],x,y,138,
                               String_Alloc(va("uiScript jarMapPicker %d ;",slot)))) {
            NativeButton(menu,name,x,y,138,140);
            card->textalignx=6; card->textaligny=102; card->textscale=.64f;
        }
        if (auto* shot=AddItem(menu,va("shot%d",slot),nullptr,x+5,y+5,128,nullptr)) {
            shot->window.rectClient.h=86;
            shot->window.style=WINDOW_STYLE_SHADER;
        }
        NativeLabel(menu,va("id%d",slot),pickerIds[slot],x+6,y+120,126,.53f);
    }
    NativeLabel(menu,"details",pickerDetails,32,386,576,.68f);
    NativePanel(menu,"footerLine",32,420,576,1,true);
    AddItem(menu,"back","Back",32,434,84,"uiScript jarMapPicker close ;");
    AddItem(menu,"previous","Previous",128,434,90,"uiScript jarMapPicker previous ;");
    AddItem(menu,"next","Next",226,434,76,"uiScript jarMapPicker next ;");
    NativeLabel(menu,"page",pickerPageText,318,443,140,.62f);
    AddItem(menu,"launch","Start map",478,434,130,"uiScript jarMapPicker launch ;");
    NativeButton(menu,"back",32,434,84);
    NativeButton(menu,"previous",128,434,90);
    NativeButton(menu,"next",226,434,76);
    NativeButton(menu,"launch",478,434,130);
    Menu_PostParse(menu);
}
}

bool UI_JAR_MapPickerActive() {
    const auto* menu=Menus_FindByName(mapPickerMenu);
    return menu && Menu_GetFocused()==menu;
}

static void UpdateMapPicker() {
    if (!UI_JAR_MapPickerActive()) return;
    RefreshPickerFilter();
    AdvancePickerSearch();
    if (pickerSearchCursor<pickerMaps.size()) {
        const int index=int(pickerSearchCursor++);
        auto& map=pickerMaps[index];
        PickerBsp bsp(("maps/"+map.name+".bsp").c_str());
        InspectPickerInfo(map,bsp);
        if (PickerMatchesQuery(map)) {
            const auto existing=std::lower_bound(pickerMatches.begin(),pickerMatches.end(),index);
            if (existing==pickerMatches.end() || *existing!=index) {
                pickerMatches.insert(existing,index);
                if (pickerSelected>=0)
                    pickerPage=int((std::lower_bound(pickerMatches.begin(),pickerMatches.end(),pickerSelected)-pickerMatches.begin())/mapCards);
            }
        }
        AdvancePickerSearch();
        UpdatePickerPage();
        return;
    }
    for (int slot=0;slot<mapCards;++slot) {
        const int index=PickerCardIndex(slot);
        if (index<0 || pickerMaps[index].previewRead) continue;
        InspectPickerMap(pickerMaps[index]);
        UpdatePickerPage();
        break;
    }
}

static void MovePickerPage(int delta) {
    const int page=std::max(0,std::min(pickerPage+delta,PickerLastPage()));
    if (page==pickerPage) return;
    pickerPage=page;
    pickerSelected=-1;
    UpdatePickerPage();
}

bool UI_JAR_MapPickerKey(int key,qboolean down) {
    if (!down || !UI_JAR_MapPickerActive() || UI_JAR_TextEntryActive()) return false;
    if (key!=A_MWHEELUP && key!=A_MWHEELDOWN) return false;
    MovePickerPage(key==A_MWHEELUP ? -1 : 1);
    return true;
}

static void MapPickerAction(const char* action) {
    if (!std::strcmp(action,"open")) {
        UI_JAR_OverlayClose();
        BuildPickerMenu();
        if (!Menus_FindByName(mapPickerMenu)) return;
        const auto* menu=Menu_GetFocused();
        pickerParent=menu && menu->window.name && !std::strcmp(menu->window.name,"ingameMainMenu")
            ? "ingameMainMenu" : "mainMenu";
        Cvar_Set("ui_jar_mapSearch","");
        pickerSkill=std::atoi(Cvar_VariableString("g_spskill"));
        pickerBuildScript=std::atoi(Cvar_VariableString("com_buildScript"))!=0;
        DiscoverPickerMaps();
        FilterPickerMaps();
        Menus_CloseAll();
        Menus_ActivateByName(mapPickerMenu);
        return;
    }
    if (!UI_JAR_MapPickerActive()) return;
    if (!std::strcmp(action,"close")) {
        UI_JAR_EndTextEntry();
        Menus_CloseAll();
        Menus_ActivateByName(pickerParent.c_str());
    } else if (!std::strcmp(action,"previous") || !std::strcmp(action,"next")) {
        MovePickerPage(!std::strcmp(action,"previous") ? -1 : 1);
    } else if (!std::strcmp(action,"launch")) {
        if (RefreshPickerFilter()) return;
        if (pickerSelected<0 || !pickerMaps[pickerSelected].info.playable) return;
        auto& map=pickerMaps[pickerSelected];
        const auto& name=map.name;
        if (!jar::mapPicker::SafeName(name)) return;
        {
            PickerBsp bsp(("maps/"+name+".bsp").c_str());
            InspectPickerInfo(map,bsp);
        }
        if (!map.info.playable) { UpdatePickerPage(); return; }
        UI_JAR_EndTextEntry();
        Menus_CloseAll();
        ui.Key_SetCatcher(ui.Key_GetCatcher()&~KEYCATCH_UI);
        Cvar_Set("cl_paused","0");
        ui.Cmd_ExecuteText(EXEC_APPEND,va("map \"%s\"\n",name.c_str()));
    } else if (action[0]>='0' && action[0]<'0'+mapCards && !action[1]) {
        if (RefreshPickerFilter()) return;
        pickerSelected=PickerCardIndex(action[0]-'0');
        if (pickerSelected>=0 && !pickerMaps[pickerSelected].previewRead) InspectPickerMap(pickerMaps[pickerSelected]);
        UpdatePickerPage();
    }
}
