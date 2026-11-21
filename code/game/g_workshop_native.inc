#include "g_door_camera.h"
#include "g_workshop_links.inc"
void releaseHeld(bool freeze){
 clearHoldToken();if(held==noEntity)return;const auto entity=held;held=noEntity;
 if(entity>=ENTITYNUM_WORLD||!g_entities[entity].inuse)return;
 if(auto*o=owned(entity)){if(prop(o->catalog)){o->frozen=freeze?1:0;physicsWorld.Release(entity);
   if(freeze)physicsInvalidate(entity);return;}}
 if(freeze)nativeBegin(g_entities[entity],true);else nativeRelease(entity,true);
}
#include "g_workshop_duplicate.inc"
void toolAlternate(const trace_t& shot){
 if(toolMode==12){linkFirst=0;message("Link source cleared.");return;}
 if(toolMode==4){undoEdit();return;}
 if(toolMode==8){if(selectToolShot(shot))duplicateTool(g_entities[selected]);return;}
 vec3_t contact;
 if(!selectToolShot(shot,contact))return;
 if(toolMode==1){tool("rotate_back",contact);return;}
 if(toolMode==2)tool("release");else if(toolMode==3)tool("freeze");
 else if(toolMode==7)message("Unlink: left removes links from the aimed prop.");
 else message("Entity selected; choose a tool in Q.");
}
void impactSound(unsigned entity,float speed){
 static int lastImpact[MAX_GENTITIES]{},frame=-1,played=0;
 if(frame!=level.time){frame=level.time;played=0;}
 if(entity>=ENTITYNUM_WORLD||speed<48||played>=8)return;
 auto&last=lastImpact[entity];if(last>0&&last<=level.time&&level.time-last<150)return;
 auto&e=g_entities[entity];if(!e.inuse)return;last=level.time;++played;
 G_Sound(&e,G_SoundIndex(speed>180?"sound/effects/metal_bounce2.mp3":"sound/effects/metal_bounce.mp3"));
}
#include "g_workshop_actorphysics.inc"
void physicsImpactSounds(){for(const auto&hit:physicsWorld.Impacts())impactSound(hit.prop,hit.speed);}
