// Production MP overlay entrypoint with a controlled native file reader.
#include "../../codemp/qcommon/q_shared.h"
#include <map>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
static std::map<std::string,std::string> packs;
static std::map<std::string,long> files;
static std::vector<std::string> reads;
static qboolean expectedUnique=qfalse;
static cvar_t* Cvar_Get(const char*name,const char*initial,int flags,const char*) {
 assert(flags==CVAR_ARCHIVE);auto item=packs.emplace(name,initial);
 static cvar_t value;value.string=const_cast<char*>(item.first->second.c_str());return &value;
}
#include "../../codemp/qcommon/jar_appearance_read.inc"
static long FS_FOpenFileReadOriginal(const char*name,fileHandle_t*f,qboolean unique) {
 assert(unique==expectedUnique);reads.emplace_back(name?name:"<null>");
 if(!name||!f)return -1;
 const auto found=files.find(name);*f=found==files.end()?0:7;
 return found==files.end()?-1:found->second;
}
static long FS_ReadFile(const char*name,void**buffer){assert(!buffer);auto f=files.find(name);return f==files.end()?-1:f->second;}
#include "../../codemp/client/snd_jar_overlay.inc"
static long Read(const char*name){reads.clear();fileHandle_t f=0;long n=FS_FOpenFileRead(name,&f,expectedUnique);assert((n>=0)==(f==7));return n;}
int main(){
 files["fonts/arialnb.fontdat"]=8204;files["jar/font/sans/fonts/arialnb.fontdat"]=8204;
 assert(Read("fonts/arialnb.fontdat")==8204&&reads.size()==1&&reads[0]=="jar/font/sans/fonts/arialnb.fontdat");assert(packs["jar_fontPack"]=="sans");
 files["jar/ui/up2/gfx/menus/icon.tga"]=64;
 assert(Read("GFX\\Menus\\Icon.TGA")==64&&reads.size()==1&&reads[0]=="jar/ui/up2/gfx/menus/icon.tga");assert(packs["jar_uiPack"]=="up2");
 expectedUnique=qtrue;assert(Read("fonts/arialnb.fontdat")==8204);expectedUnique=qfalse;
 // The real reader can reject an overlay (missing file or pure policy).
 // The adapter must then call it again on the original, preserving spelling.
 files.erase("jar/font/sans/fonts/arialnb.fontdat");assert(Read("fonts/arialnb.fontdat")==8204&&reads.size()==2&&reads[1]=="fonts/arialnb.fontdat");
 files["Fonts/ARIALNB.fontdat"]=8204;assert(Read("Fonts/ARIALNB.fontdat")==8204&&reads[1]=="Fonts/ARIALNB.fontdat");
 for(const char*pack:{"stock","../base",";quit","","UP2"}){packs["jar_fontPack"]=pack;assert(Read("fonts/arialnb.fontdat")==8204&&reads.size()==1);}
 // Namespaced custom packs are supported at the console, without touching
 // unrelated file classes or recursively applying an overlay to an overlay.
 packs["jar_fontPack"]="custom";files["jar/font/custom/fonts/arialnb.fontdat"]=0;assert(Read("fonts/arialnb.fontdat")==0&&reads.size()==1);
 for(const char*name:{"maps/mp/ffa3.bsp","models/players/kyle/model.glm","autoexec.cfg","saves/world.jrw","vm/cgame.qvm","scripts/a.shader","sound/weapons/saber/on.wav","effects/saber/a.efx","gfx/effects/sabers/blade.tga","gfx/2d/dlight.tga","jar/font/custom/fonts/arialnb.fontdat"}){Read(name);assert(reads.size()==1&&reads[0]==name);}
 // All three cosmetic categories follow the same native reader and pure-pak
 // failure path as fonts/UI. Changing packs never redirects model/map/code.
 struct Case{const char*category,*cvar,*pack,*source;};
 for(const auto&c:{Case{"saber","jar_saberPack","soft","gfx/effects/sabers/blue_line.jpg"},Case{"sound","jar_soundPack","ot","sound/weapons/saber/saberon.wav"},Case{"fx","jar_fxPack","mbii","effects/blaster/shot.efx"}}){
  packs[c.cvar]=c.pack;std::string key=std::string("jar/")+c.category+"/"+c.pack+"/"+c.source;files[key]=128;files[c.source]=64;
  assert(Read(c.source)==128&&reads.size()==1&&reads[0]==key);files.erase(key);assert(Read(c.source)==64&&reads.size()==2&&reads[1]==c.source);
  for(const char*bad:{"stock","../base",";quit","UPPER"}){packs[c.cvar]=bad;assert(Read(c.source)==64&&reads.size()==1);}
  packs[c.cvar]="stock";
 }
 // Decoder dispatch uses the selected sample extension and new length, while
 // absent/empty samples fall back to the original sound and encoding.
 packs["jar_soundPack"]="mbii";char loadName[MAX_QPATH];
 auto select=[&](const char*name,const char*expected){Q_strncpyz(loadName,name,sizeof(loadName));int len=(int)strlen(loadName);char*ext=loadName+len-4;S_JAR_SelectSound(loadName,sizeof(loadName),len,ext);assert(!strcmp(loadName,expected)&&len==(int)strlen(expected)&&ext==loadName+len-4);};
 files["jar/sound/mbii/sound/weapons/saber/saberon.mp3"]=300;
 select("sound/weapons/saber/saberon.wav","jar/sound/mbii/sound/weapons/saber/saberon.mp3");
 files["jar/sound/mbii/sound/weapons/saber/saberon.wav"]=400;
 select("sound/weapons/saber/saberon.wav","jar/sound/mbii/sound/weapons/saber/saberon.wav");
 files.erase("jar/sound/mbii/sound/weapons/saber/saberon.mp3");
 select("sound/weapons/saber/saberon.mp3","jar/sound/mbii/sound/weapons/saber/saberon.wav");
 files["jar/sound/mbii/sound/weapons/saber/saberon.wav"]=0;select("sound/weapons/saber/saberon.wav","sound/weapons/saber/saberon.wav");
 select("sound/chars/kyle/line.wav","sound/chars/kyle/line.wav");select("jar/sound/mbii/sound/weapons/saber/saberon.wav","jar/sound/mbii/sound/weapons/saber/saberon.wav");
 files["jar/sound/mbii/sound/weapons/saber/saberon.wav"]=400;packs["jar_soundPack"]="stock";select("sound/weapons/saber/saberon.wav","sound/weapons/saber/saberon.wav");
 puts("MP cosmetic overlays: saber/sound/FX native lookup and fallback; MP3/WAV selection, source identity, decoder extension, empty samples and retail fallback passed.");
 assert(Read("fonts/missing.fontdat")==-1&&reads.size()==2);
 std::string longName="fonts/"+std::string(MAX_QPATH,'a');Read(longName.c_str());assert(reads.size()==1);
 reads.clear();assert(FS_FOpenFileRead(nullptr,nullptr,qfalse)==-1&&reads.size()==1);
 puts("MP appearance: native overlay/fallback calls, normalization, archive defaults, stock/custom packs, handle modes and excluded file classes passed.");
}
