// Native SP flows: retain every action, conditional, feeder and live model.
// Layout signatures exclude custom menus. GPL-2.0-only.
enum class NativeFlowKind {Difficulty,Settings,Archives,Missions,Weapons,Force,Debrief,Datapad,Dialog};
struct NativeFlow {const char*name;int count;uint32_t hash;NativeFlowKind kind;const char*title;};
static const NativeFlow nativeFlows[]={
#include "ui_jar_native_layouts.inc"
};
static uint32_t NativeLayoutHash(const menuDef_t*m){
 uint32_t h=2166136261u;auto byte=[&](unsigned char c){h=(h^c)*16777619u;};
 for(int i=0;i<m->itemCount;++i){const auto&item=*m->items[i];const char*n=item.window.name?item.window.name:"";
  for(;*n;++n)byte((*n>='A'&&*n<='Z')?*n-'A'+'a':*n);byte(0);
  const auto&r=item.window.rectClient;int values[]={item.type,int(r.x*100+(r.x>=0?.5f:-.5f)),int(r.y*100+(r.y>=0?.5f:-.5f)),int(r.w*100+(r.w>=0?.5f:-.5f)),int(r.h*100+(r.h>=0?.5f:-.5f))};
  for(int v:values)for(int b=0;b<4;++b)byte(unsigned(v)>>(b*8));
 }return h;
}
static bool NativeNamed(const itemDef_t*i,const char*n){return i->window.name&&!Q_stricmp(i->window.name,n);}
static void NativeFlowButton(itemDef_t*i){
 i->window.style=i->type==ITEM_TYPE_BUTTON && !i->text?WINDOW_STYLE_EMPTY:WINDOW_STYLE_FILLED;i->window.flags|=WINDOW_BACKCOLORSET;
 NativeColor(jar::ui::panel,i->window.backColor);i->window.border=WINDOW_BORDER_FULL;i->window.borderSize=1;NativeColor(jar::ui::edge,i->window.borderColor);
 if(i->text){i->textalignment=ITEM_ALIGN_LEFT;i->textalignx=8;i->textaligny=3;}
}
static void NativeFlowBase(menuDef_t*m){
 m->window.border=WINDOW_BORDER_NONE;m->window.style=WINDOW_STYLE_FILLED;m->window.background=0;m->window.flags|=WINDOW_BACKCOLORSET;NativeColor(jar::ui::background,m->window.backColor);
 NativeColor(jar::ui::accent,m->focusColor);NativeColor(jar::ui::muted,m->descColor);m->descX=32;m->descY=427;m->descScale=.58f;m->descAlignment=ITEM_ALIGN_LEFT;
 for(int i=0;i<m->itemCount;++i){auto*item=m->items[i];const char*n=item->window.name?item->window.name:"";
  // Keep meaningful icons, screenshots, previews and conditionally shown content.
  // Only obsolete framing/glow paint is suppressed; native scripts may still target it.
  if(NativeNamed(item,"background")||NativeNamed(item,"background_text")||NativeNamed(item,"background_text_b")||NativeNamed(item,"starwars")||NativeNamed(item,"scan")||std::strstr(n,"_glow")||std::strstr(n,"_undertext")||NativeNamed(item,"left_frame")||NativeNamed(item,"right_frame")||!Q_strncmp(n,"eyecandy",8)||NativeNamed(item,"botlf")){
   item->window.background=0;item->window.style=WINDOW_STYLE_EMPTY;item->window.border=WINDOW_BORDER_NONE;if(std::strstr(n,"_undertext"))item->text=nullptr;continue;}
  if(NativeNamed(item,"window")||NativeNamed(item,"driver_background")||NativeNamed(item,"warn_background")||NativeNamed(item,"really_background")||NativeNamed(item,"top_background")||NativeNamed(item,"bottom_background")){
   item->window.background=0;item->window.style=WINDOW_STYLE_FILLED;item->window.flags|=WINDOW_BACKCOLORSET;NativeColor(jar::ui::panel,item->window.backColor);item->window.border=WINDOW_BORDER_FULL;item->window.borderSize=1;NativeColor(jar::ui::edge,item->window.borderColor);
  }
  if(item->type==ITEM_TYPE_MODEL)continue;
  if(item->text||item->type==ITEM_TYPE_MULTI||item->type==ITEM_TYPE_BIND||item->type==ITEM_TYPE_YESNO||item->type==ITEM_TYPE_SLIDER){
   item->font=UI_RegisterFont("arialnb");item->textStyle=ITEM_TEXTSTYLE_SHADOWED;NativeColor(jar::ui::text,item->window.foreColor);item->textscale=.68f;
  }
  if(item->text){item->window.background=0;if(item->type!=ITEM_TYPE_BUTTON)item->window.style=WINDOW_STYLE_EMPTY;}
  if(m->window.name&&!Q_stricmp(m->window.name,"videodriverMenu")&&NativeNamed(item,"game_version")){
   item->font=UI_RegisterFont("arialnb");item->textscale=.6f;NativeColor(jar::ui::text,item->window.foreColor);item->window.rectClient={16,16,408,258};
  }
  if(item->type==ITEM_TYPE_BUTTON)NativeFlowButton(item);
  if(item->type==ITEM_TYPE_LISTBOX){NativeColor(jar::ui::edge,item->window.borderColor);NativeColor(jar::ui::accent,item->window.outlineColor);NativeColor(jar::ui::panel,item->window.backColor);NativeColor(jar::ui::text,item->window.foreColor);item->window.style=WINDOW_STYLE_FILLED;item->window.flags|=WINDOW_BACKCOLORSET;}
  if(!item->text && (std::strstr(n,"_box")||std::strstr(n,"_outline")||std::strstr(n,"_hex_background"))){item->window.background=0;NativeFlowButton(item);}
  item->textRect={};
 }
}
static void NativeFlowLayout(menuDef_t*m,const NativeFlow&flow){
 NativeFlowBase(m);const int original=m->itemCount;
 if(flow.kind==NativeFlowKind::Dialog){
  // Dialogs use their own local window rectangle and native heading/body.
  // A zero-area marker avoids painting a full-screen header inside a popup.
  auto*marker=AddItem(m,"jarNativeTitle",nullptr,0,0,0,nullptr);if(marker)marker->window.rectClient.h=0;
  Menu_PostParse(m);JAR_Log(1,"ui","native_flow","menu=%s kind=%d original=%d final=%d",flow.name,int(flow.kind),original,m->itemCount);return;
 }
 const bool configuration=flow.kind==NativeFlowKind::Settings||flow.kind==NativeFlowKind::Difficulty||flow.kind==NativeFlowKind::Archives;
 for(int i=0;i<original;++i){auto*item=m->items[i];auto&r=item->window.rectClient;const rectDef_t old=r;const char*n=item->window.name?item->window.name:"";
  if(item->text){item->window.background=0;if(item->type!=ITEM_TYPE_BUTTON)item->window.style=WINDOW_STYLE_EMPTY;}
  if(configuration){
   if(item->type==ITEM_TYPE_TEXT && !item->text && (std::strstr(n,"background")||!Q_strncmp(n,"highlight",9)||std::strstr(n,"_button"))){
    item->window.background=0;item->window.style=WINDOW_STYLE_EMPTY;item->window.border=WINDOW_BORDER_NONE;
    if(NativeNamed(item,"really_background")||NativeNamed(item,"vidrestart_background")){item->window.style=WINDOW_STYLE_FILLED;item->window.flags|=WINDOW_BACKCOLORSET;NativeColor(jar::ui::panel,item->window.backColor);}
   }
   if(old.y==126 && item->type==ITEM_TYPE_BUTTON){const int col=old.x<160?0:old.x<320?1:old.x<480?2:3;r={32.f+col*148,50,132,27};}
   else if(old.y>=164 && old.y<425){r.y=86+(old.y-164)*1.28f;r.h=std::min(old.h*1.28f,418-r.y);
    if(flow.kind==NativeFlowKind::Settings){if(old.x>=250 && old.w>=280){r.x=244;r.w=364;item->textalignment=ITEM_ALIGN_LEFT;item->textalignx=0;item->textaligny=1;}
     else if(item->type==ITEM_TYPE_BUTTON && old.x<250){r.x=32;r.w=190;r.h=26;}}
   }
   if(old.y>=438 && item->type==ITEM_TYPE_BUTTON){r={old.x<220?32.f:old.x<420?242.f:452.f,442,156,28};}
   if(flow.kind==NativeFlowKind::Difficulty){const char*names[]={"easybut","medbut","hardbut","vhardbut"};for(int row=0;row<4;++row)if(NativeNamed(item,names[row]))r={120,136.f+row*64,400,44};}
   if(flow.kind==NativeFlowKind::Archives){
    if(item->type==ITEM_TYPE_LISTBOX)r={32,142,360,248};
    if(NativeNamed(item,"loadgamepic")||NativeNamed(item,"savegamepic"))r={420,142,188,141};
    if(item->type==ITEM_TYPE_BUTTON && old.x>=430 && old.y>=350 && old.y<430){r={420,304+(old.y-360)*1.35f,188,28};item->textscale=.6f;}
    if(NativeNamed(item,"savegamedesc"))r={32,115,576,22};
    if(NativeNamed(item,"savegame_desc"))r={32,395,360,20};
   }
  }else if(flow.kind==NativeFlowKind::Weapons){
   if(old.y<30 && item->text){r={32,50,576,24};item->textalignment=ITEM_ALIGN_LEFT;item->textalignx=0;}
   else if(old.y>=30 && old.y<175){const bool lower=old.y>=90;r.y=lower?162:82;
    if(std::strstr(n,"_icon")){r.w=r.h=64;}
    else if(std::strstr(n,"hex_background")){r.h=66;item->window.background=0;NativeFlowButton(item);}
   }
   if(NativeNamed(item,"weapon_desc"))r={132,246,476,86};
   if(NativeNamed(item,"weapon_icon"))r={32,246,88,88};
   if(NativeNamed(item,"instruction_title"))r={48,245,456,24};
   if(NativeNamed(item,"instruction_text"))r={48,273,448,120};
   if(NativeNamed(item,"setup_background"))r={32,239,576,173};
   if(NativeNamed(item,"inst_done_button"))r={506,349,100,34};
   if(old.y>=358 && old.y<430 && std::strcmp(n,"inst_done_button")){r.y=354;r.h=64;}
  }else if(flow.kind==NativeFlowKind::Force){
   if(old.y<30 && item->text){r={32,50,576,22};item->textalignment=ITEM_ALIGN_LEFT;item->textalignx=0;}
   else if(old.y>=30 && old.y<255){r.y=old.y+34;if(std::strstr(n,"_iconpic")){r.w=r.h=48;r.y+=8;}}
   if(NativeNamed(item,"force_desc"))r={157,293,451,62};
   if(NativeNamed(item,"force_icon")||NativeNamed(item,"big_sense"))r={32,302,104,104};
   if(std::strstr(n,"_level")){r.x=157;r.y=362;r.w=451;r.h=57;}
   if(NativeNamed(item,"sense_text")||NativeNamed(item,"saboff_text"))r={157,293,451,62};
  }else if(flow.kind==NativeFlowKind::Missions){
   if(old.y<300 && item->type!=ITEM_TYPE_MODEL){r.y=52+old.y*.74f;r.h=old.h*.74f;}
   if(!Q_strncmp(n,"miss",4) && item->type==ITEM_TYPE_BUTTON && n[4]>='1'&&n[4]<='5')r={32,290.f+(n[4]-'1')*27,356,24};
   if(NativeNamed(item,"briefing_background")){r={24,284,376,140};item->window.background=0;NativeFlowButton(item);}
   if(NativeNamed(item,"briefing_text")||!Q_strncmp(n,"accept",6)){r={32,290,360,128};item->textscale=.67f;}
   if(item->type==ITEM_TYPE_MODEL)r={426,282,174,146};
   if(NativeNamed(item,"map_title")){item->text=nullptr;}
   if(NativeNamed(item,"backbut"))r={32,442,170,28};
   if(!Q_strncmp(n,"accbut",6))r={238,442,170,28};
   if(NativeNamed(item,"nextscreen_button"))r={452,442,156,28};
  }else if(flow.kind==NativeFlowKind::Debrief){
   if(old.y<50 && item->text){r={32,50,576,26};item->textalignment=ITEM_ALIGN_LEFT;item->textalignx=0;}
   else if(old.y>=50 && old.y<280){r.x=std::max(32.f,old.x+20);r.y=old.y+28;r.w=std::min(old.w,608-r.x);item->textscale=.6f;}
   if(item->type==ITEM_TYPE_MODEL)r={428,304,164,128};
   if(old.y>=310 && old.y<440 && item->text){r={32,314,372,110};item->textscale=.64f;}
  }else if(flow.kind==NativeFlowKind::Datapad){
   if(NativeNamed(item,"screen_title")){item->text=nullptr;}
   if(item->type==ITEM_TYPE_BUTTON && old.y==420){r={20+(old.x-20)*.97f,442,114,28};item->textscale=.62f;}
   else if(old.y<410 && old.y>=30){r.y=52+(old.y-30)*.94f;r.h=old.h*.94f;}
   if(item->type==ITEM_TYPE_MODEL)r={304,70,288,300};
  }
  if(!configuration && flow.kind!=NativeFlowKind::Missions && flow.kind!=NativeFlowKind::Dialog && old.y>=440 && item->type==ITEM_TYPE_BUTTON){
   r={old.x<200?32.f:old.x<390?238.f:412.f,442,old.x<200?170.f:196.f,28};item->textscale=.66f;
  }
  if(flow.kind==NativeFlowKind::Debrief && old.y>=440 && item->type==ITEM_TYPE_BUTTON){
   if(old.w<=100)r={old.x>=490?492.f:352.f,442,old.x>=490?116.f:132.f,28};
   else r={388,442,220,28};
  }
  item->textRect={};
 }
 NativeLabel(m,"jarNativeTitle",flow.title,32,18,576,.9f);
 NativePanel(m,"jarFlowHeader",32,44,576,1,true);
 if(flow.kind!=NativeFlowKind::Dialog)NativePanel(m,"jarFlowFooter",32,436,576,1,true);
 if(flow.kind==NativeFlowKind::Settings){NativePanel(m,"jarFlowNavigation",24,106,206,311);NativePanel(m,"jarFlowContent",236,106,380,311);}
 if(flow.kind==NativeFlowKind::Difficulty)NativeLabel(m,"jarDifficultyHint","Select a challenge, then continue to character creation.",32,405,576,.65f);
 Menu_PostParse(m);JAR_Log(1,"ui","native_flow","menu=%s kind=%d original=%d final=%d",flow.name,int(flow.kind),original,m->itemCount);
}
static void NativeAdditionalFlows(){
 for(const auto&flow:nativeFlows){auto*m=Menus_FindByName(flow.name);if(!m||Menu_FindItemByName(m,"jarNativeTitle"))continue;
  const auto hash=NativeLayoutHash(m);if(m->itemCount!=flow.count||hash!=flow.hash){JAR_Log(1,"ui","native_flow_unqualified","menu=%s count=%d hash=%u",flow.name,m->itemCount,hash);continue;}
  NativeFlowLayout(m,flow);
 }
}

// Native mission animations specify stock pixel coordinates. Map only their
// visual rectangles into the qualified layout; leave script actions unchanged.
void UI_JAR_NativeAnimatedRect(menuDef_t*menu,const char*name,rectDef_t&rect){
 if(!menu||!name||!menu->window.name||!Menu_FindItemByName(menu,"jarNativeTitle"))return;
 if(!Q_stricmp(menu->window.name,"ingameForceSelect")&&!Q_stricmp(name,"deallocate_fbutton")){rect.y+=34;return;}
 if(Q_stricmp(menu->window.name,"ingameMissionSelect1")&&Q_stricmp(menu->window.name,"ingameMissionSelect2")&&Q_stricmp(menu->window.name,"ingameMissionSelect3"))return;
 bool map=!Q_strncmp(name,"loc_but",7);
 for(const char*planet:{"planet_name","tatooine","bakura","blenjeel","corellia","krildor","narkreeta","dosuun","zonju","coruscant","big_planet","yalara","chandrila","ordmantell","tanaab","byss"})if(!Q_stricmp(name,planet))map=true;
 if(map){rect.y=52+rect.y*.74f;rect.h*=.74f;}
}
