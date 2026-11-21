#include "qcommon/jar_music.h"
#include "qcommon/jar_scriptrunner.h"
#include "qcommon/jar_workshop_state.h"
// Exercise the real menu controller/builder using headless engine/UI services.
#include "ui/ui_local.h"
#include "ui/ui_jar.h"
#include "ui/jar_map_thumbnail.h"
#include "menudef.h"
#include "qcommon/jar_graphics.h"
#include "qcommon/jar_ui_navigation.h"
#include "qcommon/jar_overlay_input.h"
#include "qcommon/jar_ui_theme.h"
#include "qcommon/jar_live_randomizer.h"
#include "game/weapons.h"
#include "qcommon/qcommon.h"
#include "test_support.h"
#include <map>
#include <memory>
#include <vector>

uiimport_t ui = {};
uiInfo_t uiInfo = {};
struct Variable { cvar_t cvar = {}; std::string value, pending; };
static std::map<std::string, Variable> vars;
static std::map<std::string, menuDef_t> menus;
static std::vector<std::unique_ptr<char[]>> memory;
static menuDef_t *focused;
static int catcher;
static std::string commands;
static bool packsAvailable=true;
static const char *catalogFiles[]={"good.jap","bad.jap","duplicate.jap","longlabel.jap"};
static const char *catalogData[]={"font\r\nlocalfont\r\nLocal Test Font\r\n","font\n../unsafe\nBad pack","font\nsans\nDuplicate","font\nlonglabel\nThis label is deliberately too long to fit in a menu safely"};

void *UI_Alloc(int size) { memory.emplace_back(new char[size]()); return memory.back().get(); }
const char *String_Alloc(const char *str) {
    auto *copy = static_cast<char *>(UI_Alloc(int(std::strlen(str)+1)));
    std::strcpy(copy, str); return copy;
}
static bool editingSearch=false;
bool UI_JAR_TextEntryActive(){return editingSearch;}
void UI_JAR_EndTextEntry(){editingSearch=false;}
void Item_Init(itemDef_t *item) { std::memset(item, 0, sizeof(*item)); }
menuDef_t *Menus_FindByName(const char *name) {
    auto it = menus.find(name); return it == menus.end() ? nullptr : &it->second;
}
menuDef_t *Menu_CreateJAR(const char *name) {
    auto& menu = menus[name];
    menu.window.name = const_cast<char *>(String_Alloc(name));
    return &menu;
}
void Menu_PostParse(menuDef_t *) {}
itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *name) {
    for (int i = 0; i < menu->itemCount; ++i)
        if (!std::strcmp(menu->items[i]->window.name, name)) return menu->items[i];
    return nullptr;
}
menuDef_t *Menu_GetFocused() { return focused; }
void Menus_CloseAll() { focused = nullptr; }
menuDef_t *Menus_ActivateByName(const char *name) { return focused = Menus_FindByName(name); }
int UI_RegisterFont(const char *) { return 4; }
cvar_t *Cvar_FindVar(const char *name) {
    auto it = vars.find(name); return it == vars.end() ? nullptr : &it->second.cvar;
}
cvar_t *Cvar_Get(const char *name, const char *value, int flags) {
    auto it = vars.emplace(name, Variable{});
    auto& var = it.first->second;
    if (it.second) var.value = value;
    var.cvar.string = const_cast<char *>(var.value.c_str());
    var.cvar.flags |= flags;
    return &var.cvar;
}
cvar_t *Cvar_Set2(const char *name, const char *value, qboolean force) {
    auto *cvar = Cvar_Get(name, value, 0);
    auto& var = vars[name];
    if (!force && (cvar->flags & CVAR_LATCH)) {
        var.pending = value;
        cvar->latchedString = const_cast<char *>(var.pending.c_str());
    } else {
        var.pending.clear(); cvar->latchedString=nullptr;
        var.value = value;
        cvar->string = const_cast<char *>(var.value.c_str());
    }
    return cvar;
}
void Cvar_Set(const char *name, const char *value) { Cvar_Set2(name, value, qtrue); }
char *Cvar_VariableString(const char *name) {
    auto *cvar = Cvar_FindVar(name); return cvar ? cvar->string : const_cast<char *>("");
}
qboolean String_Parse(const char **args, const char **out) { *out = *args; return **args ? qtrue : qfalse; }

static void ConsumeVideoLatches() {
    for (const char *name : {"r_mode","r_customwidth","r_customheight","r_fullscreen","r_noborder"}) {
        auto& var=vars[name];
        if (var.cvar.latchedString) { var.value=var.pending; var.cvar.string=const_cast<char *>(var.value.c_str()); var.pending.clear(); var.cvar.latchedString=nullptr; }
    }
}
void Cmd_AddCommand(const char *,xcommand_t) {}
static size_t pickerFilePosition;
int FS_Seek(fileHandle_t file,long offset,int origin) {
    CHECK(file==201 && offset>=0);
    pickerFilePosition=(origin==FS_SEEK_CUR ? pickerFilePosition : 0)+offset;
    return 0;
}

static void CheckMapPicker() {
    using namespace jar::mapPicker;
    CHECK(SafeName("gm_construct") && SafeName("mp/duel1"));
    for (const char* invalid:{"../base","maps//map","bad;quit","/abs","test\\map","map\nquit"})
        CHECK(!SafeName(invalid));
    static const char start[]="{ \"classname\" \"worldspawn\" \"message\" \"Courtyard\" }\n"
                              "{ \"classname\" \"info_player_start\" \"origin\" \"0 0 8\" }\n";
    static const char named[]="{ \"classname\" \"worldspawn\" }\n"
                              "{ \"classname\" \"info_player_start\" \"targetname\" \"other_map\" }\n";
    static const char difficulty[]="{ classname worldspawn message \"Temple Rear Approach\" }\n"
        "{ classname info_player_start notsingle 1 origin \"1 2 3\" }\n"
        "{ classname info_player_deathmatch spawnflags 512 origin \"10 20 30\" }\n";
    CHECK(ReadEntities(start).playable && ReadEntities(start).start[2]==8);
    CHECK(!ReadEntities(named).playable && !ReadEntities("{\"classname\" \"missing\"}").playable);
    const auto easy=ReadEntities(difficulty,StartMode::SinglePlayer,0);
    CHECK(easy.playable && easy.start[2]==30 && easy.title=="Temple Rear Approach");
    CHECK(!ReadEntities(difficulty,StartMode::SinglePlayer,1).playable);
    CHECK(ReadEntities(difficulty,StartMode::SinglePlayer,1,true).start[2]==30);
    CHECK(ReadEntities(difficulty,StartMode::MultiplayerFFA).start[2]==3);
    CHECK(!ReadEntities(std::string(start)+" { classname \"dangling").playable);
    CHECK(!ReadEntities("{ classname worldspawn } { classname info_player_start targetname \"\" }").playable);
    static std::vector<char> bsp[3];
    const char* entities[]={start,named,difficulty};
    for (int i=0;i<3;++i) {
        dheader_t header{};
        header.ident=BSP_IDENT;
        header.version=BSP_VERSION;
        bsp[i].resize(sizeof(header));
        const auto append=[&](int id,const void* data,size_t bytes) {
            header.lumps[id]={int(bsp[i].size()),int(bytes)};
            const auto* first=static_cast<const char*>(data);
            bsp[i].insert(bsp[i].end(),first,first+bytes);
        };
        dshader_t shader{"textures/courtyard/floor",0,0};
        dmodel_t world{{-128,-128,0},{128,128,80},0,1,0,0};
        drawVert_t verts[3]{};
        verts[0].xyz[0]=-128;
        verts[1].xyz[0]=128;
        verts[2].xyz[1]=128;
        const int indices[3]={0,1,2};
        dsurface_t surface{};
        surface.surfaceType=MST_PLANAR;
        surface.numVerts=surface.numIndexes=3;
        append(LUMP_ENTITIES,entities[i],std::strlen(entities[i])+1);
        append(LUMP_SHADERS,&shader,sizeof(shader));
        append(LUMP_MODELS,&world,sizeof(world));
        append(LUMP_DRAWVERTS,verts,sizeof(verts));
        append(LUMP_DRAWINDEXES,indices,sizeof(indices));
        append(LUMP_SURFACES,&surface,sizeof(surface));
        std::memcpy(bsp[i].data(),&header,sizeof(header));
    }
    static int fileMap,writes,authoredShots,geometryReads;
    static bool remounted=false;
    static std::string cachedImage;
    const auto oldUi=ui;
    ui.FS_GetFileList=[](const char* path,const char* extension,char* buffer,int size) {
        CHECK(!std::strcmp(path,"maps") && !std::strcmp(extension,".bsp"));
        const char files[]="workshop/custom.bsp\0retail.bsp\0named.bsp\0../escape.bsp\0"
                           "a0.bsp\0a1.bsp\0a2.bsp\0a3.bsp\0a4.bsp\0a5.bsp\0a6.bsp\0";
        CHECK(size>int(sizeof(files)));
        std::memcpy(buffer,files,sizeof(files));
        return 11;
    };
    ui.FS_FOpenFile=[](const char* path,fileHandle_t* file,fsMode_t mode) {
        *file=0;
        if (!std::strncmp(path,"maps/",5)) {
            fileMap=std::strcmp(path,"maps/named.bsp")==0 ? 1 :
                std::strcmp(path,"maps/a0.bsp")==0 ? (remounted?1:2) : 0;
            *file=201;
            pickerFilePosition=0;
            return int(bsp[fileMap].size());
        }
        if (!std::strcmp(path,"levelshots/retail.jpg")) { *file=202; return 15; }
        if (!std::strncmp(path,"jar/mapshots/",13)) {
            if (mode==FS_WRITE) { cachedImage=path; *file=203; return 0; }
            if (path==cachedImage) { *file=203; return int(Image{}.size()); }
        }
        return -1;
    };
    ui.FS_Read=[](void* data,int count,fileHandle_t file) {
        CHECK(file==201 && pickerFilePosition+count<=bsp[fileMap].size());
        if (pickerFilePosition>sizeof(dheader_t)) ++geometryReads;
        std::memcpy(data,bsp[fileMap].data()+pickerFilePosition,count);
        pickerFilePosition+=count;
        return count;
    };
    ui.FS_Write=[](const void* data,int size,fileHandle_t file) {
        CHECK(file==203 && size==int(Image{}.size()));
        const auto* pixels=static_cast<const uint8_t*>(data);
        CHECK(pixels[2]==2 && pixels[12]==thumbWidth && pixels[14]==thumbHeight);
        CHECK(*std::max_element(pixels+18,pixels+size)>90);
        ++writes;
        return size;
    };
    ui.R_RegisterShaderNoMip=[](const char* path) {
        if (!std::strcmp(path,"levelshots/retail")) { ++authoredShots; return 7; }
        CHECK(path==cachedImage);
        return 8;
    };
    auto visible=[](menuDef_t* menu,const char* name) {
        const auto* item=Menu_FindItemByName(menu,name);
        return item && (item->window.flags&WINDOW_VISIBLE);
    };
    auto action=[](const char* value) { CHECK(UI_JAR_Script("jarMapPicker",&value)); };
    Menus_ActivateByName("ingameMainMenu");
    action("open");
    auto* menu=Menu_GetFocused();
    CHECK(menu && UI_JAR_MapPickerActive() && menu->fullScreen);
    CHECK(visible(menu,"next") && !visible(menu,"launch"));
    CHECK(UI_JAR_MapPickerKey(A_MWHEELDOWN,qtrue));
    CHECK(!visible(menu,"next") && visible(menu,"previous"));
    UI_JAR_Update(11);
    CHECK(authoredShots==1 && Menu_FindItemByName(menu,"shot0")->window.background==7);
    UI_JAR_Update(12);
    CHECK(writes==1 && Menu_FindItemByName(menu,"shot1")->window.background==8);
    Cvar_Set("ui_jar_mapSearch","named");
    UI_JAR_Update(13);
    action("0");
    action("launch");
    CHECK(UI_JAR_MapPickerActive() && !visible(menu,"launch"));
    action("close");
    CHECK(Menu_GetFocused()==Menus_FindByName("ingameMainMenu"));
    action("open");
    Cvar_Set("g_spskill","1");
    Cvar_Set("ui_jar_mapSearch","TEMPLE REAR");
    const int previousGeometryReads=geometryReads;
    UI_JAR_Update(14);
    CHECK(visible(menu,"card0") && !visible(menu,"card1") && !visible(menu,"launch"));
    CHECK(std::strcmp(Menu_FindItemByName(menu,"card0")->text,"Temple Rear Approach")==0 && geometryReads==previousGeometryReads);
    action("0");
    commands.clear();action("launch");
    CHECK(commands.empty() && !visible(menu,"launch"));
    Cvar_Set("g_spskill","0");
    UI_JAR_Update(15);
    action("0");
    CHECK(visible(menu,"launch"));
    remounted=true;action("launch");remounted=false;
    CHECK(commands.empty() && !visible(menu,"launch"));
    action("close");action("open");
    Cvar_Set("ui_jar_mapSearch","CUSTOM");
    UI_JAR_Update(16);
    CHECK(!visible(menu,"card1"));
    action("0");
    CHECK(visible(menu,"launch") && strstr(Menu_FindItemByName(menu,"details")->text,"autosave"));
    commands.clear();
    action("launch");
    CHECK(commands=="map \"workshop/custom\"\n" && !UI_JAR_MapPickerActive());
    ui=oldUi;
}

static void Script(const char *name, const char *arg = "") { CHECK(UI_JAR_Script(name, &arg)); }
static void CheckOriginal() {
    CHECK(!std::strcmp(Cvar_VariableString("cl_renderer"), "rdjar-vanilla"));
    CHECK(!std::strcmp(Cvar_VariableString("jar_npcThinkLOD"), "0"));
    CHECK(!std::strcmp(Cvar_VariableString("com_maxfps"), "117"));
    CHECK(commands.empty());
}

int main() {
    CHECK(jar::MenuNavigationKey(A_JOY1,false)==A_ENTER);
    CHECK(jar::MenuNavigationKey(A_JOY2,false)==A_ESCAPE);
    CHECK(jar::MenuNavigationKey(A_JOY28,false)==A_CURSOR_UP);
    CHECK(jar::MenuNavigationKey(A_JOY29,false)==A_CURSOR_RIGHT);
    CHECK(jar::MenuNavigationKey(A_JOY30,false)==A_CURSOR_DOWN);
    CHECK(jar::MenuNavigationKey(A_JOY31,false)==A_CURSOR_LEFT);
    for(int key=A_JOY0;key<=A_JOY31;++key)CHECK(jar::MenuNavigationKey(key,true)==key);
    CHECK(jar::MenuNavigationKey(A_MOUSE1,false)==A_MOUSE1);
    CHECK(jar::MenuNavigationKey(A_JOY3,false)==A_JOY3);

    return RunTests("graphics menu: entry points, cancel/revert, custom values and one Apply restart", [] {
        ui.Key_SetCatcher = [](int value) {catcher=value;};
        ui.Key_GetCatcher = [](){return catcher;};
        ui.R_RegisterShaderNoMip=[](const char* path){CHECK(path && *path);return 1;};
        ui.FS_FOpenFile = [](const char *name,fileHandle_t *file,fsMode_t) {
            if (jar::Prefix(name,"jar/catalog/")) {
                for (int i=0; i<4; ++i) if (!std::strcmp(name+12,catalogFiles[i])) { *file=101+i; return int(std::strlen(catalogData[i])); }
                *file=0; return -1;
            }
            *file=packsAvailable ? 1 : 0; return packsAvailable ? 8 : -1;
        };
        ui.FS_GetFileList = [](const char *,const char *,char *buffer,int size) {
            int used=0; for (const char *name:catalogFiles) { int n=int(std::strlen(name))+1; CHECK(used+n<size); std::memcpy(buffer+used,name,n); used+=n; } return 4;
        };
        ui.FS_Read = [](void *buffer,int size,fileHandle_t handle) { CHECK(handle>=101 && handle<105); std::memcpy(buffer,catalogData[handle-101],size); return size; };
        ui.FS_FCloseFile = [](fileHandle_t) {};
        ui.Cmd_ExecuteText = [](int, const char *text) { commands += text; };
        Menu_CreateJAR("mainMenu"); Menu_CreateJAR("ingameMainMenu");
        Cvar_Get("cl_renderer", "rdjar-vanilla", CVAR_ARCHIVE | CVAR_LATCH);
        Cvar_Get("jar_npcThinkLOD", "0", CVAR_ARCHIVE);
        Cvar_Get("com_maxfps", "117", CVAR_ARCHIVE); // custom value outside the menu's choices
        UI_JAR_LoadMenus();
        CHECK(menus.size() == 15);
        auto main = Menus_FindByName("mainMenu"), pause = Menus_FindByName("ingameMainMenu");
        CHECK(Menu_FindItemByName(main, "jarGraphics") && Menu_FindItemByName(pause, "jarGraphics"));
        const int count = main->itemCount;
        UI_JAR_LoadMenus();
        CHECK(menus.size() == 15 && main->itemCount == count);
        int controls = 0;
        for (const auto& pair : menus) for (int i = 0; i < pair.second.itemCount; ++i) {
            const auto *item = pair.second.items[i];
            CHECK(item->window.rectClient.y + item->window.rectClient.h <= 480);
            CHECK(item->window.rectClient.x + item->window.rectClient.w <= 640);
            if (item->cvar) {
                ++controls;
                CHECK(item->typeData);
                if (item->type == ITEM_TYPE_NUMERICFIELD) { CHECK(static_cast<editFieldDef_t *>(item->typeData)->maxChars == (!std::strcmp(item->window.name, jar::pickupCvars[0]) ? 10 : 5)); continue; }
                CHECK(item->type == ITEM_TYPE_MULTI);
                const auto *multi = static_cast<multiDef_t *>(item->typeData);
                CHECK(multi->strDef && multi->count > 1 && multi->count <= MAX_MULTI_CVARS);
            }
        }
        CHECK(controls == jar::graphicsSettingCount+4);
        for(const char *page:{"jarRandomizer","jarSoundRandomizer","jarCvarRandomizer"}) {
            const auto menu=Menus_FindByName(page);const auto item=Menu_FindItemByName(menu,"jar_randomMode");CHECK(item);
            const auto choices=static_cast<multiDef_t *>(item->typeData);
            CHECK(!strcmp(choices->cvarList[0],"Seed-fixed mapping")&&!strcmp(choices->cvarList[1],"Per-event reroll"));
            for(int i=0;i<menu->itemCount;++i)if(menu->items[i]->cvar)CHECK(menu->items[i]->window.rectClient.y+menu->items[i]->window.rectClient.h<=337);
        }
        const auto *hud=Menu_FindItemByName(Menus_FindByName("jarInterface"),"cg_hudFiles");
        CHECK(hud && hud->type==ITEM_TYPE_MULTI);
        UI_JAR_Open("interface"); CHECK(focused==Menus_FindByName("jarInterface"));
        UI_JAR_Open("Styles"); CHECK(focused==Menus_FindByName("jarStyles"));
        const auto *face=Menu_FindItemByName(focused,"jar_facePreset");
        CHECK(face && static_cast<multiDef_t *>(face->typeData)->count==4);
        CHECK(Menu_FindItemByName(focused,"jar_faceSensitivity"));
        Cvar_Set("ui_jar_jar_facePreset","3"); Cvar_Set("ui_jar_jar_faceSensitivity","1.5"); Script("jarApply");
        CHECK(!std::strcmp(Cvar_VariableString("jar_facePreset"),"3")); CHECK(commands.empty());
        CHECK(Cvar_FindVar("jar_facePreset")->flags&CVAR_ARCHIVE);
        UI_JAR_Open("Styles"); Script("jarPreset","2");
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_jar_facePreset"),"3")); Script("jarCancel");
        UI_JAR_Open("Styles"); Cvar_Set("ui_jar_jar_facePreset","0"); Script("jarRevert");
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_jar_facePreset"),"3"));
        Cvar_Set("ui_jar_jar_facePreset","0");Cvar_Set("ui_jar_jar_faceSensitivity","1");Script("jarApply");
        CHECK(commands.empty());

        UI_JAR_Open("unknown"); CHECK(focused==Menus_FindByName("jarDisplay"));
        const auto *hudChoices=static_cast<multiDef_t *>(hud->typeData);
        CHECK(hudChoices->count==2);
        CHECK(!std::strcmp(hudChoices->cvarStr[0],"ui/jahud.txt"));
        CHECK(!std::strcmp(hudChoices->cvarStr[1],"1"));
        focused=main; Cvar_Set("cg_hudFiles","ui/custom_hud.txt"); UI_JAR_Open();
        Script("jarApply"); CHECK(commands.empty());
        Cvar_Set("ui_jar_cg_hudFiles","1"); Script("jarCancel");
        CHECK(!std::strcmp(Cvar_VariableString("cg_hudFiles"),"ui/custom_hud.txt"));
        UI_JAR_Open(); Cvar_Set("ui_jar_cg_hudFiles","1"); Script("jarApply");
        CHECK(commands=="vid_restart\n"); commands.clear();
        CHECK(!std::strcmp(Cvar_VariableString("cg_hudFiles"),"1"));
        CHECK(Cvar_FindVar("cg_hudFiles")->flags & CVAR_ARCHIVE);
        UI_JAR_Open(); Cvar_Set("ui_jar_cg_hudFiles","ui/jahud.txt"); Script("jarApply");
        CHECK(commands=="vid_restart\n"); commands.clear();
        const auto *fontChoices=static_cast<multiDef_t *>(Menu_FindItemByName(Menus_FindByName("jarInterface"),"jar_fontPack")->typeData);
        CHECK(fontChoices->count==4);
        CHECK(!std::strcmp(fontChoices->cvarStr[3],"localfont"));
        focused = main;
        UI_JAR_Open();
        CHECK(Menu_FindItemByName(Menus_FindByName("jarRandomizer"), jar::pickupCvars[0]));
        for (unsigned i=0; i<jar::randomizerCvarCount; ++i) {
            CHECK(Cvar_FindVar(jar::pickupCvars[i])->flags & CVAR_SAVEGAME);
            CHECK(!std::strcmp(Cvar_VariableString(jar::pickupCvars[i]), i==0 ? "1" : "0"));
        }
        Cvar_Set("ui_jar_jar_randomSeed", "2147483648");
        Cvar_Set("ui_jar_jar_randomWeapons", "1"); Script("jarApply");
        CHECK(!std::strcmp(Cvar_VariableString("jar_randomWeapons"), "0"));
        CHECK(commands.empty());
        Script("jarRevert"); Cvar_Set("ui_jar_jar_randomWeapons", "1"); Script("jarCancel");
        CHECK(!std::strcmp(Cvar_VariableString("jar_randomWeapons"), "0"));
        UI_JAR_Open(); Cvar_Set("ui_jar_jar_randomSeed", "2147483647");
        Cvar_Set("ui_jar_jar_randomWeapons", "1"); Script("jarApply");
        CHECK(!std::strcmp(Cvar_VariableString("jar_randomSeed"), "2147483647"));
        CHECK(!std::strcmp(Cvar_VariableString("jar_randomWeapons"), "1"));
        CHECK(commands.empty() && !UI_JAR_DisplayTrialActive());
        Script("jarPreset", "0");
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_jar_randomWeapons"), "1"));
        Script("jarRevert");
        Cvar_Set("ui_jar_jar_randomWeapons", "0"); Script("jarApply");
        UI_JAR_Open(); CheckOriginal();
        Script("jarApply"); CheckOriginal(); // no incidental normalization of console values
        Script("jarPreset", "2"); CheckOriginal();
        Script("jarCancel"); CheckOriginal(); CHECK(focused == main);
        focused = pause; uiInfo.inGameLoad = qtrue;
        UI_JAR_Open(); Script("jarPreset", "1"); Script("jarRevert"); Script("jarApply"); CheckOriginal();
        Script("jarCancel"); CHECK(focused == pause);
        UI_JAR_Open(); Script("jarPreset", "2"); Script("jarApply");
        CHECK(commands == "vid_restart\n");
        CHECK(!std::strcmp(Cvar_VariableString("jar_npcThinkLOD"), "1"));
        CHECK(!std::strcmp(Cvar_VariableString("cl_renderer"), "rdjar-vanilla"));
        CHECK(!std::strcmp(Cvar_FindVar("cl_renderer")->latchedString, "rdjar-rend2"));
        CHECK(Cvar_FindVar("r_hdr")->flags & CVAR_ARCHIVE);
        commands.clear();
        // Reading the pending renderer does not consume its latch.
        UI_JAR_Open();
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_cl_renderer"), "rdjar-rend2"));
        CHECK(!std::strcmp(Cvar_VariableString("cl_renderer"), "rdjar-vanilla"));
        Script("jarApply"); CHECK(commands.empty());
        packsAvailable=false;
        Cvar_Set("ui_jar_jar_soundPack","mbii"); Script("jarApply"); CHECK(commands.empty());
        CHECK(std::strcmp(Cvar_VariableString("jar_soundPack"),"mbii"));
        packsAvailable=true; Script("jarRevert");
        // Invalid input is rejected atomically, including other pending changes.
        Cvar_Set("ui_jar_r_customwidth","999999999999"); Cvar_Set("ui_jar_jar_soundPack","mbii");
        Script("jarApply"); CHECK(commands.empty());
        CHECK(std::strcmp(Cvar_VariableString("jar_soundPack"),"mbii"));
        Script("jarRevert");
        Cvar_Set("r_mode","4"); Cvar_Set("r_customwidth","1600"); Cvar_Set("r_customheight","1024"); Cvar_Set("r_fullscreen","0"); Cvar_Set("r_noborder","0");
        for (const char *name : {"r_mode","r_customwidth","r_customheight","r_fullscreen","r_noborder"}) Cvar_FindVar(name)->flags |= CVAR_LATCH;
        UI_JAR_Open();
        Cvar_Set("ui_jar_r_customwidth","2560"); Cvar_Set("ui_jar_r_customheight","1440");
        Script("jarApply"); CHECK(commands=="vid_restart\n"); CHECK(UI_JAR_DisplayTrialActive());
        CHECK(!std::strcmp(Cvar_VariableString("r_mode"),"4"));
        CHECK(!std::strcmp(Cvar_FindVar("r_mode")->latchedString,"-1"));
        ConsumeVideoLatches();
        CHECK(!std::strcmp(Cvar_VariableString("r_mode"),"-1"));
        commands.clear(); UI_JAR_Update(1000); CHECK(focused==Menus_FindByName("jarConfirmDisplay"));
        UI_JAR_Update(20999); CHECK(commands.empty());
        UI_JAR_Update(21000); CHECK(commands=="vid_restart\n"); CHECK(!UI_JAR_DisplayTrialActive());
        CHECK(!std::strcmp(Cvar_FindVar("r_mode")->latchedString,"4"));
        ConsumeVideoLatches();
        CHECK(!std::strcmp(Cvar_VariableString("r_mode"),"4"));
        CHECK(!std::strcmp(Cvar_VariableString("r_customwidth"),"1600"));
        commands.clear(); UI_JAR_Open(); Script("jarDesktop"); Script("jarApply");
        ConsumeVideoLatches();
        CHECK(!std::strcmp(Cvar_VariableString("r_mode"),"-2"));
        CHECK(!std::strcmp(Cvar_VariableString("r_fullscreen"),"2"));
        UI_JAR_Update(22000); Script("jarKeepVideo"); CHECK(!UI_JAR_DisplayTrialActive());
        commands.clear(); UI_JAR_Update(999999); CHECK(commands.empty());
        UI_JAR_Open(); Cvar_Set("ui_jar_jar_soundPack","mbii"); Script("jarApply");
        CHECK(commands=="snd_restart\nvid_restart\n");
        commands.clear(); UI_JAR_Open(); Cvar_Set("ui_jar_jar_fontPack","localfont"); Script("jarApply");
        CHECK(commands=="vid_restart\n"); CHECK(!std::strcmp(Cvar_VariableString("jar_fontPack"),"localfont"));

        commands.clear(); UI_JAR_Open("randomizer");
        CHECK(focused==Menus_FindByName("jarRandomizer"));
        Cvar_Set("ui_jar_jar_npcSafety","0");Cvar_Set("ui_jar_jar_actorContinuity","1");
        Script("jarRandomPreset","2");
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_jar_npcSafety"),"1")&&!std::strcmp(Cvar_VariableString("ui_jar_jar_actorContinuity"),"0"));
        CHECK(!std::strcmp(Cvar_VariableString("ui_jar_jar_randomEnemies"),"1"));
        CHECK(!std::strcmp(Cvar_VariableString("jar_randomEnemies"),"0"));
        Script("jarCancel"); CHECK(!std::strcmp(Cvar_VariableString("jar_randomEnemies"),"0"));
        UI_JAR_Open("randomizer"); Script("jarRandomPreset","2"); Script("jarApply");
        CHECK(commands.empty());
        for (unsigned i=1;i<jar::randomizerCvarCount;++i) CHECK(!std::strcmp(Cvar_VariableString(jar::pickupCvars[i]),"1"));
        Cvar_Set("sv_running","1");Cvar_Set("mapname","t1_surprise");
        Cvar_Set("jar_randomStatus","Current map: saved world"); UI_JAR_Update(0);
        CHECK(!std::strcmp(Menu_FindItemByName(focused,"activeState")->text,"Current map: saved world"));
        Script("jarRandomRestartAsk");CHECK(focused==Menus_FindByName("jarConfirmRandomizer") && commands.empty());
        Script("jarRandomRestartCancel");CHECK(focused==Menus_FindByName("jarRandomizer") && commands.empty());
        Script("jarRandomRestartConfirm");CHECK(commands.empty());
        Script("jarRandomRestartAsk");Script("jarRandomRestartConfirm");CHECK(commands=="maptransition t1_surprise\n");commands.clear();
        UI_JAR_Open("randomizer");Cvar_Set("mapname","bad;quit");Script("jarRandomRestartAsk");CHECK(focused==Menus_FindByName("jarRandomizer"));
        Script("jarRandomRestartConfirm");CHECK(commands.empty());Cvar_Set("mapname","t1_surprise");
        Script("jarRandomPreset","0");Script("jarRandomRestartAsk");CHECK(focused==Menus_FindByName("jarRandomizer"));
        Script("jarApply");for (unsigned i=1;i<jar::randomizerCvarCount;++i) CHECK(!std::strcmp(Cvar_VariableString(jar::pickupCvars[i]),"0"));
        Script("jarRandomPreset","1");Script("jarApply");CHECK(!std::strcmp(Cvar_VariableString("jar_randomWeapons"),"1") && !std::strcmp(Cvar_VariableString("jar_randomEnemies"),"0"));

        CHECK(!std::strcmp(Cvar_VariableString("jar_npcSafety"),"1")&&!std::strcmp(Cvar_VariableString("jar_actorContinuity"),"0"));
        UI_JAR_Open("Sound");CHECK(focused==Menus_FindByName("jarSoundRandomizer"));
        Script("jarSectionPreset","1");Script("jarApply");CHECK(commands=="jar_audio_refresh\n");commands.clear();
        CHECK(!UI_JAR_DisplayTrialActive());
        for(const char *name:{"jar_randomEffects","jar_randomUI","jar_randomLoops","jar_randomMusic","jar_randomVoices"})CHECK(!std::strcmp(Cvar_VariableString(name),"1"));
        Cvar_Set("ui_jar_jar_randomMode","1");Script("jarApply");CHECK(commands=="jar_audio_refresh\n");commands.clear();
        CHECK(!strcmp(Cvar_VariableString("jar_randomMode"),"1"));
        UI_JAR_Open("Entity");Script("jarRandomPreset","2");CHECK(!strcmp(Cvar_VariableString("ui_jar_jar_randomMode"),"1"));Script("jarRevert");
        UI_JAR_Open("Sound");Cvar_Set("ui_jar_jar_randomMode","bad");Script("jarApply");CHECK(commands.empty()&&!strcmp(Cvar_VariableString("jar_randomMode"),"1"));Script("jarRevert");
        Script("jarAudioRestore");CHECK(commands=="jar_audio_restore\n");commands.clear();
        UI_JAR_Open("Cvars");CHECK(focused==Menus_FindByName("jarCvarRandomizer"));CHECK(commands=="jar_cvar_status\n");commands.clear();
        Script("jarSectionPreset","1");Script("jarApply");CHECK(commands=="jar_cvar_apply\n");commands.clear();
        CHECK(!UI_JAR_DisplayTrialActive());Script("jarCvarRestore");CHECK(commands=="jar_cvar_restore\n");
        // Advanced is the direct entry. Opening, cancelling and no-op Apply
        // preserve native/custom/latched values and randomizer ownership state.
        commands.clear(); focused=main;
        Cvar_Set("jar_randomSeed","73"); Cvar_Set("jar_randomMode","1");
        Cvar_Set("jar_randomProjectiles","1"); Cvar_Set("jar_actorRecovery","1");
        Cvar_Set("jar_sandbox","1"); Cvar_Set("r_ssao","custom");
        Cvar_Set("r_dynamiclight","1"); Cvar_Set("jar_live5_base","0"); Cvar_Set("jar_live5_last","1");
        UI_JAR_Open();
        CHECK(focused==Menus_FindByName("jarDisplay") && !Menus_FindByName("jarSimple"));
        CHECK(!Menu_FindItemByName(focused,"simple"));
        CHECK(!strcmp(focused->onESC,"uiScript jarCancel ;"));
        const char *unused="2";
        CHECK(!UI_JAR_Script("jarBuildPreset",&unused));
        CHECK(!UI_JAR_Script("jarSimple",&unused));
        Script("jarApply"); CHECK(commands.empty());
        Script("jarCancel"); CHECK(focused==main);
        CHECK(!strcmp(Cvar_VariableString("jar_randomSeed"),"73"));
        CHECK(!strcmp(Cvar_VariableString("jar_randomMode"),"1"));
        CHECK(!strcmp(Cvar_VariableString("jar_randomProjectiles"),"1"));
        CHECK(!strcmp(Cvar_VariableString("jar_actorRecovery"),"1"));
        CHECK(!strcmp(Cvar_VariableString("jar_sandbox"),"1"));
        CHECK(!strcmp(Cvar_VariableString("r_ssao"),"custom"));
        CHECK(!strcmp(Cvar_VariableString("r_dynamiclight"),"1"));
        CHECK(!strcmp(Cvar_VariableString("jar_live5_base"),"0"));
        CHECK(!strcmp(Cvar_VariableString("jar_live5_last"),"1"));
        // Explicit category changes still apply across graphics restarts.
        UI_JAR_Open("Cvars"); commands.clear(); Script("jarSectionPreset","1");
        Cvar_Set("ui_jar_r_ssao","0"); Script("jarApply");
        CHECK(commands=="vid_restart\njar_cvar_apply\n"); commands.clear();

        // Real workshop menus dispatch bounded game actions and resume gameplay.
        commands.clear(); Cvar_Set("sv_running","1"); Cvar_Set("jar_inventoryLastPage","1");
        Cvar_Set("jar_inventoryPageIndex","0"); Cvar_Set("jar_inventoryId0","42");
        Cvar_Set("jar_inventoryLabel0","Shortblade");
        Script("jarWorkshopOpen","inventory");
        CHECK(focused==Menus_FindByName("jarInventory"));
        CHECK(!strcmp(Menu_FindItemByName(focused,"choice0")->text,"Shortblade"));
        CHECK(Menu_FindItemByName(focused,"choice0")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"choice1")->window.flags&WINDOW_VISIBLE));
        Menu_FindItemByName(focused,"choice1")->window.flags|=WINDOW_HASFOCUS;
        Script("jarWorkshopOpen","inventory");
        CHECK(!(Menu_FindItemByName(focused,"choice1")->window.flags&(WINDOW_VISIBLE|WINDOW_HASFOCUS)));
        Cvar_Set("jar_inventoryId1","43");Cvar_Set("jar_inventoryLabel1","Energy pike");
        Script("jarWorkshopOpen","inventory");CHECK(Menu_FindItemByName(focused,"choice1")->window.flags&WINDOW_VISIBLE);
        CHECK(!strcmp(Cvar_VariableString("cl_paused"),"1"));
        Cvar_Set("jar_inventoryPageIndex","1");Script("jarWorkshopPage","1");
        commands.clear();Script("jarWorkshopPage","1");
        CHECK(commands.find("jar_inventory page 1\n")!=std::string::npos);
        Cvar_Set("jar_inventoryPageIndex","0");commands.clear();Script("jarWorkshopPage","-1");
        CHECK(commands.find("jar_inventory page 0\n")!=std::string::npos);
        commands.clear();Script("jarWorkshopEquip","0");
        CHECK(commands=="jar_inventory equip 42\n" && !focused);
        CHECK(!strcmp(Cvar_VariableString("cl_paused"),"0"));
        commands.clear();Script("jarWorkshopEquip","8");CHECK(commands.empty());
        // The replacement entry is an overlay: no pause, five tabs, icon per
        // catalog item, persistent clicks, close on release/off/lost focus.
        Cvar_Set("jar_sandbox","1"); catcher=0; commands.clear();
        UI_JAR_OverlayHeld(true); CHECK(UI_JAR_OverlayActive());
        CHECK(catcher==KEYCATCH_UI && !strcmp(Cvar_VariableString("cl_paused"),"0"));
        CHECK(focused==Menus_FindByName("jarSpawnOverlay0") && !focused->fullScreen);
        int icons=0,cards=0;
        for(int tab=0;tab<5;++tab){auto* menu=Menus_FindByName(va("jarSpawnOverlay%d",tab));CHECK(menu && !menu->fullScreen);
            for(int i=0;i<menu->itemCount;++i){auto* item=menu->items[i];const auto&r=item->window.rectClient;
                CHECK(r.x>=0 && r.y>=0 && r.x+r.w<=640 && r.y+r.h<=480);
                if(!strncmp(item->window.name,"icon",4)){++icons;if(item->window.flags&WINDOW_VISIBLE)CHECK(item->window.background);}
                if(!strncmp(item->window.name,"card",4))++cards;
            }
        }
        CHECK(cards==60 && icons==cards);
        Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 1001\n" && UI_JAR_OverlayActive());
        commands.clear();Script("jarOverlayPage","next");
        CHECK(!strcmp(Menu_FindItemByName(focused,"card0")->text,"Kejim crate"));
        Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 1013\n");
        commands.clear();Script("jarOverlaySpawn","bad");CHECK(commands.empty());
        CHECK(Menu_FindItemByName(focused,"card8")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"pageNext")->window.flags&WINDOW_VISIBLE);
        Script("jarOverlaySpawn","7");CHECK(commands=="jar_sb spawn 1020\n");
        commands.clear();Script("jarOverlayTab","1");Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 3004\n");
        commands.clear();Script("jarOverlayTab","0");Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 1013\n");
        commands.clear();Script("jarOverlayPage","previous");Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 1001\n");
        commands.clear();Cvar_Set("jar_box3d","0");Script("jarOverlayPhysics","toggle");CHECK(commands=="jar_sb physics on\n");
        commands.clear();Cvar_Set("jar_box3d","1");Script("jarOverlayPhysics","toggle");CHECK(commands=="jar_sb physics off\n");
        commands.clear();Script("jarOverlayTab","2");CHECK(focused==Menus_FindByName("jarSpawnOverlay2"));
        Script("jarOverlayPage","next");Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb weapon 2101\n");
        auto*weaponCard=Menu_FindItemByName(focused,"card0");weaponCard->window.rect=weaponCard->window.rectClient;
        uiInfo.uiDC.cursorx=weaponCard->window.rect.x+25;uiInfo.uiDC.cursory=weaponCard->window.rect.y+10;
        commands.clear();CHECK(UI_JAR_OverlayKey(A_MOUSE2,qtrue));CHECK(commands=="jar_sb spawn 2101\n");
        commands.clear();CHECK(UI_JAR_OverlayKey(A_MOUSE2,qfalse));CHECK(commands.empty());
        CHECK(UI_JAR_OverlayKey(A_MOUSE1,qtrue));CHECK(commands=="jar_sb weapon 2101\n");
        commands.clear();Script("jarOverlayPage","previous");Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb weapon 2002\n");
        commands.clear();Script("jarOverlayPage","next");
        commands.clear();Script("jarOverlayMode","rotate");CHECK(commands=="jar_sb give toolgun\njar_sb mode rotate\njar_sb equip toolgun\n");
        for(const char* mode:{"weld","hinge","unlink","duplicate","rope","thruster","activate","link"}){commands.clear();Script("jarOverlayMode",mode);CHECK(commands==std::string("jar_sb give toolgun\njar_sb mode ")+mode+"\njar_sb equip toolgun\n");}
        CHECK(!Menu_FindItemByName(focused,"limitMore")&&!Menu_FindItemByName(focused,"limitLess"));
        Cvar_Set("jar_toolMode","10");Cvar_Set("jar_thrusterForce","100000");Script("jarOverlayProperty","up");CHECK(atof(Cvar_VariableString("jar_thrusterForce"))==100000);
        Script("jarOverlayProperty","down");CHECK(atof(Cvar_VariableString("jar_thrusterForce"))==99000);
        Cvar_Set("jar_toolMode","9");Cvar_Set("jar_ropeSlack","1");Script("jarOverlayProperty","down");CHECK(atof(Cvar_VariableString("jar_ropeSlack"))==1);
        commands.clear();Script("jarOverlaySpawn","999");Script("jarOverlayMode","bad;quit");CHECK(commands.empty());
        // Search covers new entries and literal input never becomes a command.
        Script("jarOverlayTab","1");Cvar_Set("jar_spawnSearch","rancor");UI_JAR_Update(0);
        CHECK(strstr(Menu_FindItemByName(focused,"card0")->descText,"rancor"));
        commands.clear();Script("jarOverlaySpawn","0");CHECK(commands.find("jar_sb spawn ")==0);
        Script("jarOverlayTab","3");
        for(const char* term:{"Bobbing platform","func_bobbing"}){
            Cvar_Set("jar_spawnSearch",term);UI_JAR_Update(0);
            CHECK(!strcmp(Menu_FindItemByName(focused,"card0")->text,"Bobbing platform"));
            commands.clear();Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 98040309\n");
        }
        for(const char* term:{"Gravity change","target_gravity_change"}){
            Cvar_Set("jar_spawnSearch",term);UI_JAR_Update(0);
            CHECK(!strcmp(Menu_FindItemByName(focused,"card0")->text,"Gravity change"));
            commands.clear();Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 95709066\n");
        }
        for (const char* term : {"Force Boon", "item_force_boon"}) {
            Cvar_Set("jar_spawnSearch",term);UI_JAR_Update(0);
            CHECK(!strcmp(Menu_FindItemByName(focused,"card0")->text,"Force Boon"));
            commands.clear();Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 78298821\n");
        }
        Cvar_Set("jar_spawnSearch","item_force_enlighten_light");UI_JAR_Update(0);
        CHECK(strstr(Menu_FindItemByName(focused,"card0")->descText,"Light Force Enlightenment"));
        commands.clear();Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 81293752\n");
        Cvar_Set("jar_spawnSearch","item_force_enlighten_dark");UI_JAR_Update(0);
        CHECK(strstr(Menu_FindItemByName(focused,"card0")->descText,"Dark Force Enlightenment"));
        commands.clear();Script("jarOverlaySpawn","0");CHECK(commands=="jar_sb spawn 73654328\n");
        Cvar_Set("jar_spawnSearch",";quit");UI_JAR_Update(0);commands.clear();Script("jarOverlaySpawn","0");CHECK(commands.empty());
        CHECK(!(Menu_FindItemByName(focused,"card0")->window.flags&WINDOW_VISIBLE));
        Script("jarOverlaySearch","clear");CHECK(Menu_FindItemByName(focused,"card0")->window.flags&WINDOW_VISIBLE);
        editingSearch=true;CHECK(UI_JAR_OverlayTyping());UI_JAR_OverlayHeld(false);CHECK(UI_JAR_OverlayActive());
        editingSearch=false;
        UI_JAR_OverlayHeld(false);CHECK(UI_JAR_OverlayActive());UI_JAR_OverlayClose();CHECK(!UI_JAR_OverlayActive() && !focused && catcher==0);
        CHECK(!strcmp(Cvar_VariableString("cl_paused"),"0"));
        UI_JAR_OverlayHeld(true);Cvar_Set("jar_sandbox","0");UI_JAR_Update(0);
        CHECK(!UI_JAR_OverlayActive() && !focused && catcher==0);
        UI_JAR_OverlayHeld(true);CHECK(!UI_JAR_OverlayActive());
        Cvar_Set("jar_sandbox","1");UI_JAR_OverlayHeld(true);focused=main;UI_JAR_Update(0);
        CHECK(!UI_JAR_OverlayActive() && focused==main); // don't close a replacement native menu
        UI_JAR_OverlayHeld(true);Cvar_Set("jar_toolMode","12");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"propertyMore")->window.flags&WINDOW_VISIBLE);
        Script("jarOverlayProperty","up");UI_JAR_Update(0);CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"1"));
        CHECK(strstr(Menu_FindItemByName(focused,"toolValue")->text,"Reverse"));Script("jarOverlayProperty","down");UI_JAR_Update(0);CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"0"));
        UI_JAR_OverlayHeld(false);
        // Entity context is independently held, unpaused and token-scoped.
        catcher=0;commands.clear();UI_JAR_ContextHeld(true);
        CHECK(UI_JAR_OverlayActive()&&focused==Menus_FindByName("jarEntityContext")&&!focused->fullScreen);
        CHECK(commands=="jar_sb context open\n"&&!strcmp(Cvar_VariableString("cl_paused"),"0"));commands.clear();
        for(int i=0;i<focused->itemCount;++i){const auto&r=focused->items[i]->window.rectClient;CHECK(r.x>=0&&r.y>=0&&r.x+r.w<=640&&r.y+r.h<=480);}
        UI_JAR_OverlayHeld(false);CHECK(UI_JAR_OverlayActive()); // releasing Q must not close C
        CHECK(Menu_FindItemByName(focused,"status") && Menu_FindItemByName(focused,"close"));
        CHECK(!(Menu_FindItemByName(focused,"tabDistance")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","122");
        Cvar_Set("jar_contextDistanceAllowed","1");Cvar_Set("jar_contextDistanceRange","192");
        Cvar_Set("jar_contextDistanceSubjects","2");Cvar_Set("jar_contextDistanceArmed","1");
        UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"tabDistance")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","distance_page");
        CHECK(Menu_FindItemByName(focused,"distanceApply")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"distanceSubjects")->text,"NPC"));
        Script("jarContext","distance_link_3");
        CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"3")&&commands.empty());
        Cvar_Set("ui_jar_contextDistanceRange","4097");Script("jarContext","distance");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextDistanceRange","4096");Script("jarContext","distance");
        CHECK(commands=="jar_sb context distance 122 4096\n");commands.clear();
        Script("jarContext","distance_arm");CHECK(commands=="jar_sb context distance_arm 122\n");commands.clear();
        Cvar_Set("jar_contextDistanceAllowed","0");Cvar_Set("jar_linkOutput","0");
        CHECK(!(Menu_FindItemByName(focused,"applyHealth")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","123");Cvar_Set("jar_contextHealthAllowed","1");Cvar_Set("jar_contextHealth","100");
        Cvar_Set("jar_contextPosition0","10");Cvar_Set("jar_contextPosition1","20");Cvar_Set("jar_contextPosition2","30");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"applyHealth")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("ui_jar_contextHealth","250");editingSearch=true;UI_JAR_ContextHeld(false);CHECK(UI_JAR_OverlayActive());
        Script("jarContext","health");CHECK(commands=="jar_sb context health 123 250\n"&&!editingSearch);commands.clear();
        Script("jarContext","position");CHECK(commands=="jar_sb context position 123 10.000 20.000 30.000\n");commands.clear();
        for(const char*name:{"rope","thruster","unlink","weld","hinge"}){Script("jarContext",name);CHECK(commands==std::string("jar_sb context ")+name+" 123\n");commands.clear();}
        CHECK(strstr(Menu_FindItemByName(focused,"action8")->action,"weld")&&strstr(Menu_FindItemByName(focused,"action9")->action,"hinge"));
        for(const char*bad:{"nan","1;quit","100001","1.5"}){Cvar_Set("ui_jar_contextHealth",bad);Script("jarContext","health");CHECK(commands.empty());}
        Cvar_Set("ui_jar_contextPosition0","1;quit");Script("jarContext","position");CHECK(commands.empty());
        uiInfo.uiDC.cursorx=100;uiInfo.uiDC.cursory=200;Script("jarContext","pick");CHECK(commands=="jar_sb context pick 100.000 200.000\n");commands.clear();
        Cvar_Set("jar_contextToken","149");Cvar_Set("jar_contextHealthAllowed","0");Cvar_Set("jar_contextVehicleAllowed","1");
        const char* vehicleMax[]={"100","60","25","0","40","0"};
        const char* vehicleValue[]={"95","45","15","0","12","0"};
        for(int i=0;i<6;++i){Cvar_Set(va("jar_contextVehicleMax%d",i),vehicleMax[i]);Cvar_Set(va("jar_contextVehicleValue%d",i),vehicleValue[i]);}
        UI_JAR_Update(0);Script("jarContext","vehicle_page");
        CHECK(Menu_FindItemByName(focused,"tabVehicle")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"vehicleValue0")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"vehicleValue3")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"vehicleApply")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("ui_jar_contextVehicleValue2","26");Script("jarContext","vehicle_resources");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextVehicleValue2","20");Script("jarContext","vehicle_resources");
        CHECK(commands=="jar_sb context vehicle_resources 149 95 45 20 0 12 0\n");commands.clear();
        Cvar_Set("jar_contextMountedVehicle","1");Cvar_Set("jar_contextToken","150");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"vehicleApply")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"undo")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"world")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"action3")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","player");Script("jarContext","pick");Script("jarContext","remove");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextVehicleValue1","55");Script("jarContext","vehicle_resources");
        CHECK(commands=="jar_sb context vehicle_resources 150 95 55 15 0 12 0\n");commands.clear();
        Script("jarContext","refresh");CHECK(commands=="jar_sb context refresh 150\n");commands.clear();
        Script("jarContext","revert");CHECK(commands=="jar_sb context revert 150\n");commands.clear();
        Cvar_Set("jar_contextMountedVehicle","0");
        Cvar_Set("jar_contextVehicleAllowed","0");
        Cvar_Set("jar_contextToken","151");Cvar_Set("jar_contextNPCAllowed","1");Cvar_Set("jar_contextNPCTeamsAllowed","1");
        Cvar_Set("jar_contextNPCFireAllowed","1");Cvar_Set("jar_contextNPCAltAllowed","1");
        Cvar_Set("jar_contextNPCAmmo","30");Cvar_Set("jar_contextNPCAlternate","0");Cvar_Set("jar_contextNPCManualAmmo","0");
        Cvar_Set("jar_contextNPCTeam","0");Cvar_Set("jar_contextNPCTargetTeam","1");Cvar_Set("jar_contextNPCArmor","33");
        UI_JAR_Update(0);Script("jarContext","npc_page");
        CHECK(Menu_FindItemByName(focused,"npcStatusOpen")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"npcStatusTeam")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"npcStatusTeam")->text,"Free"));
        Script("jarContext","npc_team");Script("jarContext","npc_target");
        CHECK(commands.empty()&&strstr(Menu_FindItemByName(focused,"npcStatusTeam")->text,"Player"));
        Cvar_Set("ui_jar_contextNPCArmor","32768");Script("jarContext","npc_status");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextNPCArmor","90");Script("jarContext","npc_status");
        CHECK(commands=="jar_sb context npc_status 151 1 2 90\n");commands.clear();
        CHECK(Menu_FindItemByName(focused,"npcFireAmmo")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"npcFireSupply")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"npcFireSupply")->text,"Native refill"));
        Script("jarContext","npc_supply");
        CHECK(strstr(Menu_FindItemByName(focused,"npcFireSupply")->text,"Manual"));
        CHECK(Menu_FindItemByName(focused,"npcFireMode")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","npc_alt_fire");CHECK(strstr(Menu_FindItemByName(focused,"npcFireMode")->text,"Alternate"));
        Cvar_Set("ui_jar_contextNPCAmmo","-2");Script("jarContext","npc_fire");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextNPCAmmo","-1");Script("jarContext","npc_fire");
        CHECK(commands=="jar_sb context npc_fire 151 -1 1 1\n");commands.clear();
        Cvar_Set("jar_contextToken","152");Cvar_Set("jar_contextNPCTeamsAllowed","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"npcStatusTeam")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"npcStatusLocked")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"npcStatusArmor")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextNPCAltAllowed","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"npcFireMode")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","153");
        Cvar_Set("jar_contextNPCWeaponChoices",va("%u",(1u<<WP_NONE)|(1u<<WP_BLASTER)|(1u<<WP_REPEATER)));
        Cvar_Set("jar_contextNPCWeapon",va("%d",WP_BLASTER));UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"npcWeaponOpen")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","npc_weapon_page");
        CHECK(Menu_FindItemByName(focused,"npcWeaponEquip")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"npcWeaponHeld")->text,"E-11"));
        Script("jarContext","npc_weapon_next");
        CHECK(strstr(Menu_FindItemByName(focused,"npcWeaponEquip")->text,"Imperial repeater"));
        Script("jarContext","npc_weapon");
        CHECK(commands==va("jar_sb context npc_weapon 153 %d\n",WP_REPEATER));commands.clear();
        Script("jarContext","npc_weapon_next");
        CHECK(strstr(Menu_FindItemByName(focused,"npcWeaponEquip")->text,"Unarmed"));
        Script("jarContext","npc_weapon_previous");
        CHECK(strstr(Menu_FindItemByName(focused,"npcWeaponEquip")->text,"Imperial repeater"));
        Cvar_Set("jar_contextNPCWeaponChoices","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"npcWeaponEquip")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","154");Cvar_Set("jar_contextNPCOrderAllowed","1");
        Cvar_Set("jar_contextNPCOrderMode","0");Cvar_Set("jar_contextNPCOrderDistance","110");
        UI_JAR_Update(0);Script("jarContext","npc_order_page");
        CHECK(Menu_FindItemByName(focused,"npcOrderOpen")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"npcOrderDistance")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","npc_order_mode");Script("jarContext","npc_order");
        CHECK(commands=="jar_sb context npc_order 154 1 110.000\n");commands.clear();
        Script("jarContext","npc_order_mode");
        CHECK(Menu_FindItemByName(focused,"npcOrderDistance")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("ui_jar_contextNPCOrderDistance","71");Script("jarContext","npc_order");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextNPCOrderDistance","185");Script("jarContext","npc_order");
        CHECK(commands=="jar_sb context npc_order 154 2 185.000\n");commands.clear();
        Cvar_Set("jar_contextNPCGoalStatus","Goal: west_gate");
        Cvar_Set("jar_contextNPCGoalLinked","1");
        Script("jarContext","npc_order_mode");
        CHECK(Menu_FindItemByName(focused,"npcOrderClearGoal")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"npcOrderGoal")->text,"west_gate"));
        Script("jarContext","npc_order");
        CHECK(commands=="jar_sb context npc_order 154 3 110.000\n");commands.clear();
        Script("jarContext","npc_goal_clear");
        CHECK(commands=="jar_sb context npc_goal_clear 154\n");commands.clear();
        Cvar_Set("jar_contextNPCPatrolStatus","Current waypoint: 43");
        Script("jarContext","npc_order_mode");
        CHECK(Menu_FindItemByName(focused,"npcOrderPatrol")->window.flags&WINDOW_VISIBLE);
        CHECK(strstr(Menu_FindItemByName(focused,"npcOrderPatrol")->text,"43"));
        Script("jarContext","npc_order");
        CHECK(commands=="jar_sb context npc_order 154 4 110.000\n");commands.clear();
        Cvar_Set("jar_contextNPCOrderAllowed","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"npcOrderApply")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextNPCAllowed","0");Cvar_Set("jar_contextNPCFireAllowed","0");
        Cvar_Set("jar_contextToken","148");Cvar_Set("jar_contextShooterAllowed","1");
        Cvar_Set("jar_contextShooterInterval","500");Cvar_Set("jar_contextShooterVariation","75");
        UI_JAR_Update(0);Script("jarContext","shooter_page");
        CHECK(Menu_FindItemByName(focused,"world")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"player")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"tabShooter")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"shooterApply")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","shooter_interval");CHECK(commands=="jar_sb context shooter_interval 148 500 75\n");commands.clear();
        Cvar_Set("ui_jar_contextShooterInterval","99");Script("jarContext","shooter_interval");CHECK(commands.empty());
        Script("jarContext","shooter_repeat");CHECK(commands=="jar_sb context shooter_repeat 148\n");commands.clear();
        Cvar_Set("jar_contextShooterAllowed","0");
        Cvar_Set("jar_contextToken","147");Cvar_Set("jar_contextSecretAllowed","1");Cvar_Set("jar_contextSecretFound","1");
        UI_JAR_Update(0);Script("jarContext","secret_page");
        CHECK(Menu_FindItemByName(focused,"tabSecret")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"secretRearm")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","secret_rearm");CHECK(commands=="jar_sb context secret_rearm 147\n");commands.clear();
        Cvar_Set("jar_contextSecretAllowed","0");
        Cvar_Set("jar_contextToken","146");Cvar_Set("jar_contextReplicatorAllowed","1");
        Cvar_Set("jar_contextReplicatorHeal","5");Cvar_Set("jar_contextReplicatorDelay","4000");
        UI_JAR_Update(0);Script("jarContext","replicator_page");
        CHECK(Menu_FindItemByName(focused,"tabReplicator")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","replicator_apply");CHECK(commands=="jar_sb context replicator_apply 146 5 4000\n");commands.clear();
        Cvar_Set("jar_contextReplicatorAllowed","0");
        Cvar_Set("jar_contextToken","150");Cvar_Set("jar_contextHealthAllowed","0");Cvar_Set("jar_contextHolocronAllowed","1");Cvar_Set("jar_contextHolocronLevel","2");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"editHolocron")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"applyHealth")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","holocron");CHECK(commands=="jar_sb context holocron 150 2\n");commands.clear();
        Cvar_Set("jar_contextHolocronAllowed","0");Cvar_Set("jar_contextPickupQuantityAllowed","1");
        Cvar_Set("jar_contextPickupQuantity","48");Cvar_Set("jar_contextToken","151");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"applyPickupQuantity")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","pickup_quantity");CHECK(commands=="jar_sb context pickup_quantity 151 48\n");commands.clear();
        Cvar_Set("jar_contextPickupDuration","1");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"editPickupQuantity")->text,"Duration (sec): "));
        CHECK(!strcmp(Menu_FindItemByName(focused,"applyPickupQuantity")->text,"Apply duration"));
        Cvar_Set("ui_jar_contextPickupQuantity","60");
        Script("jarContext","pickup_quantity");CHECK(commands=="jar_sb context pickup_quantity 151 60\n");commands.clear();
        Cvar_Set("jar_contextPickupDuration","0");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"editPickupQuantity")->text,"Quantity: "));
        Cvar_Set("jar_contextPickupLifetimeAllowed","1");Cvar_Set("jar_contextPickupLifetime","30000");
        Cvar_Set("jar_contextToken","152");UI_JAR_Update(0);Script("jarContext","pickup_lifetime_page");
        CHECK(Menu_FindItemByName(focused,"pickupLifetimeApply")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("ui_jar_contextPickupLifetime","0");
        Script("jarContext","pickup_lifetime");CHECK(commands=="jar_sb context pickup_lifetime 152 0\n");commands.clear();
        Cvar_Set("jar_contextPickupLifetimeAllowed","0");
        Cvar_Set("jar_contextPickupQuantityAllowed","0");
        Cvar_Set("jar_contextTrailAllowed","1");Cvar_Set("jar_contextTrailLinked","1");
        Cvar_Set("jar_contextTrailAim","Aim: entity 95");Cvar_Set("jar_contextToken","157");
        UI_JAR_Update(0);Script("jarContext","properties");
        CHECK(Menu_FindItemByName(focused,"trailAim")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"trailUnlink")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","trail_unlink");CHECK(commands=="jar_sb context trail_unlink 157\n");commands.clear();
        Cvar_Set("jar_contextTrailLinked","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"trailUnlink")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextTrailAllowed","0");
        Cvar_Set("jar_contextDeployedShieldAllowed","1");Cvar_Set("jar_contextDeployedShieldHealth","150");
        Cvar_Set("jar_contextDeployedShieldLeft","30");Cvar_Set("jar_contextDeployedShieldRight","80");
        Cvar_Set("jar_contextDeployedShieldHeight","112");Cvar_Set("jar_contextToken","153");UI_JAR_Update(0);
        Script("jarContext","deployed_shield_page");
        CHECK(Menu_FindItemByName(focused,"deployedShieldApply")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","deployed_shield");CHECK(commands=="jar_sb context deployed_shield 153 150 0 30.000 80.000 112.000\n");commands.clear();
        Cvar_Set("ui_jar_contextDeployedShieldHeight","254");Script("jarContext","deployed_shield");CHECK(commands.empty());
        Cvar_Set("jar_contextDeployedShieldAllowed","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"tabDeployedShield")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextMountedChargeAllowed","1");Cvar_Set("jar_contextMountedChargeEditable","1");
        Cvar_Set("jar_contextMountedChargeHealth","25");Cvar_Set("jar_contextMountedChargeDamage","100");
        Cvar_Set("jar_contextMountedChargeRadius","320");Cvar_Set("jar_contextToken","154");UI_JAR_Update(0);
        Script("jarContext","mounted_charge_page");
        CHECK(Menu_FindItemByName(focused,"mountedChargeApply")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"mountedChargeRemaining")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","mounted_charge");CHECK(commands=="jar_sb context mounted_charge 154 25 100 320 0\n");commands.clear();
        Cvar_Set("jar_contextMountedChargeProximity","1");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"mountedChargeRemaining")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("ui_jar_contextMountedChargeRemaining","120001");Script("jarContext","mounted_charge");CHECK(commands.empty());
        Cvar_Set("jar_contextMountedChargeEditable","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"mountedChargeApply")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"mountedChargeStatus")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextMountedChargeAllowed","0");UI_JAR_Update(0);
        for(const char*bad:{"0","4","-1","1.5","nan","1;quit"}){Cvar_Set("ui_jar_contextHolocronLevel",bad);Script("jarContext","holocron");CHECK(commands.empty());}

        Cvar_Set("jar_contextRailAllowed","1");Cvar_Set("jar_contextRailKind","1");Cvar_Set("jar_contextRailLength","512");Cvar_Set("jar_contextRailWidth","256");Cvar_Set("jar_contextRailCell","64");Cvar_Set("jar_contextRailSpeed","128");Cvar_Set("jar_contextRailDelay","0");Cvar_Set("jar_contextRailCount","1");Cvar_Set("jar_contextRailEnabled","1");Cvar_Set("jar_contextRailCenter","0");Cvar_Set("jar_contextToken","158");UI_JAR_Update(0);Script("jarContext","rail_page");
        CHECK(focused->itemCount<MAX_MENUITEMS&&Menu_FindItemByName(focused,"tabRail")->window.flags&WINDOW_VISIBLE&&Menu_FindItemByName(focused,"railTrackLength")->window.flags&WINDOW_VISIBLE&&!(Menu_FindItemByName(focused,"railCenter")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","rail_enabled");Script("jarContext","rail");CHECK(commands=="jar_sb context rail 158 512.000 256.000 64 128.000 0.000 1 0 0\n");commands.clear();
        for(const char*bad:{"31","257","nan","33.5","1;quit"}){Cvar_Set("ui_jar_contextRailCell",bad);Script("jarContext","rail");CHECK(commands.empty());}Cvar_Set("ui_jar_contextRailCell","64");Cvar_Set("ui_jar_contextRailEnabled",".5");Script("jarContext","rail");CHECK(commands.empty());
        Cvar_Set("jar_contextRailKind","3");Cvar_Set("jar_contextToken","159");UI_JAR_Update(0);Script("jarContext","rail_page");CHECK(!(Menu_FindItemByName(focused,"railTrackSpeed")->window.flags&WINDOW_VISIBLE)&&!(Menu_FindItemByName(focused,"railWidth")->window.flags&WINDOW_VISIBLE)&&(Menu_FindItemByName(focused,"railCenter")->window.flags&WINDOW_VISIBLE));Script("jarContext","rail_center");Script("jarContext","rail");CHECK(commands=="jar_sb context rail 159 512.000 256.000 64 128.000 0.000 1 1 1\n");commands.clear();
        Cvar_Set("jar_contextRailAllowed","0");Cvar_Set("jar_contextToken","160");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"tabRail")->window.flags&WINDOW_VISIBLE)&&!(Menu_FindItemByName(focused,"railApply")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextHolocronAllowed","0");Cvar_Set("jar_contextWaypointAllowed","1");Cvar_Set("jar_contextWaypointRange","512");Cvar_Set("jar_contextWaypointEnabled","1");Cvar_Set("jar_contextWaypointStatus","Connected to native navigation");Cvar_Set("jar_contextToken","156");UI_JAR_Update(0);Script("jarContext","waypoint_page");
        CHECK(focused->itemCount<MAX_MENUITEMS&&Menu_FindItemByName(focused,"tabWaypoint")->window.flags&WINDOW_VISIBLE&&Menu_FindItemByName(focused,"waypointApply")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("ui_jar_contextWaypointRange","0");Script("jarContext","waypoint_enabled");Script("jarContext","waypoint");CHECK(commands=="jar_sb context waypoint 156 0.000 0\n");commands.clear();
        for(const char*bad:{"-1","1025","nan","1;quit"}){Cvar_Set("ui_jar_contextWaypointRange",bad);Script("jarContext","waypoint");CHECK(commands.empty());}Cvar_Set("ui_jar_contextWaypointRange","64");Cvar_Set("ui_jar_contextWaypointEnabled",".5");Script("jarContext","waypoint");CHECK(commands.empty());
        Cvar_Set("jar_contextWaypointAllowed","0");Cvar_Set("jar_contextToken","157");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"waypointApply")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"tabWaypoint")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextSkyAllowed","1");Cvar_Set("jar_contextSkyEnabled","0");Cvar_Set("jar_contextSkyEntities","1");Cvar_Set("jar_contextToken","155");UI_JAR_Update(0);Script("jarContext","sky_page");
        CHECK(Menu_FindItemByName(focused,"skyApply")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));CHECK(focused->itemCount<MAX_MENUITEMS);
        Script("jarContext","sky_enabled");Script("jarContext","sky_entities");Script("jarContext","sky");CHECK(commands=="jar_sb context sky 155 1 0\n");commands.clear();
        for(const char*bad:{"2","-1",".5","nan","1;quit"}){Cvar_Set("ui_jar_contextSkyEnabled",bad);Script("jarContext","sky");CHECK(commands.empty());}Cvar_Set("jar_contextSkyAllowed","0");
        Cvar_Set("jar_contextHolocronAllowed","0");Cvar_Set("jar_contextCombatAllowed","1");Cvar_Set("jar_contextCombatFlags","0");Cvar_Set("jar_contextCombatEnabled","1");Cvar_Set("jar_contextCombatStatus","Available to native AI");Cvar_Set("jar_contextToken","154");UI_JAR_Update(0);Script("jarContext","combat_page");
        CHECK(Menu_FindItemByName(focused,"combatApply")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));CHECK(focused->itemCount<MAX_MENUITEMS);
        Script("jarContext","combat_flag1");Script("jarContext","combat_flag5");Script("jarContext","combat_enabled");Script("jarContext","combat");CHECK(commands=="jar_sb context combat 154 34 0\n");commands.clear();
        for(const char*bad:{"64","-1","1.5","nan","1;quit"}){Cvar_Set("ui_jar_contextCombatFlags",bad);Script("jarContext","combat");CHECK(commands.empty());}
        Cvar_Set("jar_contextCombatAllowed","0");Cvar_Set("jar_contextHolocronAllowed","0");Cvar_Set("jar_contextReferenceAllowed","1");Cvar_Set("jar_contextReferenceName","destination");Cvar_Set("jar_contextReferenceOwner","");Cvar_Set("jar_contextReferenceRadius","48");Cvar_Set("jar_contextReferenceEnabled","1");Cvar_Set("jar_contextReferenceGoal","1");Cvar_Set("jar_contextToken","153");UI_JAR_Update(0);Script("jarContext","reference_page");
        CHECK(focused->itemCount<MAX_MENUITEMS&&Menu_FindItemByName(focused,"referenceName")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"referenceOwner")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","reference");CHECK(commands=="jar_sb context reference 153 64657374696e6174696f6e \"\" 48 1\n");commands.clear();Script("jarContext","reference_enabled");Script("jarContext","reference");CHECK(commands=="jar_sb context reference 153 64657374696e6174696f6e \"\" 48 0\n");commands.clear();
        for(const char*bad:{"","bad;quit","two words"}){Cvar_Set("ui_jar_contextReferenceName",bad);Script("jarContext","reference");CHECK(commands.empty());}
        Cvar_Set("jar_contextReferenceAllowed","0");Cvar_Set("jar_contextPoseAllowed","1");Cvar_Set("jar_contextPoseLast","19");Cvar_Set("jar_contextPoseFrame","5");Cvar_Set("jar_contextToken","152");UI_JAR_Update(0);Script("jarContext","pose_page");
        CHECK(focused->itemCount<MAX_MENUITEMS);CHECK(Menu_FindItemByName(focused,"poseFrame")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","pose");CHECK(commands=="jar_sb context pose 152 5\n");commands.clear();
        Script("jarContext","pose_next");CHECK(commands=="jar_sb context pose 152 6\n");commands.clear();Script("jarContext","pose_prev");CHECK(commands=="jar_sb context pose 152 4\n");commands.clear();Script("jarContext","pose_reset");CHECK(commands=="jar_sb context pose 152 0\n");commands.clear();
        for(const char*bad:{"-1","20","1.5","nan","1;quit"}){Cvar_Set("ui_jar_contextPoseFrame",bad);Script("jarContext","pose");CHECK(commands.empty());}
        Cvar_Set("jar_contextPoseAllowed","0");Cvar_Set("jar_contextTravelAllowed","1");Cvar_Set("jar_contextTravelMap","hoth2");Cvar_Set("jar_contextTravelSpawn","");Cvar_Set("jar_contextTravelHub","0");Cvar_Set("jar_contextToken","151");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"tabTravel")->window.flags&WINDOW_VISIBLE);Script("jarContext","travel_page");CHECK(Menu_FindItemByName(focused,"travelMap")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","travel");CHECK(commands=="jar_sb context travel 151 686f746832 \"\" 0\n");commands.clear();Script("jarContext","travel_hub");Script("jarContext","travel");CHECK(commands=="jar_sb context travel 151 686f746832 \"\" 1\n");commands.clear();
        Cvar_Set("ui_jar_contextTravelMap","bad;quit");Script("jarContext","travel");CHECK(commands.empty());
        Cvar_Set("jar_contextTravelAllowed","0");Cvar_Set("jar_contextToken","124");Cvar_Set("jar_contextHealthAllowed","0");Cvar_Set("jar_contextVolumeAllowed","1");
        Cvar_Set("jar_contextVolume0","32");Cvar_Set("jar_contextVolume1","128");Cvar_Set("jar_contextVolume2","64");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"applyHealth")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"editHolocron")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"applyVolume")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"volumeSpaceArmed")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextSpaceAllowed","1");Cvar_Set("jar_contextSpaceArmed","1");Cvar_Set("jar_contextSpaceEnabled","0");UI_JAR_Update(0);
        const auto*vacuum=Menu_FindItemByName(focused,"volumeSpaceArmed");const auto*spaceEnabled=Menu_FindItemByName(focused,"volumeSpaceEnabled");
        CHECK((vacuum->window.flags&WINDOW_VISIBLE)&&!strcmp(vacuum->text,"Vacuum: On"));
        CHECK((spaceEnabled->window.flags&WINDOW_VISIBLE)&&!strcmp(spaceEnabled->text,"Enabled: Off"));
        Script("jarContext","space_armed");CHECK(commands=="jar_sb context space_armed 124\n");commands.clear();
        Script("jarContext","space_enabled");CHECK(commands=="jar_sb context space_enabled 124\n");commands.clear();
        Cvar_Set("jar_contextSpaceAllowed","0");
        for(int i=0;i<3;++i)CHECK(Menu_FindItemByName(focused,va("editVolume%d",i))->window.flags&WINDOW_VISIBLE);
        Script("jarContext","volume");CHECK(commands=="jar_sb context volume 124 32.000 128.000 64.000\n");commands.clear();
        for(const char*bad:{"nan","1;quit","8193","7","-10"}){Cvar_Set("ui_jar_contextVolume0",bad);Script("jarContext","volume");CHECK(commands.empty());}
        Cvar_Set("jar_contextToken","125");Cvar_Set("jar_contextVolumeAllowed","0");Cvar_Set("jar_contextLabelAllowed","1");Cvar_Set("jar_contextText","Hangar 1");Cvar_Set("jar_contextLocation","Hangar 1");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"editText")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"applyVolume")->window.flags&WINDOW_VISIBLE));
        CHECK(!strcmp(Menu_FindItemByName(focused,"info")->descText,"Hangar 1"));CHECK(!strcmp(Cvar_VariableString("ui_jar_contextText"),"Hangar 1"));
        Script("jarContext","label");CHECK(commands=="jar_sb context label 125 48616e6761722031\n");commands.clear();
        for(const char*bad:{"","   ","a;quit","a\"b","a\\b","a\nb"}){Cvar_Set("ui_jar_contextText",bad);Script("jarContext","label");CHECK(commands.empty());}
        const std::string longLabel(64,'a');Cvar_Set("ui_jar_contextText",longLabel.c_str());Script("jarContext","label");CHECK(commands.empty());
        const std::string maxLabel(63,'a');Cvar_Set("ui_jar_contextText",maxLabel.c_str());Script("jarContext","label");CHECK(commands.size()==152&&commands.back()=='\n');commands.clear();
        Cvar_Set("jar_contextVolumeAllowed","1");UI_JAR_Update(0);
        const auto* label=Menu_FindItemByName(focused,"editText");const auto* applyLabel=Menu_FindItemByName(focused,"applyText");
        CHECK((label->window.flags&WINDOW_VISIBLE)&&(applyLabel->window.flags&WINDOW_VISIBLE));
        const auto& labelRect=label->window.rectClient;const auto& applyLabelRect=applyLabel->window.rectClient;
        CHECK(labelRect.x+labelRect.w<=applyLabelRect.x);
        for(int i=0;i<3;++i){const auto* dimension=Menu_FindItemByName(focused,va("editVolume%d",i));CHECK(dimension->window.flags&WINDOW_VISIBLE);CHECK(labelRect.y+labelRect.h<=dimension->window.rectClient.y);CHECK(applyLabelRect.y+applyLabelRect.h<=dimension->window.rectClient.y);}
        CHECK(Menu_FindItemByName(focused,"applyVolume")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","0");editingSearch=true;UI_JAR_Update(0);CHECK(!editingSearch);Script("jarContext","health");CHECK(commands.empty());
        CHECK(!(Menu_FindItemByName(focused,"editText")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"applyVolume")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextVolumeAllowed","0");
        Cvar_Set("jar_contextToken","126");Cvar_Set("jar_contextLabelAllowed","0");Cvar_Set("jar_contextMusicAllowed","1");Cvar_Set("jar_contextText",jar::music::defaultTrack);UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editText")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("ui_jar_contextText","music/death_music.mp3 music/endcredits.mp3");Script("jarContext","music");CHECK(commands=="jar_sb context music 126 6d757369632f64656174685f6d757369632e6d7033206d757369632f656e64637265646974732e6d7033\n");commands.clear();
        Cvar_Set("ui_jar_contextText","music/death_music.mp3;quit");Script("jarContext","music");CHECK(commands.empty());Script("jarContext","music_next");CHECK(commands=="jar_sb context music_next 126\n");commands.clear();
        Cvar_Set("jar_contextParm0","+5");Cvar_Set("jar_contextParmAllowed0","1");Cvar_Set("jar_contextParm15","last");Cvar_Set("jar_contextParmAllowed15","1");Script("jarContext","parms");
        CHECK(Menu_FindItemByName(focused,"parmEdit")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"tabParms")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE));
        CHECK(!strcmp(Cvar_VariableString("ui_jar_contextParm"),"+5"));Script("jarContext","parm");CHECK(commands=="jar_sb context parm 126 1 2b35\n");commands.clear();
        Script("jarContext","parm_prev");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextParm"),"last"));Script("jarContext","parm_clear");CHECK(commands=="jar_sb context parm 126 16 \n");commands.clear();
        Cvar_Set("jar_contextToken","127");Cvar_Set("jar_contextParm15","new selection");UI_JAR_Update(0);CHECK(!strcmp(Cvar_VariableString("ui_jar_contextParm"),"new selection"));
        Cvar_Set("ui_jar_contextParm","a;quit");Script("jarContext","parm");CHECK(commands=="jar_sb context parm 127 16 613b71756974\n");commands.clear();
        Cvar_Set("ui_jar_contextParm",std::string(64,'a').c_str());Script("jarContext","parm");CHECK(commands.empty());
        Script("jarContext","properties");CHECK(!(Menu_FindItemByName(focused,"parmEdit")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","128");Cvar_Set("jar_contextScriptAllowed","1");Cvar_Set("jar_contextScript",jar::scriptRunner::defaultScript);Cvar_Set("jar_contextScriptDelay","0.250");Cvar_Set("jar_contextScriptWait","1.000");Cvar_Set("jar_contextScriptCount","-1");Cvar_Set("jar_contextScriptActivator","1");UI_JAR_Update(0);Script("jarContext","script_page");
        CHECK(Menu_FindItemByName(focused,"tabScript")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"scriptPath")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"parmEdit")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","script");CHECK(commands=="jar_sb context script 128 "+jar::workshop::Hex(jar::scriptRunner::defaultScript)+" 0.250 1.000 -1 1\n");commands.clear();
        Script("jarContext","script_receiver");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextScriptActivator"),"0"));Script("jarContext","script_next");CHECK(commands.empty()&&strcmp(Cvar_VariableString("ui_jar_contextScript"),jar::scriptRunner::defaultScript));
        for(const char*bad:{"../bad","foo;quit",""}){Cvar_Set("ui_jar_contextScript",bad);Script("jarContext","script");CHECK(commands.empty());}
        Cvar_Set("ui_jar_contextScript",jar::scriptRunner::defaultScript);for(const char*bad:{"-2","1.5","100001","nan"}){Cvar_Set("ui_jar_contextScriptCount",bad);Script("jarContext","script");CHECK(commands.empty());}
        Cvar_Set("jar_contextToken","129");Cvar_Set("jar_contextScriptAllowed","0");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"tabScript")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"scriptPath")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","130");Cvar_Set("jar_contextBoundaryAllowed","1");Cvar_Set("jar_contextBoundaryTime","3.000");Cvar_Set("jar_contextBoundaryPreserve","1");UI_JAR_Update(0);Script("jarContext","boundary_page");
        CHECK(Menu_FindItemByName(focused,"tabBoundary")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"boundaryTime")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"scriptPath")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","boundary");CHECK(commands=="jar_sb context boundary 130 3.000 1\n");commands.clear();Script("jarContext","boundary_policy");Script("jarContext","boundary");CHECK(commands=="jar_sb context boundary 130 3.000 0\n");commands.clear();
        for(const char*bad:{"0","60.001","nan","1;quit"}){Cvar_Set("ui_jar_contextBoundaryTime",bad);Script("jarContext","boundary");CHECK(commands.empty());}
        Cvar_Set("jar_contextToken","131");Cvar_Set("jar_contextBoundaryAllowed","0");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"tabBoundary")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"boundaryTime")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","1315");Cvar_Set("jar_contextReflectionAllowed","1");Cvar_Set("jar_contextReflectionRadius","512");Cvar_Set("jar_contextReflectionEnabled","1");Cvar_Set("jar_reflectionStatus","Requires Rend2 renderer");UI_JAR_Update(0);Script("jarContext","reflection_page");
        CHECK(Menu_FindItemByName(focused,"tabReflection")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"reflectionRadius")->window.flags&WINDOW_VISIBLE);CHECK(!strcmp(Menu_FindItemByName(focused,"reflectionStatus")->text,"Requires Rend2 renderer"));CHECK(!(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","reflection_enabled");Script("jarContext","reflection");CHECK(commands=="jar_sb context reflection 1315 512.000 0\n");commands.clear();Script("jarContext","reflection_refresh");CHECK(commands=="jar_sb context reflection_refresh 1315\n");commands.clear();
        for(const char*bad:{"15","32769","nan","1;quit"}){Cvar_Set("ui_jar_contextReflectionRadius",bad);Script("jarContext","reflection");CHECK(commands.empty());}Cvar_Set("ui_jar_contextReflectionRadius","512");Cvar_Set("ui_jar_contextReflectionEnabled","0.5");Script("jarContext","reflection");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","1316");Cvar_Set("jar_contextReflectionAllowed","0");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"tabReflection")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"reflectionRadius")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","132");Cvar_Set("jar_contextCloudAllowed","1");Cvar_Set("jar_contextCloudRadius","512");Cvar_Set("jar_contextCloudInner","128");Cvar_Set("jar_contextCloudCurve","0");Cvar_Set("jar_contextCloudTube","0");Cvar_Set("jar_contextCloudAlternate","0");Cvar_Set("jar_contextCloudEnabled","1");UI_JAR_Update(0);Script("jarContext","cloud_page");
        CHECK(Menu_FindItemByName(focused,"tabCloud")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"cloudRadius")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"boundaryTime")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","cloud_tube");Script("jarContext","cloud_alternate");Script("jarContext","cloud_enabled");Script("jarContext","cloud");CHECK(commands=="jar_sb context cloud 132 512.000 128.000 0.000 1 1 0\n");commands.clear();
        for(const char*bad:{"15","32769","nan","1;quit"}){Cvar_Set("ui_jar_contextCloudRadius",bad);Script("jarContext","cloud");CHECK(commands.empty());}
        Cvar_Set("ui_jar_contextCloudRadius","512");Cvar_Set("ui_jar_contextCloudInner","505");Script("jarContext","cloud");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","134");Cvar_Set("jar_contextCloudAllowed","0");Cvar_Set("jar_contextWeatherAllowed","1");Cvar_Set("jar_contextWeatherKind","3");Cvar_Set("jar_contextWeatherMode","0");Cvar_Set("jar_contextWeatherDensity","1");Cvar_Set("jar_contextWeatherSize","1");Cvar_Set("jar_contextWeatherAlpha",".5");Cvar_Set("jar_contextWeatherSpeed","500");Cvar_Set("jar_contextWeatherEnabled","1");UI_JAR_Update(0);Script("jarContext","weather_page");
        CHECK(Menu_FindItemByName(focused,"tabWeather")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"weatherSpeed")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"weatherDensity")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"cloudRadius")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","weather_mode");Script("jarContext","weather_enabled");Script("jarContext","weather");CHECK(commands=="jar_sb context weather 134 1 1.000 1.000 0.500 500.000 0\n");commands.clear();
        for(const char*bad:{"-1","5001","nan","1;quit"}){Cvar_Set("ui_jar_contextWeatherSpeed",bad);Script("jarContext","weather");CHECK(commands.empty());}Cvar_Set("ui_jar_contextWeatherSpeed","500");Cvar_Set("ui_jar_contextWeatherMode","3");Script("jarContext","weather");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","135");Cvar_Set("jar_contextWeatherKind","0");UI_JAR_Update(0);CHECK(Menu_FindItemByName(focused,"weatherDensity")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"weatherSpeed")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","136");Cvar_Set("jar_contextWeatherKind","4");UI_JAR_Update(0);CHECK(Menu_FindItemByName(focused,"weatherSpeed")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"weatherDensity")->window.flags&WINDOW_VISIBLE));Script("jarContext","weather");CHECK(commands=="jar_sb context weather 136 0 1.000 1.000 0.500 500.000 1\n");commands.clear();
        Cvar_Set("jar_contextToken","137");Cvar_Set("jar_contextWeatherKind","5");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"weatherSpeed")->window.flags&WINDOW_VISIBLE));Script("jarContext","weather_mode");Script("jarContext","weather");CHECK(commands=="jar_sb context weather 137 1 1.000 1.000 0.500 500.000 1\n");commands.clear();Script("jarContext","weather_mode");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextWeatherMode"),"0"));
        Cvar_Set("jar_contextWeatherAllowed","0");
        Cvar_Set("jar_contextToken","138");Cvar_Set("jar_contextLaserAllowed","1");Cvar_Set("jar_contextLaserAction","0");Cvar_Set("jar_contextLaserYaw","0");Cvar_Set("jar_contextLaserPitch","0");Cvar_Set("jar_contextLaserSpeed","30");Cvar_Set("jar_contextLaserDamage","5");Cvar_Set("jar_contextLaserDuration","3");Cvar_Set("jar_contextLaserRange","4096");UI_JAR_Update(0);Script("jarContext","laser_page");
        CHECK(Menu_FindItemByName(focused,"tabLaser")->window.flags&WINDOW_VISIBLE);CHECK(Menu_FindItemByName(focused,"laserYaw")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"weatherMode")->window.flags&WINDOW_VISIBLE));Script("jarContext","laser");CHECK(commands=="jar_sb context laser 138 0 0.000 0.000 30.000 5 3.000 4096.000\n");commands.clear();
        for(const char*bad:{"-1","1001","1.5","nan","1;quit"}){Cvar_Set("ui_jar_contextLaserDamage",bad);Script("jarContext","laser");CHECK(commands.empty());}Cvar_Set("ui_jar_contextLaserDamage","5");Script("jarContext","laser_action");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextLaserAction"),"1"));Cvar_Set("jar_contextLaserAllowed","0");
        Cvar_Set("jar_contextToken","164");Cvar_Set("jar_contextLaserEmitterAllowed","1");
        Cvar_Set("jar_contextLaserEmitterDamage","25");Cvar_Set("jar_contextLaserEmitterFiring","1");
        Cvar_Set("jar_contextLaserEmitterEnabled","1");Cvar_Set("jar_contextLaserEmitterAim","Aim: forward");
        UI_JAR_Update(0);Script("jarContext","laser_page");
        CHECK(Menu_FindItemByName(focused,"tabLaser")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"laserEmitterDamage")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"laserYaw")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","laser_emitter_damage");CHECK(commands=="jar_sb context laser_emitter_damage 164 25\n");commands.clear();
        Cvar_Set("ui_jar_contextLaserEmitterDamage","1.5");Script("jarContext","laser_emitter_damage");CHECK(commands.empty());
        Script("jarContext","laser_emitter_firing");CHECK(commands=="jar_sb context laser_emitter_firing 164\n");commands.clear();
        Script("jarContext","laser_emitter_enabled");CHECK(commands=="jar_sb context laser_emitter_enabled 164\n");commands.clear();
        Script("jarContext","laser_emitter_unlink");CHECK(commands=="jar_sb context laser_emitter_unlink 164\n");commands.clear();
        Cvar_Set("jar_contextLaserEmitterAllowed","0");
        Cvar_Set("jar_contextToken","166");Cvar_Set("jar_contextStationAllowed","1");
        Cvar_Set("jar_contextStationCapacity","100");Cvar_Set("jar_contextStationCharge","17");
        Cvar_Set("jar_contextStationRecharge","250");
        UI_JAR_Update(0);Script("jarContext","station_page");
        CHECK(Menu_FindItemByName(focused,"tabStation")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"stationCapacity")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"laserEmitterDamage")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","station_apply");CHECK(commands=="jar_sb context station_apply 166 100 250\n");commands.clear();
        Script("jarContext","station_charge");CHECK(commands=="jar_sb context station_charge 166 17\n");commands.clear();
        Cvar_Set("ui_jar_contextStationRecharge","99");Script("jarContext","station_apply");CHECK(commands.empty());
        Cvar_Set("ui_jar_contextStationCharge","101");Script("jarContext","station_charge");CHECK(commands.empty());
        Script("jarContext","station_refill");CHECK(commands=="jar_sb context station_refill 166\n");commands.clear();
        Script("jarContext","station_unlimited");CHECK(commands=="jar_sb context station_unlimited 166\n");commands.clear();
        Script("jarContext","station_enabled");CHECK(commands=="jar_sb context station_enabled 166\n");commands.clear();
        Cvar_Set("jar_contextStationAllowed","0");
        Cvar_Set("jar_contextToken","167");Cvar_Set("jar_contextBreakableAllowed","1");
        Cvar_Set("jar_contextBreakableCapacity","40");Cvar_Set("jar_contextBreakableFuse","250");
        Cvar_Set("jar_contextBreakableChunks","100");Cvar_Set("jar_contextBreakableSplash","50");
        Cvar_Set("jar_contextBreakableRadius","80");
        UI_JAR_Update(0);Script("jarContext","breakable_page");
        CHECK(Menu_FindItemByName(focused,"tabBreakable")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"breakableCapacity")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"stationCapacity")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","breakable_apply");CHECK(commands=="jar_sb context breakable_apply 167 40 250 100 50 80\n");commands.clear();
        Cvar_Set("ui_jar_contextBreakableChunks","301");Script("jarContext","breakable_apply");CHECK(commands.empty());
        Script("jarContext","breakable_mode");CHECK(commands=="jar_sb context breakable_mode 167\n");commands.clear();
        Script("jarContext","breakable_repair");CHECK(commands=="jar_sb context breakable_repair 167\n");commands.clear();
        Cvar_Set("jar_contextBreakableAllowed","0");
        Cvar_Set("jar_contextToken","168");Cvar_Set("jar_contextUsableAllowed","1");
        Cvar_Set("jar_contextUsableCapacity","40");Cvar_Set("jar_contextUsableReuse","500");Cvar_Set("jar_linkOutput","0");
        UI_JAR_Update(0);Script("jarContext","usable_page");
        CHECK(Menu_FindItemByName(focused,"tabUsable")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"usableCapacity")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"breakableCapacity")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","usable_apply");CHECK(commands=="jar_sb context usable_apply 168 40 500\n");commands.clear();
        Cvar_Set("ui_jar_contextUsableReuse","60001");Script("jarContext","usable_apply");CHECK(commands.empty());
        Script("jarContext","usable_role");CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"1")&&commands.empty());
        Script("jarContext","usable_unlink");CHECK(commands=="jar_sb context usable_unlink 168\n");commands.clear();
        Script("jarContext","usable_rearm");CHECK(commands=="jar_sb context usable_rearm 168\n");commands.clear();
        Cvar_Set("jar_contextUsableAllowed","0");Cvar_Set("jar_linkOutput","0");
        Cvar_Set("jar_contextToken","169");Cvar_Set("jar_contextSpeakerAllowed","1");
        Cvar_Set("jar_contextSpeakerPeriod","1000");Cvar_Set("jar_contextSpeakerVariation","100");
        UI_JAR_Update(0);Script("jarContext","speaker_page");
        CHECK(Menu_FindItemByName(focused,"tabSpeaker")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"speakerPeriod")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"usableCapacity")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","speaker_apply");CHECK(commands=="jar_sb context speaker_apply 169 1000 100\n");commands.clear();
        Cvar_Set("ui_jar_contextSpeakerVariation","501");Script("jarContext","speaker_apply");CHECK(commands.empty());
        Script("jarContext","speaker_next");CHECK(commands=="jar_sb context speaker_next 169\n");commands.clear();
        Cvar_Set("jar_contextSpeakerAllowed","0");
        Cvar_Set("jar_contextToken","170");Cvar_Set("jar_contextAngularAllowed","1");Cvar_Set("jar_contextAngularKind","1");
        Cvar_Set("jar_contextAngularRate","-120");Cvar_Set("jar_contextAngularPeriod","2000");
        UI_JAR_Update(0);Script("jarContext","angular_page");
        CHECK(Menu_FindItemByName(focused,"angularSpeed")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"angularPeriod")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","angular_apply");CHECK(commands=="jar_sb context angular_apply 170 -120 2000\n");commands.clear();
        Cvar_Set("ui_jar_contextAngularRate","0");Script("jarContext","angular_apply");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","171");Cvar_Set("jar_contextAngularKind","2");Cvar_Set("jar_contextAngularRate","22");
        UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"angularPeriod")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"angularSpeed")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","angular_apply");CHECK(commands=="jar_sb context angular_apply 171 22 2000\n");commands.clear();
        Cvar_Set("jar_contextAngularAllowed","0");

        Cvar_Set("jar_contextToken","139");Cvar_Set("jar_contextPortalAllowed","1");Cvar_Set("jar_contextPortalCamera","0");Cvar_Set("jar_contextPortalWidth","128");Cvar_Set("jar_contextPortalHeight","192");Cvar_Set("jar_contextPortalMirror","0");Cvar_Set("jar_contextPortalEnabled","1");UI_JAR_Update(0);Script("jarContext","portal_page");CHECK(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"laserYaw")->window.flags&WINDOW_VISIBLE));Script("jarContext","portal");CHECK(commands=="jar_sb context portal 139 128.000 192.000 0 1\n");commands.clear();
        Cvar_Set("jar_contextPortalLink","Camera link: #12 (Off)");Cvar_Set("jar_contextPortalCanUnlink","1");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"portalLink")->text,"Camera link: #12 (Off)"));
        CHECK(Menu_FindItemByName(focused,"portalUnlink")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","portal_unlink");CHECK(commands=="jar_sb context portal_unlink 139\n");commands.clear();
        Cvar_Set("jar_contextPortalCanUnlink","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"portalUnlink")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","portal_unlink");CHECK(commands.empty());
        for(const char*bad:{"15","1025","nan","1;quit"}){Cvar_Set("ui_jar_contextPortalWidth",bad);Script("jarContext","portal");CHECK(commands.empty());}Cvar_Set("ui_jar_contextPortalWidth","128");Script("jarContext","portal_mode");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextPortalMirror"),"1"));Cvar_Set("jar_contextToken","140");Cvar_Set("jar_contextPortalCamera","1");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"portalMode")->window.flags&WINDOW_VISIBLE);Cvar_Set("jar_contextPortalAllowed","0");
        Cvar_Set("jar_contextToken","144");Cvar_Set("jar_contextCrystalAllowed","1");Cvar_Set("jar_contextCrystalHitDamage","16");Cvar_Set("jar_contextCrystalHitRadius","32");Cvar_Set("jar_contextCrystalBlastDamage","40");Cvar_Set("jar_contextCrystalBlastRadius","80");Cvar_Set("jar_contextCrystalSolid","1");UI_JAR_Update(0);Script("jarContext","crystal_page");CHECK(focused->itemCount<MAX_MENUITEMS);CHECK(Menu_FindItemByName(focused,"crystalHitDamage")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"tabCrystal")->window.flags&WINDOW_VISIBLE);Script("jarContext","crystal");CHECK(commands=="jar_sb context crystal 144 16 32.000 40 80 1\n");commands.clear();
        for(const char*bad:{"-1","1001","1.5","nan","1;quit"}){Cvar_Set("ui_jar_contextCrystalHitDamage",bad);Script("jarContext","crystal");CHECK(commands.empty());}Cvar_Set("ui_jar_contextCrystalHitDamage","16");Cvar_Set("ui_jar_contextCrystalBlastRadius","80.5");Script("jarContext","crystal");CHECK(commands.empty());Script("jarContext","crystal_solid");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextCrystalSolid"),"0"));Cvar_Set("jar_contextCrystalAllowed","0");
        Cvar_Set("jar_contextToken","143");Cvar_Set("jar_contextDoorAllowed","1");Cvar_Set("jar_contextDoorTravel","48");Cvar_Set("jar_contextDoorSpeed","120");Cvar_Set("jar_contextDoorExtended","0");UI_JAR_Update(0);Script("jarContext","door_page");CHECK(Menu_FindItemByName(focused,"doorTravel")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE));Script("jarContext","door");CHECK(commands=="jar_sb context door 143 48.000 120.000 0\n");commands.clear();
        for(const char*bad:{"7","257","nan","1;quit"}){Cvar_Set("ui_jar_contextDoorTravel",bad);Script("jarContext","door");CHECK(commands.empty());}Script("jarContext","door_extended");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextDoorExtended"),"1"));Cvar_Set("jar_contextDoorAllowed","0");
        Cvar_Set("jar_contextToken","142");Cvar_Set("jar_contextWelderAllowed","1");Cvar_Set("jar_contextWelderReach","15");Cvar_Set("jar_contextWelderRadius","45");Cvar_Set("jar_contextWelderInterval",".2");Cvar_Set("jar_contextWelderDamage","10");Cvar_Set("jar_contextWelderEnabled","1");UI_JAR_Update(0);Script("jarContext","welder_page");CHECK(Menu_FindItemByName(focused,"welderReach")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE));Script("jarContext","welder");CHECK(commands=="jar_sb context welder 142 15.000 45.000 0.200 10 1\n");commands.clear();
        for(const char*bad:{"0","257","nan","1;quit"}){Cvar_Set("ui_jar_contextWelderReach",bad);Script("jarContext","welder");CHECK(commands.empty());}Script("jarContext","welder_enabled");CHECK(!strcmp(Cvar_VariableString("ui_jar_contextWelderEnabled"),"0"));Cvar_Set("jar_contextWelderAllowed","0");
        Cvar_Set("jar_contextToken","141");Cvar_Set("jar_contextSpotAllowed","1");Cvar_Set("jar_contextSpotRange","2048");Cvar_Set("jar_contextSpotCone","8");Cvar_Set("jar_contextSpotLight","225");for(int i=0;i<3;++i)Cvar_Set(va("jar_contextSpotColor%d",i),"1");Cvar_Set("jar_contextSpotTracking","1");Cvar_Set("jar_contextSpotEnabled","1");UI_JAR_Update(0);Script("jarContext","spot_page");CHECK(Menu_FindItemByName(focused,"spotRange")->window.flags&WINDOW_VISIBLE);CHECK(!(Menu_FindItemByName(focused,"portalWidth")->window.flags&WINDOW_VISIBLE));Script("jarContext","spot");CHECK(commands=="jar_sb context spot 141 2048.000 8.000 225.000 1.000 1.000 1.000 1 1\n");commands.clear();
        for(const char*bad:{"63","8193","nan","1;quit"}){Cvar_Set("ui_jar_contextSpotRange",bad);Script("jarContext","spot");CHECK(commands.empty());}Cvar_Set("jar_linkOutput","0");Script("jarContext","spot_output");CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"1"));Cvar_Set("jar_linkOutput","0");Cvar_Set("jar_contextSpotAllowed","0");
        Cvar_Set("jar_contextToken","145");Cvar_Set("jar_contextBobbingAllowed","1");
        Cvar_Set("jar_contextBobbingAmplitude","12");Cvar_Set("jar_contextBobbingPeriod","4000");
        Cvar_Set("jar_contextBobbingEnabled","1");Cvar_Set("jar_contextBobbingRunning","1");
        UI_JAR_Update(0);Script("jarContext","bobbing_page");
        CHECK(Menu_FindItemByName(focused,"tabBobbing")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"bobbingAmplitude")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"editPosition0")->window.flags&WINDOW_VISIBLE)&&focused->itemCount<MAX_MENUITEMS);
        Script("jarContext","bobbing");CHECK(commands=="jar_sb context bobbing 145 12 4000\n");commands.clear();
        for(const char*bad:{"0","129","1.25","nan","1;quit"}){Cvar_Set("ui_jar_contextBobbingAmplitude",bad);Script("jarContext","bobbing");CHECK(commands.empty());}
        Script("jarContext","bobbing_running");CHECK(commands=="jar_sb context bobbing_running 145\n");commands.clear();
        Script("jarContext","bobbing_enabled");CHECK(commands=="jar_sb context bobbing_enabled 145\n");commands.clear();
        Cvar_Set("jar_contextBobbingAllowed","0");
        Cvar_Set("jar_contextToken","146");Cvar_Set("jar_contextMovementKind","1");
        Cvar_Set("jar_contextMovementGravity","420");Cvar_Set("jar_contextMovementFriction","4");
        Cvar_Set("jar_contextMovementCustomGravity","1");UI_JAR_Update(0);Script("jarContext","movement_page");
        CHECK(Menu_FindItemByName(focused,"tabMovement")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"movementGravity")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"movementFriction")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"movementMap")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"movementEnabled")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","movement_gravity");CHECK(commands=="jar_sb context movement_gravity 146 420\n");commands.clear();
        Script("jarContext","movement_friction");CHECK(commands=="jar_sb context movement_friction 146 4\n");commands.clear();
        Script("jarContext","movement_map");CHECK(commands=="jar_sb context movement_map 146\n");commands.clear();
        for(const char* bad:{"-1","11","3.1","nan","1;quit"}){
            Cvar_Set("ui_jar_contextMovementFriction",bad);Script("jarContext","movement_friction");CHECK(commands.empty());
        }
        Cvar_Set("jar_contextToken","147");Cvar_Set("jar_contextMovementKind","3");Cvar_Set("jar_contextMovementEnabled","1");
        UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"movementGravity")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"movementFriction")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"movementEnabled")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"movementMap")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","movement_enabled");CHECK(commands=="jar_sb context movement_enabled 147\n");commands.clear();
        Cvar_Set("jar_contextMovementKind","0");
        Cvar_Set("jar_contextToken","148");Cvar_Set("jar_contextLogicKind","1");
        Cvar_Set("jar_contextLogicDelay","1250");Cvar_Set("jar_contextLogicJitter","250");
        UI_JAR_Update(0);Script("jarContext","logic_page");
        CHECK(Menu_FindItemByName(focused,"logicDelayMs")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicCounterHits")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_apply");CHECK(commands=="jar_sb context logic_apply 148 1250 250\n");commands.clear();
        Cvar_Set("ui_jar_contextLogicJitter","1251");Script("jarContext","logic_apply");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","149");Cvar_Set("jar_contextLogicKind","2");Cvar_Set("jar_contextLogicHits","4");
        UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"logicCounterHits")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicDelayMs")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_apply");CHECK(commands=="jar_sb context logic_apply 149 4\n");commands.clear();
        Script("jarContext","logic_reset");CHECK(commands=="jar_sb context logic_reset 149\n");commands.clear();
        Cvar_Set("jar_linkOutput","0");Script("jarContext","logic_output");
        CHECK(!strcmp(Cvar_VariableString("jar_linkOutput"),"1")&&commands.empty());
        Cvar_Set("jar_contextToken","150");Cvar_Set("jar_contextLogicKind","3");Cvar_Set("jar_contextLogicEnabled","1");
        Cvar_Set("jar_contextLogicDelay","500");Cvar_Set("jar_contextLogicJitter","150");
        UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"logicTimerMs")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"logicVariation")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicDelayMs")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_apply");CHECK(commands=="jar_sb context logic_apply 150 500 150\n");commands.clear();
        Script("jarContext","logic_running");CHECK(commands=="jar_sb context logic_running 150\n");commands.clear();
        Cvar_Set("jar_contextToken","151");Cvar_Set("jar_contextLogicKind","4");Cvar_Set("jar_randomLinkSlot","2");
        Cvar_Set("jar_contextLogicOutput3","84: target_print");
        UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"logicSlotSelect")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicApply")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_next_slot");CHECK(!strcmp(Cvar_VariableString("jar_randomLinkSlot"),"3")&&commands.empty());
        CHECK(!strcmp(Menu_FindItemByName(focused,"logicSlotDestination")->text,"84: target_print"));
        Script("jarContext","logic_clear_output");CHECK(commands=="jar_sb context logic_clear_output 151 4\n");commands.clear();
        Cvar_Set("jar_contextToken","152");Cvar_Set("jar_contextLogicKind","5");
        Cvar_Set("jar_contextLogicDelay","400");Cvar_Set("jar_contextLogicCooldown","750");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"logicRelayCooldown")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicVariation")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_apply");CHECK(commands=="jar_sb context logic_apply 152 400 750\n");commands.clear();
        Cvar_Set("jar_contextToken","153");Cvar_Set("jar_contextLogicKind","6");Cvar_Set("jar_contextLogicPoints","-300");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"logicScorePoints")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","logic_apply");CHECK(commands=="jar_sb context logic_apply 153 -300\n");commands.clear();
        Cvar_Set("ui_jar_contextLogicPoints","-10001");Script("jarContext","logic_apply");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","154");Cvar_Set("jar_contextLogicKind","7");Cvar_Set("jar_contextLogicKillMode","2");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"logicKillMode")->text,"Mode: Electrical"));
        CHECK(!(Menu_FindItemByName(focused,"logicApply")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_kill_mode");CHECK(commands=="jar_sb context logic_kill_mode 154\n");commands.clear();
        Cvar_Set("jar_contextToken","155");Cvar_Set("jar_contextLogicKind","8");
        Cvar_Set("jar_contextLogicOutput1","34: ammo_blaster");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"tabLogic")->text,"Give"));
        CHECK(Menu_FindItemByName(focused,"logicSlotSelect")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicRandomRepeat")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_next_slot");
        CHECK(!strcmp(Cvar_VariableString("jar_giveLinkSlot"),"1")&&!strcmp(Cvar_VariableString("jar_randomLinkSlot"),"3"));
        CHECK(!strcmp(Menu_FindItemByName(focused,"logicSlotDestination")->text,"34: ammo_blaster"));
        Script("jarContext","logic_clear_output");CHECK(commands=="jar_sb context logic_clear_output 155 2\n");commands.clear();
        Cvar_Set("jar_contextToken","156");Cvar_Set("jar_contextLogicKind","9");
        Cvar_Set("jar_contextLogicOutput1","33: trigger_hurt (use off)");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"tabLogic")->text,"Enable"));
        CHECK(Menu_FindItemByName(focused,"logicSlotSelect")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"logicApply")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","logic_next_slot");
        CHECK(!strcmp(Cvar_VariableString("jar_useGateLinkSlot"),"1"));
        CHECK(!strcmp(Menu_FindItemByName(focused,"logicSlotDestination")->text,"33: trigger_hurt (use off)"));
        Script("jarContext","logic_clear_output");CHECK(commands=="jar_sb context logic_clear_output 156 2\n");commands.clear();
        Cvar_Set("jar_contextToken","157");Cvar_Set("jar_contextLogicKind","10");UI_JAR_Update(0);
        CHECK(!strcmp(Menu_FindItemByName(focused,"tabLogic")->text,"Disable"));
        Script("jarContext","logic_enabled");CHECK(commands=="jar_sb context logic_enabled 157\n");commands.clear();
        Cvar_Set("jar_contextLogicKind","0");
        Cvar_Set("jar_contextToken","158");Cvar_Set("jar_contextContactKind","1");
        Cvar_Set("jar_contextContactDelay","125");Cvar_Set("jar_contextContactInterval","500");
        Cvar_Set("jar_contextContactVariation","40");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"tabContact")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","contact_page");
        CHECK(Menu_FindItemByName(focused,"contactRepeatInterval")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","contact_apply");CHECK(commands=="jar_sb context contact_apply 158 125 500 40\n");commands.clear();
        Cvar_Set("jar_contextToken","159");Cvar_Set("jar_contextContactKind","2");
        Cvar_Set("jar_contextContactSpent","1");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"contactRepeatInterval")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"contactRearm")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","contact_rearm");CHECK(commands=="jar_sb context contact_rearm 159\n");commands.clear();
        Cvar_Set("jar_contextToken","160");Cvar_Set("jar_contextContactKind","3");
        Cvar_Set("jar_contextContactSpent","0");Cvar_Set("jar_contextContactDamage","45");
        Cvar_Set("jar_contextContactInterval","900");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"contactHurtDamage")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"contactDelay")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","contact_hurt_apply");CHECK(commands=="jar_sb context contact_hurt_apply 160 45 900\n");commands.clear();
        Cvar_Set("jar_contextContactKind","0");
        Cvar_Set("jar_contextToken","161");Cvar_Set("jar_contextPushKind","1");
        Cvar_Set("jar_contextPushSpeed","750");Cvar_Set("jar_contextPushInterval","350");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"tabPush")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","push_page");CHECK(Menu_FindItemByName(focused,"pushSpeed")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","push_apply");CHECK(commands=="jar_sb context push_apply 161 750 350\n");commands.clear();
        CHECK(Menu_FindItemByName(focused,"pushVehicles")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","push_vehicles");CHECK(commands=="jar_sb context push_vehicles 161\n");commands.clear();
        Cvar_Set("jar_contextPushKind","0");
        Cvar_Set("jar_contextToken","162");Cvar_Set("jar_contextTeleportKind","1");
        Cvar_Set("jar_contextTeleportInterval","700");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"tabTeleport")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","teleport_page");CHECK(Menu_FindItemByName(focused,"teleportInterval")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","teleport_apply");CHECK(commands=="jar_sb context teleport_apply 162 700\n");commands.clear();
        CHECK(Menu_FindItemByName(focused,"teleportProps")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"teleportAssemblies")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"teleportFrozenProps")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"teleportMovers")->window.flags&WINDOW_VISIBLE);
        CHECK(Menu_FindItemByName(focused,"teleportWorldAnchors")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","teleport_assemblies");CHECK(commands=="jar_sb context teleport_assemblies 162\n");commands.clear();
        Script("jarContext","teleport_frozen_props");CHECK(commands=="jar_sb context teleport_frozen_props 162\n");commands.clear();
        Script("jarContext","teleport_movers");CHECK(commands=="jar_sb context teleport_movers 162\n");commands.clear();
        Script("jarContext","teleport_world_anchors");CHECK(commands=="jar_sb context teleport_world_anchors 162\n");commands.clear();
        Script("jarContext","teleport_projectiles");CHECK(commands=="jar_sb context teleport_projectiles 162\n");commands.clear();
        Cvar_Set("jar_contextToken","163");Cvar_Set("jar_contextTeleportKind","2");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"teleportProjectiles")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"teleportAssemblies")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"teleportFrozenProps")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"teleportMovers")->window.flags&WINDOW_VISIBLE));
        CHECK(!(Menu_FindItemByName(focused,"teleportWorldAnchors")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"teleportVehicles")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","teleport_vehicles");CHECK(commands=="jar_sb context teleport_vehicles 163\n");commands.clear();
        Cvar_Set("jar_contextTeleportKind","0");
        Cvar_Set("jar_contextToken","133");Cvar_Set("jar_contextCloudAllowed","0");UI_JAR_Update(0);CHECK(!(Menu_FindItemByName(focused,"tabCloud")->window.flags&WINDOW_VISIBLE));CHECK(!(Menu_FindItemByName(focused,"cloudRadius")->window.flags&WINDOW_VISIBLE));CHECK(Menu_FindItemByName(focused,"editMusic")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"editPropMass")->window.flags&WINDOW_VISIBLE));
        Cvar_Set("jar_contextToken","164");Cvar_Set("jar_contextMusicAllowed","0");Cvar_Set("jar_contextPropPhysicsAllowed","1");
        Cvar_Set("jar_contextPropMass","225");Cvar_Set("jar_contextPropGravity","1");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"editPropMass")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","prop_physics");CHECK(commands=="jar_sb context prop_physics 164 225\n");commands.clear();
        Cvar_Set("ui_jar_contextPropMass","1.5");Script("jarContext","prop_physics");CHECK(commands.empty());
        Script("jarContext","prop_gravity");CHECK(commands=="jar_sb context prop_gravity 164\n");commands.clear();
        Cvar_Set("jar_contextToken","165");Cvar_Set("jar_contextRopeCount","2");
        Cvar_Set("jar_contextRopeId","27");Cvar_Set("jar_contextRopeLength","120.125");UI_JAR_Update(0);
        CHECK(focused->itemCount<MAX_MENUITEMS&&(Menu_FindItemByName(focused,"openRopes")->window.flags&WINDOW_VISIBLE));
        Script("jarContext","rope_page");CHECK(Menu_FindItemByName(focused,"ropePropertyLength")->window.flags&WINDOW_VISIBLE);
        CHECK(!(Menu_FindItemByName(focused,"editPropMass")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"ropePropertyNext")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","rope_length");CHECK(commands=="jar_sb context rope_length 165 27 120.125000\n");commands.clear();
        Cvar_Set("ui_jar_contextRopeLength","8193");Script("jarContext","rope_length");CHECK(commands.empty());
        Script("jarContext","rope_next");CHECK(commands=="jar_sb context rope_next 165\n");commands.clear();
        Cvar_Set("jar_contextRopeCount","0");Cvar_Set("jar_contextToken","166");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"openRopes")->window.flags&WINDOW_VISIBLE));
        CHECK(Menu_FindItemByName(focused,"editPropMass")->window.flags&WINDOW_VISIBLE);
        Cvar_Set("jar_contextToken","167");Cvar_Set("jar_contextThrusterId","28");
        Cvar_Set("jar_contextThrusterForce","5000");Cvar_Set("jar_contextThrusterEnabled","1");UI_JAR_Update(0);
        CHECK(Menu_FindItemByName(focused,"openThruster")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","thruster_page");CHECK(Menu_FindItemByName(focused,"thrusterPropertyForce")->window.flags&WINDOW_VISIBLE);
        Script("jarContext","thruster_power");Script("jarContext","thruster_direction");
        Script("jarContext","thruster_properties");CHECK(commands=="jar_sb context thruster_properties 167 28 5000.000000 0 1\n");commands.clear();
        Cvar_Set("ui_jar_contextThrusterForce","100001");Script("jarContext","thruster_properties");CHECK(commands.empty());
        Cvar_Set("jar_contextToken","168");Cvar_Set("jar_contextThrusterId","0");UI_JAR_Update(0);
        CHECK(!(Menu_FindItemByName(focused,"openThruster")->window.flags&WINDOW_VISIBLE));
        UI_JAR_ContextHeld(false);CHECK(UI_JAR_OverlayActive());UI_JAR_OverlayClose();CHECK(!UI_JAR_OverlayActive()&&catcher==0&&commands=="jar_sb context clear\n");
        UI_JAR_ContextHeld(true);Cvar_Set("jar_sandbox","0");UI_JAR_Update(0);CHECK(!UI_JAR_OverlayActive()&&catcher==0);Cvar_Set("jar_sandbox","1");
        Cvar_Set("sv_running","0");catcher=0;commands.clear();
        for(const char* command:{"+forward","+back","+moveleft","+moveright","+moveup","+movedown","+walk"," +speed "})CHECK(jar::overlay::MovementCommand(command));
        CHECK(jar::overlay::MovementBinding("+moveup; +attack"));CHECK(jar::overlay::MovementBinding("echo x; +forward"));
 CHECK(!jar::overlay::MovementBinding("+attack; quit"));
 for(const char* command:{"+attack","+altattack","+use","+forward;quit","+moveupx",""})CHECK(!jar::overlay::MovementCommand(command));
        uint32_t blocked=0;
        CHECK(jar::overlay::FilterButtons(3,false,2,blocked)==3);
        CHECK(jar::overlay::FilterButtons(3,true,2,blocked)==2);
        CHECK(jar::overlay::FilterButtons(3,false,2,blocked)==2);
        CHECK(jar::overlay::FilterButtons(0,false,2,blocked)==0);
        CHECK(jar::overlay::FilterButtons(1,false,2,blocked)==1);

        // Qualified retail controls keep their actions/conditions/data, and the
        // layout pass is idempotent across opening Advanced/reloading UI.
        menus.clear();
        auto add=[](menuDef_t *menu,const char *name,int type,const char *text) {
            auto *item=static_cast<itemDef_t *>(UI_Alloc(sizeof(itemDef_t)));
            Item_Init(item); item->parent=menu; item->window.name=const_cast<char *>(String_Alloc(name));
            item->type=type; item->text=const_cast<char *>(text); item->window.flags=WINDOW_VISIBLE;
            item->window.rectClient={10,10,100,20}; item->action="retail action unchanged";
            item->cvarTest="native_condition"; item->cvarFlags=CVAR_SHOW;
            menu->items[menu->itemCount++]=item; return item;
        };
        main=Menu_CreateJAR("mainMenu"); pause=Menu_CreateJAR("ingameMainMenu");
        for (auto *menu:{main,pause}) {
            for (const char *name:{"loadgamebutton","controlsbutton","setupbutton","exitgamebutton"}) add(menu,name,ITEM_TYPE_BUTTON,"Label");
            add(menu,menu==main ? "newgamebutton" : "resume",ITEM_TYPE_BUTTON,"Primary");
        }
        auto *decor=add(main,"background",ITEM_TYPE_TEXT,nullptr);
        decor->window.flags|=WINDOW_DECORATION; decor->window.background=99;
        auto *undertext=add(main,"newgamebutton_undertext",ITEM_TYPE_TEXT,"Obsolete duplicate");
        undertext->window.flags|=WINDOW_DECORATION;
        auto *custom=Menu_CreateJAR("characterMenu");
        auto *unknown=add(custom,"custom_selector",ITEM_TYPE_BUTTON,"Custom mod");
        auto *customLoad=Menu_CreateJAR("loadscreen");
        auto *wrongBrief=add(customLoad,"briefingtext",ITEM_TYPE_MULTI,"Custom selector");
        multiDef_t customMulti={}; customMulti.cvarList[1]="must survive"; wrongBrief->typeData=&customMulti;
        add(customLoad,"mappic",ITEM_TYPE_TEXT,nullptr);
        UI_JAR_LoadMenus();
        CHECK(customLoad->itemCount==2 && !strcmp(customMulti.cvarList[1],"must survive"));
        CHECK(wrongBrief->window.rectClient.x==10);
        auto *resume=Menu_FindItemByName(pause,"resume");
        CHECK(resume->window.rectClient.x==32 && resume->window.rectClient.y==156);
        CHECK(!strcmp(resume->action,"retail action unchanged") && !strcmp(resume->cvarTest,"native_condition"));
        CHECK(resume->cvarFlags==CVAR_SHOW);
        CHECK(strstr(pause->onOpen,"setfocus resume"));
        CHECK(!decor->window.background && decor->window.style==WINDOW_STYLE_EMPTY);
        CHECK(!undertext->text && !undertext->window.background);
        const char *order[]={"newgamebutton","loadgamebutton","jarMaps","jarGraphics","controlsbutton","setupbutton","exitgamebutton"};
        for (int i=0;i<7;++i) CHECK(!strcmp(main->items[main->itemCount-7+i]->window.name,order[i]));
        CHECK(custom->itemCount==1 && unknown->window.rectClient.x==10);
        const int mainCount=main->itemCount,pauseCount=pause->itemCount;
        UI_JAR_LoadMenus(); CHECK(main->itemCount==mainCount && pause->itemCount==pauseCount);
        // Replay every qualified retail layout, including duplicate names and
        // conditionally overlapping controls. Selection scripts and data survive.
        std::vector<menuDef_t*> nativeMenus;menuDef_t*nativeFixture=nullptr;
        auto NativeFixtureBegin=[&](const char*name){nativeFixture=Menu_CreateJAR(name);nativeMenus.push_back(nativeFixture);};
        auto NativeFixtureItem=[&](const char*name,int type,float x,float y,float w,float h,bool text){auto*item=add(nativeFixture,name,type,text?"Native label":nullptr);item->window.background=42;item->window.rectClient={x,y,w,h};item->special=73;};
#include "native_menu_layouts.inc"
        struct PreservedControl{itemDef_t*item;int type;const char*action;const char*cvarTest;int cvarFlags;float special;};
        std::vector<PreservedControl> nativeControls;
        for(auto*menu:nativeMenus)for(int i=0;i<menu->itemCount;++i){auto*item=menu->items[i];nativeControls.push_back({item,item->type,item->action,item->cvarTest,item->cvarFlags,item->special});}
        UI_JAR_LoadMenus();
        for(auto*menu:nativeMenus){CHECK(Menu_FindItemByName(menu,"jarNativeTitle"));CHECK(menu->itemCount<MAX_MENUITEMS);}
        for(const auto&c:nativeControls){CHECK(c.item->type==c.type && c.item->action==c.action && c.item->cvarTest==c.cvarTest && c.item->cvarFlags==c.cvarFlags && c.item->special==c.special);}
        auto*mission=Menus_FindByName("ingameMissionSelect1");CHECK(Menu_FindItemByName(mission,"miss1")->window.rectClient.x==32);
        CHECK(Menu_FindItemByName(mission,"nextscreen_button")->window.rectClient.x==452);
        CHECK(Menu_FindItemByName(Menus_FindByName("ingameWpnSelect"),"saber_button")->window.rectClient.y==82);
        CHECK(Menu_FindItemByName(Menus_FindByName("ingameWpnSelect"),"saber_button")->window.style==WINDOW_STYLE_EMPTY);
        CHECK(Menu_FindItemByName(Menus_FindByName("ingameWpnSelect"),"saber_icon")->window.background==42);
        CHECK(Menu_FindItemByName(Menus_FindByName("ingameForceSelect"),"heal_fbutton")->window.style==WINDOW_STYLE_EMPTY);
        rectDef_t destination={192,20,256,256};UI_JAR_NativeAnimatedRect(mission,"tatooine",destination);
        CHECK(destination.x==192 && std::fabs(destination.y-66.8f)<.001f && std::fabs(destination.h-189.44f)<.001f);
        rectDef_t forceUndo={33,95,115,46};UI_JAR_NativeAnimatedRect(Menus_FindByName("ingameForceSelect"),"deallocate_fbutton",forceUndo);CHECK(forceUndo.y==129 && forceUndo.h==46);
        rectDef_t unrelated={20,420,380,22};UI_JAR_NativeAnimatedRect(mission,"button_glow",unrelated);CHECK(unrelated.y==420);
        std::vector<int> nativeCounts;for(auto*menu:nativeMenus)nativeCounts.push_back(menu->itemCount);
        UI_JAR_LoadMenus();for(unsigned i=0;i<nativeMenus.size();++i)CHECK(nativeMenus[i]->itemCount==nativeCounts[i]);
        // Same menu name with a changed control layout must remain a custom menu.
        menus.erase("setupMenu");auto*customSettings=Menu_CreateJAR("setupMenu");auto*customOption=add(customSettings,"custom",ITEM_TYPE_BUTTON,"Custom");
        UI_JAR_LoadMenus();CHECK(customSettings->itemCount==1 && customOption->window.rectClient.x==10);
        CHECK(jar::ui::LoadingStage(-1)==0 && jar::ui::LoadingStage(100)==9);
        CHECK(jar::ui::LoadingWidth(0)==0 && jar::ui::LoadingWidth(9)==576);
        for(int stage=1;stage<=9;++stage) CHECK(jar::ui::LoadingWidth(stage)>jar::ui::LoadingWidth(stage-1));
        CheckMapPicker();
    });
}

