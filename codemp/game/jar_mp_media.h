// Stock picker defaults and bounded virtual-filesystem paths. GPL-2.0-only.
#pragma once
#include <string.h>
static const char*jarSpeakerChoices[]={"sound/weapons/bryar/fire.mp3","sound/movers/switches/switch1.mp3","sound/movers/switches/button_08.mp3","sound/interface/secret_area.mp3","sound/interface/shieldcon_run.wav"};
static const char*jarEffectChoices[]={"sparks/spark","sparks/bluesparks","sparks/spark_nosnd","env/fire","env/impact_dust"};
static int JAR_MediaPath(int kind,const char*path){
 size_t n=strlen(path);if(!n||n>63||path[0]=='/'||path[n-1]=='/'||strstr(path,"..")||strstr(path,"//"))return 0;
 for(const char*p=path;*p;++p)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='/'||*p=='_'||*p=='-'||*p=='.'))return 0;
 if(kind==9)return n>10&&!strncmp(path,"sound/",6)&&(!strcmp(path+n-4,".wav")||!strcmp(path+n-4,".mp3"));
 return kind==10&&!strchr(path,'.')&&strncmp(path,"effects/",8);
}
static const char*JAR_MediaCycle(int kind,const char*current,int direction){
 const char**choices=kind==9?jarSpeakerChoices:jarEffectChoices;int count=5,index=0;
 for(int i=0;i<count;++i)if(!strcmp(current,choices[i])){index=i;break;}
 return choices[(index+count+(direction<0?-1:1))%count];
}
