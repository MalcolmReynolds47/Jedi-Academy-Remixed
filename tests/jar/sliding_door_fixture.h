// Run the production door implementation with native sound and link spies.
#include "game/g_button_sandbox.h"
static int doorSounds=0;
static int secretSounds=0;
static const char* doorOutput=nullptr;
int G_ModelIndex(const char* model){
 if(!strcmp(model,"models/map_objects/mp/shield.md3"))return 25;
 if(!strcmp(model,"charge_fixture.glm"))return 24;
 if(!strcmp(model,"models/map_objects/nar_shaddar/cup.md3"))return 22;
 if(!strcmp(model,"models/items/medpac.md3"))return 23;
 if(!strcmp(model,"models/map_objects/kejim/impcam_base.md3"))return 26;
 if(!strcmp(model,"models/map_objects/kejim/impcam.md3"))return 27;
 if(!strcmp(model,"models/weapons2/laser_trap/laser_trap.md3"))return 28;
 CHECK(!strcmp(model,JAR_SLIDING_DOOR_MODEL)||!strcmp(model,JAR_BUTTON_MODEL)||!strcmp(model,JAR_LIFT_MODEL)||!strcmp(model,"models/map_objects/imperial/crate1.md3"));return 21;
}
int G_SoundIndex(const char* sound){
 if(!strcmp(sound,"sound/movers/doors/forcefield_lp.wav"))return 40;
 if(!strcmp(sound,"sound/movers/doors/forcefield_on.wav"))return 41;
 if(!strcmp(sound,"sound/movers/doors/forcefield_off.wav"))return 42;
 if(!strcmp(sound,"sound/weapons/laser_trap/hum_loop.wav"))return 39;
 if(!strcmp(sound,"sound/weapons/laser_trap/fire.mp3"))return 43;
 if(!strcmp(sound,"sound/weapons/laser_trap/stick.mp3"))return 44;
 if(!strcmp(sound,JAR_DOOR_START_SOUND))return 31;
 if(!strcmp(sound,JAR_DOOR_MOVE_SOUND))return 32;
 if(!strcmp(sound,JAR_BUTTON_PRESS_SOUND))return 34;
 if(!strcmp(sound,JAR_BUTTON_RETURN_SOUND))return 35;
 if(!strcmp(sound,JAR_LIFT_START_SOUND))return 36;
 if(!strcmp(sound,JAR_LIFT_MOVE_SOUND))return 37;
 if(!strcmp(sound,JAR_LIFT_STOP_SOUND))return 38;
 CHECK(!strcmp(sound,JAR_DOOR_STOP_SOUND));return 33;
}
void G_Sound(gentity_t*,int sound){
 if(sound==41||sound==42)return;
 if(sound==81){++secretSounds;return;}
 CHECK(sound==31||sound==33||sound==34||sound==35||sound==36||sound==38);++doorSounds;
}
void G_UseTargets2(gentity_t*,gentity_t*,const char* output){doorOutput=output;}
bool G_JAR_WorkshopControls(const gentity_t* e){return nativeControls.count(e->s.number)!=0;}
#include "game/g_sliding_door.inc"
#include "game/g_button_sandbox.inc"
#include "game/g_bobbing_sandbox.inc"
