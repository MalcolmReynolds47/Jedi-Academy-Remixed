// Server-confirmed selection details and typed property input. GPL-2.0-only.
static const char*contextKinds[]={"No selection","Prop","NPC","Vehicle","Player","Pickup","Entity"};
static unsigned contextIdentity,contextCatalog;
static int contextKind,contextCaps,contextFrozen,contextGravity,contextHealth,contextArmor,contextQuantity;
static int contextActorGravity,contextActorFriction;
static float contextMass;
static char contextInput[JAR_MUSIC_CUE_SIZE];
static int contextPage,contextControllerKind;static jarControllerState_t contextController;
#include "../../shared/qcommon/jar_context_menu.h"
static qboolean ControllerInputKey(int key);
static void MenuClose(void);
static void MenuStatus(const char*message);
static void ContextProperty(const char*action);
#include "cg_jar_transform.inc"
static void ContextClear(void){contextIdentity=contextCatalog=0;contextKind=contextCaps=contextEditing=contextTransformPage=contextPage=contextControllerKind=0;}
static int ContextField(void){return contextCaps&JAR_CONTEXT_MASS?JAR_PROPERTY_MASS:contextCaps&JAR_CONTEXT_HEALTH?JAR_PROPERTY_HEALTH:contextCaps&JAR_CONTEXT_QUANTITY?JAR_PROPERTY_QUANTITY:0;}
void CG_JAR_Selection(void){
 int entity,frame,frames,kind,caps,frozen,gravity,health,quantity;unsigned identity,catalog;float mass;
 if(!SelectionNumber(CG_Argv(1),-1,ENTITYNUM_WORLD-1,&entity)||!SelectionNumber(CG_Argv(2),0,65535,&frame)||
  !SelectionNumber(CG_Argv(3),0,65536,&frames)||(frames&&frame>=frames)||!JAR_MP_ParseId(CG_Argv(4),&identity)||
  !SelectionNumber(CG_Argv(5),0,JAR_CONTEXT_OTHER,&kind)||!JAR_MP_ParseId(CG_Argv(6),&catalog)||
  !SelectionNumber(CG_Argv(7),0,8191,&caps)||!SelectionNumber(CG_Argv(8),0,1,&frozen)||!SelectionNumber(CG_Argv(9),0,1,&gravity)||
  !JAR_ParseInt(CG_Argv(11),&health)||!JAR_ParseInt(CG_Argv(12),&quantity))return;
 if(!JAR_ParseFloat(CG_Argv(10),&mass)||mass<0)return;
 float transform[6];for(int i=0;i<6;++i)if(!JAR_ParseFloat(CG_Argv(13+i),&transform[i]))return;
 int actorGravity=-1,actorFriction=-1,armor=0;
 // Older servers end after the transform. Preserve their selection reply.
 if((kind==JAR_CONTEXT_PLAYER||kind==JAR_CONTEXT_NPC)&&
    SelectionNumber(CG_Argv(19),-1,JAR_MP_ACTOR_MOVEMENT_MAX,&actorGravity)&&
    SelectionNumber(CG_Argv(20),-1,JAR_MP_ACTOR_MOVEMENT_MAX,&actorFriction))caps|=JAR_CONTEXT_MOVEMENT;
 if(kind==JAR_CONTEXT_PLAYER&&SelectionNumber(CG_Argv(21),0,SHRT_MAX,&armor))caps|=JAR_CONTEXT_ARMOR;
 if((entity>=0&&(!identity||!kind))||(entity<0&&(identity||kind||caps)))return;
 if(selectedEntity!=entity||contextIdentity!=identity){poseEditing=qfalse;contextEditing=contextTransformPage=contextPage=contextControllerKind=0;}
 selectedEntity=entity;poseFrame=frame;poseFrames=frames;contextIdentity=identity;contextCatalog=catalog;
 contextKind=kind;contextCaps=caps;contextFrozen=frozen;contextGravity=gravity;contextMass=mass;contextHealth=health;contextArmor=armor;contextQuantity=quantity;
 contextActorGravity=actorGravity;contextActorFriction=actorFriction;
 memcpy(contextTransform,transform,sizeof(transform));
 if(!contextEditing&&!contextTransformPage)ContextTransformReload();
 if(!(caps&JAR_CONTEXT_TRANSFORM)&&contextEditing>=100&&contextEditing<106)contextEditing=0;
 if(!(caps&JAR_CONTEXT_CONTROLLER))contextControllerKind=0;
 if(!(caps&(JAR_CONTEXT_CONTROLLER|JAR_CONTEXT_MOVEMENT))&&contextEditing>=200)contextEditing=0;
 if(contextEditing&&contextEditing<100&&contextEditing!=ContextField()&&
    !(contextEditing==JAR_PROPERTY_ARMOR&&(caps&JAR_CONTEXT_ARMOR)))contextEditing=0;
}
static void ContextProperty(const char*action){if(selectedEntity>=0&&contextIdentity){char command[192];Com_sprintf(command,sizeof(command),"jar_property %s %u",action,contextIdentity);trap->SendClientCommand(command);}}
static qboolean ContextInputKey(int key){
 if(contextEditing>=200)return ControllerInputKey(key);
 if(!contextEditing)return qfalse;
 if(key&K_CHAR_FLAG){
  JAR_NumberAppend(contextInput,sizeof(contextInput),key&~K_CHAR_FLAG,contextEditing==JAR_PROPERTY_MASS||contextEditing>=100);
  return qtrue;
 }
 if(key==A_BACKSPACE){size_t len=strlen(contextInput);if(len)contextInput[len-1]=0;return qtrue;}
 if(key==A_ENTER||key==A_KP_ENTER){
  if(contextEditing>=100){ContextTransformCommit();return qtrue;}
  char action[48];
  if(contextEditing==JAR_PROPERTY_MASS){
   float value;
   if(!JAR_ParseFloat(contextInput,&value)){MenuStatus("Enter a valid number for mass.");return qtrue;}
   Com_sprintf(action,sizeof(action),"mass_%.9g",value);
  }else{
   int value;
   if(!JAR_ParseInt(contextInput,&value)){MenuStatus("Enter a valid whole number.");return qtrue;}
   const char*field=contextEditing==JAR_PROPERTY_ARMOR?"armor":contextEditing==JAR_PROPERTY_HEALTH?"health":"quantity";
   Com_sprintf(action,sizeof(action),"%s_%d",field,value);
  }
  ContextProperty(action);contextEditing=0;return qtrue;
 }
 return qfalse;
}
#include "cg_jar_context_panel.inc"
