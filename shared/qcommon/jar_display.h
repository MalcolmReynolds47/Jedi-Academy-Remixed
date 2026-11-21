// Shared display limits, choices and pending-value rules. GPL-2.0-only.
#ifndef JAR_DISPLAY_H
#define JAR_DISPLAY_H
#include <string.h>
#include <stdlib.h>
#define JAR_DISPLAY_NAMES {"r_mode","r_customwidth","r_customheight","r_fullscreen","r_noborder"}
#define JAR_RESOLUTION_CHOICES {{"Desktop","-2"},{"Custom","-1"},{"800 x 600","4"},{"1024 x 768","6"},{"1600 x 1200","9"}}
#define JAR_WINDOW_CHOICES {{"Windowed","0"},{"Exclusive fullscreen","1"},{"Borderless fullscreen","2"}}
#define JAR_BORDER_CHOICES {{"Normal","0"},{"No border","1"}}
static int JAR_DecimalInRange(const char *text,int minimum,int maximum){
 if(!text||!*text)return 0;unsigned value=0;
 for(const char*p=text;*p;++p){if(*p<'0'||*p>'9'||value>(unsigned)maximum/10)return 0;value=value*10+(*p-'0');if(value>(unsigned)maximum)return 0;}
 return value>=(unsigned)minimum;
}
static int JAR_ResolutionValid(int width,int height){return width>=320&&height>=200&&width<=16384&&height<=16384&&(long long)width*height<=67108864;}
static int JAR_DisplayValidate(const char*const values[5],const char*const original[5],const char**mode){
 int changed=0,dimensions=0;*mode=values[0];
 for(int n=0;n<5;++n){if(!strcmp(values[n],original[n]))continue;changed=1;
  if(n==0){if(strcmp(values[n],"-2")&&strcmp(values[n],"-1")&&strcmp(values[n],"4")&&strcmp(values[n],"6")&&strcmp(values[n],"9"))return 0;}
  else if(n==1||n==2){if(!JAR_DecimalInRange(values[n],n==1?320:200,16384))return 0;dimensions=1;}
  else if(strcmp(values[n],"0")&&strcmp(values[n],"1")&&(n!=3||strcmp(values[n],"2")))return 0;
 }
 if(!changed)return 1;
 if(!strcmp(values[3],"2")){*mode="-2";return 1;}
 if(dimensions||!strcmp(values[0],"-1")){
  if(!JAR_DecimalInRange(values[1],320,16384)||!JAR_DecimalInRange(values[2],200,16384)||!JAR_ResolutionValid(atoi(values[1]),atoi(values[2])))return 0;
 }
 if(dimensions)*mode="-1";return 1;
}
#endif
