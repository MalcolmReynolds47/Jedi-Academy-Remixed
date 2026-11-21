// Native BSP parser/admission/root/controller and speaker setup. Entity field
// allocation, file/registry, collision/link and actual teleport are boundaries.
#include "subbsp_fixture.h"
#include "game/g_functions.h"
#include "game/g_bsp_section_copy.h"
#include "game/g_savegame_fields.h"
#include "game/g_bsp_section_wasteland_movers.h"
#include "game/g_siege_item.h"
#include "game/g_siege_korriban.h"
#include "game/g_siege_korriban_actor.h"
#include "game/g_laser_arm.h"
#include "game/g_volume_geometry.h"
#include "game/b_local.h"
#include "game/g_mover_motion.h"
#include "game/g_workshop_placement.h"
#include "game/g_contact_trigger.h"
#include "game/anims.h"
#include "cgame/cg_breakable_effects.h"
#include "qcommon/jar_duplicate.h"
#include "qcommon/jar_savegame.h"
#include "chunk_save.h"
#include "game/g_bsp_registry.inc"
#include <deque>
#include <array>
#include <fstream>
#include <iterator>
static std::deque<std::string> sectionStrings;
static std::vector<std::string> sectionSounds;
static std::vector<std::string> sectionFeedback;
static void sectionMessage(int client,const char* format,...){
    CHECK(client==0);char message[200];va_list args;va_start(args,format);
    vsnprintf(message,sizeof(message),format,args);va_end(args);sectionFeedback.emplace_back(message);
}
static int dustEvents,fireEvents,fireDamageTicks;
static bool siegeMediaFixture;
static int siegeEffectsPlayed,siegeTorchHits;
static std::vector<std::string> siegeSceneryAssets,siegeEffectAssets;
static int trafficEvents;
static int siegeBreaks, siegePainOutputs;
static std::vector<std::string> siegeRoundSounds;
static bool brushHazardBlocks;
char*G_NewString(const char*s){sectionStrings.emplace_back(s);return const_cast<char*>(sectionStrings.back().c_str());}
void AddSpawnField(char*k,char*v){CHECK(numSpawnVars<MAX_SPAWN_VARS);spawnVars[numSpawnVars][0]=G_AddSpawnVarToken(k);spawnVars[numSpawnVars++][1]=G_AddSpawnVarToken(v);}
qboolean G_SpawnFloat(const char*k,const char*d,float*out){char*v;const auto present=G_SpawnString(k,d,&v);*out=float(atof(v));return present;}
void G_SetOrigin(gentity_t*e,const vec3_t p){VectorCopy(p,e->currentOrigin);VectorCopy(p,e->s.pos.trBase);e->s.pos.trType=TR_STATIONARY;e->s.pos.trTime=e->s.pos.trDuration=0;VectorClear(e->s.pos.trDelta);}
#include "game/g_entity_angles.inc"
void SetClientViewAngle(gentity_t*,vec3_t){CHECK(false);}
int G_SoundIndex(const char*name){sectionSounds.emplace_back(name);return int(sectionSounds.size());}
int G_ModelIndex(const char*name){
    if(!strcmp(name,"models/map_objects/bespin/twinpodcc.md3"))return 11;
    for(int i=0;i<3;++i)if(!strcmp(name,jar::shaft::sceneryModels[i]))return 12+i;
    if(strstr(name,"models/map_objects/yavin/")||!strcmp(name,"models/map_objects/desert/rock.md3"))return 15;
    if(!strncmp(name,jar::taspir::sceneryPrefix,strlen(jar::taspir::sceneryPrefix)))return 16;
    if(!strcmp(name,jar::factory::baseModel))return 17;
    if(!strcmp(name,jar::wasteland::crateModel))return 20;
    for(unsigned i=0;i<ARRAY_LEN(jar::factory::flags);++i)if(!strcmp(name,jar::factory::flags[i].model))return 18+i;
    for(unsigned i=0;i<ARRAY_LEN(jar::siegeKorriban::deliveries);++i)
        if(!strcmp(name,jar::siegeKorriban::deliveries[i].itemModel))return 21+i;
    if(!strcmp(name,jar::siegeKorriban::coffinModel))return 25;
    if(siegeMediaFixture&&strstr(name,".md3")){
        const auto found=std::find(siegeSceneryAssets.begin(),siegeSceneryAssets.end(),name);
        if(found!=siegeSceneryAssets.end())return 80+int(found-siegeSceneryAssets.begin());
        siegeSceneryAssets.emplace_back(name);return 79+int(siegeSceneryAssets.size());
    }
    CHECK(false);return 0;
}
void G_SetMovedir(vec3_t angles,vec3_t direction){
    AngleVectors(angles,direction,nullptr,nullptr);
    VectorClear(angles);
}
qboolean G_SpawnVector(const char*k,const char*d,float*out){char*v;const auto present=G_SpawnString(k,d,&v);CHECK(sscanf(v,"%f %f %f",out,out+1,out+2)==3);return present;}
void G_JAR_SetBrushModel(gentity_t*e,const char*name){gi.SetBrushModel(e,name);}
void EvaluateTrajectory(const trajectory_t*t,int time,vec3_t out){
    if(t->trType==TR_SINE){CHECK(t->trDuration>0);VectorMA(t->trBase,std::sin((time-t->trTime)*2*M_PI/t->trDuration),t->trDelta,out);}
    else if(t->trType==TR_LINEAR_STOP)VectorMA(t->trBase,std::max(0,std::min(time-t->trTime,t->trDuration))*.001f,t->trDelta,out);
    else if(t->trType==TR_NONLINEAR_STOP){
        CHECK(t->trDuration>0);
        const float phase=float(std::max(0,std::min(time-t->trTime,t->trDuration)))/t->trDuration;
        VectorMA(t->trBase,t->trDuration*.001f*std::sin(.5f*M_PI*phase),t->trDelta,out);
    }
    else{CHECK(t->trType==TR_STATIONARY||t->trType==TR_INTERPOLATE);VectorCopy(t->trBase,out);}
}
#include "game/g_subbsp_movers.h"
#include "game/g_mover_initmovertrdata.inc"
#include "game/g_mover_init.inc"
#include "game/g_mover_sp_func_bobbing.inc"
#include "game/g_mover_sp_func_door.inc"
#include "game/g_mover_spawnplattrigger.inc"
#include "game/g_mover_sp_func_plat.inc"
qboolean G_SpawnAngleHack(const char*,const char*,float*){return qfalse;}
int G_EffectIndex(const char* name){
    if(!siegeMediaFixture)return !strcmp(name,jar::bspSectionFX::dust)?5:4;
    const auto found=std::find(siegeEffectAssets.begin(),siegeEffectAssets.end(),name);
    if(found!=siegeEffectAssets.end())return 80+int(found-siegeEffectAssets.begin());
    siegeEffectAssets.emplace_back(name);return 79+int(siegeEffectAssets.size());
}
sfxHandle_t CAS_GetBModelSound(const char*,int){return -1;}
void G_RadiusDamage(const vec3_t,gentity_t* source,float damage,float radius,gentity_t* ignore,int){
    CHECK(source==ignore);
    if(source->s.time2==jar::siegeKorriban::effectTag){
        CHECK(source->spawnflags==4&&damage==5&&radius==16&&G_SiegeKorribanRoom(G_SubBSPOwner(source)));
        ++siegeTorchHits;
    }else{CHECK(jar::bspSection::Torch(*source));++fireDamageTicks;}
}
void G_AddEvent(gentity_t* e,int event,int param){
    if(e->s.time2==jar::siegeKorriban::effectTag){
        CHECK(event==EV_PLAY_EFFECT&&param==e->fxID);
        ++siegeEffectsPlayed;
        return;
    }
    if(event==EV_GENERAL_SOUND){
        CHECK(jar::bspSection::TrafficSpeaker(*e));
        if(e->s.time2==jar::hilltops::birdSpeakerTag){
            CHECK(param>0&&size_t(param)<=sectionSounds.size());
            const std::string& played=sectionSounds[param-1];
            CHECK(std::any_of(std::begin(jar::hilltops::birdCalls),std::end(jar::hilltops::birdCalls),[&](const char* call){return played==call;}));
        }else CHECK(param==e->noise_index);
        ++trafficEvents;return;
    }
    CHECK(event==EV_PLAY_EFFECT&&(param==4||param==5));if(param==4)++fireEvents;else ++dustEvents;
}
void G_UseTargets2(gentity_t*,gentity_t*,const char*);
void G_Sound(gentity_t* actor,int sound){
    if(!siegeMediaFixture)return;
    CHECK(sound>0&&size_t(sound)<=sectionSounds.size());
    const auto& asset=sectionSounds[sound-1];
    if(asset!=jar::siegeKorriban::winSound&&asset!=jar::siegeKorriban::lossSound)return;
    CHECK(actor==&g_entities[0]);
    siegeRoundSounds.push_back(asset);
}
void G_PlayEffect(int,const vec3_t,const vec3_t){}
#define FX_ENT_RADIUS 32
int BMS_START=0,BMS_MID=1,BMS_END=2;
void G_PlayDoorSound(gentity_t*,int stage){CHECK(stage==BMS_START||stage==BMS_END);}
void G_PlayDoorLoopSound(gentity_t* e){e->s.loopSound=23;}
void SetMoverState(gentity_t*,moverState_t,int);
void MatchTeam(gentity_t* e,int state,int time){
    for(auto* part=e;part;part=part->teamchain)SetMoverState(part,moverState_t(state),time);
}
#include "game/g_door_trigger_spawn.inc"
#include "game/g_mover_touch_doortrigger.inc"
char*vtos(const vec3_t p);
static gentity_t* triggeredPlatform;
void Use_SiegeFixtureMover(gentity_t*,gentity_t*,gentity_t*);
void Use_BinaryMover(gentity_t* mover,gentity_t* source,gentity_t* actor){
    if(G_SiegeKorribanRoom(G_SubBSPOwner(mover)))Use_SiegeFixtureMover(mover,source,actor);
    else triggeredPlatform=mover;
}
#include "game/g_mover_touch_platcentertrigger.inc"
#include "game/g_fx_runner.inc"
#include "game/g_bsp_section_hurt.inc"
char*vtos(const vec3_t p){static char text[128];Com_sprintf(text,sizeof(text),"%g %g %g",p[0],p[1],p[2]);return text;}
static int teleports=0,traceCalls=0,links=0;static bool blocked=false;
void TeleportPlayer(gentity_t*player,vec3_t p,vec3_t angles){++teleports;G_SetOrigin(player,p);G_SetAngles(player,angles);}
#include "game/g_bsp_section.inc"
#include "game/g_bsp_section_pickup.inc"
#include "game/g_target_sp_speaker.inc"
static gitem_t sectionItems[std::max(ARRAY_LEN(jar::taspir::pickups),ARRAY_LEN(jar::dropzone::pickups))];
static bool missingRiftPickup;
static const jar::bspSection::Profile*profile=&jar::bspSection::profiles[0];
static gitem_t* RiftFixtureItem(const char* name){
    size_t count;
    const auto* specs=jar::bspSection::PickupSpecs(profile->mode,count);
    for(size_t i=0;i<count;++i){
        if(strcmp(name,specs[i].name))continue;
        if(missingRiftPickup&&i==0)return nullptr;
        auto& item=sectionItems[i];
        item.classname=const_cast<char*>(specs[i].name);
        item.world_model=const_cast<char*>("models/items/fixture.md3");
        item.giType=!strncmp(name,"weapon_",7)?IT_WEAPON:IT_AMMO;
        const auto* enlightenment=jar::enlightenment::Definition(name);
        if(enlightenment||!strcmp(name,jar::forceBoon::classname)){
            item.giType=IT_POWERUP;
            item.giTag=enlightenment?enlightenment->tag:PW_FORCE_BOON;
            item.pickup_sound=const_cast<char*>("sound/items/fixture.wav");
        }
        item.quantity=1;
        return &item;
    }
    return nullptr;
}
void FinishSpawningItem(gentity_t* e){
    const auto* root=G_BSPSectionPickupRoot(*e);
    CHECK(root&&e->item);
    VectorSet(e->mins,-8,-8,-2);VectorSet(e->maxs,8,8,16);
    if(!BSPSectionPlacePickup(*e,*root)){G_FreeEntity(e);return;}
    e->s.eType=ET_ITEM;e->contents=CONTENTS_TRIGGER|CONTENTS_ITEM;
    e->svFlags|=SVF_PLAYER_USABLE;e->e_TouchFunc=touchF_Touch_Item;e->e_UseFunc=useF_Use_Item;
    gi.linkentity(e);
}
qboolean PInUse(unsigned int index){return g_entities[index].inuse;}
static std::vector<gentity_t*> sectionTargets;
void target_random_use(gentity_t*,gentity_t*,gentity_t*);
void GEntity_UseFunc(gentity_t* target,gentity_t* source,gentity_t* actor){
    sectionTargets.push_back(target);
    if(target->e_UseFunc==useF_target_random_use)target_random_use(target,source,actor);
    if(target->e_UseFunc==useF_fx_runner_use)fx_runner_use(target,source,actor);
    if(target->e_UseFunc==useF_siege_item_use)siege_item_use(target,source,actor);
    if(target->e_UseFunc==useF_siege_objective_use)siege_objective_use(target,source,actor);
    if(target->e_UseFunc==useF_siege_end_use)siege_end_use(target,source,actor);
    if(target->e_UseFunc==useF_siege_checkpoint_use)siege_checkpoint_use(target,source,actor);
    if(target->e_UseFunc==useF_func_usable_use)func_usable_use(target,source,actor);
    if(target->e_UseFunc==useF_funcBBrushUse)funcBBrushUse(target,source,actor);
    if(target->e_UseFunc==useF_Use_Target_Delay)Use_Target_Delay(target,source,actor);
    if(target->e_UseFunc==useF_Use_BinaryMover&&G_SiegeKorribanRoom(G_SubBSPOwner(target)))
        Use_BinaryMover(target,source,actor);
    if(target->e_UseFunc==useF_target_relay_use)G_UseTargets(target,actor);
    if(target->e_UseFunc==useF_target_counter_use&&target->count>0&&--target->count==0)G_UseTargets(target,actor);
    if(target->e_UseFunc==useF_Use_Target_Print){
        CHECK(G_SiegeKorribanPrint(*target));
        if(target->targetname&&!strcmp(target->targetname,"firstgateattack"))++siegePainOutputs;
    }
}
#include "game/g_entity_find.inc"
#include "game/g_entity_targets.inc"
#include "game/g_siege_item.inc"
bool G_JAR_WorkshopOwns(const gentity_t*){return false;}
qboolean G_ActivateBehavior(gentity_t*,int){return qfalse;}
stringID_table_t TeamTable[]={{"", -1}};
void G_SetEnemy(gentity_t* e,gentity_t* enemy){e->enemy=enemy;}
void G_Damage(gentity_t*,gentity_t*,gentity_t*,const vec3_t,const vec3_t,int,int,int,int){CHECK(false);}
gentity_t* G_TempEntity(const vec3_t,int){CHECK(false);return nullptr;}
void G_JAR_WorkshopBreakableDetach(gentity_t*){CHECK(false);}
void AddSightEvent(gentity_t*,vec3_t,float,alertEventLevel_e,float){}
void AddSoundEvent(gentity_t*,vec3_t,float,alertEventLevel_e,qboolean,qboolean){}
void CG_MiscModelExplosion(const vec3_t,const vec3_t,int,material_t,const BreakableDebrisFrame* frame){
    CHECK(frame);++siegeBreaks;
}
void CG_Chunks(int,const vec3_t,const vec3_t,const vec3_t,const vec3_t,float,int,material_t,int,float,int,
               const BreakableDebrisFrame* frame){CHECK(frame);}
static void CacheChunkEffects(material_t){}
gentity_t* G_TestEntityPosition(gentity_t*){CHECK(false);return nullptr;}
#include "game/g_breakable_brush.inc"
#include "game/g_breakable_brush_spawn.inc"
#include "game/g_target_delay.inc"
#include "game/g_usable.cpp"
#include "game/g_logic_timer.inc"
#include "game/g_target_random.inc"
static void ContactFire(gentity_t&){CHECK(false);}
static void ContactUse(gentity_t&,gentity_t*){CHECK(false);}
void G_ContactTouchAt(gentity_t&,gentity_t*,const vec3_t){CHECK(false);}
int Pilot_ActivePilotCount(){CHECK(false);return 0;}
qboolean G_PointInBounds(const vec3_t,const vec3_t,const vec3_t){CHECK(false);return qfalse;}
#include "game/g_trigger_init.inc"
#include "game/g_trigger_multiple.inc"
#include "game/g_trigger_multiple_spawn.inc"
static unsigned held=ENTITYNUM_NONE;static float holdDistance=144;static vec3_t eyePosition{};
void eye(float*origin,float*direction){VectorCopy(eyePosition,origin);VectorSet(direction,1,0,0);}
void NPC_SetAnim(gentity_t*,int,int,int,int){throw std::runtime_error("Unexpected actor hold");}
bool setupMoved(gentity_t&e){return G_BSPSectionMoved(e);}
#include "game/g_workshop_native_control.inc"
// Force application/pusher collision has separate native regression fixtures.
static bool sectionHoldBlocked;
bool G_JAR_MoverTranslate(gentity_t& root,const vec3_t requested,vec3_t actual){
 if(sectionHoldBlocked){VectorClear(actual);return false;}
 VectorCopy(requested,actual);VectorAdd(root.currentOrigin,actual,root.currentOrigin);
 VectorAdd(root.s.pos.trBase,actual,root.s.pos.trBase);
 return true;
}
bool G_JAR_MoverRotate(gentity_t&,const vec3_t){return false;}
#include "game/g_mover_motion.inc"
#include "game/g_mover_train.inc"
int INV_GoodieKeyCheck(gentity_t*){CHECK(false);return 0;}
void CalcTeamDoorCenter(gentity_t* e,vec3_t center){jar::bspSection::Center(*e,center);}
#define Use_BinaryMover Use_SiegeFixtureMover
#include "game/g_mover_binary.inc"
#undef Use_BinaryMover
static int nextSlot=100,sectionRegistrations=0;
static bool resourceLoaded=false;
static int sectionModels=0,baseModels=3;
static bool displacedBrush=false;
static bool omitNextChild=false;
static std::string entityText;
static std::vector<byte> sectionBytes;
static int sectionOpen(const char*name,fileHandle_t*h,fsMode_t mode){CHECK(mode==FS_READ&&std::string(name)==std::string("maps/")+(profile->model+1)+".bsp");*h=1;return int(sectionBytes.size());}
static int sectionRead(void*out,int size,fileHandle_t){memcpy(out,sectionBytes.data(),size);return size;}
static int sectionRegistry(int slot,char*name,int size,int*first,int*count,unsigned*hash){
    *hash=LittleLong(Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size())));
    if(slot==-1){*name=0;*first=0;*count=baseModels;return 1;}
    if(slot==0&&resourceLoaded){Q_strncpyz(name,profile->model,size);*first=baseModels;*count=sectionModels;return 1;}return 0;
}
static void sectionBrush(gentity_t*e,const char*name){
    int model=0;
    if(name[0]=='*'){model=atoi(name+1);CHECK(model>0&&model<sectionModels&&resourceLoaded);}
    else{CHECK(!strcmp(name,profile->model));++sectionRegistrations;resourceLoaded=true;}
    e->bmodel=qtrue;e->contents=CONTENTS_SOLID;e->s.modelindex=baseModels+model;
    dheader_t h;memcpy(&h,sectionBytes.data(),sizeof(h));dmodel_t m;memcpy(&m,sectionBytes.data()+h.lumps[LUMP_MODELS].fileofs+model*sizeof(m),sizeof(m));
    VectorCopy(m.mins,e->mins);VectorCopy(m.maxs,e->maxs);
    if(displacedBrush&&model==1)e->maxs[0]+=10000;
}
static gentity_t*allocate(const char*classname){auto&e=g_entities[nextSlot++];CHECK(nextSlot<ENTITYNUM_WORLD);e.inuse=qtrue;e.s.number=nextSlot-1;e.s.groundEntityNum=ENTITYNUM_NONE;globals.num_entities=nextSlot;e.classname=G_NewString(classname);G_SubBSPAllocated(&e);return &e;}
gentity_t* G_Spawn(){return allocate("noclass");}
static void sectionEntity(){
    if(omitNextChild){omitNextChild=false;return;}
    auto*e=allocate(field("classname"));point("origin",e->s.origin);point("angles",e->s.angles);VectorCopy(e->s.origin,e->currentOrigin);VectorCopy(e->s.angles,e->currentAngles);VectorCopy(e->s.angles,e->s.apos.trBase);
    if(field("targetname"))e->targetname=G_NewString(field("targetname"));if(field("target"))e->target=G_NewString(field("target"));
    if(field("team"))e->team=G_NewString(field("team"));
    if(field("spawnflags"))e->spawnflags=atoi(field("spawnflags"));
    if(!strcmp(e->classname,"target_speaker")&&field("soundset"))e->soundSet=G_NewString(field("soundset"));
    if(G_BSPSectionPosition(e,G_SubBSPOwner(e)))return;
    if(auto* item=FindItem(e->classname)){
        e->item=item;e->e_ThinkFunc=thinkF_FinishSpawningItem;e->nextthink=100;return;
    }
    if(!strcmp(e->classname,"misc_model_static")){
        e->model=G_NewString(field("model"));CHECK(G_BSPSectionScenery(e,G_SubBSPOwner(e)));return;
    }
    if(!strcmp(e->classname,"target_speaker")){SP_target_speaker(e);return;}
    if(jar::wasteland::GunEntity(*e)){
        e->health=e->max_health=jar::wasteland::gunHealth;
        e->count=jar::wasteland::gunAmmo;
        e->takedamage=qtrue;
        e->contents=CONTENTS_BODY;
        e->e_UseFunc=useF_emplaced_gun_use;
        VectorSet(e->mins,-30,-30,-5);VectorSet(e->maxs,30,30,60);
        VectorCopy(e->currentAngles,e->pos1);
        G_SetOrigin(e,e->s.origin);gi.linkentity(e);return;
    }
    if(!strcmp(e->classname,"func_bobbing")){e->model=G_NewString(field("model"));e->model2=G_NewString(field("model2"));SP_func_bobbing(e);return;}
    if(!strcmp(e->classname,"func_plat")){
        e->model=G_NewString(field("model"));if(const auto* sound=field("soundset"))e->soundSet=G_NewString(sound);
        SP_func_plat(e);return;
    }
    if(!strcmp(e->classname,"func_door")){
        e->model=G_NewString(field("model"));if(const auto* sound=field("soundset"))e->soundSet=G_NewString(sound);
        G_SpawnFloat("speed","0",&e->speed);G_SpawnFloat("wait","0",&e->wait);G_SpawnInt("delay","0",&e->delay);
        e->alt_fire=field("linear")?qtrue:qfalse;
        SP_func_door(e);return;
    }
    if(!strcmp(e->classname,"func_train")){
        e->model=G_NewString(field("model"));e->soundSet=G_NewString(field("soundset"));
        e->speed=100;e->damage=atoi(field("dmg"));G_JAR_SetBrushModel(e,e->model);InitMover(e);
        e->e_ReachedFunc=reachedF_Reached_Train;e->e_ThinkFunc=thinkF_Think_SetupTrainTargets;e->nextthink=FRAMETIME;return;
    }
    if(!strcmp(e->classname,"path_corner")){
        G_SpawnFloat("speed","0",&e->speed);G_SpawnFloat("wait","0",&e->wait);return;
    }
    if(!strcmp(e->classname,"trigger_multiple")){
        // Native trigger initialization is covered by the contact fixture.
        e->model=G_NewString(field("model"));G_JAR_SetBrushModel(e,e->model);
        e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;
        e->e_TouchFunc=touchF_Touch_Multi;e->e_UseFunc=useF_Use_Multi;
        e->delay=atoi(field("delay"))*1000;e->wait=atof(field("wait"));gi.linkentity(e);return;
    }
    if(!strcmp(e->classname,"func_breakable")){
        // The focused breakable fixture executes the native callbacks.
        e->model=G_NewString(field("model"));if(field("soundSet"))e->soundSet=G_NewString(field("soundSet"));
        e->health=atoi(field("health"));e->material=field("material")?material_t(atoi(field("material"))):MAT_METAL;
        if(e->spawnflags&16)e->flags|=FL_DMG_BY_SABER_ONLY;
        e->takedamage=qtrue;e->s.eType=ET_MOVER;e->e_DieFunc=dieF_funcBBrushDie;
        e->e_UseFunc=useF_funcBBrushUse;e->e_PainFunc=painF_funcBBrushPain;
        G_SetOrigin(e,e->s.origin);G_JAR_SetBrushModel(e,e->model);gi.linkentity(e);return;
    }
    if(!strcmp(e->classname,"func_glass")){
        e->model=G_NewString(field("model"));e->health=1;e->material=MAT_GLASS;
        e->takedamage=qtrue;e->s.eType=ET_MOVER;e->e_DieFunc=dieF_funcGlassDie;e->e_UseFunc=useF_funcGlassUse;
        e->svFlags|=SVF_GLASS_BRUSH|SVF_BBRUSH;
        G_SetOrigin(e,e->s.origin);G_JAR_SetBrushModel(e,e->model);gi.linkentity(e);return;
    }
    if(!strcmp(e->classname,"trigger_hurt")){
        // The dedicated fixture executes native SP_trigger_hurt and hurt_touch.
        // Pass its brush and gameplay fields through real section adaptation here.
        e->model=G_NewString(field("model"));G_JAR_SetBrushModel(e,e->model);
        e->contents=CONTENTS_TRIGGER;e->svFlags=SVF_NOCLIENT;e->e_TouchFunc=touchF_hurt_touch;
        e->damage=atoi(field("dmg"));e->wait=field("wait")?1000:0;e->spawnflags|=4;
        gi.linkentity(e);return;
    }
    if(!strcmp(e->classname,"fx_runner")){
        e->fxFile=G_NewString(field("fxfile"));if(field("soundset"))e->soundSet=G_NewString(field("soundset"));
        SP_fx_runner(e);return;
    }
    if(!strcmp(e->classname,"func_timer")){SP_func_timer(e);return;}
    if(!strcmp(e->classname,"target_random")){SP_target_random(e);return;}
    CHECK(!strcmp(e->classname,"info_null")||!strcmp(e->classname,"info_notnull")||!strcmp(e->classname,"target_position"));G_SetOrigin(e,e->s.origin);
}
static void Init(const char*path=nullptr){
    optionalSpawn=true;sectionSpawnCallback=sectionEntity;
    size_t pickupKinds;
    if(jar::bspSection::PickupSpecs(profile->mode,pickupKinds))findSectionItem=RiftFixtureItem;
    if(path){std::ifstream in(path,std::ios::binary);CHECK(in.good());sectionBytes.assign(std::istreambuf_iterator<char>(in),{});}
    else {
        entityText="{ classname worldspawn }\n{ classname info_player_duel1 origin \"20 30 40\" angle 90 }\n{ classname target_speaker origin \"100 20 40\" noise sound/test.wav spawnflags 1 }\n{ classname info_player_intermission target v1 origin \"40 50 60\" }\n{ classname info_notnull targetname v1 origin \"50 60 70\" }\n";
        dheader_t h{};h.ident=BSP_IDENT;h.version=BSP_VERSION;h.lumps[LUMP_MODELS]={sizeof(h),sizeof(dmodel_t)};
        h.lumps[LUMP_ENTITIES]={sizeof(h)+sizeof(dmodel_t),int(entityText.size()+1)};sectionBytes.assign(h.lumps[0].fileofs+h.lumps[0].filelen,0);memcpy(sectionBytes.data(),&h,sizeof(h));
        dmodel_t m{};VectorSet(m.mins,-200,-100,-20);VectorSet(m.maxs,200,100,200);memcpy(sectionBytes.data()+sizeof(h),&m,sizeof(m));memcpy(sectionBytes.data()+h.lumps[0].fileofs,entityText.c_str(),entityText.size()+1);
    }
    dheader_t h;memcpy(&h,sectionBytes.data(),sizeof(h));entityText.assign(reinterpret_cast<const char*>(sectionBytes.data()+h.lumps[0].fileofs),h.lumps[0].filelen);entityText.push_back(0);sectionModels=h.lumps[LUMP_MODELS].filelen/sizeof(dmodel_t);
    gi.Printf=print;gi.FS_FOpenFile=sectionOpen;gi.FS_Read=sectionRead;gi.FS_FCloseFile=[](fileHandle_t){};gi.FS_ReadFile=[](const char*,void**)->long{return 1;};gi.BSPModelAt=sectionRegistry;
    gi.SendServerCommand=sectionMessage;
    gi.EntityContact=[](const vec3_t,const vec3_t,const gentity_t* brush){
        CHECK(G_BSPSectionBrushHurt(*brush));return brushHazardBlocks?qtrue:qfalse;
    };
    gi.SetBrushModel=sectionBrush;gi.SetActiveSubBSP=[](int model)->const char*{return model<0?nullptr:entityText.c_str();};
    gi.linkentity=[](gentity_t*e){++links;e->linked=qtrue;};gi.unlinkentity=[](gentity_t*e){e->linked=qfalse;};gi.GetConfigstring=[](int,char*s,int){*s=0;};gi.bIsFromZone=[](const void*,memtag_t)->qboolean{return qfalse;};
    gi.trace=[](trace_t*t,const vec3_t start,const vec3_t,const vec3_t,const vec3_t end,int pass,int mask,EG2_Collision type,int){
        *t={};t->fraction=1;t->startsolid=blocked?qtrue:qfalse;VectorCopy(end,t->endpos);++traceCalls;
        if(type==G2_SINGLE_BRUSH){
            CHECK(pass>=0&&pass<globals.num_entities&&jar::bspSection::Portable(g_entities[pass]));
            CHECK(mask==(MASK_SOLID|CONTENTS_PLAYERCLIP)&&std::fabs(Distance(start,end)-4096)<.02f);
            t->fraction=1.f/256;t->entityNum=pass;
            vec3_t toward;VectorSubtract(end,start,toward);
            VectorMA(start,t->fraction,toward,t->endpos);
        }
    };
    G_ResetSubBSPSpawnContext();
}
static gentity_t*Spawn(const vec3_t origin,const vec3_t angles){auto*e=allocate(profile->card);VectorCopy(origin,e->s.origin);VectorCopy(origin,e->currentOrigin);G_SetAngles(e,angles);CHECK(G_BSPSectionSpawn(e));return e;}
static bool Near(const vec3_t a,const vec3_t b){return Distance(a,b)<.02f;}
static void Exercise(const char*path){
    Init(path);jar::bspAdmission::Report report;CHECK(jar::bspAdmission::Inspect(sectionBytes.data(),sectionBytes.size(),report,profile->mode));
    vec3_t origin{1000,2000,300},angles{0,30,0},lo,hi;std::string reason;
    const bool rift=profile->mode==jar::bspSection::Mode::Rift;
    const bool korribanRoom=profile->mode==jar::bspSection::Mode::Korriban;
    const bool sentinelRoom=profile->mode==jar::bspSection::Mode::Sentinel;
    const bool tatooineRoom=profile->mode==jar::bspSection::Mode::Tatooine;
    const bool taspirRoom=profile->mode==jar::bspSection::Mode::Taspir;
    const bool factoryRoom=profile->mode==jar::bspSection::Mode::Factory;
    const bool coruscantRoom=profile->mode==jar::bspSection::Mode::Coruscant;
    const bool hilltopsRoom=profile->mode==jar::bspSection::Mode::Hilltops;
    const bool dropzoneRoom=profile->mode==jar::bspSection::Mode::DropZone;
    const bool wastelandRoom=profile->mode==jar::bspSection::Mode::Wasteland;
    const bool ctfLifts=coruscantRoom||hilltopsRoom;
    const bool ctfBrushes=ctfLifts||dropzoneRoom||wastelandRoom;
    if(rift||korribanRoom||sentinelRoom||tatooineRoom||taspirRoom||factoryRoom||ctfBrushes){
        missingRiftPickup=true;
        CHECK(!G_BSPSectionBounds(*profile,angles,lo,hi,reason)&&sectionRegistrations==0);
        CHECK(reason.find("pickup is unavailable")!=std::string::npos);
        missingRiftPickup=false;
    }
    if(profile->mode==jar::bspSection::Mode::Breakable){
        baseModels=MAX_SUBMODELS-sectionModels;
        CHECK(!G_BSPSectionBounds(*profile,angles,lo,hi,reason)&&sectionRegistrations==0);
        CHECK(reason.find("brush-model capacity")!=std::string::npos);
        --baseModels;
        CHECK(G_BSPSectionBounds(*profile,angles,lo,hi,reason)&&sectionRegistrations==0);
        baseModels=3;
    }
    CHECK(G_BSPSectionBounds(*profile,angles,lo,hi,reason));
    if(tatooineRoom)CHECK(lo[2]==jar::tatooine::minimumTravelZ&&report.mins[2]>lo[2]);
    const auto card=std::find_if(std::begin(jar::catalog::entries),std::end(jar::catalog::entries),
        [](const auto& entry){return !strcmp(entry.name,profile->card);});
    CHECK(card!=std::end(jar::catalog::entries)&&jar::duplicate::MapSection(card->id));
    for(const float yaw:{0.f,30.f,90.f,145.f}){
        const vec3_t facing={0,yaw,0};
        jar::duplicate::Object node{};node.source=1;node.catalog=card->id;
        VectorCopy(facing,node.angles);
        jar::bspSection::Hull(report.mins,report.maxs,facing,node.mins,node.maxs);
        jar::duplicate::Snapshot copy;copy.objects.push_back(node);
        vec3_t low,high;
        CHECK(jar::duplicate::Bounds(copy,low,high)&&Near(low,node.mins)&&Near(high,node.maxs));
        CHECK(G_JAR_WorkshopSectionInWorld(origin,low,high));
        vec3_t outside;VectorCopy(origin,outside);outside[0]=MAX_WORLD_COORD-high[0]+1;
        CHECK(!G_JAR_WorkshopSectionInWorld(outside,low,high));
    }
    auto*root=Spawn(origin,angles);std::vector<gentity_t*> members;CHECK(G_SubBSPMembers(root,members));CHECK(int(members.size())==report.entities-1+
        (taspirRoom?jar::taspir::helperCount:wastelandRoom?jar::wasteland::helpers:report.ownedHelpers));
    CHECK(root->e_UseFunc==useF_target_teleporter_use&&G_SubBSPNamespace(root)!=0&&Near(root->pos1,origin));
    CHECK(!strcmp(root->classname,"misc_bsp")&&!strcmp(root->model,profile->model));
    CHECK(jar::bspSection::MatchesCard(*root,*profile));
    for(const auto&other:jar::bspSection::profiles)
        if(&other!=profile)CHECK(!jar::bspSection::MatchesCard(*root,other));
    CHECK(root->linked&&root->bmodel&&root->s.eType==ET_MOVER&&root->s.modelindex==baseModels);
    CHECK(!(root->s.eFlags&EF_PERMANENT)&&(root->svFlags&SVF_BROADCAST)&&!(root->svFlags&SVF_NOCLIENT));
    std::vector<gentity_t*> brushes;gentity_t*removedBrush=nullptr;
    const bool hoth=!strcmp(profile->model,"#mp/duel9");
    const bool yavin=profile->mode==jar::bspSection::Mode::Yavin;
    gentity_t* yavinTimer=nullptr;
    int dust=0;
    const bool generator=!strcmp(profile->model,"#mp/duel2");
    const bool shaft=profile->mode==jar::bspSection::Mode::Shaft;
    gentity_t* riftUnion=nullptr,*riftPickup=nullptr;
    std::vector<gentity_t*> korribanFalls;
    int pickups=0;
    gentity_t* shaftLifts[2]{},*shaftPads[2]{};
    int sceneryCount=0;
    std::vector<gentity_t*> sentinelLifts,sentinelPads;
    gentity_t* sentinelAmmo=nullptr,*sentinelDetPack=nullptr;
    int speakers=0,starts=0,bobs=0,fires=0,hurts=0,skyPoints=0;gentity_t*bob=nullptr,*hazard=nullptr;
    const bool capturePractice=jar::bspSection::CapturePractice(profile->mode);
    for(auto*e:members){CHECK(G_SubBSPOwner(e)==root);if(jar::bspSection::Start(e->message)||(capturePractice&&jar::factory::StartTeam(e->message)))++starts;
        CHECK(!jar::bspSection::Position(e->classname));if(e->s.eType==ET_SPEAKER){
            CHECK((e->s.loopSound>0||jar::bspSection::TrafficSpeaker(*e))&&e->e_UseFunc==useF_Use_Target_Speaker);++speakers;
        }
        CHECK(strcmp(e->classname,"misc_skyportal"));
        if(e->message&&!strcmp(e->message,"misc_skyportal")){
            CHECK(!strcmp(e->classname,"info_notnull")&&!e->linked&&e->e_ThinkFunc==thinkF_NULL);
            ++skyPoints;
        }
        if(G_BSPSectionBreakable(*e)){
            CHECK(e->linked&&e->takedamage&&G_BSPSectionBreakableRoot(*e)==root);
            if(factoryRoom)CHECK(e->health==1&&e->material==MAT_GLASS&&e->e_DieFunc==dieF_funcGlassDie);
            else if(yavin)CHECK(e->health>=2&&e->health<=10&&e->material==MAT_METAL&&e->spawnflags==2064&&(e->flags&FL_DMG_BY_SABER_ONLY)&&!e->soundSet);
            else CHECK(e->health>=1&&e->health<=3&&e->material==MAT_LT_STONE&&e->spawnflags==2048&&!strcmp(e->soundSet,"stoneblocks"));
            brushes.push_back(e);
        }
        if(e->bmodel&&e->s.eType==ET_MOVER&&e->s.modelindex>0&&!(e->svFlags&SVF_NOCLIENT))
            CHECK(!(e->s.eFlags&EF_PERMANENT)&&(e->svFlags&SVF_BROADCAST));
        if(!strcmp(e->classname,"info_null"))CHECK(e->e_ThinkFunc==thinkF_NULL&&!e->linked);
        if(!strcmp(e->classname,"func_bobbing")){CHECK(e->s.pos.trType==TR_SINE&&e->s.pos.trDuration==3000&&e->s.modelindex2==11);++bobs;bob=e;}
        if(!strcmp(e->classname,"trigger_hurt")){
            if(ctfBrushes){
                CHECK(G_BSPSectionBrushHurt(*e)&&e->s.time2==BSPSectionBrushHurtTag);
                if(wastelandRoom&&(e->s.modelindex-root->s.modelindex==7||e->s.modelindex-root->s.modelindex==14))
                    CHECK(e->damage==9999&&e->spawnflags==12);
                else CHECK(e->damage==Q3_INFINITE&&e->spawnflags==(12|32));
                ++hurts;
                continue;
            }
            CHECK(!e->bmodel&&e->linked&&(e->contents&CONTENTS_TRIGGER)&&e->e_TouchFunc==touchF_hurt_touch);
            if(rift){
                const bool compound=e->s.time2==BSPSectionRiftUnionTag;
                const auto& box=jar::rift::fallBoxes[compound?0:2];
                CHECK(Near(e->s.origin2,box.mins)&&Near(e->s.angles2,box.maxs));
                if(compound)riftUnion=e;
            }else if(korribanRoom){
                CHECK(e->s.time2==BSPSectionHurtTag&&e->spawnflags==(4|8|32));
                const auto& box=jar::korriban::fallBoxes[korribanFalls.size()];
                CHECK(Near(e->s.origin2,box.mins)&&Near(e->s.angles2,box.maxs));
                korribanFalls.push_back(e);
            }else CHECK(Near(e->s.origin2,report.hurtMins)&&Near(e->s.angles2,report.hurtMaxs));
            CHECK(e->damage==((generator||hoth||shaft||rift||korribanRoom||sentinelRoom||factoryRoom)?Q3_INFINITE:50));
            if(hoth)CHECK(e->spawnflags==(4|8|32)&&e->wait==0);
            ++hurts;hazard=e;
        }
        if(e->item){
            CHECK((rift||korribanRoom||sentinelRoom||tatooineRoom||taspirRoom||factoryRoom||ctfBrushes)&&G_BSPSectionPickupRoot(*e)==root&&e->s.eType==ET_ITEM&&e->linked);
            CHECK(e->e_TouchFunc==touchF_Touch_Item&&e->nextthink==0&&(e->svFlags&SVF_BROADCAST));
            if(!strcmp(e->classname,"weapon_repeater"))riftPickup=e;
            if(sentinelRoom&&!strcmp(e->classname,"weapon_concussion_rifle"))sentinelAmmo=e;
            if(sentinelRoom&&!strcmp(e->classname,"weapon_det_pack"))sentinelDetPack=e;
            ++pickups;
        }
        if(jar::bspSection::ShaftLift(*e)){
            if(shaft){
                CHECK(e->alt_fire&&e->speed==500&&e->wait==3000&&e->delay==200&&e->moverState==MOVER_POS1);
                CHECK(e->flags&jarDetachedMoverAreaPortal);
                CHECK(std::fabs(Distance(e->pos1,e->pos2)-264)<.02f);
                shaftLifts[e->s.modelindex-root->s.modelindex-2]=e;
            }
        }
        if(sentinelRoom&&jar::bspSection::SectionLiftRoot(*e)==root)sentinelLifts.push_back(e);
        if(G_BSPSectionBrushPad(*e))sentinelPads.push_back(e);
        if(e->s.time2==BSPSectionPadTag){
            CHECK(G_BSPSectionBoxTagged(*e)&&e->wait==4&&e->delay==200);
            shaftPads[shaftPads[0]?1:0]=e;
        }
        if(!strcmp(e->classname,"misc_model_static")){
            CHECK(e->s.eType==ET_GENERAL&&e->linked&&!e->contents);
            if(factoryRoom||hilltopsRoom||dropzoneRoom)CHECK(e->s.modelindex==17&&e->s.modelScale[0]==1);
            else if(wastelandRoom)CHECK((e->s.modelindex==17||e->s.modelindex==20)&&e->s.modelScale[0]==1);
            else if(taspirRoom)CHECK(e->s.modelindex==(!strcmp(e->model,jar::shaft::sceneryModels[0])?12:16)&&
                e->s.modelScale[0]>=1&&e->s.modelScale[0]<=2);
            else if(yavin)CHECK(e->s.modelindex==15&&e->s.modelScale[0]>=.5f&&e->s.modelScale[0]<=2);
            else CHECK(e->s.modelindex>=12&&e->s.modelindex<=14&&e->s.modelScale[2]==(e->s.modelindex==14?.5f:e->s.modelindex==13?.76f:1.f));
            ++sceneryCount;
        }
        if(!strcmp(e->classname,"fx_runner")){
            CHECK(e->s.eType==ET_GENERAL&&!e->soundSet&&!e->fxFile);
            if(yavin&&e->spawnflags==2){CHECK(e->e_UseFunc==useF_fx_runner_use&&e->fxID==5&&!e->noise_index&&e->nextthink==-1);++dust;}
            else{CHECK(e->e_ThinkFunc==thinkF_fx_runner_link&&e->fxID==4&&e->noise_index>0&&e->nextthink==400);++fires;}
        }
        if(!strcmp(e->classname,"func_timer"))yavinTimer=e;
    }
    CHECK(starts==report.entryPoints);
    CHECK(skyPoints==(hoth||yavin?1:0));
    vec3_t skyOrigin;
    CHECK(G_BSPSectionSkyOrigin(*root,skyOrigin)==(hoth||yavin));
    const vec3_t expectedSky={yavin?32.f:4352.f,yavin?52.f:1024.f,yavin?-832.f:640.f};
    if(hoth||yavin){
        CHECK(Near(skyOrigin,expectedSky));
        CHECK(Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size()))==(yavin?jar::yavinSky::checksum:jar::hothSky::checksum));
    }
    CHECK(bobs==(profile->mode==jar::bspSection::Mode::Bobbing?1:0)&&fires==(profile->mode==jar::bspSection::Mode::Fire?6:korribanRoom?jar::korriban::fireCount:yavin?16:0));
    CHECK(dust==(yavin?74:0));
    CHECK(speakers==(profile->mode==jar::bspSection::Mode::Stationary?(path?4:1):
        profile->mode==jar::bspSection::Mode::Breakable?2:hoth?8:shaft?3:generator?1:korribanRoom?jar::korriban::windCount:sentinelRoom?12:coruscantRoom?41:hilltopsRoom?36:dropzoneRoom?jar::dropzone::speakerCount:0));
    CHECK(brushes.size()==(profile->mode==jar::bspSection::Mode::Breakable?69:yavin?30:factoryRoom?ARRAY_LEN(jar::factory::windows):0));
    CHECK(hurts==(rift?2:korribanRoom?8:coruscantRoom?3:hilltopsRoom||wastelandRoom?4:dropzoneRoom?5:profile->mode==jar::bspSection::Mode::Hurt||shaft||sentinelRoom||taspirRoom||factoryRoom?1:0));
    CHECK(sceneryCount==(shaft||wastelandRoom?6:yavin?51:taspirRoom?jar::taspir::sceneryCount:factoryRoom||hilltopsRoom||dropzoneRoom?2:0));
    CHECK(pickups==(rift?jar::rift::pickupCount:korribanRoom?jar::korriban::pickupCount:sentinelRoom?jar::sentinel::pickupCount:
        tatooineRoom?jar::tatooine::pickupCount:taspirRoom?jar::taspir::pickupCount:factoryRoom?jar::factory::pickupCount:
        coruscantRoom?jar::coruscant::pickupCount:hilltopsRoom?jar::hilltops::pickupCount:dropzoneRoom?jar::dropzone::pickupCount:
        wastelandRoom?jar::wasteland::pickupCount:0));
    const auto checkRiftContact=[&]{
        CHECK(riftUnion);
        vec3_t axes[3];AnglesToAxis(riftUnion->currentAngles,axes);
        const vec3_t samples[3]={{6000,2000,-1000},{8000,4500,-1000},{8000,2000,-1000}};
        for(int i=0;i<3;++i){
            vec3_t world;VectorCopy(riftUnion->currentOrigin,world);
            for(int a=0;a<3;++a)VectorMA(world,samples[i][a],axes[a],world);
            CHECK(G_BSPSectionHurtContact(*riftUnion,world,world)==(i<2));
        }
    };
    if(rift){
        checkRiftContact();
    }
    if(rift||korribanRoom){
        CHECK(riftPickup&&BSPSectionSchedulePickup(*riftPickup));
        CHECK(riftPickup->nextthink==5000&&riftPickup->e_ThinkFunc==thinkF_RespawnItem);
        CHECK((riftPickup->s.eFlags&EF_NODRAW)&&!riftPickup->contents&&riftPickup->e_TouchFunc==touchF_NULL);
    }
    if(shaft){
        CHECK(starts==9&&Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size()))==jar::shaft::checksum);
        for(int i=0;i<2;++i){
            CHECK(shaftLifts[i]&&shaftPads[i]&&!strcmp(shaftLifts[i]->targetname,shaftPads[i]->target));
            vec3_t center,axis[3];
            AnglesToAxis(shaftPads[i]->currentAngles,axis);
            VectorCopy(shaftPads[i]->currentOrigin,center);
            for(int a=0;a<3;++a)VectorMA(center,(report.padMins[i][a]+report.padMaxs[i][a])*.5f,axis[a],center);
            CHECK(G_BSPSectionBoxContact(*shaftPads[i],center,center));
            CHECK(!G_BSPSectionBoxContact(*shaftPads[1-i],center,center));
        }
    }
    std::vector<gentity_t*> travelers;
    CHECK(G_BSPSectionTeleportMembers(*root,travelers)&&travelers==members);
    for(auto* e:travelers){
        vec3_t original,landing,base,authored;
        VectorCopy(e->currentOrigin,original);VectorCopy(e->s.pos.trBase,base);VectorCopy(e->s.origin,authored);
        const auto time=e->s.pos.trTime,duration=e->s.pos.trDuration,nextthink=e->nextthink;
        const auto flags=e->s.eFlags;const auto linked=e->linked;const auto trajectory=e->s.pos.trType;
        const vec3_t offset={128,-96,64};VectorAdd(original,offset,landing);
        G_BSPSectionTranslateChild(*e,landing);
        vec3_t expected;VectorAdd(base,offset,expected);CHECK(Near(e->s.pos.trBase,expected));
        VectorAdd(authored,offset,expected);CHECK(Near(e->s.origin,expected));
        CHECK(Near(e->currentOrigin,landing)&&e->s.pos.trType==trajectory&&e->s.pos.trTime==time&&
              e->s.pos.trDuration==duration&&e->nextthink==nextthink&&e->linked==linked);
        G_BSPSectionTranslateChild(*e,original);
        CHECK(Near(e->currentOrigin,original)&&Near(e->s.pos.trBase,base)&&e->s.eFlags==flags);
    }
    root->pos1[0]+=1;CHECK(!G_BSPSectionTeleportMembers(*root,travelers));root->pos1[0]-=1;
    std::vector<std::array<float,3>> before;for(auto*e:members)before.push_back({e->currentOrigin[0],e->currentOrigin[1],e->currentOrigin[2]});
    vec3_t next{1400,1600,700},rotation{20,90,-10};G_SetOrigin(root,next);G_SetAngles(root,rotation);CHECK(G_BSPSectionMoved(*root));
    if(rift)checkRiftContact();
    for(auto* fall:korribanFalls){
        vec3_t axis[3],center;
        AnglesToAxis(fall->currentAngles,axis);
        VectorCopy(fall->currentOrigin,center);
        for(int a=0;a<3;++a)VectorMA(center,(fall->s.origin2[a]+fall->s.angles2[a])*.5f,axis[a],center);
        CHECK(G_BSPSectionHurtContact(*fall,center,center));
    }
    for(size_t i=0;i<members.size();++i){vec3_t local,rotated,expected;VectorSubtract(before[i].data(),origin,local);jar::angular::RebaseVector(local,angles,rotation,rotated);VectorAdd(next,rotated,expected);CHECK(Near(members[i]->currentOrigin,expected));}
    if(bob){vec3_t worldUp,expected;VectorSet(worldUp,0,0,20);jar::angular::RebaseVector(worldUp,angles,rotation,expected);
        CHECK(Near(bob->s.pos.trDelta,expected)&&bob->s.pos.trDuration==3000&&bob->s.pos.trType==TR_SINE);}
    for(auto*child:members)if(!strcmp(child->classname,"fx_runner")){
        vec3_t forward,up={0,0,1},expected;AngleVectors(child->currentAngles,forward,nullptr,nullptr);
        if(yavin){
            G_SubBSPCapture scope(child);
            const auto* aim=G_Find(nullptr,FOFS(targetname),child->target);
            CHECK(aim&&G_SubBSPOwner(aim)==root);
            VectorSubtract(aim->currentOrigin,child->currentOrigin,expected);VectorNormalize(expected);
            CHECK(Near(forward,expected));
        }else{
            jar::angular::RebaseVector(up,vec3_origin,rotation,expected);CHECK(Near(forward,expected)&&child->fxID==4&&child->noise_index>0&&child->nextthink==400);
        }
    }
    if(hazard){
        vec3_t axis[3],center,lo,hi,hullLo,hullHi;
        AnglesToAxis(hazard->currentAngles,axis);VectorCopy(hazard->currentOrigin,center);
        for(int i=0;i<3;++i)VectorMA(center,(hazard->s.origin2[i]+hazard->s.angles2[i])*.5f,axis[i],center);
        VectorSet(lo,center[0]-2,center[1]-2,center[2]-2);VectorSet(hi,center[0]+2,center[1]+2,center[2]+2);
        CHECK(G_BSPSectionHurtContact(*hazard,lo,hi));
        jar::bspSection::Hull(hazard->s.origin2,hazard->s.angles2,rotation,hullLo,hullHi);
        if(hazard->s.time2==BSPSectionRiftUnionTag){
            const auto& box=jar::rift::fallBoxes[1];
            vec3_t secondLo,secondHi;
            jar::bspSection::Hull(box.mins,box.maxs,rotation,secondLo,secondHi);
            AddPointToBounds(secondLo,hullLo,hullHi);AddPointToBounds(secondHi,hullLo,hullHi);
        }
        CHECK(Near(hazard->mins,hullLo)&&Near(hazard->maxs,hullHi));
    }
    if(!brushes.empty()){
        {
            BSPSectionMotionScope motion(*root);CHECK(motion&&motion.Mask(MASK_SOLID)&CONTENTS_BODY);
            for(auto*brush:brushes)CHECK(!brush->linked);
        }
        for(auto*brush:brushes)CHECK(brush->linked);
        // A rider blocks motion before any brushes are unlinked.
        auto*rider=allocate("fixture_rider");rider->s.groundEntityNum=brushes.front()->s.number;
        {BSPSectionMotionScope motion(*root);CHECK(!motion&&brushes.front()->linked);}
        CHECK(!G_BSPSectionSupportsActors(*root));
        gclient_t riderClient{};
        rider->client=&riderClient;
        CHECK(G_BSPSectionSupportsActors(*root));
        rider->s.groundEntityNum=ENTITYNUM_NONE;
        riderClient.ps.groundEntityNum=root->s.number;
        CHECK(G_BSPSectionSupportsActors(*root));
        riderClient.ps.groundEntityNum=ENTITYNUM_NONE;
        Vehicle_t landed{};
        landed.m_LandTrace.entityNum=brushes.front()->s.number;
        landed.m_LandTrace.fraction=.5f;
        rider->m_pVehicle=&landed;
        CHECK(G_BSPSectionSupportsActors(*root));
        landed.m_LandTrace.fraction=1;
        CHECK(!G_BSPSectionSupportsActors(*root));
        rider->m_pVehicle=nullptr;
        rider->client=nullptr;
        G_FreeEntity(rider);
        auto*first=brushes.front();vec3_t old;VectorCopy(first->currentOrigin,old);
        first->currentOrigin[0]+=10000;
        {BSPSectionMotionScope motion(*root);CHECK(!motion&&first->linked);}
        VectorCopy(old,first->currentOrigin);
        removedBrush=brushes.back();G_FreeEntity(removedBrush);
        std::vector<gentity_t*> survivors;CHECK(G_SubBSPMembers(root,survivors));
        CHECK(survivors.size()+1==members.size()&&!G_BSPSectionBreakable(*removedBrush));
        CHECK(G_EntityLifetime(removedBrush)==0);
        // Reuse the former brush slot as an unrelated entity.
        removedBrush->inuse=qtrue;removedBrush->classname=G_NewString("fixture_reused");removedBrush->contents=0;
        G_SubBSPAllocated(removedBrush);CHECK(!G_SubBSPOwner(removedBrush));
    }
    const int previousLinks=links;CHECK(G_BSPSectionMoved(*root)&&links==previousLinks);
    // A reconstructed copy has independent ownership, labels and sound entities.
    auto*copy=Spawn(next,rotation);std::vector<gentity_t*> copied;CHECK(G_SubBSPMembers(copy,copied)&&copied.size()==members.size());
    CHECK(G_SubBSPNamespace(copy)!=G_SubBSPNamespace(root));for(size_t i=0;i<copied.size();++i)CHECK(copied[i]!=members[i]&&(members[i]==removedBrush||Near(copied[i]->currentOrigin,members[i]->currentOrigin)));
    if(wastelandRoom){
        using namespace jar::wasteland;
        CHECK(Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size()))==checksum);
        const auto brush=[](const std::vector<gentity_t*>& owned,const gentity_t& room,int model){
            const auto item=std::find_if(owned.begin(),owned.end(),[&](const gentity_t* child){
                return child->bmodel&&child->s.modelindex-room.s.modelindex==model;
            });
            CHECK(item!=owned.end());return *item;
        };
        for(const auto& leaf:doorLeaves){
            CHECK(DoorTeamMember(*root,*brush(members,*root,leaf.model))&&
                  DoorTeamMember(*copy,*brush(copied,*copy,leaf.model)));
        }
        for(const auto& spec:liftPads){
            auto* pad=brush(members,*root,spec.model);
            CHECK(G_BSPSectionBrushPad(*pad)&&pad->delay==200&&pad->wait==6);
            G_UseTargets(pad,&g_entities[0]);
            CHECK(sectionTargets==std::vector<gentity_t*>{brush(members,*root,lifts[spec.lift].model)});
            sectionTargets.clear();
        }
        for(int model:platforms)
            CHECK(std::fabs(Distance(brush(members,*root,model)->pos1,brush(members,*root,model)->pos2)-384)<.02f);
        std::vector<gentity_t*> localGuns,copyGuns;
        for(auto* child:members)if(GunEntity(*child))localGuns.push_back(child);
        for(auto* child:copied)if(GunEntity(*child))copyGuns.push_back(child);
        CHECK(localGuns.size()==2&&copyGuns.size()==2);
        gentity_t& disabled=*localGuns[0];
        disabled.health=0;
        disabled.takedamage=qfalse;
        disabled.e_ThinkFunc=thinkF_wasteland_gun_respawn;
        disabled.nextthink=level.time+9000;
        localGuns[1]->count=100;
        const int now=level.time;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.guns.size()==2&&state.lifts.size()==8&&
              state.pads.size()==2&&state.hazards.size()==4);
        level.time+=300;
        CHECK(jar::bspSection::RestoreCopyState(*copy,state));
        CHECK(copyGuns[0]->health==0&&copyGuns[0]->e_ThinkFunc==thinkF_wasteland_gun_respawn&&
              copyGuns[0]->nextthink==level.time+9000&&!copyGuns[0]->takedamage);
        CHECK(disabled.nextthink==now+9000&&copyGuns[1]->health==gunHealth&&copyGuns[1]->count==100);
        ChunkSave savedGun;ojk::SavedGameHelper h(&savedGun);
        copyGuns[0]->sg_export(h);h.write_chunk(INT_ID('W','G','U','N'));h.read_chunk(INT_ID('W','G','U','N'));
        gentity_t restoredGun{};restoredGun.sg_import(h);h.ensure_all_data_read();
        CHECK(restoredGun.s.time2==gunTag&&restoredGun.e_ThinkFunc==thinkF_wasteland_gun_respawn&&
              restoredGun.nextthink==copyGuns[0]->nextthink&&!restoredGun.takedamage);
        level.time=now;
    }
    if(dropzoneRoom){
        using namespace jar::dropzone;
        CHECK(Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size()))==checksum);
        const auto member=[&](const std::vector<gentity_t*>& list,const gentity_t& room,int model){
            const auto found=std::find_if(list.begin(),list.end(),[&](const gentity_t* child){
                return child->bmodel&&child->s.modelindex==room.s.modelindex+model;
            });
            CHECK(found!=list.end());return *found;
        };
        auto* host=allocate("fixture_host_lift");host->targetname=G_NewString("t56");host->e_UseFunc=useF_Use_BinaryMover;
        for(const auto& spec:movers){
            const auto* mover=member(members,*root,spec.model);
            CHECK(BSPSectionLiftFits(*root,*mover)&&mover->damage==spec.damage&&mover->delay==spec.delay);
            const float localZ=spec.Platform()?0.f:spec.flags&1?1.f:spec.angle==-1?1.f:-1.f;
            if(spec.Platform())CHECK(mover->owner==mover&&std::fabs(Distance(mover->pos1,mover->pos2)-376.f)<.02f);
            else {
                vec3_t basis[3],direction;
                AnglesToAxis(root->currentAngles,basis);VectorSubtract(mover->pos2,mover->pos1,direction);
                VectorNormalize(direction);CHECK(std::fabs(DotProduct(direction,basis[2])-localZ)<.002f);
            }
        }
        for(const auto& spec:pads){
            auto* pad=member(members,*root,spec.model);
            CHECK(G_BSPSectionBrushPad(*pad)&&pad->delay==spec.delay&&pad->wait==spec.wait);
            G_UseTargets(pad,&g_entities[0]);
            CHECK(sectionTargets==std::vector<gentity_t*>{member(members,*root,movers[spec.lift].model)});
            sectionTargets.clear();
        }
        G_FreeEntity(host);
        const int now=level.time;
        auto* pad=member(members,*root,12);
        auto* platform=member(members,*root,11);
        const auto rest=*platform;
        SetMoverState(platform,MOVER_1TO2,now-250);
        EvaluateTrajectory(&platform->s.pos,now,platform->currentOrigin);
        pad->e_ThinkFunc=thinkF_multi_trigger_run;pad->nextthink=now+75;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.lifts.size()==ARRAY_LEN(movers)&&
              state.pads.size()==ARRAY_LEN(pads)&&state.hazards.size()==ARRAY_LEN(falls)&&state.speakers.size()==speakerCount);
        level.time=2000;
        CHECK(jar::bspSection::RestoreCopyState(*copy,state));
        auto* copiedPad=member(copied,*copy,12);
        auto* copiedPlatform=member(copied,*copy,11);
        CHECK(copiedPad->e_ThinkFunc==thinkF_multi_trigger_run&&copiedPad->nextthink==2075);
        CHECK(copiedPlatform->owner==copiedPlatform&&copiedPlatform->moverState==MOVER_1TO2&&copiedPlatform->s.pos.trTime==1750);
        CHECK(Near(copiedPlatform->currentOrigin,platform->currentOrigin));
        G_UseTargets(copiedPad,&g_entities[0]);
        CHECK(sectionTargets==std::vector<gentity_t*>{copiedPlatform});
        sectionTargets.clear();level.time=now;
        *platform=rest;
        pad->e_ThinkFunc=thinkF_NULL;pad->nextthink=0;
        SetMoverState(copiedPlatform,MOVER_POS1,now);
        copiedPad->e_ThinkFunc=thinkF_NULL;copiedPad->nextthink=0;
    }
    if(ctfLifts){
        const auto& pads=hilltopsRoom?jar::hilltops::pads:jar::coruscant::pads;
        const auto& lifts=hilltopsRoom?jar::hilltops::lifts:jar::coruscant::lifts;
        CHECK(sentinelPads.size()==ARRAY_LEN(pads));
        gentity_t* traffic=nullptr;
        int trafficCount=0,flagCount=0;
        for(auto* child:members){
            if(FactoryFlagRoot(*child)==root){CHECK(child->enemy&&FactoryFlagRoot(*child->enemy)==root);++flagCount;}
            if(jar::bspSection::TrafficSpeaker(*child)){
                CHECK(sectionSounds[child->noise_index-1]==(hilltopsRoom?jar::hilltops::exterior:jar::coruscant::flyby)&&!child->s.clientNum);
                CHECK((child->s.loopSound>0)==hilltopsRoom);
                traffic=child;++trafficCount;
            }
        }
        CHECK(flagCount==2&&trafficCount==(hilltopsRoom?8:6));
        const auto sameModel=[&](const std::vector<gentity_t*>& children,const gentity_t& room,int model){
            const auto found=std::find_if(children.begin(),children.end(),[&](const gentity_t* child){
                return child->bmodel&&child->s.modelindex-room.s.modelindex==model;
            });
            CHECK(found!=children.end());return *found;
        };
        auto* host=allocate("fixture_host_lift");host->targetname=G_NewString(lifts[0].target);host->e_UseFunc=useF_Use_BinaryMover;
        for(const auto& spec:pads){
            auto* pad=sameModel(members,*root,spec.model);
            CHECK(G_BSPSectionBrushPad(*pad)&&pad->delay==spec.delay&&pad->wait==spec.wait);
            G_UseTargets(pad,&g_entities[0]);
            CHECK(sectionTargets.size()==1&&sectionTargets[0]==sameModel(members,*root,lifts[spec.lift].model));
            sectionTargets.clear();
            if(spec.yaw>=0){
                const vec3_t localAngles={0,float(spec.yaw),0};vec3_t local,world;
                AngleVectors(localAngles,local,nullptr,nullptr);
                jar::angular::RebaseVector(local,vec3_origin,root->currentAngles,world);
                CHECK((pad->spawnflags&2)&&Near(world,pad->movedir));
            }
        }
        G_FreeEntity(host);
        const int now=level.time,oldDeadline=traffic->nextthink;
        const int firstModel=hilltopsRoom?3:1,secondModel=hilltopsRoom?4:2;
        auto* first=sameModel(members,*root,firstModel);auto* second=sameModel(members,*root,secondModel);
        first->e_ThinkFunc=thinkF_multi_trigger_run;first->nextthink=now+160;
        second->nextthink=now+925;traffic->nextthink=now+3250;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.pads.size()==14&&state.lifts.size()==6&&
              state.hazards.size()==size_t(hilltopsRoom?4:3)&&state.speakers.size()==size_t(hilltopsRoom?36:41));
        level.time=2000;
        const vec3_t pasteAt={-9200,-1700,950},facing={15,165,3};
        auto* pasted=Spawn(pasteAt,facing);
        CHECK(jar::bspSection::RestoreCopyState(*pasted,state));
        std::vector<gentity_t*> children;CHECK(G_SubBSPMembers(pasted,children));
        CHECK(sameModel(children,*pasted,firstModel)->nextthink==2160&&sameModel(children,*pasted,firstModel)->e_ThinkFunc==thinkF_multi_trigger_run);
        CHECK(sameModel(children,*pasted,secondModel)->nextthink==2925&&sameModel(children,*pasted,secondModel)->e_ThinkFunc==thinkF_NULL);
        const auto copiedTraffic=std::find_if(children.begin(),children.end(),[](const gentity_t* e){
            return jar::bspSection::TrafficSpeaker(*e)&&e->nextthink==5250;
        });
        CHECK(copiedTraffic!=children.end());
        CHECK(((*copiedTraffic)->s.loopSound>0)==hilltopsRoom);
        level.time=5250;section_traffic_think(*copiedTraffic);
        CHECK(trafficEvents==1&&(*copiedTraffic)->nextthink>=level.time+(hilltopsRoom?jar::hilltops::callMin:FRAMETIME)&&
              (*copiedTraffic)->nextthink<=level.time+(hilltopsRoom?jar::hilltops::callMax:18000));
        if(hilltopsRoom){
            (*copiedTraffic)->s.loopSound=0;
            level.time=(*copiedTraffic)->nextthink;section_traffic_think(*copiedTraffic);
            CHECK(trafficEvents==1&&(*copiedTraffic)->nextthink>=level.time+jar::hilltops::callMin);
        }
        G_FreeEntity(pasted);level.time=now;traffic->nextthink=oldDeadline;
        first->e_ThinkFunc=thinkF_NULL;first->nextthink=second->nextthink=0;
    }
    if(factoryRoom||dropzoneRoom||wastelandRoom){
        gentity_t* flags[2]{},*copyFlags[2]{};
        for(auto* child:members)if(FactoryFlagRoot(*child)==root)flags[jar::factory::FlagTeam(child->classname)-1]=child;
        for(auto* child:copied)if(FactoryFlagRoot(*child)==copy)copyFlags[jar::factory::FlagTeam(child->classname)-1]=child;
        CHECK(flags[0]&&flags[1]&&copyFlags[0]&&copyFlags[1]);
        auto& actor=g_entities[0];gclient_t client{};actor.client=&client;actor.inuse=qtrue;actor.health=100;
        VectorCopy(flags[0]->currentOrigin,actor.currentOrigin);
        factory_flag_use(flags[0],&actor,&actor);
        CHECK(root->count==1&&sectionFeedback.back().find("Red base selected")!=std::string::npos);
        factory_flag_use(copyFlags[0],&actor,&actor);
        VectorCopy(flags[1]->currentOrigin,actor.currentOrigin);
        factory_flag_touch(flags[1],&actor,nullptr);
        CHECK(!flags[1]->contents&&flags[1]->activator==&actor&&(flags[1]->s.eFlags&EF_NODRAW));
        CHECK(flags[1]->e_ThinkFunc==thinkF_factory_flag_think&&flags[1]->nextthink==250&&copyFlags[1]->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM));
        ChunkSave savedFlag;ojk::SavedGameHelper h(&savedFlag);
        flags[1]->sg_export(h);h.write_chunk(INT_ID('F','L','A','G'));h.read_chunk(INT_ID('F','L','A','G'));
        gentity_t restoredFlag{};restoredFlag.sg_import(h);h.ensure_all_data_read();
        CHECK(restoredFlag.e_ThinkFunc==thinkF_factory_flag_think&&restoredFlag.e_TouchFunc==touchF_factory_flag_touch&&
              restoredFlag.e_UseFunc==useF_factory_flag_use&&restoredFlag.nextthink==flags[1]->nextthink&&restoredFlag.s.eFlags==flags[1]->s.eFlags);
        VectorCopy(flags[0]->currentOrigin,actor.currentOrigin);
        factory_flag_touch(copyFlags[0],&actor,nullptr);
        CHECK(!copyFlags[0]->count&&flags[1]->activator==&actor);
        factory_flag_touch(flags[0],&actor,nullptr);
        CHECK(flags[0]->count==1&&flags[1]->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM)&&!flags[1]->activator&&
              sectionFeedback.back().find("Flag captured")!=std::string::npos);
        VectorCopy(flags[1]->currentOrigin,actor.currentOrigin);
        factory_flag_touch(flags[1],&actor,nullptr);
        actor.health=0;factory_flag_think(flags[1]);
        CHECK(flags[1]->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM)&&!flags[1]->activator&&flags[1]->nextthink==0);
        actor.health=100;factory_flag_touch(flags[1],&actor,nullptr);
        VectorSet(actor.currentOrigin,MAX_WORLD_COORD,MAX_WORLD_COORD,MAX_WORLD_COORD);factory_flag_think(flags[1]);
        CHECK(flags[1]->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM)&&!flags[1]->activator&&flags[0]->count==1);
        actor.client=nullptr;actor.inuse=qfalse;
        for(const auto* pane:brushes)if(pane!=removedBrush){
            const auto* window=jar::factory::Glass(pane->s.modelindex-root->s.modelindex);
            vec3_t verts[4],normal,basis[3];AnglesToAxis(root->currentAngles,basis);
            CHECK(window&&jar::bspSection::SectionGlassVertices(*pane,verts,normal)==window->shards);
            CHECK(std::fabs(VectorLength(normal)-1)<.001);
            for(int i=0;i<4;++i){
                vec3_t point;VectorCopy(root->currentOrigin,point);
                for(int d=0;d<3;++d)VectorMA(point,window->points[i][d],basis[d],point);
                CHECK(Near(point,verts[i]));
                vec3_t delta;VectorSubtract(point,verts[0],delta);CHECK(std::fabs(DotProduct(normal,delta))<.001);
            }
        }
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state));
        const vec3_t pasteAt={-9500,-1000,400};auto* pasted=Spawn(pasteAt,rotation);
        CHECK(jar::bspSection::RestoreCopyState(*pasted,state)&&!pasted->count);
        std::vector<gentity_t*> pastedChildren;CHECK(G_SubBSPMembers(pasted,pastedChildren));
        int panes=0;
        for(auto* child:pastedChildren){
            if(jar::bspSection::SectionGlass(*child))++panes;
            if(FactoryFlagRoot(*child)==pasted)CHECK(!child->count&&!child->activator&&child->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM));
        }
        CHECK(panes==(factoryRoom?3:0)&&flags[0]->count==1);
        G_FreeEntity(pasted);
    }
    if(speakers&&!ctfLifts){
        std::vector<gentity_t*> ambient;
        for(auto* child:members)if(jar::bspSection::AmbientSpeaker(*child))ambient.push_back(child);
        CHECK(ambient.size()==size_t(speakers));
        auto& muted=*ambient.back();
        const auto oldSound=muted.s.loopSound;
        muted.s.loopSound=0;
        const int now=level.time;
        muted.painDebounceTime=now+75;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.speakers.size()==ambient.size());
        level.time=3000;
        const vec3_t destination={-3000,2200,850},heading={12,130,-6};
        auto* pasted=Spawn(destination,heading);
        CHECK(jar::bspSection::RestoreCopyState(*pasted,state));
        std::vector<gentity_t*> children;CHECK(G_SubBSPMembers(pasted,children));
        int count=0;
        for(auto* child:children)if(jar::bspSection::AmbientSpeaker(*child)){
            const auto point=jar::bspSection::ChildPoint(*pasted,*child);
            const auto original=std::find_if(ambient.begin(),ambient.end(),[&](const gentity_t* speaker){
                return DistanceSquared(jar::bspSection::ChildPoint(*root,*speaker).data(),point.data())<.0625f;
            });
            CHECK(original!=ambient.end());
            CHECK(sectionSounds[child->noise_index-1]==sectionSounds[(*original)->noise_index-1]);
            CHECK(child->s.loopSound==(*original==&muted?0:child->noise_index));
            CHECK(child->painDebounceTime==(*original==&muted?3075:0));
            ++count;
        }
        CHECK(count==speakers&&muted.painDebounceTime==now+75&&!muted.s.loopSound);
        CHECK(sectionTargets.empty()&&!fireEvents&&!dustEvents);
        G_FreeEntity(pasted);level.time=now;muted.s.loopSound=oldSound;muted.painDebounceTime=0;
    }
    if(fires||hazard){
        const int now=level.time;
        level.time=1000;
        std::vector<gentity_t*> torches;
        for(auto* child:members)if(jar::bspSection::Torch(*child))torches.push_back(child);
        CHECK(torches.size()==size_t(fires));
        if(fires){
            fx_runner_link(torches[0]);torches[0]->nextthink=1075;
            fx_runner_link(torches[1]);fx_runner_use(torches[1],nullptr,nullptr);
            CHECK(torches[1]->nextthink==-1&&!torches[1]->s.loopSound);
        }
        if(hazard){
            hazard->painDebounceTime=level.time-325;
            hazard->aimDebounceTime=level.time;
            if(hazard->wait==0){hazard->svFlags|=SVF_INACTIVE;gi.unlinkentity(hazard);}
        }
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.fires.size()==torches.size()&&state.hazards.size()==size_t(hurts));
        level.time=3000;
        const vec3_t destination={-3000,2200,850},heading={12,130,-6};
        auto* timedCopy=Spawn(destination,heading);
        CHECK(jar::bspSection::RestoreCopyState(*timedCopy,state)&&!fireEvents&&!fireDamageTicks);
        std::vector<gentity_t*> children;CHECK(G_SubBSPMembers(timedCopy,children));
        if(fires){
            auto first=std::find_if(children.begin(),children.end(),[](const gentity_t* child){return jar::bspSection::Torch(*child)&&child->nextthink==3075;});
            auto stopped=std::find_if(children.begin(),children.end(),[](const gentity_t* child){return jar::bspSection::Torch(*child)&&child->nextthink==-1;});
            CHECK(first!=children.end()&&stopped!=children.end());
            CHECK((*first)->e_ThinkFunc==thinkF_fx_runner_think&&(*first)->s.loopSound==(*first)->noise_index&&!(*stopped)->s.loopSound);
            level.time=3075;fx_runner_think(*first);level.time=3000;
            CHECK(fireEvents==1&&fireDamageTicks==1&&torches[0]->nextthink==1075&&torches[1]->nextthink==-1);
            fireEvents=fireDamageTicks=0;
        }
        if(hazard){
            const auto moved=std::find_if(children.begin(),children.end(),[&](const gentity_t* child){
                return G_BSPSectionHurtTagged(*child)&&child->s.time2==hazard->s.time2&&Near(child->s.origin2,hazard->s.origin2);
            });
            CHECK(moved!=children.end()&&(*moved)->aimDebounceTime!=level.time);
            CHECK((*moved)->linked==hazard->linked&&((*moved)->svFlags&SVF_INACTIVE)==(hazard->svFlags&SVF_INACTIVE));
            if(hazard->wait>0)CHECK((*moved)->painDebounceTime+(*moved)->wait==3675&&hazard->painDebounceTime==675);
            hazard->painDebounceTime=hazard->aimDebounceTime=0;
            hazard->svFlags&=~SVF_INACTIVE;gi.linkentity(hazard);
        }
        G_FreeEntity(timedCopy);level.time=now;
    }
    if(removedBrush){
        auto& damaged=*brushes.front();
        const int local=damaged.s.modelindex-root->s.modelindex;
        const int originalHealth=damaged.health;
        if(damaged.health>1)--damaged.health;
        const int now=level.time;
        const int originalDustDeadline=yavin?yavinTimer->nextthink:0;
        if(yavin)yavinTimer->nextthink=now+600;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&
              state.brushHealth.size()==size_t(jar::bspSection::SectionBreakableSlots(profile->mode)));
        const auto missing=std::find(state.brushHealth.begin(),state.brushHealth.end(),0);
        CHECK(missing!=state.brushHealth.end()&&std::count(state.brushHealth.begin(),state.brushHealth.end(),0)==1);
        const int destroyedModel=int(missing-state.brushHealth.begin())+1;
        level.time=2000;
        const vec3_t destination={-3000,2200,850},heading={12,130,-6};
        auto* timedCopy=Spawn(destination,heading);
        const auto soundsBefore=sectionSounds.size();
        CHECK(jar::bspSection::RestoreCopyState(*timedCopy,state));
        std::vector<gentity_t*> children;CHECK(G_SubBSPMembers(timedCopy,children)&&children.size()+1==members.size());
        gentity_t* damagedCopy=nullptr;gentity_t* copiedTimer=nullptr;
        for(auto* child:children){
            if(G_BSPSectionBreakableRoot(*child)==timedCopy){
                const int model=child->s.modelindex-timedCopy->s.modelindex;
                CHECK(model!=destroyedModel);
                if(model==local)damagedCopy=child;
            }
            if(jar::bspSection::DustTimer(*child))copiedTimer=child;
        }
        CHECK(damagedCopy&&damagedCopy->health==damaged.health&&damagedCopy->takedamage&&damagedCopy->linked);
        CHECK(removedBrush->inuse&&G_SubBSPOwner(removedBrush)==nullptr&&sectionSounds.size()==soundsBefore&&sectionTargets.empty());
        if(yavin){
            CHECK(copiedTimer&&copiedTimer->nextthink==2600&&copiedTimer->activator==copiedTimer);
            G_UseTargets(copiedTimer,copiedTimer);
            CHECK(!sectionTargets.empty());
            for(const auto* target:sectionTargets)CHECK(G_SubBSPOwner(target)==timedCopy);
            sectionTargets.clear();dustEvents=0;
            CHECK(yavinTimer->nextthink==now+600);yavinTimer->nextthink=originalDustDeadline;
        }
        damaged.health=originalHealth;level.time=now;G_FreeEntity(timedCopy);
    }
    if(bob){
        const auto nativePath=bob->s.pos;
        vec3_t nativePosition;VectorCopy(bob->currentOrigin,nativePosition);
        const int now=level.time;
        const vec3_t destination={1800,-2100,700},heading={12,95,-8};
        auto* timedCopy=Spawn(destination,heading);
        std::vector<gentity_t*> timedChildren;CHECK(G_SubBSPMembers(timedCopy,timedChildren));
        const auto child=std::find_if(timedChildren.begin(),timedChildren.end(),[](const gentity_t* item){return jar::bspSection::Bobbing(*item);});
        CHECK(child!=timedChildren.end());
        auto& copiedBob=**child;
        for(bool running:{true,false}){
            level.time=1000;bob->s.pos.trType=running?TR_SINE:TR_INTERPOLATE;
            bob->s.pos.trTime=250;bob->radius=.25f;
            VectorMA(bob->s.origin,.75f,bob->s.pos.trDelta,bob->currentOrigin);
            jar::bspSection::CopyState state;
            CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.bobbers.size()==1);
            level.time=4000;
            CHECK(jar::bspSection::RestoreCopyState(*timedCopy,state)&&copiedBob.s.pos.trType==bob->s.pos.trType);
            vec3_t expected;VectorMA(copiedBob.s.origin,running?1.f:.75f,copiedBob.s.pos.trDelta,expected);
            CHECK(Near(copiedBob.currentOrigin,expected)&&copiedBob.s.pos.trTime==3250);
            if(!running)CHECK(copiedBob.radius==.25f&&Near(copiedBob.s.pos.trBase,expected));
        }
        G_FreeEntity(timedCopy);level.time=now;bob->s.pos=nativePath;bob->radius=0;
        VectorCopy(nativePosition,bob->currentOrigin);
    }
    if(sentinelRoom){
        CHECK(sentinelLifts.size()==10&&sentinelPads.size()==5&&sentinelAmmo&&sentinelDetPack);
        const auto* piston=sentinelLifts.front();
        CHECK(piston->soundSet&&!strcmp(piston->soundSet,jar::sentinel::pistonSet));
        auto* host=allocate("fixture_host_door");host->targetname=G_NewString(sentinelPads.front()->target);host->e_UseFunc=useF_Use_BinaryMover;
        for(size_t group=0;group<sentinelPads.size();++group){
            auto* pad=sentinelPads[group];
            CHECK(pad->s.modelindex-root->s.modelindex==jar::sentinel::pads[group].model&&pad->delay==200&&pad->wait==jar::sentinel::pads[group].wait);
            G_UseTargets(pad,&g_entities[0]);
            const size_t counts[]={5,1,1,2,1};
            CHECK(sectionTargets.size()==counts[group]);
            for(auto* lift:sectionTargets){
                CHECK(jar::bspSection::SectionLiftRoot(*lift)==root&&lift->delay==200&&lift->wait==3000&&lift->alt_fire);
                CHECK(BSPSectionLiftFits(*root,*lift));
            }
            sectionTargets.clear();
        }
        CHECK(BSPSectionSchedulePickup(*sentinelAmmo)&&sentinelAmmo->nextthink==5000);
        CHECK(BSPSectionSchedulePickup(*sentinelDetPack)&&sentinelDetPack->nextthink==40000);
        auto& lift=*sentinelLifts.front();
        lift.moverState=MOVER_1TO2;lift.s.pos.trType=TR_LINEAR_STOP;
        const int elapsed=lift.s.pos.trDuration/2;
        lift.s.pos.trTime=level.time-elapsed;
        lift.s.loopSound=23;
        auto& pad=*sentinelPads.front();pad.e_ThinkFunc=thinkF_multi_trigger_run;pad.nextthink=level.time+160;
        auto& raised=*sentinelLifts.back();raised.moverState=MOVER_POS2;
        raised.e_ThinkFunc=thinkF_ReturnToPos1;raised.nextthink=level.time+1200;
        sentinelPads.back()->nextthink=level.time+900;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.lifts.size()==10&&state.pads.size()==5&&state.pickups.size()==2);
        const int now=level.time;level.time=2000;
        const vec3_t copyAt={-3900,3300,900},copyAngles={0,160,0};
        auto* timedCopy=Spawn(copyAt,copyAngles);std::vector<gentity_t*> timedMembers;
        CHECK(G_SubBSPMembers(timedCopy,timedMembers)&&jar::bspSection::RestoreCopyState(*timedCopy,state));
        const auto copiedMember=[&](const gentity_t* source){return timedMembers[std::find(members.begin(),members.end(),source)-members.begin()];};
        auto* moving=copiedMember(&lift);vec3_t expected;
        VectorScale(moving->pos1,.5f,expected);VectorMA(expected,.5f,moving->pos2,expected);
        CHECK(moving->moverState==MOVER_1TO2&&moving->s.pos.trTime==level.time-elapsed&&moving->s.loopSound==23&&Distance(moving->currentOrigin,expected)<.2f);
        CHECK(copiedMember(&pad)->nextthink==2160&&copiedMember(&pad)->activator==copiedMember(&pad));
        CHECK(copiedMember(&raised)->moverState==MOVER_POS2&&copiedMember(&raised)->nextthink==3200&&
            Near(copiedMember(&raised)->currentOrigin,copiedMember(&raised)->pos2));
        CHECK(copiedMember(sentinelPads.back())->nextthink==2900&&copiedMember(sentinelPads.back())->e_ThinkFunc==thinkF_NULL);
        CHECK(copiedMember(sentinelAmmo)->nextthink==7000&&copiedMember(sentinelDetPack)->nextthink==42000);
        CHECK(sentinelAmmo->nextthink==5000&&pad.nextthink==160);
        BSPSectionRespawnPickup(*copiedMember(sentinelAmmo));
        CHECK(copiedMember(sentinelAmmo)->contents&CONTENTS_ITEM);
        CHECK(!(sentinelAmmo->contents&CONTENTS_ITEM));
        G_FreeEntity(timedCopy);
        level.time=now;lift.moverState=MOVER_POS1;lift.s.pos.trType=TR_STATIONARY;lift.s.pos.trTime=0;lift.s.loopSound=0;
        pad.e_ThinkFunc=thinkF_NULL;pad.nextthink=0;
        raised.moverState=MOVER_POS1;raised.e_ThinkFunc=thinkF_NULL;raised.nextthink=0;sentinelPads.back()->nextthink=0;
        BSPSectionRespawnPickup(*sentinelAmmo);BSPSectionRespawnPickup(*sentinelDetPack);
        CHECK(sentinelAmmo->e_TouchFunc==touchF_Touch_Item&&sentinelDetPack->e_TouchFunc==touchF_Touch_Item);
        G_FreeEntity(host);
    }
    if(tatooineRoom){
        CHECK(sentinelPads.size()==2&&report.ownedHelpers==1);
        gentity_t* platform=nullptr;
        gentity_t* trigger=nullptr;
        std::vector<gentity_t*> powerups;
        for(auto* child:members){
            if(!strcmp(child->classname,"func_plat"))platform=child;
            if(jar::bspSection::SectionMoverTrigger(*child))trigger=child;
            if(child->item&&child->item->giType==IT_POWERUP)powerups.push_back(child);
        }
        CHECK(platform&&trigger&&trigger->owner==platform&&platform->owner==platform);
        CHECK(Near(trigger->currentOrigin,platform->pos1));
        CHECK(Near(trigger->currentAngles,root->currentAngles));
        CHECK(platform->wait==1000&&platform->speed==250&&platform->damage==5000&&
              std::fabs(Distance(platform->pos1,platform->pos2)-296)<.02f);
        vec3_t basis[3];AnglesToAxis(trigger->currentAngles,basis);
        gentity_t rider{};gclient_t riderClient{};rider.client=&riderClient;
        VectorCopy(trigger->currentOrigin,rider.currentOrigin);
        for(int i=0;i<3;++i)VectorMA(rider.currentOrigin,.5f*(trigger->pos1[i]+trigger->pos2[i]),basis[i],rider.currentOrigin);
        Touch_PlatCenterTrigger(trigger,&rider,nullptr);
        CHECK(triggeredPlatform==platform);
        triggeredPlatform=nullptr;
        VectorMA(rider.currentOrigin,trigger->pos2[0]-trigger->pos1[0]+2,basis[0],rider.currentOrigin);
        Touch_PlatCenterTrigger(trigger,&rider,nullptr);
        CHECK(!triggeredPlatform);
        ChunkSave savedTrigger;ojk::SavedGameHelper triggerSave(&savedTrigger);
        trigger->sg_export(triggerSave);triggerSave.write_chunk(INT_ID('T','A','T','O'));triggerSave.read_chunk(INT_ID('T','A','T','O'));
        gentity_t restoredTrigger{};restoredTrigger.sg_import(triggerSave);triggerSave.ensure_all_data_read();
        CHECK(restoredTrigger.e_TouchFunc==touchF_Touch_PlatCenterTrigger&&Near(restoredTrigger.currentAngles,trigger->currentAngles)&&
              Near(restoredTrigger.pos1,trigger->pos1)&&Near(restoredTrigger.pos2,trigger->pos2));
        auto* host=allocate("fixture_host_door");
        host->targetname=G_NewString(sentinelPads.front()->target);
        host->e_UseFunc=useF_Use_BinaryMover;
        for(size_t group=0;group<sentinelPads.size();++group){
            auto* pad=sentinelPads[group];
            CHECK(pad->s.modelindex-root->s.modelindex==jar::tatooine::pads[group].model&&pad->wait==6&&pad->delay==200);
            G_UseTargets(pad,&g_entities[0]);
            CHECK(sectionTargets.size()==(group==0?4:1));
            for(auto* piston:sectionTargets){
                CHECK(jar::bspSection::SectionLiftRoot(*piston)==root&&piston->alt_fire&&piston->delay==200&&
                      piston->wait==3000&&!strcmp(piston->soundSet,jar::sentinel::pistonSet)&&BSPSectionLiftFits(*root,*piston));
            }
            sectionTargets.clear();
        }
        G_FreeEntity(host);
        CHECK(powerups.size()==3);
        for(auto* item:powerups)CHECK(BSPSectionSchedulePickup(*item)&&item->nextthink==120000);
        const auto rest=*platform;
        platform->moverState=MOVER_1TO2;
        platform->s.pos.trType=TR_NONLINEAR_STOP;
        const int elapsed=platform->s.pos.trDuration/2;
        platform->s.pos.trTime=level.time-elapsed;
        VectorSubtract(platform->pos2,platform->pos1,platform->s.pos.trDelta);
        VectorScale(platform->s.pos.trDelta,1000.f/platform->s.pos.trDuration,platform->s.pos.trDelta);
        EvaluateTrajectory(&platform->s.pos,level.time,platform->currentOrigin);
        auto* pad=sentinelPads.front();
        pad->e_ThinkFunc=thinkF_multi_trigger_run;pad->nextthink=level.time+150;
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.lifts.size()==6&&state.pads.size()==2&&state.pickups.size()==3);
        {BSPSectionMotionScope motion(*root);CHECK(motion&&!platform->linked&&trigger->linked);}
        const int now=level.time;level.time=2000;
        const vec3_t copyAt={-3900,3300,900},copyAngles={0,160,0};
        auto* timedCopy=Spawn(copyAt,copyAngles);
        std::vector<gentity_t*> timedMembers;
        CHECK(G_SubBSPMembers(timedCopy,timedMembers)&&jar::bspSection::RestoreCopyState(*timedCopy,state));
        const auto copiedMember=[&](const gentity_t* source){return timedMembers[std::find(members.begin(),members.end(),source)-members.begin()];};
        auto* moving=copiedMember(platform);
        auto* copiedTrigger=copiedMember(trigger);
        CHECK(moving->moverState==MOVER_1TO2&&moving->s.pos.trType==TR_NONLINEAR_STOP&&moving->s.pos.trTime==level.time-elapsed);
        CHECK(std::fabs(Distance(moving->currentOrigin,moving->pos1)-Distance(platform->currentOrigin,platform->pos1))<.02f);
        CHECK(copiedTrigger->owner==moving&&Near(copiedTrigger->currentOrigin,moving->pos1)&&Near(copiedTrigger->currentAngles,copyAngles));
        CHECK(copiedMember(pad)->nextthink==2150&&copiedMember(pad)->activator==copiedMember(pad));
        for(auto* item:powerups)CHECK(copiedMember(item)->nextthink==122000);
        G_FreeEntity(timedCopy);
        CHECK(!moving->inuse&&!copiedTrigger->inuse&&platform->inuse&&trigger->inuse);
        level.time=now;
        *platform=rest;
        pad->e_ThinkFunc=thinkF_NULL;pad->nextthink=0;
        for(auto* item:powerups)BSPSectionRespawnPickup(*item);
    }
    if(taspirRoom){
        using namespace jar::taspir;
        gentity_t* train=nullptr;
        std::vector<gentity_t*> automatic;
        for(auto* child:members){
            if(jar::bspSection::SectionTrainRoot(*child)==root)train=child;
            if(jar::bspSection::SectionMoverTrigger(*child))automatic.push_back(child);
        }
        CHECK(train&&train->s.pos.trType==TR_STATIONARY&&train->nextthink==3000&&train->s.loopSound==0);
        CHECK(automatic.size()==helperCount&&!strcmp(train->nextTrain->targetname,"floor1")&&
              !strcmp(train->soundSet,sounds[2].runtime));
        int doorTriggers=0;
        for(auto* trigger:automatic){
            const bool door=trigger->e_TouchFunc==touchF_Touch_DoorTrigger;
            if(door)++doorTriggers;
            CHECK(!strcmp(trigger->owner->soundSet,sounds[door?0:1].runtime));
            vec3_t basis[3];AnglesToAxis(trigger->currentAngles,basis);
            gentity_t rider{};gclient_t client{};rider.client=&client;
            VectorCopy(trigger->currentOrigin,rider.currentOrigin);
            for(int i=0;i<3;++i)VectorMA(rider.currentOrigin,.5f*(trigger->pos1[i]+trigger->pos2[i]),basis[i],rider.currentOrigin);
            if(door)Touch_DoorTrigger(trigger,&rider,nullptr);
            else Touch_PlatCenterTrigger(trigger,&rider,nullptr);
            CHECK(triggeredPlatform==trigger->owner);
            triggeredPlatform=nullptr;
        }
        CHECK(doorTriggers==int(ARRAY_LEN(doors)));
        const auto rest=*train;
        const int now=level.time;
        auto* host=allocate("host_floor_marker");host->targetname=G_NewString("floor2_stop");host->e_UseFunc=useF_Use_BinaryMover;
        level.time=train->nextthink;train->nextthink=0;Think_BeginMoving(train);
        CHECK(train->s.loopSound==23&&train->s.pos.trType==TR_NONLINEAR_STOP);
        ++level.time;Reached_Train(train);
        CHECK(sectionTargets.empty()&&!strcmp(train->nextTrain->targetname,"floor2_stop")&&train->s.pos.trDuration==4266);
        level.time+=train->s.pos.trDuration/2;
        EvaluateTrajectory(&train->s.pos,level.time,train->currentOrigin);
        jar::bspSection::CopyState phase;
        CHECK(jar::bspSection::CaptureCopyState(*root,phase)&&phase.lifts.size()==helperCount&&phase.trains.size()==1);
        {BSPSectionMotionScope motion(*root);CHECK(motion&&!train->linked);}
        const vec3_t destination={-2500,3100,930},heading={15,120,-20};
        auto* movingCopy=Spawn(destination,heading);
        CHECK(jar::bspSection::RestoreCopyState(*movingCopy,phase));
        std::vector<gentity_t*> children;
        CHECK(G_SubBSPMembers(movingCopy,children)&&children.size()==members.size());
        auto* copiedTrain=children[std::find(members.begin(),members.end(),train)-members.begin()];
        CHECK(jar::bspSection::SectionTrainRoot(*copiedTrain)==movingCopy&&
              jar::bspSection::SectionTrainCorner(*copiedTrain->nextTrain,*movingCopy)&&copiedTrain->nextTrain!=train->nextTrain);
        CHECK(copiedTrain->s.pos.trTime==train->s.pos.trTime&&copiedTrain->s.loopSound==23&&
              std::fabs(Distance(copiedTrain->currentOrigin,copiedTrain->pos1)-Distance(train->currentOrigin,train->pos1))<.02f);
        level.time=train->s.pos.trTime+train->s.pos.trDuration;
        Reached_Train(train);
        CHECK(!strcmp(train->nextTrain->targetname,"floor2")&&train->s.pos.trType==TR_STATIONARY&&train->nextthink==level.time+3000);
        CHECK(jar::bspSection::CaptureCopyState(*root,phase)&&phase.trains[0].remaining==3000);
        CHECK(jar::bspSection::RestoreCopyState(*movingCopy,phase)&&copiedTrain->nextthink==level.time+3000&&
              copiedTrain->e_ThinkFunc==thinkF_Think_BeginMoving&&copiedTrain->s.pos.trType==TR_STATIONARY);
        ChunkSave savedTrain;ojk::SavedGameHelper trainSave(&savedTrain);
        copiedTrain->sg_export(trainSave);trainSave.write_chunk(INT_ID('T','S','P','R'));trainSave.read_chunk(INT_ID('T','S','P','R'));
        gentity_t restoredTrain{};restoredTrain.sg_import(trainSave);trainSave.ensure_all_data_read();
        CHECK(restoredTrain.nextthink==copiedTrain->nextthink&&restoredTrain.e_ThinkFunc==thinkF_Think_BeginMoving&&
              restoredTrain.e_ReachedFunc==reachedF_Reached_Train&&Near(restoredTrain.currentOrigin,copiedTrain->currentOrigin));
        const char* nextStops[]={"floor3_stop","floor2a_stop","floor1_stop"};
        for(const auto* stop:nextStops){
            level.time=train->nextthink;train->nextthink=0;Think_BeginMoving(train);
            ++level.time;Reached_Train(train);
            CHECK(!strcmp(train->nextTrain->targetname,stop)&&train->s.pos.trDuration==4266);
            level.time+=train->s.pos.trDuration;
            Reached_Train(train);
            CHECK(train->s.pos.trType==TR_STATIONARY&&train->nextthink==level.time+3000);
        }
        CHECK(sectionTargets.empty()&&Near(train->currentOrigin,rest.currentOrigin));
        G_FreeEntity(movingCopy);G_FreeEntity(host);
        level.time=now;*train=rest;
    }
    if(rift||korribanRoom){
        const auto index=std::find(members.begin(),members.end(),riftPickup)-members.begin();
        CHECK(copied[index]->e_TouchFunc==touchF_Touch_Item&&riftPickup->nextthink==5000);
        jar::bspSection::CopyState state;
        CHECK(jar::bspSection::CaptureCopyState(*root,state)&&state.pickups.size()==1);
        CHECK(jar::bspSection::RestoreCopyState(*copy,state)&&copied[index]->nextthink==5000);
        CHECK(!(copied[index]->contents&CONTENTS_ITEM));
        BSPSectionRespawnPickup(*copied[index]);
        CHECK((copied[index]->contents&CONTENTS_ITEM)&&!(riftPickup->contents&CONTENTS_ITEM));
        ChunkSave savedItem;ojk::SavedGameHelper itemSave(&savedItem);
        riftPickup->sg_export(itemSave);itemSave.write_chunk(INT_ID('R','I','F','T'));itemSave.read_chunk(INT_ID('R','I','F','T'));
        gentity_t restoredItem{};restoredItem.sg_import(itemSave);itemSave.ensure_all_data_read();
        CHECK(restoredItem.s.time2==jar::rift::pickupTag&&restoredItem.nextthink==5000&&
              restoredItem.e_ThinkFunc==thinkF_RespawnItem&&(restoredItem.s.eFlags&EF_NODRAW));
        BSPSectionRespawnPickup(*riftPickup);
        CHECK(riftPickup->e_TouchFunc==touchF_Touch_Item&&riftPickup->contents==(CONTENTS_TRIGGER|CONTENTS_ITEM));
    }
    if(hoth||yavin){
        CHECK(G_BSPSectionSkyOrigin(*root,skyOrigin)&&Near(skyOrigin,expectedSky));
        CHECK(G_BSPSectionSkyOrigin(*copy,skyOrigin)&&Near(skyOrigin,expectedSky));
    }
    if(yavin){
        CHECK(yavinTimer&&yavinTimer->nextthink==FRAMETIME&&yavinTimer->wait==5&&yavinTimer->random==3);
        auto* host=allocate("fixture_host_dust");
        host->targetname=G_NewString("t562");host->e_UseFunc=useF_fx_runner_use;
        const int savedTime=level.time;level.time=yavinTimer->nextthink;
        func_timer_think(yavinTimer);
        CHECK(dustEvents==1&&sectionTargets.size()==2);
        CHECK(G_SubBSPOwner(sectionTargets[0])==root&&G_SubBSPOwner(sectionTargets[1])==root);
        CHECK(sectionTargets[1]->nextthink==-1&&yavinTimer->nextthink>=level.time+2000&&yavinTimer->nextthink<=level.time+8000);
        ChunkSave savedTimer;ojk::SavedGameHelper timerSave(&savedTimer);
        yavinTimer->sg_export(timerSave);timerSave.write_chunk(INT_ID('Y','A','V','N'));timerSave.read_chunk(INT_ID('Y','A','V','N'));
        gentity_t restoredTimer{};restoredTimer.sg_import(timerSave);timerSave.ensure_all_data_read();
        CHECK(restoredTimer.nextthink==yavinTimer->nextthink&&restoredTimer.e_ThinkFunc==thinkF_func_timer_think);
        level.time=savedTime;sectionTargets.clear();G_FreeEntity(host);
    }
    if(shaft){
        auto* host=allocate("fixture_host_lift");
        host->targetname=G_NewString("t29");host->e_UseFunc=useF_Use_BinaryMover;
        G_UseTargets(shaftPads[0],&g_entities[0]);
        CHECK(sectionTargets==std::vector<gentity_t*>{shaftLifts[0]});
        sectionTargets.clear();
        G_FreeEntity(host);
        for(size_t i=0;i<members.size();++i){
            if(jar::bspSection::ShaftLift(*members[i])){
                CHECK(jar::bspSection::ShaftLift(*copied[i])&&Near(copied[i]->pos1,members[i]->pos1)&&Near(copied[i]->pos2,members[i]->pos2));
                CHECK(!strcmp(copied[i]->targetname,members[i]->targetname));
            }
        }
        auto& lift=*shaftLifts[0];
        const auto rest=lift.s.pos;
        lift.moverState=MOVER_1TO2;lift.s.pos.trType=TR_LINEAR_STOP;lift.s.pos.trTime=1000;
        VectorSubtract(lift.pos2,lift.pos1,lift.s.pos.trDelta);
        VectorScale(lift.s.pos.trDelta,1000.f/lift.s.pos.trDuration,lift.s.pos.trDelta);
        VectorMA(lift.pos1,.1f,lift.s.pos.trDelta,lift.currentOrigin);
        lift.nextthink=0;
        const vec3_t turned={-18,36,43};
        vec3_t velocity,endpoint;
        jar::angular::RebaseVector(lift.s.pos.trDelta,rotation,turned,velocity);
        VectorSubtract(lift.pos2,next,endpoint);
        jar::angular::RebaseVector(endpoint,rotation,turned,endpoint);
        VectorAdd(endpoint,next,endpoint);
        G_SetAngles(root,turned);
        CHECK(G_BSPSectionMoved(*root)&&BSPSectionLiftFits(*root,lift));
        CHECK(Near(lift.s.pos.trDelta,velocity)&&Near(lift.pos2,endpoint)&&lift.s.pos.trTime==1000);
        ChunkSave savedLift;
        ojk::SavedGameHelper liftSave(&savedLift);
        lift.sg_export(liftSave);liftSave.write_chunk(INT_ID('S','H','F','T'));
        liftSave.read_chunk(INT_ID('S','H','F','T'));
        gentity_t restoredLift{};restoredLift.sg_import(liftSave);liftSave.ensure_all_data_read();
        CHECK(restoredLift.moverState==MOVER_1TO2&&Near(restoredLift.pos2,lift.pos2)&&
              restoredLift.s.pos.trTime==1000&&Near(restoredLift.s.pos.trDelta,velocity));
        const int oldTime=level.time;level.time=1100;
        shaftPads[0]->e_ThinkFunc=thinkF_multi_trigger_run;shaftPads[0]->nextthink=1220;
        shaftPads[1]->nextthink=1450;
        shaftLifts[1]->moverState=MOVER_POS2;shaftLifts[1]->e_ThinkFunc=thinkF_ReturnToPos1;
        shaftLifts[1]->nextthink=1800;
        jar::bspSection::CopyState motion;
        CHECK(jar::bspSection::CaptureCopyState(*root,motion)&&motion.lifts.size()==2&&motion.pads.size()==2);
        level.time=3000;
        const vec3_t destination={1800,-2100,700},heading={12,95,-8};
        auto* timedCopy=Spawn(destination,heading);
        CHECK(jar::bspSection::RestoreCopyState(*timedCopy,motion));
        std::vector<gentity_t*> timedChildren;CHECK(G_SubBSPMembers(timedCopy,timedChildren));
        gentity_t* pendingPad=nullptr;gentity_t* movingLift=nullptr;
        for(auto* child:timedChildren){
            if(jar::bspSection::ShaftLift(*child)&&child->s.modelindex==timedCopy->s.modelindex+2){
                movingLift=child;
                vec3_t expected;VectorMA(child->pos1,.1f,child->s.pos.trDelta,expected);
                CHECK(child->moverState==MOVER_1TO2&&child->s.pos.trTime==2900&&Near(child->currentOrigin,expected));
            }else if(jar::bspSection::ShaftLift(*child)){
                CHECK(child->moverState==MOVER_POS2&&child->nextthink==3700&&Near(child->currentOrigin,child->pos2));
            }else if(jar::bspSection::CopyablePad(*child)&&child->e_ThinkFunc==thinkF_multi_trigger_run){
                pendingPad=child;CHECK(child->nextthink==3120&&child->activator==child);
            }else if(jar::bspSection::CopyablePad(*child))CHECK(child->nextthink==3350);
        }
        CHECK(pendingPad&&movingLift);
        G_UseTargets(pendingPad,pendingPad);CHECK(sectionTargets==std::vector<gentity_t*>{movingLift});
        CHECK(shaftPads[0]->nextthink==1220&&shaftLifts[1]->nextthink==1800);
        sectionTargets.clear();G_FreeEntity(timedCopy);level.time=oldTime;
        shaftPads[0]->e_ThinkFunc=thinkF_NULL;shaftPads[0]->nextthink=shaftPads[1]->nextthink=0;
        shaftLifts[1]->moverState=MOVER_POS1;shaftLifts[1]->nextthink=0;
        G_SetAngles(root,rotation);CHECK(G_BSPSectionMoved(*root));
        lift.s.pos=rest;lift.moverState=MOVER_POS1;VectorCopy(lift.pos1,lift.currentOrigin);
    }
    // Earlier saves stored runtime section brushes as map-start baselines.
    copy->s.eFlags|=EF_PERMANENT;copy->svFlags&=~SVF_BROADCAST;
    for(auto*child:copied)if(child->bmodel&&child->s.eType==ET_MOVER){
        child->s.eFlags|=EF_PERMANENT;child->svFlags&=~SVF_BROADCAST;
    }
    static jar::workshop::State legacy;
    legacy={};
    legacy.nextOwned=9;
    CHECK(jar::workshop::InsertOwnership(legacy,0,{1,1200,1001,1}));
    CHECK(jar::workshop::InsertOwnership(legacy,1,{7,1201,1002,1}));
    copy->targetname=G_NewString("jar_sb_04");
    G_BSPSectionRestoreWorkshop(*copy,legacy);
    CHECK(legacy.ownedCount==2);
    copy->targetname=G_NewString("jar_sb_4");
    G_BSPSectionRestoreWorkshop(*copy,legacy);
    CHECK(legacy.ownedCount==3&&legacy.owned[0].id==1&&legacy.owned[2].id==7);
    const auto&recovered=legacy.owned[1];
    const auto* recoveredCard=jar::catalog::Find(recovered.catalog);
    CHECK(recovered.id==4&&recovered.entity==unsigned(copy->s.number)&&recovered.frozen);
    CHECK(recoveredCard&&!strcmp(recoveredCard->name,profile->card));
    CHECK(jar::workshop::OwnedEntity(legacy,copy->s.number)==&recovered);
    CHECK(jar::workshop::Valid(legacy,ENTITYNUM_WORLD));
    G_BSPSectionRestoreWorkshop(*copy,legacy);
    CHECK(legacy.ownedCount==3);
    CHECK(!(copy->s.eFlags&EF_PERMANENT)&&(copy->svFlags&SVF_BROADCAST));
    for(auto*child:copied)if(child->bmodel&&child->s.eType==ET_MOVER&&!(child->svFlags&SVF_NOCLIENT))
        CHECK(!(child->s.eFlags&EF_PERMANENT)&&(child->svFlags&SVF_BROADCAST));
    CHECK(!(copy->s.eFlags&EF_NODRAW));
    gclient_t client{};client.ps.groundEntityNum=ENTITYNUM_NONE;auto&player=g_entities[0];player.inuse=qtrue;player.health=100;player.client=&client;VectorSet(player.mins,-16,-16,-24);VectorSet(player.maxs,16,16,40);
    player.s.groundEntityNum=ENTITYNUM_NONE;
    if(ctfBrushes){brushHazardBlocks=true;CHECK(G_BSPSectionEnter(root,&player)&&teleports==0);brushHazardBlocks=false;}
    CHECK(G_BSPSectionEnter(copy,&player)&&teleports==1);player.s.m_iVehicleNum=1;CHECK(G_BSPSectionEnter(root,&player)&&teleports==1);player.s.m_iVehicleNum=0;
    if(hazard){
        vec3_t axes[3],center;AnglesToAxis(hazard->currentAngles,axes);VectorCopy(hazard->currentOrigin,center);
        for(int i=0;i<3;++i)VectorMA(center,(hazard->s.origin2[i]+hazard->s.angles2[i])*.5f,axes[i],center);
        // Even with a clear solid hull, every unsafe local entry is refused.
        std::vector<std::array<float,3>> entries;
        const auto isStart=[&](const gentity_t* child){return jar::bspSection::Start(child->message)||(capturePractice&&jar::factory::StartTeam(child->message));};
        for(auto*child:members)if(isStart(child)){
            entries.push_back({child->currentOrigin[0],child->currentOrigin[1],child->currentOrigin[2]});
            VectorCopy(center,child->currentOrigin);
        }
        CHECK(G_BSPSectionEnter(root,&player)&&teleports==1);
        size_t i=0;for(auto*child:members)if(isStart(child)){VectorCopy(entries[i].data(),child->currentOrigin);++i;}
    }
    blocked=true;CHECK(G_BSPSectionEnter(root,&player)&&teleports==1);blocked=false;
    // Real hold/freeze/save/release must not offset the section anchor twice.
    level.time=100;held=root->s.number;nativeBegin(*root,false);
    vec3_t center;jar::bspSection::Center(*root,center);VectorCopy(center,eyePosition);eyePosition[0]+=64-holdDistance;
    vec3_t oldRoot,oldChild;VectorCopy(root->currentOrigin,oldRoot);VectorCopy(members[0]->currentOrigin,oldChild);
    nativeFrame(.05f);CHECK(std::fabs(root->currentOrigin[0]-oldRoot[0]-51.2f)<.02f);
    CHECK(std::fabs(members[0]->currentOrigin[0]-oldChild[0]-51.2f)<.02f&&Near(root->pos1,root->currentOrigin));
    sectionHoldBlocked=true;VectorCopy(root->currentOrigin,oldRoot);nativeFrame(.05f);CHECK(Near(root->currentOrigin,oldRoot));sectionHoldBlocked=false;
    nativeBegin(*root,true);ChunkSave control;ojk::SavedGameHelper ch(&control);nativeSave(false,ch);nativeControls.clear();nativeRead(false,ch);
    CHECK(nativeControls.at(root->s.number).frozen);VectorCopy(members[0]->currentOrigin,oldChild);level.time+=200;nativeRelease(root->s.number,true);
    CHECK(Near(root->pos1,root->currentOrigin)&&Near(root->pos2,root->currentAngles)&&Near(oldChild,members[0]->currentOrigin));
    CHECK(root->s.pos.trType==TR_STATIONARY&&nativeControls.empty());held=ENTITYNUM_NONE;
    // Restoring a previous transform reuses the current anchor exactly once.
    G_SetOrigin(root,next);G_SetAngles(root,rotation);CHECK(G_BSPSectionMoved(*root));
    for(size_t i=0;i<members.size();++i)if(members[i]!=removedBrush)CHECK(Near(members[i]->currentOrigin,copied[i]->currentOrigin));
    vec3_t corners[8];jar::bspSection::Corners(*root,corners);jar::bspSection::Hull(root->mins,root->maxs,root->currentAngles,lo,hi);
    for(const auto&corner:corners)for(int a=0;a<3;++a)CHECK(corner[a]>=root->currentOrigin[a]+lo[a]-.02f&&corner[a]<=root->currentOrigin[a]+hi[a]+.02f);
    // Native entity serialization preserves the controller frame and callback.
    // Full save string/pointer relocation is outside this field-codec fixture.
    ChunkSave saved;ojk::SavedGameHelper h(&saved);root->sg_export(h);h.write_chunk(INT_ID('T','B','S','P'));h.read_chunk(INT_ID('T','B','S','P'));gentity_t restored;restored.sg_import(h);h.ensure_all_data_read();
    CHECK(Near(restored.pos1,root->pos1)&&Near(restored.pos2,root->pos2)&&restored.e_UseFunc==root->e_UseFunc&&restored.s.modelindex==root->s.modelindex);
    // A native callback may decline a child after successful preflight. Never
    // publish that half-built section or affect either complete instance.
    omitNextChild=true;auto*failed=allocate(profile->card);VectorCopy(origin,failed->s.origin);G_SetAngles(failed,angles);
    CHECK(!G_BSPSectionSpawn(failed)&&!failed->inuse&&root->inuse&&copy->inuse);
    if(removedBrush){
        displacedBrush=true;auto*invalid=allocate(profile->card);VectorCopy(origin,invalid->s.origin);G_SetAngles(invalid,angles);
        CHECK(!G_BSPSectionSpawn(invalid)&&!invalid->inuse&&root->inuse&&copy->inuse);displacedBrush=false;

        ChunkSave ownership;ojk::SavedGameHelper os(&ownership);G_SubBSPOwnershipExport(false,os);
        os.write_chunk(INT_ID('T','B','S','O'));G_SubBSPOwnershipReset();os.read_chunk(INT_ID('T','B','S','O'));
        CHECK(G_SubBSPOwnershipImport(false,os));os.ensure_all_data_read();
        std::vector<gentity_t*> survivors;CHECK(G_SubBSPMembers(root,survivors)&&survivors.size()+1==members.size());
        CHECK(!G_SubBSPOwner(removedBrush)&&G_BSPSectionBreakableRoot(*brushes.front())==root);

        ChunkSave native;ojk::SavedGameHelper state(&native);const auto&brush=*brushes.front();
        brush.sg_export(state);state.write_chunk(INT_ID('T','B','S','B'));state.read_chunk(INT_ID('T','B','S','B'));
        gentity_t restoredBrush;restoredBrush.sg_import(state);state.ensure_all_data_read();
        CHECK(restoredBrush.health==brush.health&&restoredBrush.s.modelindex==brush.s.modelindex&&
              restoredBrush.e_DieFunc==brush.e_DieFunc&&restoredBrush.e_PainFunc==brush.e_PainFunc);
    }
    const int survivor=copied[0]->s.number;G_FreeEntity(root);for(auto*e:members)CHECK(bool(e->inuse)==(e==removedBrush));
    if(removedBrush)G_FreeEntity(removedBrush);CHECK(g_entities[survivor].inuse&&G_SubBSPOwner(&g_entities[survivor])==copy);
    G_FreeEntity(copy);for(auto*e:copied)CHECK(!e->inuse);CHECK(!G_SubBSPHasOwnership());player.client=nullptr;
    printf("%s: %zu children, %d local entries, %d speakers, %d bobbing movers, %d fire emitters, %d native hurt brushes, %zu breakables; moved, copied, held, frozen, serialized, entered and cleaned up.\n",profile->title,members.size(),starts,speakers,bobs,fires,hurts,brushes.size());
    if(yavin)printf("Yavin: %d aimed dust emitters, %d scenery models, one isolated timer/random chain.\n",dust,sceneryCount);
}
static void WastelandDoorTeam(){
    using namespace jar::wasteland;
    auto* root=allocate("misc_bsp");
    root->s.modelindex=baseModels;
    auto* other=allocate("misc_bsp");
    other->s.modelindex=baseModels;
    CHECK(G_SubBSPBegin(root,true)&&G_SubBSPBegin(other,true));
    auto makeLeaves=[](gentity_t& room,gentity_t* (&brushes)[models]){
        G_SubBSPCapture capture(&room);
        for(const auto& spec:doorLeaves){
            auto* part=allocate("func_door");
            part->model=G_NewString(("*"+std::to_string(spec.model)).c_str());
            part->team=G_NewString(doorTeam);
            part->bmodel=qtrue;
            part->s.modelindex=room.s.modelindex+spec.model;
            part->s.eType=ET_MOVER;
            part->moverState=MOVER_POS1;
            part->e_ThinkFunc=thinkF_Think_SpawnNewDoorTrigger;
            const float x=16.f*spec.model;
            VectorSet(part->mins,x,-4,0);
            VectorSet(part->maxs,x+12,4,32);
            part->pos2[0]=spec.angle==180?-8:8;
            brushes[spec.model]=part;
        }
    };
    gentity_t* own[models]{},*neighbor[models]{};
    makeLeaves(*root,own);
    makeLeaves(*other,neighbor);
    auto* host=allocate("func_door");
    host->team=G_NewString(doorTeam);
    auto* last=own[16];
    own[16]=neighbor[16];
    CHECK(!LinkDoorTeam(*root,own));
    for(const auto& spec:doorLeaves)CHECK(!own[spec.model]->teammaster&&!own[spec.model]->teamchain);
    own[16]=last;
    auto* leader=LinkDoorTeam(*root,own);
    auto* neighborLeader=LinkDoorTeam(*other,neighbor);
    CHECK(leader==own[5]&&neighborLeader==neighbor[5]&&!host->teammaster);
    auto* part=leader;
    for(int model:{5,16,15,6}){
        CHECK(part==own[model]&&part->teammaster==leader);
        CHECK(bool(part->flags&FL_TEAMSLAVE)==(part!=leader));
        part=part->teamchain;
    }
    CHECK(!part&&!LinkDoorTeam(*root,own));
    {
        G_SubBSPCapture capture(root);
        Think_SpawnNewDoorTrigger(leader);
    }
    std::vector<gentity_t*> children;
    CHECK(G_SubBSPMembers(root,children)&&children.size()==5);
    auto* trigger=children.back();
    CHECK(trigger->owner==leader&&trigger->e_TouchFunc==touchF_Touch_DoorTrigger);
    CHECK(trigger->pos1[0]==79&&trigger->pos2[0]==269&&trigger->pos1[1]==-125&&trigger->pos2[1]==125);
    MatchTeam(leader,MOVER_1TO2,250);
    for(const auto& spec:doorLeaves)
        CHECK(own[spec.model]->moverState==MOVER_1TO2&&own[spec.model]->s.pos.trTime==250&&
              neighbor[spec.model]->moverState==MOVER_POS1);
    G_FreeEntity(root);
    CHECK(!trigger->inuse&&neighborLeader->inuse&&G_SubBSPOwner(neighbor[16])==other&&host->inuse);
    G_FreeEntity(other);
    G_FreeEntity(host);
}

static void SiegeKorribanEntity(){
    using namespace jar::siegeKorriban;
    const std::string cls=field("classname");
    auto* next=allocate(cls.c_str());
    auto* room=G_SubBSPOwner(next);
    CHECK(room);
    for(const auto& key:{std::make_pair("targetname",&gentity_t::targetname),std::make_pair("target",&gentity_t::target),
        std::make_pair("target3",&gentity_t::target3),std::make_pair("target4",&gentity_t::target4),
        std::make_pair("paintarget",&gentity_t::paintarget),
        std::make_pair("opentarget",&gentity_t::opentarget),std::make_pair("team",&gentity_t::team),
        std::make_pair("soundset",&gentity_t::soundSet),
        std::make_pair("message",&gentity_t::message),std::make_pair("model",&gentity_t::model),std::make_pair("fxfile",&gentity_t::fxFile)}){
        const char* value=field(key.first);
        if(value)next->*key.second=G_NewString(value);
    }
    point("origin",next->s.origin);
    point("angles",next->s.angles);
    G_SetOrigin(next,next->s.origin);
    G_SetAngles(next,next->s.angles);
    if(field("spawnflags"))next->spawnflags=atoi(field("spawnflags"));
    if(field("health"))next->health=atoi(field("health"));
    if(field("delay"))next->delay=atoi(field("delay"));
    if(field("wait"))next->wait=atof(field("wait"));
    if(G_BSPSectionPosition(next,room))return;
    if(G_BSPSectionScenery(next,room)){
        const float offset=field("zoffset")?atof(field("zoffset")):0;
        const float scale=field("modelscale")?atof(field("modelscale")):1;
        CHECK(next->s.time2==sceneryTag&&next->s.angles2[2]==offset&&
              Near(next->currentOrigin,next->s.origin)&&next->s.modelScale[0]==scale&&
              next->s.modelScale[1]==scale&&next->s.modelScale[2]==scale);
        return;
    }
    CHECK(G_SiegeKorribanPrepareMedia(*next));
    if(cls=="misc_siege_item")SP_misc_siege_item(next);
    else if(cls=="info_siege_objective")SP_info_siege_objective(next);
    else if(cls=="target_siege_end")SP_target_siege_end(next);
    else if(cls=="target_relay")next->e_UseFunc=useF_target_relay_use;
    else if(cls=="target_counter"){next->e_UseFunc=useF_target_counter_use;next->count=atoi(field("count"));}
    else if(cls=="trigger_multiple")SP_trigger_multiple(next);
    else if(cls=="func_door"){
        G_SpawnFloat("speed","0",&next->speed);
        SP_func_door(next);
    }else if(cls=="trigger_hurt"){
        InitTrigger(next);
        G_SpawnInt("dmg","5",&next->damage);
        next->spawnflags|=4;
        next->e_TouchFunc=touchF_hurt_touch;
        gi.linkentity(next);
    }
    else if(cls=="func_usable")SP_func_usable(next);
    else if(cls=="func_breakable")SP_func_breakable(next);
    else if(cls=="target_delay")SP_target_delay(next);
    else if(cls=="target_print")next->e_UseFunc=useF_Use_Target_Print;
    else if(cls=="fx_runner")SP_fx_runner(next);
    else CHECK(cls=="info_notnull"||cls==rancorClass);
    CHECK(G_SiegeKorribanPrepareBrush(*next));
}

// Exercise native BSP, entity-field and ownership serialization in load order.
// The fixture supplies the string heap, actor side records and engine imports.
static void ReloadSiegeKorriban(gentity_t& root,gentity_t& neighbor,gentity_t& crystal){
    constexpr auto entityChunk=INT_ID('G','E','N','T');
    constexpr auto ownershipChunk=INT_ID('J','B','S','O');
    constexpr auto stringChunk=INT_ID('S','T','R','G');
    const auto savedNamespace=G_SubBSPNamespace(&root);
    const int oldModel=root.s.modelindex;
    const int relocatedModel=oldModel+8;
    ChunkSave archive;
    gi.saved_game=&archive;
    JarWriteBSPRegistry(false,jar::workshopSaveCurrent);
    ojk::SavedGameHelper io(&archive);
    struct Slot {int index;gclient_t* client;gNPC_t* npc;};
    std::vector<Slot> slots;
    for(int index=MAX_CLIENTS;index<globals.num_entities;++index){
        const auto& live=g_entities[index];
        if(!live.inuse)continue;
        slots.push_back({index,live.client,live.NPC});
        gentity_t entity=live;
        std::vector<std::string> strings;
        for(const auto& field:savefields_gEntity){
            auto* address=reinterpret_cast<byte*>(&entity)+field.iOffset;
            if(G_ClearTransientSaveField(&field,&entity))continue;
            switch(field.eFieldType){
            case F_STRING:{
                auto& value=*reinterpret_cast<char**>(address);
                if(value)strings.emplace_back(value);
                value=reinterpret_cast<char*>(value?intptr_t(strings.back().size()+1):-1);
                break;
            }
            case F_GENTITY:{
                auto& value=*reinterpret_cast<gentity_t**>(address);
                value=reinterpret_cast<gentity_t*>(value?value-g_entities:-1);
                break;
            }
            case F_ITEM:
                CHECK(!entity.item);
                break;
            case F_BEHAVIORSET:
                for(const auto* behavior:entity.behaviorSet)CHECK(!behavior);
                break;
            case F_BOOLPTR:
            case F_GCLIENT:
            case F_IGNORE:
                break;
            default:
                CHECK(false);
            }
        }
        entity.client=nullptr;
        entity.NPC=nullptr;
        io.write_chunk(entityChunk,entity);
        for(const auto& value:strings)io.write_chunk(stringChunk,value.c_str(),int(value.size()+1));
    }
    io.reset_buffer();
    G_SubBSPOwnershipExport(false,io);
    io.write_chunk(ownershipChunk);
    CHECK(slots.size()==2*jar::siegeKorriban::entities+1);
    G_SiegeForget(&crystal);
    G_SubBSPOwnershipReset();
    for(const auto& slot:slots)g_entities[slot.index]=gentity_t{};
    gi.RegisterBSPModel=[](const char* name,int* first,int* count,unsigned* checksum){
        CHECK(!strcmp(name,jar::siegeKorriban::resource));
        *first=baseModels+8;
        *count=sectionModels;
        *checksum=LittleLong(Com_BlockChecksum(sectionBytes.data(),int(sectionBytes.size())));
        return qtrue;
    };
    JarReadBSPRegistry(false,jar::workshopSaveCurrent);
    for(const auto& slot:slots){
        auto& entity=g_entities[slot.index];
        io.read_chunk(entityChunk,entity);
        CHECK(entity.s.number==slot.index&&entity.inuse);
        for(const auto& field:savefields_gEntity){
            auto* address=reinterpret_cast<byte*>(&entity)+field.iOffset;
            if(field.eFieldType==F_STRING){
                auto& value=*reinterpret_cast<char**>(address);
                const int length=int(reinterpret_cast<intptr_t>(value));
                value=nullptr;
                if(length<0)continue;
                std::vector<char> string(length);
                io.read_chunk(stringChunk,string.data(),length);
                CHECK(length>0&&string.back()==0);
                value=G_NewString(string.data());
            }else if(field.eFieldType==F_GENTITY){
                auto& value=*reinterpret_cast<gentity_t**>(address);
                const auto index=reinterpret_cast<intptr_t>(value);
                CHECK(index>=-1&&index<globals.num_entities);
                value=index<0?nullptr:&g_entities[index];
            }
        }
        entity.client=slot.client;
        entity.NPC=slot.npc;
        JarLinkRestoredEntity(&entity);
    }
    JarResetBSPRestore();
    io.read_chunk(ownershipChunk);
    CHECK(G_SubBSPOwnershipImport(false,io));
    io.ensure_all_data_read();
    CHECK(G_BSPSectionPublish(root)&&G_BSPSectionPublish(neighbor));
    G_SiegeRestore();
    CHECK(root.s.modelindex==relocatedModel&&neighbor.s.modelindex==relocatedModel&&
          G_SubBSPNamespace(&root)!=savedNamespace&&G_SubBSPOwner(&crystal)==&root&&
          SiegeCarried()==&crystal);
    gi.saved_game=nullptr;
}

static void SiegeKorribanGraph(const char* path){
    using namespace jar::siegeKorriban;
    profile=jar::bspSection::Resource(resource);
    CHECK(profile&&profile->mode==jar::bspSection::Mode::SiegeKorriban);
    Init(path);
    sectionSpawnCallback=SiegeKorribanEntity;
    siegeMediaFixture=true;
    gi.AdjustAreaPortalState=[](gentity_t*,qboolean){CHECK(false);};
    gi.linkentity=[](gentity_t* e){
        e->linked=qtrue;
        if(e->bmodel&&e->contents)e->s.solid=SOLID_BMODEL;
    };
    auto build=[&](const vec3_t origin,const vec3_t angles){
        auto* room=allocate(profile->card);
        VectorCopy(origin,room->s.origin);
        VectorCopy(angles,room->s.angles);
        CHECK(BSPSectionSpawn(room,profile));
        CHECK(jar::bspSection::Portable(*room)&&(room->svFlags&SVF_BROADCAST)&&
              !(room->s.eFlags&EF_PERMANENT));
        return room;
    };
    const vec3_t origin={1000,2000,300},angles={20,35,10},neighborOrigin={21000,4000,500};
    auto* room=build(origin,angles);
    auto* neighbor=build(neighborOrigin,vec3_origin);
    std::vector<gentity_t*> own,other;
    CHECK(G_SubBSPMembers(room,own)&&G_SubBSPMembers(neighbor,other)&&
          own.size()==entities-1&&other.size()==entities-1);
    int scenery=0,offsets=0,ambient=0,torches=0,namedFX=0,locations=0;
    gentity_t* torch=nullptr;
    for(auto* child:own){
        if(child->s.time2==locationTag){
            CHECK(child->message&&child->message[0]=='@'&&!strcmp(child->classname,"info_notnull"));
            ++locations;
        }else if(child->s.time2==sceneryTag){
            ++scenery;if(child->s.angles2[2])++offsets;
        }else if(child->s.time2==speakerTag){
            CHECK(child->s.loopSound>0&&sectionSounds[child->s.loopSound-1]==lavaLoop);
            ++ambient;
        }else if(child->s.time2==effectTag){
            CHECK(!child->bmodel&&!child->contents);
            CHECK(child->s.eType==ET_GENERAL&&(child->svFlags&SVF_BROADCAST));
            if(child->spawnflags==4){
                CHECK(child->s.loopSound>0&&sectionSounds[child->s.loopSound-1]==torchLoop&&
                      child->nextthink>=200&&child->nextthink<400);
                torch=child;
                ++torches;
            }else ++namedFX;
        }
    }
    CHECK(locations==10&&scenery==sceneryCount&&offsets==11&&ambient==speakerCount&&torches==torchCount&&namedFX==6&&torch);
    fx_runner_think(torch);
    CHECK(siegeEffectsPlayed==1&&siegeTorchHits==1);
    const auto byName=[](const std::vector<gentity_t*>& members,const char* name,const char* classname=nullptr){
        const auto item=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->targetname&&!strcmp(child->targetname,name)&&(!classname||!strcmp(child->classname,classname));
        });
        CHECK(item!=members.end());return *item;
    };
    const auto byModel=[](const std::vector<gentity_t*>& members,const char* model){
        const auto item=std::find_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->model&&!strcmp(child->model,model);
        });
        CHECK(item!=members.end());return *item;
    };
    const auto entryGroup=[](const std::vector<gentity_t*>& members,const char* group){
        return std::count_if(members.begin(),members.end(),[&](const gentity_t* child){
            return child->targetname&&!strcmp(child->targetname,group)&&G_SiegeKorribanEntry(*child);
        });
    };
    vec3_t axes[3];
    AnglesToAxis(angles,axes);
    const auto placed=[&](const gentity_t& point,const char* stockOrigin){
        vec3_t local,world;
        CHECK(sscanf(stockOrigin,"%f %f %f",local,local+1,local+2)==3);
        VectorCopy(origin,world);
        for(int i=0;i<3;++i)VectorMA(world,local[i],axes[i],world);
        CHECK(Near(point.currentOrigin,world));
    };
    for(const auto& spec:waypoints){
        const auto* point=byName(own,spec.name,"waypoint");
        CHECK(!strcmp(point->model,waypointModel)&&!strcmp(point->target,spec.target)&&
              (point->svFlags&SVF_NOCLIENT)&&G_SubBSPOwner(point)==room&&G_SubBSPOwner(byName(other,spec.name))==neighbor);
        placed(*point,spec.origin);
    }
    const auto firstRancor=std::find_if(own.begin(),own.end(),[](const gentity_t* e){return G_SiegeKorribanRancorSpawner(*e);});
    CHECK(firstRancor!=own.end());
    placed(**firstRancor,rancorOrigin);
    gentity_t* sectionActor;
    gclient_t rancorClient{};
    gNPC_t rancorBrain{};
    {
        G_SubBSPCapture spawnContext(*firstRancor);
        sectionActor=allocate("NPC");
        sectionActor->NPC=&rancorBrain;
        sectionActor->client=&rancorClient;
        sectionActor->s.time2=rancorTag;
        rancorClient.NPC_class=CLASS_RANCOR;
        G_SiegeKorribanRancorAllegiance(*sectionActor);
    }
    CHECK(G_SiegeKorribanRancor(*sectionActor)&&G_SubBSPOwner(sectionActor)==room&&(sectionActor->svFlags&SVF_BROADCAST)&&
          rancorClient.playerTeam==TEAM_ENEMY&&rancorClient.enemyTeam==TEAM_PLAYER);
    auto& carrier=g_entities[0];
    gclient_t client{};
    carrier.inuse=qtrue;carrier.client=&client;carrier.health=100;
    VectorSet(carrier.mins,-15,-15,-24);
    VectorSet(carrier.maxs,15,15,32);
    client.playerTeam=TEAM_PLAYER;
    sectionActor->activator=&carrier;
    CHECK(!G_SiegeKorribanRancorHolding(*sectionActor));
    carrier.activator=sectionActor;
    client.ps.eFlags|=EF_HELD_BY_RANCOR;
    CHECK(G_SiegeKorribanRancorHolding(*sectionActor));
    carrier.activator=sectionActor->activator=nullptr;
    client.ps.eFlags&=~EF_HELD_BY_RANCOR;
    level.time=1000;
    blocked=true;
    CHECK(G_BSPSectionEnter(room,&carrier)&&!teleports&&room->count==int(Round::Waiting));
    blocked=false;
    CHECK(G_BSPSectionEnter(room,&carrier)&&teleports==1&&room->count==int(Round::Active)&&
          room->nextthink==level.time+roundDuration&&room->e_ThinkFunc==thinkF_siege_korriban_round_think);
    CHECK(entryGroup(own,groupNames[0])==15&&neighbor->count==int(Round::Waiting));
    CHECK(std::any_of(own.begin(),own.end(),[&](const gentity_t* e){
        return G_SiegeKorribanEntry(*e)&&Near(e->currentOrigin,carrier.currentOrigin);
    }));
    const int roomDeadline=room->nextthink;
    ++level.time;
    CHECK(G_BSPSectionEnter(room,&carrier)&&teleports==2&&room->nextthink==roomDeadline);
    {
        ChunkSave saved;
        ojk::SavedGameHelper state(&saved);
        constexpr auto chunk=INT_ID('S','K','R','T');
        room->sg_export(state);
        state.write_chunk(chunk);
        state.read_chunk(chunk);
        gentity_t restored{};
        restored.sg_import(state);
        state.ensure_all_data_read();
        CHECK(restored.count==int(Round::Active)&&restored.nextthink==roomDeadline&&
              restored.e_ThinkFunc==thinkF_siege_korriban_round_think);
    }
    int doorCount=0,volumeCount=0,lavaCount=0;
    for(const auto* child:own){
        if(jar::bspSection::SectionLiftRoot(*child)==room){
            CHECK(child->linked&&child->bmodel&&Near(child->currentAngles,angles));
            ++doorCount;
        }
        if(G_BSPSectionBrushPad(*child)||G_BSPSectionBrushHurt(*child)){
            CHECK(child->linked&&Near(child->currentAngles,angles)&&Near(child->currentOrigin,origin));
            ++volumeCount;
            if(child->s.time2==BSPSectionLavaTag){
                CHECK(child->damage==25&&child->wait==FRAMETIME);
                ++lavaCount;
            }
        }
    }
    CHECK(doorCount==14&&volumeCount==16&&lavaCount==4);
    vec3_t roomAxes[3];
    AnglesToAxis(angles,roomAxes);
    for(const auto& spec:doors){
        const auto name="*"+std::to_string(spec.model);
        const auto* door=byModel(own,name.c_str());
        const int axis=spec.angle==-2?2:0;
        const float distance=door->maxs[axis]-door->mins[axis]-spec.lip;
        vec3_t travel,expected;
        VectorSubtract(door->pos2,door->pos1,travel);
        VectorScale(roomAxes[axis],(spec.flags&MOVER_START_ON)?distance:-distance,expected);
        CHECK(Near(travel,expected)&&Near(door->pos1,door->currentOrigin)&&
              (!spec.sound?!door->soundSet:!strcmp(door->soundSet,doorSoundSet)));
    }
    auto* lava=byModel(own,"*1");
    gi.EntityContact=[](const vec3_t lo,const vec3_t hi,const gentity_t* volume){
        if(!VectorCompare(lo,hi))return qfalse;
        CHECK(volume->s.time2==BSPSectionLavaTag);
        return Near(lo,volume->currentOrigin)?qtrue:qfalse;
    };
    CHECK(SiegeLavaAt(*byName(own,"ragnos_coffin"),lava->currentOrigin)&&
          !SiegeLavaAt(*byName(other,"ragnos_coffin"),lava->currentOrigin));
    for(const auto& pad:pressurePads){
        const auto model="*"+std::to_string(pad.model);
        auto* trigger=byModel(own,model.c_str());
        auto* crusher=byName(own,pad.target);
        auto* untouched=byName(other,pad.target);
        CHECK(crusher->moverState==MOVER_POS1&&crusher->damage==999&&untouched->moverState==MOVER_POS1);
        Touch_Multi(trigger,&carrier,nullptr);
        CHECK(crusher->moverState==MOVER_1TO2&&crusher->s.pos.trTime==level.time+50&&
              trigger->nextthink==level.time+2000&&untouched->moverState==MOVER_POS1);
        ++level.time;
        Touch_Multi(trigger,&carrier,nullptr);
        CHECK(crusher->moverState==MOVER_1TO2);
        level.time=crusher->s.pos.trTime+crusher->s.pos.trDuration;
        Reached_BinaryMover(crusher);
        CHECK(crusher->moverState==MOVER_POS2&&crusher->nextthink==level.time+2000);
        level.time=crusher->nextthink;
        ReturnToPos1(crusher);
        level.time+=crusher->s.pos.trDuration;
        Reached_BinaryMover(crusher);
        CHECK(crusher->moverState==MOVER_POS1&&untouched->moverState==MOVER_POS1);
    }
    auto* stairs=byName(own,"scepterstairs");
    auto* stairButton=byName(own,"stairbutton");
    Use_BinaryMover(stairButton,&carrier,&carrier);
    CHECK(stairButton->moverState==MOVER_1TO2&&stairs->moverState==MOVER_POS1);
    level.time=stairButton->s.pos.trTime+stairButton->s.pos.trDuration;
    Reached_BinaryMover(stairButton);
    CHECK(stairs->moverState==MOVER_1TO2&&byName(other,"scepterstairs")->moverState==MOVER_POS1);
    auto* floorSwitch=byModel(own,"*54");
    auto* floor=byName(own,"crystalroom1_plat");
    Use_BinaryMover(floorSwitch,&carrier,&carrier);
    CHECK(floorSwitch->nextthink==level.time+1000&&floor->moverState==MOVER_POS1);
    level.time=floorSwitch->nextthink;
    Use_BinaryMover_Go(floorSwitch);
    level.time=floorSwitch->s.pos.trTime+floorSwitch->s.pos.trDuration;
    Reached_BinaryMover(floorSwitch);
    CHECK(floor->nextthink==level.time+3000&&floor->moverState==MOVER_POS1&&floor->damage==9999);
    level.time=floor->nextthink;
    Use_BinaryMover_Go(floor);
    CHECK(floor->moverState==MOVER_1TO2&&byName(other,"crystalroom1_plat")->moverState==MOVER_POS1);
    CHECK(entryGroup(own,groupNames[0])==15&&entryGroup(own,groupNames[1])==0&&entryGroup(own,groupNames[2])==0);
    int breakables=0,shields=0;
    for(const auto* child:own){
        if(G_BSPSectionBreakable(*child)){
            CHECK(child->linked&&child->bmodel&&child->contents==CONTENTS_SOLID&&child->e_DieFunc==dieF_funcBBrushDie);
            ++breakables;
        }else if(child->e_UseFunc==useF_func_usable_use){
            CHECK(child->linked&&child->bmodel&&child->contents==CONTENTS_SOLID&&
                  (child->flags&jarDetachedMoverAreaPortal));
            ++shields;
        }
    }
    CHECK(breakables==37&&shields==2);
    auto* gate=byName(own,"gateopen","info_siege_objective");
    auto* gateDelay=byName(own,"gateopen","target_delay");
    auto* gateBrush=byModel(own,"*64");
    CHECK(gateBrush->health==1500&&gateBrush->noDamageTeam==TEAM_ENEMY&&
          (gateBrush->flags&FL_DMG_BY_HEAVY_WEAP_ONLY)&&gateBrush->wait==10000&&
          G_SiegeKorribanHeavyHit(*gateBrush,MOD_SABER)&&G_SiegeKorribanHeavyHit(*gateBrush,MOD_ROCKET)&&
          !G_SiegeKorribanHeavyHit(*gateBrush,MOD_BLASTER));
    funcBBrushPain(gateBrush,&carrier,&carrier,vec3_origin,10,MOD_SABER,HL_NONE);
    funcBBrushPain(gateBrush,&carrier,&carrier,vec3_origin,10,MOD_SABER,HL_NONE);
    CHECK(siegePainOutputs==1&&gateBrush->painDebounceTime==level.time+10000);
    funcBBrushDie(gateBrush,&carrier,&carrier,1500,MOD_SABER);
    CHECK(!gateBrush->linked&&!gateBrush->contents&&!gateBrush->bmodel&&siegeBreaks==1&&
          gateDelay->nextthink==level.time+5000&&byModel(other,"*64")->linked);
    CHECK(gate->setTime&&entryGroup(own,groupNames[0])==0&&entryGroup(own,groupNames[1])==15&&
          entryGroup(other,groupNames[0])==15&&entryGroup(other,groupNames[1])==0);
    auto* counter=byName(own,crystalCounter);
    auto* otherCounter=byName(other,crystalCounter);
    auto deliver=[&](const Delivery& spec){
        const auto raw=std::find_if(own.begin(),own.end(),[&](const gentity_t* item){
            return jar::siege::Item(*item)&&item->target&&!strcmp(item->target,spec.trigger);
        });
        CHECK(raw!=own.end());
        auto* item=*raw;
        auto* goal=byName(own,spec.trigger);
        CHECK(!G_SiegeTryDeliver(*goal,&carrier)&&!(item->s.eFlags&EF_NODRAW));
        siege_item_touch(item,&carrier,nullptr);
        CHECK(item->activator==&carrier&&!G_SiegeTryDeliver(*byName(other,spec.trigger),&carrier));
        if(&spec==&deliveries[0]){
            const int floorTime=floor->s.pos.trTime;
            const int floorModel=floor->s.modelindex-room->s.modelindex;
            const int gateTime=gateDelay->nextthink;
            ReloadSiegeKorriban(*room,*neighbor,*item);
            CHECK(room->count==int(Round::Active)&&room->nextthink==roomDeadline&&
                  room->e_ThinkFunc==thinkF_siege_korriban_round_think);
            CHECK(G_SubBSPMembers(room,own)&&G_SubBSPMembers(neighbor,other)&&
                  own.size()==entities&&other.size()==entities-1);
            CHECK(gate->setTime&&entryGroup(own,groupNames[1])==15&&
                  !gateBrush->linked&&!gateBrush->bmodel&&gateDelay->nextthink==gateTime);
            CHECK(floor->s.modelindex==room->s.modelindex+floorModel&&floor->moverState==MOVER_1TO2&&
                  floor->s.pos.trTime==floorTime&&G_SiegeKorribanRancor(*sectionActor));
            CHECK(item->activator==&carrier&&!item->contents&&item->e_ThinkFunc==thinkF_siege_item_think);
            blocked=true;
            CHECK(G_BSPSectionEnter(neighbor,&carrier)&&SiegeCarried()==item&&neighbor->count==int(Round::Waiting));
            blocked=false;
            CHECK(G_BSPSectionEnter(neighbor,&carrier)&&neighbor->count==int(Round::Active)&&
                  !SiegeCarried()&&Near(item->currentOrigin,item->pos1)&&item->contents==CONTENTS_TRIGGER);
            CHECK(G_BSPSectionEnter(room,&carrier)&&room->nextthink==roomDeadline);
            siege_item_touch(item,&carrier,nullptr);
            CHECK(SiegeCarried()==item);
        }
        client.playerTeam=TEAM_ENEMY;
        CHECK(!G_SiegeTryDeliver(*goal,&carrier));
        client.playerTeam=TEAM_PLAYER;
        Touch_Multi(goal,&carrier,nullptr);
        CHECK((item->s.eFlags&EF_NODRAW)&&!(goal->contents&CONTENTS_TRIGGER)&&goal->e_TouchFunc==touchF_NULL&&
              !G_SiegeTryDeliver(*goal,&carrier));
    };
    for(int i=0;i<3;++i){
        auto* pedestal=byName(own,deliveries[i].itemTarget);
        CHECK(pedestal->s.eFlags&EF_NODRAW);
        deliver(deliveries[i]);
        CHECK(!(pedestal->s.eFlags&EF_NODRAW)&&counter->count==2-i&&otherCounter->count==3&&
              byName(own,deliveries[i].target)->setTime&&!byName(other,deliveries[i].target)->setTime);
    }
    auto* crystalShield=byName(own,"crystal_shield","func_usable");
    auto* crystalEffect=byName(own,"crystal_shield","fx_runner");
    auto* arrivalBeam=byName(own,"crystalsinplacelaser","fx_runner");
    CHECK(crystalEffect->nextthink>0&&arrivalBeam->nextthink>0&&
          byName(other,"crystalsinplacelaser","fx_runner")->nextthink==-1);
    vec3_t beamDirection,expectedBeam;
    AngleVectors(arrivalBeam->currentAngles,beamDirection,nullptr,nullptr);
    VectorSubtract(byName(own,"t18")->currentOrigin,arrivalBeam->currentOrigin,expectedBeam);
    VectorNormalize(expectedBeam);
    CHECK(Near(beamDirection,expectedBeam)&&siegeEffectsPlayed>=2);
    const int crystalModelIndex=crystalShield->s.modelindex;
    for(const char* model:{"*45","*46","*51"}){
        auto* barrier=byModel(own,model);
        CHECK(barrier->health==200&&barrier->noDamageTeam==TEAM_ENEMY);
        funcBBrushDie(barrier,&carrier,&carrier,200,MOD_SABER);
    }
    CHECK(!crystalShield->linked&&!crystalShield->contents&&(crystalShield->s.eFlags&EF_NODRAW)&&
          byName(other,"crystal_shield","func_usable")->linked&&siegeBreaks==4&&
          crystalEffect->nextthink==-1&&byName(other,"crystal_shield","fx_runner")->nextthink>0);
    gi.SetBrushModel=[](gentity_t*,const char*){CHECK(false);};
    func_usable_use(crystalShield,room,&carrier);
    CHECK(crystalShield->linked&&crystalShield->contents==CONTENTS_SOLID&&crystalShield->s.solid==SOLID_BMODEL&&
          crystalShield->s.modelindex==crystalModelIndex&&!(crystalShield->s.eFlags&EF_NODRAW));
    const int fuseDeadline=level.time+5000;
    auto* vault=byModel(own,"*68");
    CHECK(vault->linked&&vault->contents==CONTENTS_SOLID&&!vault->takedamage&&
          vault->e_ThinkFunc==thinkF_funcBBrushDieGo&&vault->nextthink==fuseDeadline&&entryGroup(own,groupNames[1])==15);
    ++level.time;
    funcBBrushUse(vault,counter,&carrier);
    CHECK(vault->nextthink==fuseDeadline);
    level.time=fuseDeadline;
    funcBBrushDieGo(vault);
    CHECK(!vault->linked&&!vault->contents&&byModel(other,"*68")->linked&&siegeBreaks==5);
    CHECK(entryGroup(own,groupNames[1])==0&&entryGroup(own,groupNames[2])==16);
    auto* shield=byName(own,"coffinguard_shield","func_usable");
    CHECK(shield->linked&&shield->contents==CONTENTS_SOLID);
    deliver(deliveries[3]);
    CHECK(byName(own,"jedi_obj_5")->setTime);
    CHECK(!shield->linked&&!shield->contents&&!shield->count&&(shield->s.eFlags&EF_NODRAW)&&
          byName(own,"coffinguard_shield","fx_runner")->nextthink==-1&&
          byName(other,"coffinguard_shield","func_usable")->linked&&byName(own,"coffinshielddown")->nextthink==level.time+5000);
    auto* coffin=byName(own,"ragnos_coffin");
    auto* finish=byName(own,ending);
    CHECK(coffin->health==1800&&coffin->takedamage&&!finish->setTime);
    siege_item_die(coffin,nullptr,&carrier,1800,MOD_UNKNOWN,0,HL_NONE);
    CHECK(finish->setTime&&byName(own,"jedi_obj_6")->setTime&&!byName(other,ending)->setTime&&
          room->count==int(Round::Won)&&!room->nextthink&&room->e_ThinkFunc==thinkF_NULL&&
          siegeRoundSounds==std::vector<std::string>{winSound});
    CHECK(G_BSPSectionEnter(neighbor,&carrier)&&neighbor->count==int(Round::Active));
    auto* hint=byName(other,"redcrystalreturn");
    const size_t beforeHint=sectionFeedback.size();
    CHECK(G_SiegeKorribanPrint(*hint)&&G_SiegeKorribanPrint(*hint)&&sectionFeedback.size()==beforeHint+1);
    CHECK(G_SiegeKorribanPrint(*byName(own,"redcrystalreturn"))&&sectionFeedback.size()==beforeHint+1);
    level.time=neighbor->nextthink;
    siege_korriban_round_think(neighbor);
    CHECK(neighbor->count==int(Round::Lost)&&byName(other,ending)->setTime&&!neighbor->nextthink&&
          siegeRoundSounds.size()==2&&siegeRoundSounds.back()==lossSound);
    siege_objective_use(byName(other,"gateopen"),nullptr,&carrier);
    siege_korriban_round_think(neighbor);
    CHECK(!byName(other,"gateopen")->setTime&&siegeRoundSounds.size()==2);
    G_FreeEntity(room);
    CHECK(!sectionActor->inuse);
    CHECK(!torch->inuse&&!arrivalBeam->inuse&&byName(other,"coffinguard_shield","fx_runner")->inuse);
    CHECK(G_SubBSPMembers(neighbor,other)&&entryGroup(other,groupNames[0])==15);
    G_FreeEntity(neighbor);
    CHECK(!G_SubBSPHasOwnership());
    carrier.client=nullptr;
}

int main(int argc,char**argv){
    if(argc>3)return 2;
    if(argc==3&&!strcmp(argv[2],"--siege-korriban"))
        return RunTests("stock Siege Korriban checkpoints and objective chain",[&]{SiegeKorribanGraph(argv[1]);});
    if(argc==3){profile=jar::bspSection::Resource(argv[2]);if(!profile)return 2;}
    return RunTests("native portable stock BSP section controller",[&]{
        Exercise(argc>=2?argv[1]:nullptr);
        if(argc==1)WastelandDoorTeam();
    });
}
