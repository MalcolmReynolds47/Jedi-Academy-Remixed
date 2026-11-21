// Real native switch callbacks, owned links, media snapshots and world codec.
static void SwitchLink(gentity_t*p,gentity_t*source,gentity_t*target,int output){
 players[p->s.number].linkOutput=output;players[p->s.number].first=ENTITYNUM_NONE;
 ControllerLink(p,source->s.number,qfalse);ControllerLink(p,target->s.number,qfalse);
 assert(objects[source->s.number].output[output]==target->s.number+1);
}
static void ControllerSwitchCases(void){
 RigidFixture();imports.FS_Open=MediaOpenFixture;imports.GetConfigstring=MediaConfigFixture;imports.SendServerCommand=Send;mediaMissing=mediaFull=0;
 gentity_t*p=&g_entities[0],*on=ControllerAt("target_activate",0,100),*off=ControllerAt("target_deactivate",0,300);
 assert(on&&off&&on->use==target_activate_use&&off->use==target_deactivate_use&&!MediaHelper(on->s.number));
 const char*names[]={"target_print","target_relay","target_delay","target_counter","target_position","target_push","target_teleporter","misc_teleporter_dest","target_speaker","fx_runner","target_activate","target_deactivate"};
 for(int kind=1;kind<=12;++kind){
  gentity_t*t=ControllerAt(names[kind-1],0,500+kind*100);assert(t&&ControllerKind(t->s.number)==kind);
  SwitchLink(p,on,t,0);SwitchLink(p,off,t,0);
  int n=t->s.number,prints=controllerPrints,events=mediaEvents,count=t->count;
  if(kind==9){ControllerSelect(t);Command(0,"jar_property","controller_mode");Command(0,"jar_property","controller_playing");assert(MediaHelper(n)->s.loopSound);}
  jarControllerState_t before,after;ControllerState(n,&before);
  ControllerShot(p,off->s.number,qfalse);assert(t->flags&FL_INACTIVE);ControllerState(n,&after);before.inactive=1;assert(!memcmp(&before,&after,sizeof(before)));
  assert(controllerPrints==prints&&mediaEvents==events&&t->count==count);
  if(JAR_ControllerMedia(kind)){gentity_t*h=MediaHelper(n);assert(!h->nextthink&&!h->s.loopSound&&h->s.modelindex2==FX_STATE_OFF&&objects[n].mediaActive);}
  ControllerShot(p,on->s.number,qfalse);assert(!(t->flags&FL_INACTIVE));
  if(kind==9)assert(MediaHelper(n)->s.loopSound);
  if(kind==10)assert(MediaHelper(n)->s.modelindex2==FX_STATE_CONTINUOUS&&MediaHelper(n)->nextthink>level.time);
 }
 // Primary and Secondary change state, never execute the target. Duplicate
 // endpoints are idempotent and markers are valid even without a use callback.
 gentity_t*print=ControllerAt("target_print",0,2000),*marker=ControllerAt("target_position",0,2200);
 SwitchLink(p,on,print,0);SwitchLink(p,off,print,0);SwitchLink(p,on,marker,1);SwitchLink(p,off,marker,1);
 ControllerShot(p,off->s.number,qfalse);assert((print->flags&FL_INACTIVE)&&(marker->flags&FL_INACTIVE));
 ControllerShot(p,on->s.number,qfalse);assert(!(print->flags&FL_INACTIVE)&&!(marker->flags&FL_INACTIVE));
 SwitchLink(p,off,print,1);unsigned revision=objects[print->s.number].propertyRevision;ControllerShot(p,off->s.number,qfalse);assert(objects[print->s.number].propertyRevision==revision+1);
 ControllerShot(p,on->s.number,qfalse);SwitchLink(p,off,marker,1);
 // Six changed conditions independently prevent mutation. A protected target
 // does not suppress a different, still-permitted endpoint.
 for(int reason=0;reason<6;++reason){
  print->flags&=~FL_INACTIVE;marker->flags&=~FL_INACTIVE;
  sandbox.integer=reason==0?0:1;off->flags=reason==1?FL_INACTIVE:0;objects[off->s.number].copyPending=reason==2;
  objects[print->s.number].copyPending=reason==3;objects[print->s.number].owner=reason==4?2:reason==5?MAX_CLIENTS+1:1;
  worldEditing.integer=sharedEditing.integer=0;ControllerShot(p,off->s.number,qfalse);
  assert(!(print->flags&FL_INACTIVE));assert(!!(marker->flags&FL_INACTIVE)==(reason>=3));
 }
 sandbox.integer=1;off->flags=0;objects[off->s.number].copyPending=objects[print->s.number].copyPending=0;objects[print->s.number].owner=1;
 // A later switch invalidates a conflicting property Undo, while editing the
 // switch's own Enabled setting supports normal identity-bound Undo.
 ControllerSelect(print);Command(0,"jar_property","controller_text_4e6577");ControllerShot(p,off->s.number,qfalse);Command(0,"jar_property","property_undo");assert(!strcmp(print->message,"New")&&(print->flags&FL_INACTIVE));
 ControllerSelect(off);Command(0,"jar_property","controller_enabled");assert(off->flags&FL_INACTIVE);Command(0,"jar_property","property_undo");assert(!(off->flags&FL_INACTIVE));
 for(const char**a=(const char*[]){"controller_text_41","controller_playing","controller_mode","controller_asset_next","controller_delay_500","controller_flags_1",NULL};*a;++a){jarControllerState_t b,c;ControllerState(off->s.number,&b);Command(0,"jar_property",*a);ControllerState(off->s.number,&c);assert(!memcmp(&b,&c,sizeof(b)));}
 // Delay retains the native already-scheduled event when disabled; it cannot
 // receive a fresh activation until enabled again.
 gentity_t*delay=ControllerAt("target_delay",0,2400);SwitchLink(p,delay,print,0);SwitchLink(p,off,delay,0);ControllerShot(p,on->s.number,qfalse);
 ControllerShot(p,delay->s.number,qfalse);int deadline=delay->nextthink;ControllerShot(p,off->s.number,qfalse);ControllerShot(p,delay->s.number,qfalse);assert(delay->nextthink==deadline);int prints=controllerPrints;ControllerTick(delay,deadline-level.time);assert(controllerPrints==prints+1);
 // Foreign and map objects are rejected as links; cycles cannot enter a save.
 players[0].first=on->s.number;ControllerLink(p,off->s.number,qfalse);assert(objects[on->s.number].output[0]==off->s.number+1);
 int old=objects[off->s.number].output[0];players[0].first=off->s.number;ControllerLink(p,on->s.number,qfalse);assert(objects[off->s.number].output[0]==old);
 gentity_t*map=G_Spawn();map->classname="target_print";players[0].first=on->s.number;ControllerLink(p,map->s.number,qfalse);assert(objects[on->s.number].output[0]==off->s.number+1);players[0].first=ENTITYNUM_NONE;
 assert(!JAR_MP_SetActiveTargets(map,qfalse)&&!JAR_MP_SetActiveTargets(NULL,qtrue));
 // Exercise the unmodified authored-map name lookup through the same native
 // callbacks; an owned switch never falls through to that namespace.
 gentity_t*mapTarget=G_Spawn();mapTarget->targetname="fixture_switch_target";activeStateMapTarget=mapTarget;map->target=mapTarget->targetname;
 target_deactivate_use(map,p,p);assert(mapTarget->flags&FL_INACTIVE);target_activate_use(map,p,p);assert(!(mapTarget->flags&FL_INACTIVE));
 off->target=mapTarget->targetname;ControllerShot(p,off->s.number,qfalse);assert(!(mapTarget->flags&FL_INACTIVE));activeStateMapTarget=NULL;G_FreeEntity(mapTarget);G_FreeEntity(map);
 // Captures use catalog IDs and remapped links; disabled targets survive copy,
 // exact world roundtrip, deletion/reuse and grouped Undo.
 assert(WorldClear());on=ControllerAt("target_activate",0,100);off=ControllerAt("target_deactivate",0,300);marker=ControllerAt("target_position",0,500);print=ControllerAt("target_print",0,700);
 SwitchLink(p,on,print,0);SwitchLink(p,off,print,0);SwitchLink(p,on,marker,1);SwitchLink(p,off,marker,1);ControllerShot(p,off->s.number,qfalse);
 CopyAssembly(p,on->s.number);assert(copies[0].count==4&&CopyControllersValid(&copies[0]));static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));worldEditing.integer=1;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){int k=ControllerKind(n);if(k==11)on=&g_entities[n];if(k==12)off=&g_entities[n];if(k==1)print=&g_entities[n];if(k==5)marker=&g_entities[n];}
 assert((print->flags&FL_INACTIVE)&&(marker->flags&FL_INACTIVE));ControllerShot(p,on->s.number,qfalse);assert(!(print->flags&FL_INACTIVE)&&!(marker->flags&FL_INACTIVE));
 int slot=print->s.number;G_FreeEntity(print);print=ControllerAt("target_print",0,700);assert(print->s.number==slot&&!objects[off->s.number].output[0]);ControllerShot(p,off->s.number,qfalse);assert(!(print->flags&FL_INACTIVE));
 assert(WorldClear()&&PlaceCopy(p,&copies[0],(float[3]){1000,0,80},NULL,NULL));assert(LiveCount()==4);Undo(p,qfalse);assert(LiveCount()==0);
 // Both normal menu cards create useful Print targets as a single Undo group.
 for(int k=0;k<2;++k){players[0].linkOutput=0;aimed=ENTITYNUM_WORLD;gentity_t*e=Spawn(p,ControllerId(k?"target_deactivate":"target_activate"));assert(e&&objects[e->s.number].output[0]);int h=objects[e->s.number].output[0]-1;assert(objects[h].order==objects[e->s.number].order);Undo(p,qfalse);assert(!e->inuse&&!g_entities[h].inuse);}
 assert(WorldClear());imports.FS_Open=RigidOpen;imports.GetConfigstring=Config;PhysicsReset();
 puts("MP controller switches: 12 native target families, both outputs, media pause/resume, six permission/readiness refusals, pending Delay semantics, Undo conflicts, authored-map isolation, copy/JRW11, cleanup and menu helpers passed.");
}
