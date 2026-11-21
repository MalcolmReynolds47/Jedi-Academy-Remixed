// Retail color choices omitted from the native vehicle definitions. GPL-2.0-only.
// Include after q_shared.h. Uses the caller's native COM parse session.
#ifndef JAR_VEHICLE_VARIANTS_H
#define JAR_VEHICLE_VARIANTS_H
typedef struct {const char *name,*skin;} jarVehicleVariant_t;
static const jarVehicleVariant_t jarVehicleVariants[]={
 {"swoop_black","black"},{"swoop_blue","blue"},{"swoop_gold","gold"},
 {"swoop_green","green"},{"swoop_purple","purple"},{"swoop_red","default"},
 {"swoop_silver","silver"}
};
static const jarVehicleVariant_t *JAR_VehicleVariant(const char *name){
 unsigned i;if(name)for(i=0;i<sizeof(jarVehicleVariants)/sizeof(jarVehicleVariants[0]);++i)
  if(!Q_stricmp(name,jarVehicleVariants[i].name))return &jarVehicleVariants[i];
 return NULL;
}
static const char *JAR_VehicleBlock(const char *data,const char *name){
 const char *p=data,*t;int depth;
 if(!name||!name[0])return NULL;
 while(p){t=COM_ParseExt(&p,qtrue);if(!t[0])break;
  if(!Q_stricmp(t,name))return p; // An explicit malformed block must fail, not use the fallback.
  t=COM_ParseExt(&p,qtrue);if(strcmp(t,"{"))return NULL;
  depth=1;while(p&&depth){t=COM_ParseExt(&p,qtrue);if(!t[0])return NULL;if(!strcmp(t,"{"))++depth;else if(!strcmp(t,"}"))--depth;}
 }
 return NULL;
}
static unsigned JAR_VehicleBlockHash(const char *p){
 const char *t;const unsigned char *c;unsigned h=2166136261u;
 if(!p||strcmp(COM_ParseExt(&p,qtrue),"{"))return 0;
 while(p){t=COM_ParseExt(&p,qtrue);if(!t[0]||!strcmp(t,"{"))return 0;if(!strcmp(t,"}"))return h;
  for(c=(const unsigned char*)t;*c;++c)h=(h^*c)*16777619u;h*=16777619u;
 }
 return 0;
}
static qboolean JAR_StockSwoopHash(unsigned hash){return hash==710687386u||hash==140738443u?qtrue:qfalse;}
static const char *JAR_ResolveVehicleBlock(const char *data,const char *name,const char **skin){
 const char *p;const jarVehicleVariant_t *variant;*skin=NULL;
 p=JAR_VehicleBlock(data,name);if(p)return p;
 variant=JAR_VehicleVariant(name);if(!variant)return NULL;
 p=JAR_VehicleBlock(data,"swoop");if(!JAR_StockSwoopHash(JAR_VehicleBlockHash(p)))return NULL;
 *skin=variant->skin;return p;
}
#endif
