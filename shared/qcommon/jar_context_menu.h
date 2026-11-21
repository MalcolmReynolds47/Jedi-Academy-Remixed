// SP and MP context panel geometry and action labels. GPL-2.0-only.
#pragma once
typedef enum {JC_PANEL,JC_WORLD,JC_TAB,JC_NAME,JC_INFO,JC_SELF,JC_REFRESH,JC_FIELD,JC_ACTION,JC_UNDO,JC_CLOSE} jarContextPart_t;
typedef struct {float x,y,w,h;} jarContextRect_t;
static const char*const jarContextActions[]={"Freeze","Release","Activate","Remove","Copy","Paste","Rope","Thruster","Weld","Hinge","Unlink"};
static jarContextRect_t JAR_ContextRect(jarContextPart_t part,int i){
 jarContextRect_t r={402,148,220,20};
 switch(part){
 case JC_PANEL:r.x=390;r.y=40;r.w=242;r.h=430;break;
 case JC_WORLD:r.x=0;r.y=34;r.w=382;r.h=446;break;
 case JC_TAB:r.x=402+74.f*i;r.y=50;r.w=i==2?68:72;break;
 case JC_NAME:r.y=76;r.w=216;break;case JC_INFO:r.y=96;r.w=216;break;
 case JC_SELF:r.y=118;r.w=102;break;case JC_REFRESH:r.x=514;r.y=118;r.w=100;break;
 case JC_FIELD:r.y=148+20.f*i;break;
 case JC_ACTION:r.x=402+55.f*(i%4);r.y=374+24.f*(i/4);r.w=53;break;
 case JC_UNDO:r.y=446;r.w=104;break;case JC_CLOSE:r.x=514;r.y=446;r.w=104;break;
 }return r;
}
static int JAR_ContextHit(jarContextPart_t part,int i,float x,float y){jarContextRect_t r=JAR_ContextRect(part,i);return x>=r.x&&x<r.x+r.w&&y>=r.y&&y<r.y+r.h;}
