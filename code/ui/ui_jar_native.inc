// Restyle qualified native SP menus without replacing their actions, feeders,
// conditions or model transitions. Custom menus with different controls survive.
// Included after AddItem in ui_jar.cpp. GPL-2.0-only.
static void NativeColor(const float *from,float *to) { for (int i=0;i<4;++i) to[i]=from[i]; }
static void NativeRect(menuDef_t *menu,const char *name,float x,float y,float w,float h) {
    if (auto *item=Menu_FindItemByName(menu,name)) {
        item->window.rectClient={x,y,w,h}; item->textRect={};
    }
}
static void NativeLabel(menuDef_t *menu,const char *name,const char *text,float x,float y,float w,float scale=0.7f) {
    if (auto *item=AddItem(menu,name,text,x,y,w,nullptr)) item->textscale=scale;
}
static void NativePanel(menuDef_t *menu,const char *name,float x,float y,float w,float h,bool line=false) {
    auto *item=AddItem(menu,name,nullptr,x,y,w,nullptr); if (!item) return;
    item->window.rectClient.h=h; item->window.style=WINDOW_STYLE_FILLED;
    item->window.flags|=WINDOW_BACKCOLORSET;
    NativeColor(line ? jar::ui::edge : jar::ui::panel,item->window.backColor);
    // Panels must paint before selectors and live models.
    for (int i=menu->itemCount-1;i>0;--i) menu->items[i]=menu->items[i-1];
    menu->items[0]=item;
}
static void NativeButton(menuDef_t *menu,const char *name,float x,float y,float w,float h=32) {
    auto *item=Menu_FindItemByName(menu,name); if (!item) return;
    NativeRect(menu,name,x,y,w,h);
    item->window.style=WINDOW_STYLE_FILLED;
    item->window.flags|=WINDOW_BACKCOLORSET;
    NativeColor(jar::ui::panel,item->window.backColor);
    item->window.border=WINDOW_BORDER_FULL; item->window.borderSize=1;
    NativeColor(jar::ui::edge,item->window.borderColor);
    item->textalignx=12; item->textaligny=7;
}
static void NativeBase(menuDef_t *menu,bool loading=false) {
    menu->window.style=loading ? WINDOW_STYLE_EMPTY : WINDOW_STYLE_FILLED;
    menu->window.background=0; menu->window.flags|=WINDOW_BACKCOLORSET;
    NativeColor(jar::ui::background,menu->window.backColor);
    NativeColor(jar::ui::accent,menu->focusColor);
    NativeColor(jar::ui::muted,menu->descColor);
    menu->descX=32; menu->descY=400; menu->descScale=0.65f; menu->descAlignment=ITEM_ALIGN_LEFT;
    for (int i=0;i<menu->itemCount;++i) {
        auto *item=menu->items[i];
        if (item->type==ITEM_TYPE_MODEL) continue;
        if ((item->window.flags&WINDOW_DECORATION) && item->window.name && std::strstr(item->window.name,"_undertext"))
            item->text=nullptr;
        // Old hover scripts can still show decorative glow targets. Removing
        // their paint data keeps those scripts harmless, including transitions.
        if ((item->window.flags&WINDOW_DECORATION) && !item->text) {
            item->window.background=0; item->window.style=WINDOW_STYLE_EMPTY;
            item->window.border=WINDOW_BORDER_NONE;
            continue;
        }
        if (item->text || item->type==ITEM_TYPE_MULTI) {
            item->window.background=0; item->window.style=WINDOW_STYLE_EMPTY;
            item->font=UI_RegisterFont("arialnb"); item->textscale=0.8f;
            item->textStyle=ITEM_TEXTSTYLE_SHADOWED; item->textalignment=ITEM_ALIGN_LEFT;
            item->textalignx=0; item->textaligny=2; item->textRect={};
            NativeColor(jar::ui::text,item->window.foreColor);
        }
        if (item->type==ITEM_TYPE_LISTBOX) {
            NativeColor(jar::ui::edge,item->window.borderColor);
            NativeColor(jar::ui::accent,item->window.outlineColor);
        }
    }
}
static void NativeTopNav(menuDef_t *menu) {
    NativeLabel(menu,"jarNativeTitle","JA:R",32,17,160,0.9f);
    NativeButton(menu,"newgamebutton",210,14,80,28);
    NativeButton(menu,"loadgamebutton",298,14,80,28);
    NativeButton(menu,"controlsbutton",386,14,106,28);
    NativeButton(menu,"setupbutton",500,14,108,28);
    NativePanel(menu,"jarHeaderLine",32,48,576,1,true);
    NativePanel(menu,"jarFooterLine",32,430,576,1,true);
    NativeButton(menu,"backbutton",32,440,156);
    NativeButton(menu,"exitgamebutton",242,440,156);
    for (const char *name:{"next","continue","newgame_begin"}) NativeButton(menu,name,452,440,156);
    menu->descY=414;
}
static void NativeHubOrder(menuDef_t *menu,bool pause) {
    const char *mainOrder[]={"newgamebutton","loadgamebutton","jarMaps","jarGraphics","controlsbutton","setupbutton","exitgamebutton"};
    const char *pauseOrder[]={"resume","savegamebutton","loadgamebutton","jarMaps","jarGraphics","controlsbutton","setupbutton","exitgamebutton"};
    const char **order=pause ? pauseOrder : mainOrder;
    // Move the same item pointers to the end in column reading order. Keep all
    // native actions/conditions and leave decoration/custom controls intact.
    for (int n=0;n<(pause ? 8 : 7);++n) {
        auto *item=Menu_FindItemByName(menu,order[n]); if (!item) continue;
        int at=0; while (at<menu->itemCount && menu->items[at]!=item) ++at;
        for (int i=at;i<menu->itemCount-1;++i) menu->items[i]=menu->items[i+1];
        menu->items[menu->itemCount-1]=item;
    }
    menu->cursorItem=-1;
}
static void NativeHub(menuDef_t *menu,bool pause) {
    if (!Menu_FindItemByName(menu,"loadgamebutton") || !Menu_FindItemByName(menu,"controlsbutton") ||
        !Menu_FindItemByName(menu,pause ? "resume" : "newgamebutton")) return;
    NativeBase(menu);
    NativeLabel(menu,"jarNativeTitle","JEDI ACADEMY",32,32,576,1.5f);
    NativeLabel(menu,"jarNativeSubtitle",pause ? "REMIXED  /  MISSION PAUSED" : "REMIXED  /  SINGLE PLAYER",34,72,572,0.75f);
    NativePanel(menu,"jarHeaderLine",32,108,576,1,true);
    NativeLabel(menu,"jarPlayGroup",pause ? "CURRENT MISSION" : "YOUR JOURNEY",32,126,260);
    NativeLabel(menu,"jarSettingsGroup","CONFIGURATION",336,126,272);
    NativeButton(menu,pause ? "resume" : "newgamebutton",32,156,272,44);
    NativeButton(menu,pause ? "savegamebutton" : "loadgamebutton",32,214,272,44);
    NativeButton(menu,pause ? "loadgamebutton" : "jarGraphics",32,272,272,44);
    NativeButton(menu,"jarMaps",32,330,272,44);
    NativeButton(menu,pause ? "jarGraphics" : "controlsbutton",336,156,272,44);
    NativeButton(menu,pause ? "controlsbutton" : "setupbutton",336,214,272,44);
    if (pause) NativeButton(menu,"setupbutton",336,272,272,44);
    NativeLabel(menu,"jarHubHint",pause ? "Resume returns to your mission. Esc also resumes." :
        "Choose JA:R Settings for randomizers, graphics and camera options.",32,390,576,0.7f);
    NativePanel(menu,"jarFooterLine",32,430,576,1,true);
    NativeButton(menu,"exitgamebutton",452,440,156);
    NativeHubOrder(menu,pause);
    if (pause) menu->onOpen=String_Alloc(va("%s setfocus resume ;",menu->onOpen ? menu->onOpen : ""));
}
#include "ui_jar_native_flows.inc"
static void UI_JAR_StyleNativeMenus() {
    NativeAdditionalFlows();
    for (const char *name:{"mainMenu","ingameMainMenu","characterMenu","saberMenu","saberstyleMenu","loadscreen"}) {
        menuDef_t *menu=Menus_FindByName(name);
        if (!menu || Menu_FindItemByName(menu,"jarNativeTitle")) continue;
        if (!Q_stricmp(name,"mainMenu") || !Q_stricmp(name,"ingameMainMenu")) {
            NativeHub(menu,!Q_stricmp(name,"ingameMainMenu"));
        } else if (!Q_stricmp(name,"characterMenu")) {
            if (!Menu_FindItemByName(menu,"character") || !Menu_FindItemByName(menu,"speciesbut") ||
                !Menu_FindItemByName(menu,"torsolistbox")) continue;
            NativeBase(menu); NativeTopNav(menu);
            NativeRect(menu,"creation_title",32,58,576,22);
            NativePanel(menu,"jarSpeciesPanel",24,84,304,148);
            NativePanel(menu,"jarAppearancePanel",24,246,304,172);
            NativePanel(menu,"jarPreviewPanel",344,84,272,334);
        } else if (!Q_stricmp(name,"saberMenu")) {
            if (!Menu_FindItemByName(menu,"saber") || !Menu_FindItemByName(menu,"hiltbut") ||
                !Menu_FindItemByName(menu,"typebut_single")) continue;
            NativeBase(menu); NativeTopNav(menu);
            NativeRect(menu,"creation_title",32,58,576,22);
            NativeRect(menu,"creation_title2",32,58,576,22);
            NativePanel(menu,"jarTypePanel",24,84,184,144);
            NativePanel(menu,"jarHiltPanel",224,84,190,144);
            NativePanel(menu,"jarColorPanel",430,84,186,144);
            NativePanel(menu,"jarPreviewPanel",24,240,592,178);
        } else if (!Q_stricmp(name,"saberstyleMenu")) {
            if (!Menu_FindItemByName(menu,"new_fightingstyle_button") || !Menu_FindItemByName(menu,"saber")) continue;
            NativeBase(menu);
            NativeLabel(menu,"jarNativeTitle","JA:R  /  SABER TRAINING",32,20,576,0.85f);
            NativePanel(menu,"jarHeaderLine",32,48,576,1,true);
            NativeRect(menu,"title",32,62,576,22);
            NativeRect(menu,"fightingstylebutton1",48,106,544,24);
            NativeRect(menu,"new_fightingstyle_button",48,145,544,28);
            for (int i=0;i<menu->itemCount;++i) if (!Q_stricmp(menu->items[i]->window.name,"new_fightingstyle_desc"))
                menu->items[i]->window.rectClient={48,182,544,60};
            NativeButton(menu,"continue",452,440,156);
        } else {
            auto *briefing=Menu_FindItemByName(menu,"briefingtext");
            if (!briefing || briefing->type!=ITEM_TYPE_TEXTSCROLL || !Menu_FindItemByName(menu,"mappic")) continue;
            NativeBase(menu,true);
            NativeLabel(menu,"jarNativeTitle","JA:R  /  MISSION BRIEFING",32,18,576,0.85f);
            NativeRect(menu,"mappic",32,84,248,186);
            NativeRect(menu,"briefingtext",304,84,304,206);
            auto *brief=Menu_FindItemByName(menu,"briefingtext");
            brief->textscale=0.75f;
            if (brief->typeData) static_cast<textScrollDef_t *>(brief->typeData)->lineHeight=14;
            NativeRect(menu,"weaponicons_singlerow",32,302,576,32);
            NativeRect(menu,"weaponicons_row1",32,296,576,32);
            NativeRect(menu,"weaponicons_row2",32,330,576,32);
            NativeRect(menu,"forceicons_singlerow",32,382,576,26);
            NativeRect(menu,"forceicons_row1",32,366,576,26);
            NativeRect(menu,"forceicons_row2",32,394,576,26);
        }
        Menu_PostParse(menu);
    }
}
