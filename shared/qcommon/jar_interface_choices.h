// Shared SP/MP appearance choices. GPL-2.0-only.
#ifndef JAR_INTERFACE_CHOICES_H
#define JAR_INTERFACE_CHOICES_H
#include <string.h>
#define JAR_HUD_CHOICES {{"Academy","ui/jahud.txt"},{"Simple numeric","1"}}
#define JAR_UI_MOTION_CHOICES {{"Reduced","0"},{"Normal","1"}}
#define JAR_FONT_PACK_CHOICES {{"Retail","stock"},{"Crisp Sans","sans"},{"Crisp Serif","serif"}}
#define JAR_UI_PACK_CHOICES {{"Retail","stock"},{"2x smooth","up2"},{"4x sharpened","up4"}}
#define JAR_FONT_DETAIL_CHOICES {{"Normal","1"},{"High","2"},{"Maximum","4"}}
static int JAR_BuiltinInterfacePack(const char *category,const char *pack) {
 if(!category||!pack)return 0;
 if(!strcmp(category,"font"))return !strcmp(pack,"stock")||!strcmp(pack,"sans")||!strcmp(pack,"serif");
 if(!strcmp(category,"ui"))return !strcmp(pack,"stock")||!strcmp(pack,"up2")||!strcmp(pack,"up4");
 return 0;
}
#endif
