// Shared SP/MP cosmetic pack choices. GPL-2.0-only.
#ifndef JAR_EFFECT_CHOICES_H
#define JAR_EFFECT_CHOICES_H
#include <string.h>
#define JAR_FACE_PRESET_CHOICES {{"Stock","0"},{"Expressive","1"},{"Exaggerated","2"},{"Extreme","3"}}
#define JAR_FACE_SENSITIVITY_CHOICES {{"Subtle","0.5"},{"Normal","1"},{"Sensitive","1.5"},{"Very sensitive","2"}}
#define JAR_SABER_PACK_CHOICES {{"Retail","stock"},{"Cinematic narrow","cinematic"},{"Soft glow","soft"},{"Installed Saber GFX","saber_gfx"}}
#define JAR_SOUND_PACK_CHOICES {{"Retail","stock"},{"MBII","mbii"},{"MBII Original Trilogy","ot"},{"MBII Battlefront","bf"},{"MBII Force Unleashed","tfu"},{"MBII Plasma","plasma"}}
#define JAR_FX_PACK_CHOICES {{"Retail","stock"},{"MBII combat","mbii"}}
static int JAR_BuiltinEffectPack(const char*category,const char*pack){
 if(!category||!pack)return 0;
 if(strcmp(category,"saber")&&strcmp(category,"sound")&&strcmp(category,"fx"))return 0;
 if(!strcmp(pack,"stock"))return 1;
 if(!strcmp(category,"saber"))return !strcmp(pack,"cinematic")||!strcmp(pack,"soft")||!strcmp(pack,"saber_gfx");
 if(!strcmp(category,"sound"))return !strcmp(pack,"mbii")||!strcmp(pack,"ot")||!strcmp(pack,"bf")||!strcmp(pack,"tfu")||!strcmp(pack,"plasma");
 return !strcmp(pack,"mbii");
}
#endif
