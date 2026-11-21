#include "qcommon/jar_diagnostics.h"
// Jedi Academy Remixed graphics and performance menu. GPL-2.0-only.
#include "../server/exe_headers.h"
#include "ui_local.h"
#include "ui_jar.h"
#include "menudef.h"
#include "qcommon/jar_graphics.h"
#include "qcommon/jar_ui_theme.h"
#include "qcommon/jar_sound_randomizer.h"
#include "qcommon/jar_live_randomizer.h"

extern cvar_t *Cvar_Set2(const char *name, const char *value, qboolean force);

static char jarReturnMenu[128] = "mainMenu";
static char jarOriginal[jar::graphicsSettingCount][MAX_CVAR_VALUE_STRING];
static char jarRandomState[160] = "Current map: none. Apply, then start a mission.";
static char jarRestartMap[MAX_QPATH];
static char jarAudioState[180], jarLiveState[192];
static char jarStatus[160] = "Adjust individual settings, then Apply.";
static const char *const jarPages[] = {"jarDisplay", "jarQuality", "jarLighting", "jarMaterials", "jarCrowds", "jarInterface", "jarConsole", "jarStyles", "jarRandomizer", "jarSoundRandomizer", "jarCvarRandomizer"};
static const char *const jarPageTitles[] = {"Display", "AA / FPS", "Lighting", "Materials", "Crowds", "Interface", "Console", "Styles", "Randomizer", "Sound", "Cvars"};
static const int jarPageCount = sizeof(jarPages) / sizeof(jarPages[0]);

static const char *PendingName(size_t index) { return va("ui_jar_%s", jar::graphicsSettings[index].name); }

static const char *PackCategory(const char *cvar) {
    return !std::strcmp(cvar,"jar_fontPack") ? "font" : !std::strcmp(cvar,"jar_uiPack") ? "ui" :
        !std::strcmp(cvar,"jar_saberPack") ? "saber" : !std::strcmp(cvar,"jar_soundPack") ? "sound" : "fx";
}

// Optional catalog descriptors are data only: category, identifier, label on
// three lines. No commands, shader code or UI scripts are executed by the menu.
static void AddLocalPacks(multiDef_t *multi, const char *cvar) {
    if (!std::strstr(cvar,"Pack") || !ui.FS_GetFileList || !ui.FS_FOpenFile || !ui.FS_Read || !ui.FS_FCloseFile) return;
    char files[8192]={};
    int count=ui.FS_GetFileList("jar/catalog",".jap",files,sizeof(files));
    const char *file=files;
    for (int i=0; i<count && file<files+sizeof(files) && *file; ++i) {
        const char *end=static_cast<const char *>(std::memchr(file,0,files+sizeof(files)-file));
        if (!end) break;
        const size_t length=end-file;
        if (length+12>=MAX_QPATH) { file=end+1; continue; }
        char path[MAX_QPATH]; Com_sprintf(path,sizeof(path),"jar/catalog/%s",file);
        file+=length+1;
        fileHandle_t handle=0;
        const int bytes=ui.FS_FOpenFile(path,&handle,FS_READ);
        if (!handle) continue;
        char data[128]={};
        if (bytes<=0 || bytes>=int(sizeof(data))) { ui.FS_FCloseFile(handle); continue; }
        ui.FS_Read(data,bytes,handle); ui.FS_FCloseFile(handle);
        // Accept LF or CRLF without permitting embedded controls in a label.
        int used=0; for (int j=0; j<bytes; ++j) if (data[j]!='\r') data[used++]=data[j]; data[used]=0;
        char *id=std::strchr(data,'\n'); if (!id) continue; *id++=0;
        char *label=std::strchr(id,'\n'); if (!label) continue; *label++=0;
        if (char *end=std::strchr(label,'\n')) *end=0;
        if (std::strcmp(data,PackCategory(cvar)) || !jar::SafePackId(id) || !*label || std::strlen(label)>36) continue;
        bool duplicate=false, clean=true;
        for (const char *p=label; *p; ++p) if (*p<32 || *p>126) clean=false;
        for (int j=0; j<multi->count; ++j) duplicate |= !std::strcmp(multi->cvarStr[j],id);
        if (!clean || duplicate || multi->count>=MAX_MULTI_CVARS) continue;
        multi->cvarList[multi->count]=String_Alloc(label);
        multi->cvarStr[multi->count++]=String_Alloc(id);
    }
}

static int jarTrialState, jarTrialDeadline;
static char jarTrialText[160];
static const char *const jarVideoNames[] = {"r_mode","r_customwidth","r_customheight","r_fullscreen","r_noborder"};
static char jarVideoBackup[5][MAX_CVAR_VALUE_STRING];

static void ReadSettings() {
    for (size_t i = 0; i < jar::graphicsSettingCount; ++i) {
        const auto& setting = jar::graphicsSettings[i];
        cvar_t *cvar = Cvar_FindVar(setting.name);
        const char *value = cvar ? (cvar->latchedString ? cvar->latchedString : cvar->string) : setting.initial;
        Q_strncpyz(jarOriginal[i], value, sizeof(jarOriginal[i]));
        Cvar_Set(PendingName(i), value);
    }
}

static void ReturnToParent() {
    Menus_CloseAll();
    Menus_ActivateByName(jarReturnMenu);
}

void UI_JAR_Open(const char *page) {
    if (jarTrialState) return;
    UI_JAR_OverlayClose();
    UI_JAR_LoadMenus();
    menuDef_t *focused = Menu_GetFocused();
    const char *parent = focused ? focused->window.name : nullptr;
    if (!parent || !Q_strncmp(parent, "jar", 3))
        parent = uiInfo.inGameLoad ? "ingameMainMenu" : "mainMenu";
    Q_strncpyz(jarReturnMenu, parent, sizeof(jarReturnMenu));
    ReadSettings();
    Q_strncpyz(jarStatus, "Adjust individual settings, then Apply.", sizeof(jarStatus));
    Menus_CloseAll();
    Cvar_Set("cl_paused", "1");
    ui.Key_SetCatcher(KEYCATCH_UI);
    int selected=-1;
    for (int i=0; page && i<jarPageCount; ++i)
        if (!Q_stricmp(page,jarPageTitles[i]) || !Q_stricmp(page,jarPages[i])) selected=i;
    if (page && !Q_stricmp(page,"Entity")) selected=8;
    Menus_ActivateByName(jarPages[selected<0 ? 0 : selected]);
    if (selected==10) ui.Cmd_ExecuteText(EXEC_NOW,"jar_cvar_status\n");
}

bool UI_JAR_DisplayTrialActive() { return jarTrialState != 0; }

static void RevertVideo() {
    jarTrialState = 0;
    for (int i=0; i<5; ++i) Cvar_Set2(jarVideoNames[i],jarVideoBackup[i],qfalse);
    JAR_Log(1,"display","trial_revert","Restoring previous display configuration");
    Menus_CloseAll(); ui.Key_SetCatcher(0); Cvar_Set("cl_paused","0");
    ui.Cmd_ExecuteText(EXEC_APPEND,"vid_restart\n");
}

void UI_JAR_WorkshopUpdate();
qboolean UI_JAR_WorkshopScript(const char*,const char**);
void UI_JAR_WorkshopOpen(const char*);
static void UpdateMapPicker();
static void MapPickerAction(const char*);
void UI_JAR_Update(int realtime) {
    UI_JAR_WorkshopUpdate();
    UpdateMapPicker();
    Q_strncpyz(jarRandomState, !std::strcmp(Cvar_VariableString("sv_running"),"1") ?
        Cvar_VariableString("jar_randomStatus") : "Current map: none. Apply, then start a mission.", sizeof(jarRandomState));
    Com_sprintf(jarAudioState,sizeof(jarAudioState),"Live audio: effects %s / UI %s / loops %s / music %s / voices %s | seed %s",
        Cvar_VariableString("jar_randomEffects"),Cvar_VariableString("jar_randomUI"),Cvar_VariableString("jar_randomLoops"),Cvar_VariableString("jar_randomMusic"),Cvar_VariableString("jar_randomVoices"),Cvar_VariableString("jar_randomSeed"));
    Com_sprintf(jarLiveState,sizeof(jarLiveState),"Live values: gravity %s / speed %s / brightness %s / FOV %s / sky %s / lights %s",
        Cvar_VariableString("g_gravity"),Cvar_VariableString("g_speed"),Cvar_VariableString("jar_sceneBrightness"),Cvar_VariableString("cg_fov"),Cvar_VariableString("r_fastsky"),Cvar_VariableString("r_dynamiclight"));
    if (!jarTrialState) return;
    if (jarTrialState == 1) {
        UI_JAR_LoadMenus();
        jarTrialDeadline = realtime + 20000;
        jarTrialState = 2;
    }
    if (realtime >= jarTrialDeadline) { RevertVideo(); return; }
    Com_sprintf(jarTrialText,sizeof(jarTrialText),"Display %d x %d. Keep? Reverting in %d seconds.",uiInfo.uiDC.glconfig.vidWidth,uiInfo.uiDC.glconfig.vidHeight,(jarTrialDeadline-realtime+999)/1000);
    if (Menu_GetFocused() != Menus_FindByName("jarConfirmDisplay")) {
        Menus_CloseAll(); Menus_ActivateByName("jarConfirmDisplay");
    }
    ui.Key_SetCatcher(KEYCATCH_UI); Cvar_Set("cl_paused","1");
}

qboolean UI_JAR_Script(const char *name, const char **args) {
    if (!Q_stricmp(name,"jarMapPicker")) {
        const char* action;
        if (String_Parse(args,&action)) MapPickerAction(action);
        return qtrue;
    }
    if (UI_JAR_WorkshopScript(name,args)) return qtrue;
    if (!Q_stricmp(name,"jarAudioRestore")) {
        ui.Cmd_ExecuteText(EXEC_NOW,"jar_audio_restore\n"); ReadSettings();
        Q_strncpyz(jarStatus,"Original music and future sounds restored. Existing one-shots finish naturally.",sizeof(jarStatus)); return qtrue;
    }
    if (!Q_stricmp(name,"jarCvarRestore")) {
        ui.Cmd_ExecuteText(EXEC_NOW,"jar_cvar_restore\n"); ReadSettings();
        Q_strncpyz(jarStatus,"Owned values restored; later script or manual changes retained.",sizeof(jarStatus)); return qtrue;
    }
    if (!Q_stricmp(name,"jarSectionPreset")) {
        const char *text;
        if (!String_Parse(args,&text) || (std::strcmp(text,"0") && std::strcmp(text,"1"))) return qtrue;
        const int page = Menu_GetFocused()==Menus_FindByName(jarPages[9]) ? 9 : Menu_GetFocused()==Menus_FindByName(jarPages[10]) ? 10 : -1;
        for (size_t i=0; i<jar::graphicsSettingCount; ++i) if (jar::graphicsSettings[i].page == page) Cvar_Set(PendingName(i),text);
        Q_strncpyz(jarStatus,"Section selection pending. Apply to use it.",sizeof(jarStatus)); return qtrue;
    }
    if (!Q_stricmp(name,"jarRandomPreset")) {
        const char *text;
        if (!String_Parse(args,&text) || (std::strcmp(text,"0") && std::strcmp(text,"1") && std::strcmp(text,"2"))) return qtrue;
        const int preset = atoi(text);
        for (unsigned i=1; i<jar::randomizerCvarCount; ++i)
            Cvar_Set(va("ui_jar_%s",jar::pickupCvars[i]), preset == 2 || (preset == 1 && i <= 3) ? "1" : "0");
        Cvar_Set("ui_jar_jar_npcSafety","1");
        Cvar_Set("ui_jar_jar_actorContinuity","0");
        Cvar_Set("ui_jar_jar_actorRecovery","0");
        Q_strncpyz(jarStatus,preset == 2 ? "Entity categories ON; sound/live settings unchanged. Mission safety ON. Apply." :
            preset == 1 ? "PICKUPS pending: conservative swaps within each category. Apply." : "OFF pending: fresh maps will use original gameplay. Apply.",sizeof(jarStatus));
        return qtrue;
    }
    if (!Q_stricmp(name,"jarRandomRestartAsk")) {
        for (size_t i=0; i<jar::graphicsSettingCount; ++i)
            if (jar::graphicsSettings[i].page == 8 && std::strcmp(Cvar_VariableString(PendingName(i)),Cvar_VariableString(jar::graphicsSettings[i].name))) {
                Q_strncpyz(jarStatus,"Apply or revert pending randomizer settings before restarting.",sizeof(jarStatus)); return qtrue;
            }
        const char *map = Cvar_VariableString("mapname");
        if (std::strcmp(Cvar_VariableString("sv_running"),"1") || !*map || std::strlen(map)>=sizeof(jarRestartMap)) {
            Q_strncpyz(jarStatus,"Start a mission to use Restart level.",sizeof(jarStatus)); return qtrue;
        }
        for (const char *p=map; *p; ++p) if (!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='_' || *p=='-' || *p=='/')) {
            Q_strncpyz(jarStatus,"This map name cannot be restarted from the menu.",sizeof(jarStatus)); return qtrue;
        }
        Q_strncpyz(jarRestartMap,map,sizeof(jarRestartMap));
        Menus_CloseAll(); Menus_ActivateByName("jarConfirmRandomizer"); return qtrue;
    }
    if (!Q_stricmp(name,"jarRandomRestartCancel")) {
        jarRestartMap[0]=0; Menus_CloseAll(); Menus_ActivateByName("jarRandomizer"); return qtrue;
    }
    if (!Q_stricmp(name,"jarRandomRestartConfirm")) {
        if (Menu_GetFocused()!=Menus_FindByName("jarConfirmRandomizer") || !*jarRestartMap) return qtrue;
        if (std::strcmp(Cvar_VariableString("sv_running"),"1") || std::strcmp(Cvar_VariableString("mapname"),jarRestartMap)) {
            jarRestartMap[0]=0; Menus_CloseAll(); Menus_ActivateByName("jarRandomizer"); return qtrue;
        }
        Menus_CloseAll(); ui.Key_SetCatcher(0); Cvar_Set("cl_paused","0");
        ui.Cmd_ExecuteText(EXEC_APPEND,va("maptransition %s\n",jarRestartMap)); jarRestartMap[0]=0; return qtrue;
    }
    if (!Q_stricmp(name,"jarKeepVideo")) {
        jarTrialState = 0;
        JAR_Log(1,"display","trial_keep","Display accepted");
        ReturnToParent(); return qtrue;
    }
    if (!Q_stricmp(name,"jarRevertVideo")) { if (jarTrialState) RevertVideo(); return qtrue; }
    if (!Q_stricmp(name,"jarDesktop")) {
        Cvar_Set("ui_jar_r_mode","-2"); Cvar_Set("ui_jar_r_fullscreen","2");
        Q_strncpyz(jarStatus,"Desktop borderless pending. Apply starts a 20-second display test.",sizeof(jarStatus));
        return qtrue;
    }
    if (!Q_stricmp(name, "jarOpen")) { UI_JAR_Open(); return qtrue; }
    if (!Q_stricmp(name, "jarCancel")) { ReturnToParent(); return qtrue; }
    if (!Q_stricmp(name, "jarRevert")) {
        ReadSettings();
        Q_strncpyz(jarStatus, "Pending changes discarded.", sizeof(jarStatus));
        return qtrue;
    }
    if (!Q_stricmp(name, "jarPreset")) {
        const char *preset;
        if (!String_Parse(args, &preset)) return qtrue;
        const int index = atoi(preset);
        JAR_Log(1,"graphics","preset_pending","index=%d",index);
        if (index < 0 || index > 2) return qtrue;
        for (size_t i = 0; i < jar::graphicsSettingCount; ++i)
            if (jar::graphicsSettings[i].preset[index]) Cvar_Set(PendingName(i), jar::graphicsSettings[i].preset[index]);
        Q_strncpyz(jarStatus, index == 2 ?
            "Maximum FPS pending: lower visual detail and reduced distant idle AI." :
            "Preset pending. Review any page, then Apply to use these settings.", sizeof(jarStatus));
        return qtrue;
    }
    if (!Q_stricmp(name, "jarApply")) {
        if (jarTrialState) return qtrue;
        bool restart = false, changed = false, soundRestart = false, displayChanged = false;
        bool dimensionsChanged = false, randomizerChanged = false, audioChanged = false, musicRefresh = false, liveChanged = false;
        const bool livePage = Menu_GetFocused()==Menus_FindByName(jarPages[10]);
        // Validate the entire transaction before setting a single runtime cvar.
        for (size_t i=0; i<jar::graphicsSettingCount; ++i) {
            const auto& setting = jar::graphicsSettings[i];
            const char *value = Cvar_VariableString(PendingName(i));
            if (!std::strcmp(value,jarOriginal[i])) continue;
            if (!jar::GraphicsChoiceValid(setting,value)) {
                Com_sprintf(jarStatus,sizeof(jarStatus),"Invalid %s. Nothing applied.",setting.label); return qtrue;
            }
            if (std::strstr(setting.name,"Pack") && std::strcmp(value,"stock") && ui.FS_FOpenFile) {
                const char *category = PackCategory(setting.name);
                fileHandle_t handle=0;
                ui.FS_FOpenFile(va("jar/%s/%s/pack.txt",category,value),&handle,FS_READ);
                if (!handle) { Com_sprintf(jarStatus,sizeof(jarStatus),"%s pack is not installed. Nothing applied.",setting.label); return qtrue; }
                ui.FS_FCloseFile(handle);
            }
            dimensionsChanged |= setting.page == 0 && setting.maximum != 0;
            displayChanged |= setting.page == 0;
        }
        if (displayChanged && std::strcmp(Cvar_VariableString("ui_jar_r_fullscreen"),"2") &&
            (dimensionsChanged || !std::strcmp(Cvar_VariableString("ui_jar_r_mode"),"-1")) && !jar::ResolutionValid(atoi(Cvar_VariableString("ui_jar_r_customwidth")),atoi(Cvar_VariableString("ui_jar_r_customheight")))) {
            Q_strncpyz(jarStatus,"Invalid custom size: use 320x200 to 16384x16384, up to 64 megapixels.",sizeof(jarStatus)); return qtrue;
        }
        if (displayChanged) {
            if (!std::strcmp(Cvar_VariableString("ui_jar_r_fullscreen"),"2")) Cvar_Set("ui_jar_r_mode","-2");
            else if (dimensionsChanged) Cvar_Set("ui_jar_r_mode","-1");
            for (int i=0; i<5; ++i) Q_strncpyz(jarVideoBackup[i],Cvar_VariableString(jarVideoNames[i]),sizeof(jarVideoBackup[i]));
            jarTrialState = 1;
            JAR_Log(1,"display","trial_begin","20-second display confirmation requested");
        }
        for (size_t i = 0; i < jar::graphicsSettingCount; ++i) {
            const auto& setting = jar::graphicsSettings[i];
            const char *value = Cvar_VariableString(PendingName(i));
            // Preserve custom console values unless this control was changed.
            if (!std::strcmp(value, jarOriginal[i]) || !jar::GraphicsChoiceValid(setting, value)) continue;
            cvar_t *cvar = Cvar_Set2(setting.name, value, qfalse);
            cvar->flags |= CVAR_ARCHIVE;
            restart |= setting.restart;
            soundRestart |= !std::strcmp(setting.name,"jar_soundPack");
            changed = true;
            randomizerChanged |= setting.page == 8;
            audioChanged |= setting.page == 9;
            liveChanged |= setting.page == 10;
            musicRefresh |= !std::strcmp(setting.name,"jar_randomMusic") || ((!std::strcmp(setting.name,"jar_randomSeed") || !std::strcmp(setting.name,"jar_randomMode")) && !std::strcmp(Cvar_VariableString("jar_randomMusic"),"1"));
        }
        JAR_Log(1,"graphics","apply","changed=%d restart=%d",changed,restart);
        const bool applyLive=livePage || liveChanged;
        if (restart) {
            Menus_CloseAll();
            ui.Key_SetCatcher(0);
            Cvar_Set("cl_paused", "0");
            ui.Cmd_ExecuteText(EXEC_APPEND, soundRestart ? "snd_restart\nvid_restart\n" : "vid_restart\n");
        } else {
            ReadSettings();
            Q_strncpyz(jarStatus, livePage ? "Live roll requested. Current values are shown above; Restore originals undoes owned changes." : audioChanged ? "Audio settings applied. Future playback uses this seed; music refreshed." : randomizerChanged ? "Settings applied. Shots/audio use current seed/mode; placed entities change on a fresh map." : changed ? "Settings applied." : "No changes to apply.", sizeof(jarStatus));
        }
        // Cgame initialization resets cheat-marked native values. Roll/restore
        // after vid_restart completes, and refresh music after sound restarts.
        if (musicRefresh) ui.Cmd_ExecuteText(EXEC_APPEND,"jar_audio_refresh\n");
        if (applyLive) ui.Cmd_ExecuteText(EXEC_APPEND,"jar_cvar_apply\n");
        return qtrue;
    }
    return qfalse;
}

static itemDef_t *AddItem(menuDef_t *menu, const char *name, const char *text,
                         float x, float y, float width, const char *action) {
    if (!menu || menu->itemCount >= MAX_MENUITEMS) return nullptr;
    itemDef_t *item = static_cast<itemDef_t *>(UI_Alloc(sizeof(itemDef_t)));
    if (!item) return nullptr;
    Item_Init(item);
    item->parent = menu;
    item->window.name = const_cast<char *>(String_Alloc(name));
    item->window.rectClient = {x, y, width, 22};
    item->window.flags = WINDOW_VISIBLE | WINDOW_FORECOLORSET;
    item->type = action ? ITEM_TYPE_BUTTON : ITEM_TYPE_TEXT;
    if (!action) item->window.flags |= WINDOW_DECORATION;
    item->text = const_cast<char *>(text); // strings below have menu/session lifetime
    item->font = UI_RegisterFont("arialnb");
    item->textscale = 0.85f;
    item->textStyle = ITEM_TEXTSTYLE_SHADOWED;
    item->textaligny = 2;
    VectorSet4(item->window.foreColor, 0.88f, 0.92f, 0.96f, 1.0f);
    item->action = action;
    menu->items[menu->itemCount++] = item;
    return item;
}

#include "ui_jar_native.inc"
#include "ui_jar_overlay.inc"
#include "ui_jar_map_picker.inc"
#include "ui_jar_workshop.inc"

void UI_JAR_LoadMenus() {
    Cvar_Get("jar_ropeSlack","1.1",CVAR_ARCHIVE);Cvar_Get("jar_thrusterForce","5000",CVAR_ARCHIVE);Cvar_Get("jar_thrusterReverse","0",CVAR_ARCHIVE);
    // Register before the first level, including savegame persistence.
    Cvar_Get("jar_randomMode","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    for (unsigned i = 0; i < jar::randomizerCvarCount; ++i)
        Cvar_Get(jar::pickupCvars[i], i == 0 ? "1" : "0", CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART);
    Cvar_Get("jar_npcSafety","1",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_actorContinuity","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    Cvar_Get("jar_actorRecovery","0",CVAR_ARCHIVE|CVAR_SAVEGAME|CVAR_NORESTART);
    for (const char *name : jar::soundCvars) Cvar_Get(name,"0",CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART);
    for (const auto& setting : jar::liveSettings) Cvar_Get(setting.toggle,"0",CVAR_ARCHIVE | CVAR_SAVEGAME | CVAR_NORESTART);
    if (!Menus_FindByName(jarPages[0])) {
        for (int page = 0; page < jarPageCount; ++page) {
            menuDef_t *menu = Menu_CreateJAR(jarPages[page]);
            if (!menu) return;
            menu->fullScreen = qtrue;
            menu->window.style = WINDOW_STYLE_FILLED;
            menu->window.flags |= WINDOW_BACKCOLORSET;
            VectorSet4(menu->window.backColor, 0.025f, 0.04f, 0.065f, 1.0f);
            VectorSet4(menu->focusColor, 0.35f, 0.85f, 1.0f, 1.0f);
            VectorSet4(menu->disableColor, 0.35f, 0.38f, 0.42f, 1.0f);
            VectorSet4(menu->descColor, 0.7f, 0.8f, 0.9f, 1.0f);
            menu->descX = 24; menu->descY = 360; menu->descScale = 0.65f;
            menu->onESC = "uiScript jarCancel ;";
            AddItem(menu, "title", "JA:R  /  ADVANCED", 24, 24, 390, nullptr);
            AddItem(menu, "sandbox", "Sandbox", 500, 24, 115, "uiScript jarWorkshopOpen sandbox ;");
            for (int tab = 0; tab < 9; ++tab) {
                itemDef_t *item = AddItem(menu, va("tab%d", tab), jarPageTitles[tab], 24.0f+tab*66, 62, 65,
                    String_Alloc(va("close %s ; open %s ;", jarPages[page], jarPages[tab])));
                if (item) item->textscale = 0.60f;
                if (item && tab == (page>=8 ? 8 : page)) VectorSet4(item->window.foreColor, 0.35f, 0.85f, 1.0f, 1.0f);
            }
            if (page >= 8) {
                const char *sectionNames[] = {"Entity","Sound","Cvars"};
                for (int section=0; section<3; ++section) {
                    itemDef_t *button = AddItem(menu,va("section%d",section),sectionNames[section],24.0f+196*section,94,185,
                        String_Alloc(va("close %s ; open %s ;",jarPages[page],jarPages[section+8])));
                    if (button && page==section+8) VectorSet4(button->window.foreColor,0.35f,0.85f,1,1);
                }
            }
            if (page == 8) {
                AddItem(menu,"randomOff","Off",24,121,175,"uiScript jarRandomPreset 0 ;");
                AddItem(menu,"randomPickups","Pickups",208,121,175,"uiScript jarRandomPreset 1 ;");
                AddItem(menu,"randomChaos","All Entity",392,121,195,"uiScript jarRandomPreset 2 ;");
            } else if (page>=9) {
                AddItem(menu,"sectionOff","All Off",24,121,250,"uiScript jarSectionPreset 0 ;");
                AddItem(menu,"sectionOn","All On",320,121,280,"uiScript jarSectionPreset 1 ;");
            } else {
                AddItem(menu, "balanced", "Balanced graphics", 24, 99, 300, "uiScript jarPreset 0 ;");
                AddItem(menu, "quality", "Quality", 345, 99, 95, "uiScript jarPreset 1 ;");
                AddItem(menu, "maxfps", "Maximum FPS", 450, 99, 170, "uiScript jarPreset 2 ;");
            }
            int row = 0;
            for (size_t i = 0; i < jar::graphicsSettingCount; ++i) {
                const auto& setting = jar::graphicsSettings[i];
                if (setting.page != page && !(page>8 && (!std::strcmp(setting.name,"jar_randomSeed") || !std::strcmp(setting.name,"jar_randomMode")))) continue;
                itemDef_t *item = AddItem(menu, setting.name, setting.label, 32, (page>=8 ? 151.0f+row++*(page==8?14:20) : 140.0f+row++*25), 570, "");
                if (!item) continue;
                if(page==8){item->window.rectClient.h=14;item->textscale=0.65f;}
                if (setting.maximum) {
                    item->type = ITEM_TYPE_NUMERICFIELD;
                    item->cvar = String_Alloc(PendingName(i));
                    item->descText = setting.description;
                    auto *edit = static_cast<editFieldDef_t *>(UI_Alloc(sizeof(editFieldDef_t)));
                    if (!edit) { item->disabled = qtrue; continue; }
                    memset(edit,0,sizeof(*edit)); edit->maxChars=setting.maximum > 99999 ? 10 : 5; edit->maxPaintChars=edit->maxChars+1;
                    edit->minVal=float(setting.minimum); edit->maxVal=float(setting.maximum);
                    item->typeData=edit;
                    continue;
                }
                item->type = ITEM_TYPE_MULTI;
                item->cvar = String_Alloc(PendingName(i));
                item->descText = setting.description;
                multiDef_t *multi = static_cast<multiDef_t *>(UI_Alloc(sizeof(multiDef_t)));
                if (!multi) { item->disabled = qtrue; continue; }
                memset(multi, 0, sizeof(*multi));
                multi->strDef = qtrue;
                for (const auto& choice : setting.choices) {
                    if (!choice.value) break;
                    multi->cvarList[multi->count] = choice.label;
                    multi->cvarStr[multi->count++] = choice.value;
                }
                AddLocalPacks(multi,setting.name);
                item->typeData = multi;
                if (setting.modern) {
                    item->cvarTest = "ui_jar_cl_renderer";
                    item->enableCvar = "rdjar-rend2";
                    item->cvarFlags = CVAR_ENABLE;
                }
            }
            if (page == 0) {
                AddItem(menu,"desktop","Use desktop borderless",32,285,560,"uiScript jarDesktop ;");
                AddItem(menu,"desktopInfo",String_Alloc(va("Detected desktop: %s x %s at %s Hz",Cvar_VariableString("jar_desktopWidth"),Cvar_VariableString("jar_desktopHeight"),Cvar_VariableString("jar_desktopHz"))),32,312,570,nullptr);
            }
            if (page >= 8) {
                itemDef_t *active = AddItem(menu,"activeState",page==8 ? jarRandomState : page==9 ? jarAudioState : jarLiveState,24,337,590,nullptr);
                if (active) active->textscale=0.55f;
                const char *label = page==8 ? "Restart level with applied settings..." : page==9 ? "Restore original audio" : "Restore originals / disable live features";
                const char *action = page==8 ? "uiScript jarRandomRestartAsk ;" : page==9 ? "uiScript jarAudioRestore ;" : "uiScript jarCvarRestore ;";
                itemDef_t *restore = AddItem(menu,"sectionRestore",label,24,358,590,action);
                if (restore) restore->textscale=0.65f;
                menu->descY=389; menu->descScale=0.58f;
            }
            itemDef_t *status = AddItem(menu, "status", jarStatus, 24, page>=8 ? 402 : 388, 590, nullptr);
            if (status) status->textscale = page>=8 ? 0.58f : 0.7f;
            const char *hintText = page==8 ? "Entity changes apply on fresh maps. Full saves keep their world. Safety OFF can break missions." :
                page==9 ? "One shared installed audio pool. Dialogue keeps original script timing. No audio/video restart." :
                page==10 ? "Fixed values per seed; reroll mode changes on Apply. Gravity/speed scale map baselines. Save/load retains values." :
                "Apply reloads changed assets/video. Display changes require confirmation.";
            itemDef_t *hint = AddItem(menu,"hint",hintText,24,page>=8 ? 425 : 410,590,nullptr);
            if (hint) { hint->textscale=page>=8 ? 0.55f : 0.7f; hint->window.rectClient.h=16; }
            AddItem(menu, "apply", page==10 ? "Apply live roll" : "Apply", 24, 446, 180, "uiScript jarApply ;");
            AddItem(menu, "revert", "Revert pending", 220, 446, 190, "uiScript jarRevert ;");
            AddItem(menu, "back", "Back / Cancel", 440, 446, 175, "uiScript jarCancel ;");
            Menu_PostParse(menu);
        }
    }
    if (!Menus_FindByName("jarConfirmRandomizer")) {
        menuDef_t *menu=Menu_CreateJAR("jarConfirmRandomizer");
        if (menu) {
            menu->fullScreen=qtrue; menu->window.style=WINDOW_STYLE_FILLED;
            menu->window.flags |= WINDOW_BACKCOLORSET;
            VectorSet4(menu->window.backColor,0.025f,0.04f,0.065f,1);
            VectorSet4(menu->focusColor,0.35f,0.85f,1,1);
            menu->onESC="uiScript jarRandomRestartCancel ;";
            AddItem(menu,"restartTitle","Restart this level from the beginning?",48,140,545,nullptr);
            AddItem(menu,"restartWarning","Level progress resets. Autosaves replaced; manual saves kept.",48,180,545,nullptr);
            AddItem(menu,"restartYes","Restart with applied randomizer settings",48,235,545,"uiScript jarRandomRestartConfirm ;");
            AddItem(menu,"restartNo","Cancel - keep playing",48,280,545,"uiScript jarRandomRestartCancel ;");
            Menu_PostParse(menu);
        }
    }
    if (!Menus_FindByName("jarConfirmDisplay")) {
        menuDef_t *menu = Menu_CreateJAR("jarConfirmDisplay");
        if (menu) {
            menu->fullScreen=qtrue; menu->window.style=WINDOW_STYLE_FILLED;
            menu->window.flags |= WINDOW_BACKCOLORSET;
            VectorSet4(menu->window.backColor,0.025f,0.04f,0.065f,1);
            VectorSet4(menu->focusColor,0.35f,0.85f,1,1);
            menu->onESC="uiScript jarRevertVideo ;";
            AddItem(menu,"trialTitle",jarTrialText,48,140,545,nullptr);
            AddItem(menu,"keep","Keep this display",48,215,500,"uiScript jarKeepVideo ;");
            AddItem(menu,"restore","Restore previous display",48,255,500,"uiScript jarRevertVideo ;");
            Menu_PostParse(menu);
        }
    }
    // Add a new entry without replacing the retail menus or assets.
    for (const char *name : {"mainMenu", "ingameMainMenu"}) {
        menuDef_t *menu = Menus_FindByName(name);
        if (!menu) continue;
        bool added=false;
        if (!Menu_FindItemByName(menu,"jarGraphics")) {
            if (auto* item=AddItem(menu,"jarGraphics","JA:R Settings",24,444,215,"uiScript jarOpen ;")) {
                item->descText="Display, graphics, appearance and seeded Chaos randomization.";
                added=true;
            }
        }
        if (!Menu_FindItemByName(menu,"jarMaps")) {
            if (auto* item=AddItem(menu,"jarMaps","Maps",252,444,140,"uiScript jarMapPicker open ;")) {
                item->descText="Browse and launch installed maps.";
                added=true;
            }
        }
        if (added) Menu_PostParse(menu);
    }
    UI_JAR_StyleNativeMenus();
}
