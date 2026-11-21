// Native media callbacks, real object ownership/copy/codec and network snapshots.
// Engine audio/rendering and filesystem boundaries are deterministic imports.
static char mediaExisting[64];
static int MediaOpenFixture(const char*path,fileHandle_t*f,fsMode_t mode){
 if(!strncmp(path,"sound/",6)||!strncmp(path,"effects/",8)){*f=mediaMissing?0:1;return mediaMissing?-1:100;}
 return RigidOpen(path,f,mode);
}
static void MediaConfigFixture(int n,char*b,int size){
 if((n>CS_EFFECTS&&n<CS_EFFECTS+MAX_FX)||(n>CS_SOUNDS&&n<CS_SOUNDS+MAX_SOUNDS))Q_strncpyz(b,mediaFull?(mediaExisting[0]?mediaExisting:"occupied"):"",size);
 else Config(n,b,size);
}
static void MediaText(gentity_t*e,const char*value){char hex[127],action[150];ControllerSelect(e);JAR_ControllerHex(value,hex);snprintf(action,sizeof(action),"controller_text_%s",hex);Command(0,"jar_property",action);}
static void MediaCases(void){
 RigidFixture();imports.FS_Open=MediaOpenFixture;imports.GetConfigstring=MediaConfigFixture;imports.SendServerCommand=Send;mediaMissing=mediaFull=mediaEvents=0;mediaExisting[0]=0;
 gentity_t*p=&g_entities[0],*speaker=ControllerAt("target_speaker",0,100);assert(speaker);int s=speaker->s.number;gentity_t*h=MediaHelper(s);assert(h&&h->s.eType==ET_SPEAKER&&PhysicsProp(s)&&speaker->s.generic1==250&&!objects[h->s.number].owner);
 assert(!h->s.clientNum&&!h->s.loopSound&&VectorCompare(h->r.currentOrigin,h->s.pos.trBase));
 ControllerShot(p,s,qfalse);assert(mediaEvents==1&&mediaEventEntity==h->s.number&&mediaEventType==EV_GENERAL_SOUND);
 ControllerSelect(speaker);Command(0,"jar_property","controller_mode");assert(speaker->spawnflags==2&&!objects[s].mediaActive);
 ControllerShot(p,s,qfalse);assert(objects[s].mediaActive&&h->s.loopSound);ControllerShot(p,s,qfalse);assert(!objects[s].mediaActive&&!h->s.loopSound);
 Command(0,"jar_property","controller_playing");assert(objects[s].mediaActive&&h->s.loopSound);Command(0,"jar_property","controller_enabled");assert(!h->s.loopSound&&objects[s].mediaActive);Command(0,"jar_property","property_undo");assert(h->s.loopSound);
 Command(0,"jar_property","controller_mode");assert(speaker->spawnflags==4&&!h->s.loopSound&&(h->r.svFlags&SVF_BROADCAST));ControllerShot(p,s,qfalse);assert(mediaEventType==EV_GLOBAL_SOUND);
 Command(0,"jar_property","controller_mode");assert(speaker->spawnflags==8&&!h->r.svFlags);ControllerShot(p,s,qfalse);assert(mediaEventEntity==p->s.number&&mediaEventType==EV_GENERAL_SOUND);
 int count=mediaEvents;ControllerInvoke(speaker,p,speaker);assert(mediaEvents==count); // no fake activator
 Command(0,"jar_property","controller_mode");Command(0,"jar_property","controller_delay_1000");Command(0,"jar_property","controller_random_200");Command(0,"jar_property","controller_playing");assert(objects[s].mediaActive&&h->nextthink>level.time);
 ControllerTick(h,1000);assert(mediaEvents==count+1&&h->nextthink>=level.time+800&&h->nextthink<=level.time+1200);
 Command(0,"jar_property","controller_delay_199");assert(speaker->delay==1000);
 sandbox.integer=0;MediaFrame();assert(!h->nextthink&&!h->s.loopSound);ControllerTick(h,2000);assert(mediaEvents==count+1);sandbox.integer=1;MediaFrame();assert(h->nextthink>level.time);
 MediaText(speaker,"sound/movers/switches/button_08.mp3");assert(!strcmp(speaker->message,"sound/movers/switches/button_08.mp3"));Command(0,"jar_property","property_undo");assert(!strcmp(speaker->message,jarSpeakerChoices[0]));
 const char*bad[]={"../sound/fire.mp3","sound/../fire.mp3","sound//fire.mp3","sound/fire.exe","C:/fire.mp3","sound/UPPER.mp3"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){MediaText(speaker,bad[i]);assert(!strcmp(speaker->message,jarSpeakerChoices[0]));}
 mediaMissing=1;MediaText(speaker,jarSpeakerChoices[1]);assert(!strcmp(speaker->message,jarSpeakerChoices[0]));mediaMissing=0;
 mediaFull=1;MediaText(speaker,jarSpeakerChoices[1]);assert(!strcmp(speaker->message,jarSpeakerChoices[0]));Q_strncpyz(mediaExisting,jarSpeakerChoices[1],64);MediaText(speaker,jarSpeakerChoices[1]);assert(!strcmp(speaker->message,jarSpeakerChoices[1]));mediaFull=0;mediaExisting[0]=0;
 // Effect helpers preserve native ET_FX fields without replacing body model/bounds.
 gentity_t*effect=ControllerAt("fx_runner",0,400);assert(effect);int f=effect->s.number;gentity_t*fx=MediaHelper(f);assert(fx&&fx->s.eType==ET_FX&&fx->s.modelindex2==FX_STATE_CONTINUOUS&&fx->s.speed==500&&effect->s.generic1==250&&effect->s.modelindex);
 gentity_t*destination=ControllerAt("target_position",0,600),*print=ControllerAt("target_print",0,800);int d=destination->s.number;
 ControllerLink(p,f,qfalse);ControllerLink(p,d,qfalse);assert(objects[f].output[0]==d+1);MediaFrame();vec3_t direction,forward;VectorSubtract(destination->r.currentOrigin,fx->r.currentOrigin,direction);VectorNormalize(direction);AngleVectors(fx->s.angles,forward,NULL,NULL);assert(DotProduct(direction,forward)>.999f);
 players[0].linkOutput=1;ControllerLink(p,f,qfalse);ControllerLink(p,print->s.number,qfalse);assert(objects[f].output[1]==print->s.number+1);players[0].linkOutput=0;
 int prints=controllerPrints;ControllerTick(fx,500);assert(controllerPrints==prints+1&&fx->s.modelindex2==FX_STATE_CONTINUOUS);
 ControllerSelect(effect);Command(0,"jar_property","controller_mode");assert(effect->spawnflags==2&&!objects[f].mediaActive&&!fx->nextthink);
 for(int i=0;i<25;++i){ControllerShot(p,f,qfalse);assert(fx->s.modelindex2>=FX_STATE_ONE_SHOT&&fx->s.modelindex2<FX_STATE_ONE_SHOT_LIMIT&&fx->nextthink<=0);}assert(controllerPrints==prints+26);
 // Native's tenth one-shot state must never accidentally become continuous.
 assert(!objects[f].mediaActive);Command(0,"jar_property","controller_playing");assert(!objects[f].mediaActive);
 objects[d].owner=2;int state=fx->s.modelindex2;ControllerShot(p,f,qfalse);assert(fx->s.modelindex2==FX_STATE_OFF&&controllerPrints==prints+26);objects[d].owner=1;
 MediaText(effect,"effects/sparks/spark.efx");assert(!strcmp(effect->message,jarEffectChoices[0]));MediaText(effect,"sparks/bluesparks");assert(!strcmp(effect->message,"sparks/bluesparks"));Command(0,"jar_property","controller_delay_900");Command(0,"jar_property","controller_random_100");assert(fx->s.speed==900&&fx->s.time==100);
 Command(0,"jar_property","controller_mode");Command(0,"jar_property","controller_playing");assert(objects[f].mediaActive&&fx->s.modelindex2==FX_STATE_CONTINUOUS);
 sandbox.integer=0;MediaFrame();assert(fx->s.modelindex2==FX_STATE_OFF&&!fx->nextthink);sandbox.integer=1;MediaFrame();assert(fx->s.modelindex2==FX_STATE_CONTINUOUS);
 // A moving/rotating body moves its native helper in the same frame.
 Move(effect,(float[3]){400,100,150});MediaFrame();assert(fx->r.currentOrigin[1]==100&&fx->r.currentOrigin[2]>effect->r.currentOrigin[2]);
 // Copy / persisted worlds contain bodies and links, not helper slot numbers.
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],badBytes[WORLD_BYTES];assert(WorldCapture(&saved)&&saved.copy.count==4);int size=WorldEncode(&saved,bytes),at=0;assert(size&&WorldGet(bytes,&at)==0x4957524au&&WorldDecode(bytes,size,&decoded));
 CopyAssembly(p,f);assert(copies[0].count==3&&CopyControllersValid(&copies[0]));
 assert(WorldClear());for(int n=MAX_CLIENTS;n<level.num_entities;++n)assert(!mediaParent[n]&&!g_entities[n].inuse);
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));int seen=0;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)>=9){++seen;assert(MediaHelper(n)&&objects[n].mediaActive);if(ControllerKind(n)==10){assert(!strcmp(g_entities[n].message,"sparks/bluesparks")&&g_entities[n].delay==900);MediaFrame();assert(MediaHelper(n)->s.modelindex2==FX_STATE_CONTINUOUS);}}
 assert(seen==2&&WorldClear());assert(PlaceCopy(p,&copies[0],(float[3]){1000,0,100},NULL,NULL));f=players[0].selected;assert(ControllerKind(f)==10&&MediaHelper(f)&&objects[f].output[0]);Undo(p,qfalse);assert(WorldClear());
 // Rehashed malformed state and absent assets reject before creating any body.
 memcpy(badBytes,bytes,size);at=WORLD_HEADER+264;WorldPut(badBytes,&at,2);WorldTestHash(badBytes,size);decoded.generation=42;assert(!WorldDecode(badBytes,size,&decoded)&&decoded.generation==42);
 mediaMissing=1;int live=LiveCount();assert(!PlaceCopy(p,&copies[0],vec3_origin,NULL,NULL)&&LiveCount()==live);assert(!ControllerAt("fx_runner",0,100)&&LiveCount()==live);mediaMissing=0;
 // Externally freeing a helper cannot retain a pointer into a reused slot.
 effect=ControllerAt("fx_runner",0,100);f=effect->s.number;int oldHelper=MediaHelper(f)->s.number;G_FreeEntity(&g_entities[oldHelper]);gentity_t*replacement=G_Spawn();assert(replacement->s.number==oldHelper);ControllerShot(p,f,qfalse);assert(MediaHelper(f)&&MediaHelper(f)!=replacement);G_FreeEntity(effect);assert(replacement->inuse&&!mediaParent[oldHelper]);G_FreeEntity(replacement);
 imports.FS_Open=RigidOpen;imports.GetConfigstring=Config;PhysicsReset();puts("MP media: native sound/FX snapshots, modes, timers, rotation/aim links, emission outputs, one-shot wrap, asset/reserve refusal, typed edits/Undo, copy/JRW10, cleanup and slot lifetime passed.");
}
