// Server request policy; also exercised without the game runtime. GPL-2.0-only.
#pragma once
#include <limits.h>
#include <string.h>
static inline int JAR_MP_IsCommand(const char *command){
 static const char *names[]={"jar_spawn","jar_equip","jar_tool","jar_setting","jar_undo","jar_cleanup","jar_distance","jar_select","jar_property","jar_release","jar_allowTools"};
 for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i)if(!strcmp(command,names[i]))return 1;
 return 0;
}
static inline int JAR_MP_ParseId(const char *text,unsigned *value){
 unsigned n=0;if(!text||!*text)return 0;
 for(;*text;++text){unsigned digit=(unsigned)(*text-'0');if(digit>9||n>(UINT_MAX-digit)/10)return 0;n=n*10+digit;}
 *value=n;return 1;
}
static inline int JAR_MP_CanEdit(int client,int owner,int shared){return owner>0&&(owner==client+1||shared);}
static inline int JAR_MP_RequestReady(int now,int *next){if(now<*next)return 0;*next=now+100;return 1;}
