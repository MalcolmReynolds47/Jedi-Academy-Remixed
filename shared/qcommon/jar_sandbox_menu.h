// Shared SP/MP Sandbox presentation. Engine adapters own input and world actions.
// GPL-2.0-only.
#pragma once
#include <string.h>
#define JAR_MENU_FONT "arialnb"
#define JAR_MENU_TABS 6
#define JAR_MENU_MAP_SECTIONS 5
#define JAR_MENU_CARDS 12
#define JAR_MENU_TOOLS 13
#define JAR_MENU_PANEL_COLOR .025f,.04f,.065f,.94f
#define JAR_MENU_CARD_COLOR .08f,.12f,.18f,1.f
#define JAR_MENU_TEXT_COLOR .88f,.92f,.96f,1.f
#define JAR_MENU_FOCUS_COLOR .35f,.9f,1.f,1.f
#define JAR_MENU_STATUS_COLOR .08f,.08f,.08f,.96f
#define JAR_THRUSTER_DEFAULT 5000.f
#define JAR_THRUSTER_STEP 1000.f
// Keep the original numeric tabs and place Map Sections beside Entities.
static const char* const jarMenuTabs[JAR_MENU_TABS]={"Props","NPCs","Weapons","Entities","Vehicles","Map Sections"};
typedef struct {const char *name,*command,*left,*right;int mp;} jarMenuTool_t;
static const jarMenuTool_t jarMenuTools[JAR_MENU_TOOLS]={
 {"Select","select","Select aimed entity","Select aimed entity",7},
 {"Rotate","rotate","Turn clockwise around hit","Turn counterclockwise around hit",8},
 {"Freeze","freeze","Freeze entity","Release entity",0},
 {"Gravity","release","Release with gravity","Freeze entity",9},
 {"Remove","remove","Remove aimed entity","Undo last spawn or creation",1},
 {"Weld","weld","Choose entity/world anchors","Cancel first anchor",5},
 {"Hinge","hinge","Choose entity/world anchors","Cancel first anchor",6},
 {"Unlink","unlink","Remove attached links","Inspect selection",10},
 {"Duplicate","duplicate","Copy / paste at crosshair","Copy another entity",3},
 {"Rope","rope","Choose two anchor points","Cancel first anchor",4},
 {"Thruster","thruster","Attach / toggle thruster","Remove thruster",2},
 {"Activate","activate","Fire / toggle entity","Inspect entity",11},
 {"Link","link","Choose source then target","Cancel source selection",12}
};
static int JAR_MenuToolIndex(int mp){int i;for(i=0;i<JAR_MENU_TOOLS;++i)if(jarMenuTools[i].mp==mp)return i;return 0;}
typedef enum {JM_PANEL,JM_TITLE,JM_HOLD,JM_TAB,JM_SEARCH,JM_CLEAR,JM_TOOLS,JM_CARD,JM_ICON,JM_PREVIOUS,JM_PAGE,JM_NEXT,JM_TOOL,JM_PROPERTIES,JM_DETAIL,JM_VALUE,JM_LESS,JM_MORE,JM_THRUST_DIRECTION,JM_UNDO,JM_CLEANUP,JM_PHYSICS,JM_COUNT,JM_STATUS} jarMenuPart_t;
typedef struct {float x,y,w,h,scale,tx,ty;int fill;} jarMenuRect_t;
static jarMenuRect_t JAR_MenuRect(jarMenuPart_t part,int i){
 jarMenuRect_t r={0,0,0,22,.85f,0,2,0};
 switch(part){
 case JM_PANEL:r.x=12;r.y=32;r.w=616;r.h=418;r.fill=1;break;
 case JM_TITLE:r.x=26;r.y=44;r.w=300;break;
 case JM_HOLD:r.x=280;r.y=46;r.w=334;r.scale=.58f;break;
 case JM_TAB:{static const float offset[]={0,34,64,111,205,153};static const float width[]={32,28,45,40,43,50};
  r.x=26+offset[i];r.y=78;r.w=width[i];r.scale=i==JAR_MENU_MAP_SECTIONS?.39f:.48f;break;}
 case JM_SEARCH:r.x=26;r.y=108;r.w=202;r.h=20;r.scale=.5f;break;
 case JM_CLEAR:r.x=240;r.y=108;r.w=25;r.h=20;r.scale=.5f;break;
 case JM_TOOLS:r.x=278;r.y=79;r.w=100;r.scale=.7f;break;
 case JM_CARD:r.x=26+80.f*(i%3);r.y=134+57.f*(i/3);r.w=74;r.h=53;r.scale=.43f;r.tx=5;r.ty=39;r.fill=2;break;
 case JM_ICON:r.x=45+80.f*(i%3);r.y=138+57.f*(i/3);r.w=36;r.h=32;break;
 case JM_PREVIOUS:r.x=26;r.y=365;r.w=90;r.h=19;r.scale=.55f;break;
 case JM_PAGE:r.x=107;r.y=365;r.w=100;r.h=19;r.scale=.55f;break;
 case JM_NEXT:r.x=211;r.y=365;r.w=65;r.h=19;r.scale=.55f;break;
 case JM_TOOL:r.x=278;r.y=106+18.f*i;r.w=96;r.h=18;r.scale=.53f;break;
 case JM_PROPERTIES:r.x=390;r.y=108;r.w=220;r.scale=.7f;break;
 case JM_DETAIL:r.x=390;r.y=144+20.f*i;r.w=220;r.scale=.52f;break;
 case JM_VALUE:r.x=390;r.y=254;r.w=220;r.scale=.53f;break;
 case JM_LESS:r.x=390;r.y=279;r.w=45;r.scale=.6f;break;
 case JM_MORE:r.x=446;r.y=279;r.w=45;r.scale=.6f;break;
 case JM_THRUST_DIRECTION:r.x=390;r.y=307;r.w=220;r.scale=.53f;break;
 case JM_UNDO:r.x=278;r.y=383;r.w=90;r.scale=.55f;break;
 case JM_CLEANUP:r.x=390;r.y=383;r.w=120;r.scale=.55f;break;
 case JM_PHYSICS:r.x=516;r.y=383;r.w=98;r.scale=.53f;break;
 case JM_COUNT:r.x=26;r.y=407;r.w=580;r.scale=.53f;break;
 case JM_STATUS:r.x=8;r.y=8;r.w=624;r.scale=.5f;r.tx=8;r.ty=4;r.fill=3;break;
 }
 return r;
}
static int JAR_MenuHit(jarMenuPart_t part,int i,float x,float y){jarMenuRect_t r=JAR_MenuRect(part,i);return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
static int JAR_MenuFold(int c){return c>='A'&&c<='Z'?c+'a'-'A':c;}
static int JAR_MenuMatches(const char*name,const char*query){
 const char *p,*a,*b;if(!*query)return 1;
 for(p=name;*p;++p){for(a=p,b=query;*a&&*b&&JAR_MenuFold((unsigned char)*a)==JAR_MenuFold((unsigned char)*b);++a,++b){}if(!*b)return 1;}return 0;
}
static void JAR_MenuLabel(const char*name,char*out,int size){
 const char *end=name+strlen(name),*start=strrchr(name,'/');int n;
 start=start?start+1:name;
 if(start>name&&!strcmp(start,"model.glm")){end=start-1;start=end;while(start>name&&start[-1]!='/')--start;}
 n=(int)(end-start);if(n>size-1)n=size-1;if(n<0)n=0;
 memcpy(out,start,n);out[n]=0;if(n>16&&size>=17){out[13]='.';out[14]='.';out[15]='.';out[16]=0;}
}
