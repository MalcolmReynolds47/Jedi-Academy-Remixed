#include "cgame/cg_jar_rack_item.inc"

static int rackDrawCalls;
static refEntity_t rackDrawn;
static void RackItemDrawCapture(const refEntity_t *ent){++rackDrawCalls;rackDrawn=*ent;}

static void RackItemDrawCheck(const centity_t *cent,const gitem_t *item,const itemInfo_t *media,int model){
 const int calls=rackDrawCalls;
 CG_JAR_DrawRackItem(cent,item,media);
 assert(rackDrawCalls==calls+1&&rackDrawn.reType==RT_MODEL&&rackDrawn.hModel==model);
 assert(rackDrawn.ghoul2==media->g2Models[0]&&rackDrawn.radius==media->radius[0]);
 assert(VectorCompare(rackDrawn.origin,cent->lerpOrigin)&&VectorCompare(rackDrawn.oldorigin,cent->lerpOrigin));
 vec3_t expected[3];AnglesToAxis(cent->lerpAngles,expected);
 assert(VectorCompare(rackDrawn.angles,cent->lerpAngles)&&!memcmp(expected,rackDrawn.axis,sizeof(expected)));
 assert(!rackDrawn.renderfx&&!rackDrawn.nonNormalizedAxes&&!rackDrawn.customShader&&rackDrawn.shaderRGBA[3]==255);
 assert(VectorCompare(rackDrawn.modelScale,(vec3_t){1,1,1}));
}

static void RackItemPresentationCases(void){
 void (*draw)(const refEntity_t*)=trap->R_AddRefEntityToScene;
 trap->R_AddRefEntityToScene=RackItemDrawCapture;
 static int ghoul;
 itemInfo_t media={0};media.models[0]=42;media.g2Models[0]=&ghoul;media.radius[0]=60;
 centity_t cent={0};entityState_t *state=&cent.currentState;
 state->eType=ET_ITEM;state->generic1=JAR_MP_RACK_ITEM_TAG;state->modelindex2=1;state->otherEntityNum=70;
 VectorSet(state->angles,0,30,0);VectorSet(state->pos.trBase,100,50,42);
 VectorSet(cent.lerpAngles,-74,118,4);VectorSet(cent.lerpOrigin,102,53,43);
 const int weapons[]={WP_BLASTER,WP_REPEATER,WP_ROCKET_LAUNCHER};
 gitem_t item={0};item.giType=IT_WEAPON;
 for(int i=0;i<ARRAY_LEN(weapons);++i){
  item.giTag=weapons[i];assert(CG_JAR_IsRackItem(state,&item));
  RackItemDrawCheck(&cent,&item,&media,42);
 }
 item.giTag=WP_SABER;assert(!CG_JAR_IsRackItem(state,&item));
 media.g2Models[0]=NULL;media.radius[0]=0;
 VectorSet(cent.lerpAngles,12,125,-9);VectorSet(cent.lerpOrigin,109,68,26);
 const int ammo[]={AMMO_BLASTER,AMMO_METAL_BOLTS,AMMO_POWERCELL};item.giType=IT_AMMO;
 for(int i=0;i<ARRAY_LEN(ammo);++i){
  item.giTag=ammo[i];assert(CG_JAR_IsRackItem(state,&item));
  RackItemDrawCheck(&cent,&item,&media,42);
 }
 item.giTag=AMMO_ROCKETS;assert(!CG_JAR_IsRackItem(state,&item));
 item.giTag=AMMO_BLASTER;
 state->eType=ET_GENERAL;assert(!CG_JAR_IsRackItem(state,&item));state->eType=ET_ITEM;
 state->modelindex2=0;assert(!CG_JAR_IsRackItem(state,&item));state->modelindex2=1;
 state->otherEntityNum=ENTITYNUM_WORLD;assert(!CG_JAR_IsRackItem(state,&item));state->otherEntityNum=70;
 state->generic1=0;assert(!CG_JAR_IsRackItem(state,&item));state->generic1=JAR_MP_RACK_ITEM_TAG;

 item.giType=IT_HEALTH;item.classname="item_medpak";
 assert(!CG_JAR_IsRackItem(state,&item));
 item.classname="item_medpak_instant";
 const int medpack=51;state->modelindex2=medpack;
 cgs.gameModels[medpack]=77;
 assert(CG_JAR_IsRackItem(state,&item));RackItemDrawCheck(&cent,&item,&media,77);
 cgs.gameModels[medpack]=0;RackItemDrawCheck(&cent,&item,&media,42);
 const int invalid[]={-1,0,MAX_MODELS};
 for(int i=0;i<ARRAY_LEN(invalid);++i){
  state->modelindex2=invalid[i];RackItemDrawCheck(&cent,&item,&media,42);
 }
 state->generic1=JAR_MP_CARGO_DROP_TAG;state->otherEntityNum=ENTITYNUM_NONE;
 assert(!CG_JAR_IsRackItem(state,&item)); // Force Pull switches to the separate stock medpack path.
 trap->R_AddRefEntityToScene=draw;
 puts("MP rack item presentation: stock guns and ammo, medpack override/fallback, unit-scale shelf poses and detached pickup isolation passed.");
}
