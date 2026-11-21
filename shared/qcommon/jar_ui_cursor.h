// Shared menu pointer geometry; retain square physical pixels on wide displays.
#pragma once
#include <float.h>
typedef struct {float x,y,w,h;} jarCursorBox_t;
static inline jarCursorBox_t JAR_MenuCursor(float x,float y,float sx,float sy){
 if(!(sx>0&&sx<=FLT_MAX))sx=1;if(!(sy>0&&sy<=FLT_MAX))sy=1;
 float pixels=28*sy;if(pixels<28)pixels=28;if(pixels>64)pixels=64;
 float w=pixels/sx,h=pixels/sy;jarCursorBox_t box={x-w*(12.f/256),y-h*(12.f/256),w,h};return box;
}
