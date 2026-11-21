// Controller edits exercise the production command, native use, copy and codec.
static char controllerEcho[400],printedMessage[200];
static void ControllerPropertySend(int c,const char*s){Send(c,s);if(!strncmp(s,"jarcontroller ",14))Q_strncpyz(controllerEcho,s,sizeof(controllerEcho));if(!strncmp(s,"cp ",3))Q_strncpyz(printedMessage,s,sizeof(printedMessage));}
static void ControllerSelect(gentity_t*e){aimed=e->s.number;Command(0,"jar_select","");assert(players[0].selected==e->s.number);}
static void ControllerPropertyCases(void){
 RigidFixture();imports.SendServerCommand=ControllerPropertySend;gentity_t*p=&g_entities[0];
 gentity_t*print=ControllerAt("target_print",0,100),*delay=ControllerAt("target_delay",0,300),*counter=ControllerAt("target_counter",0,500),*relay=ControllerAt("target_relay",0,700),*position=ControllerAt("target_position",0,900);
 ControllerSelect(print);assert(ContextCaps(p,print->s.number)&JAR_CONTEXT_CONTROLLER);assert(strstr(controllerEcho,"53616e64626f782074617267657420616374697661746564"));
 Command(0,"jar_property","controller_text_48656c6c6f20746865726521");assert(!strcmp(print->message,"Hello there!"));ControllerShot(p,print->s.number,qfalse);assert(!strcmp(printedMessage,"cp \"Hello there!\""));
 Command(0,"jar_property","property_undo");assert(!strcmp(print->message,"Hello there!")); // activation invalidated the edit
 Command(0,"jar_property","controller_text_4f6c64");Command(0,"jar_property","property_undo");assert(!strcmp(print->message,"Hello there!"));
 for(const char**s=(const char*[]){"controller_text_00","controller_text_22","controller_text_3b","controller_text_5c","controller_text_0a","controller_text_a","controller_text_gg","controller_text_","controller_delay_2000","controller_count_3",NULL};*s;++s){Command(0,"jar_property",*s);assert(!strcmp(print->message,"Hello there!"));}
 Command(0,"jar_property","controller_flags_4");assert(print->spawnflags==4);Command(0,"jar_property","property_undo");assert(!print->spawnflags);
 ControllerSelect(delay);Command(0,"jar_property","controller_delay_2500");Command(0,"jar_property","controller_random_500");assert(fabsf(delay->wait-2.5f)<.0001f&&fabsf(delay->random-.5f)<.0001f);
 for(const char**s=(const char*[]){"controller_delay_0","controller_delay_-49","controller_delay_2147483648","controller_delay_nan","controller_delay_1.5","controller_random_2501","controller_delay_400","controller_flags_2",NULL};*s;++s){Command(0,"jar_property",*s);assert(delay->wait==2.5f&&delay->random==.5f&&!delay->spawnflags);}
 ControllerLink(p,delay->s.number,qfalse);ControllerLink(p,print->s.number,qfalse);Command(0,"jar_property","controller_flags_1");ControllerShot(p,delay->s.number,qfalse);int deadline=delay->nextthink;assert(deadline>=level.time+2000&&deadline<=level.time+3000);ControllerShot(p,delay->s.number,qfalse);assert(delay->nextthink==deadline);
 Command(0,"jar_property","controller_delay_3500");assert(delay->nextthink==deadline);Command(0,"jar_property","property_undo");assert(delay->wait==2.5f&&delay->nextthink==deadline);
 ControllerSelect(counter);Command(0,"jar_property","controller_count_3");Command(0,"jar_property","controller_repeats_1");assert(counter->count==3&&counter->genericValue1==3&&counter->bounceCount==1);
 ControllerLink(p,counter->s.number,qfalse);ControllerLink(p,print->s.number,qfalse);int printed=controllerPrints;
 for(int i=0;i<3;++i)ControllerShot(p,counter->s.number,qfalse);assert(counter->count==3&&counter->bounceCount==0&&controllerPrints==printed+1);
 ControllerShot(p,counter->s.number,qfalse);Command(0,"jar_property","controller_reset");assert(counter->count==3);Command(0,"jar_property","property_undo");assert(counter->count==2);
 Command(0,"jar_property","controller_repeats_-1");assert(counter->bounceCount==-1);Command(0,"jar_property","controller_count_0");assert(counter->genericValue1==3);
 ControllerSelect(relay);Command(0,"jar_property","controller_flags_1");Command(0,"jar_property","controller_enabled");assert(relay->spawnflags==1&&(relay->flags&FL_INACTIVE));
 ControllerLink(p,relay->s.number,qfalse);ControllerLink(p,print->s.number,qfalse);printed=controllerPrints;ControllerShot(p,relay->s.number,qfalse);assert(printed==controllerPrints);
 ControllerSelect(position);Command(0,"jar_property","controller_text_4d61726b6572");assert(!strcmp(position->message,"Marker"));
 // A revoked editor or a stale slot identity cannot overwrite retained settings.
 players[1].selected=position->s.number;Command(1,"jar_property","controller_text_426164");assert(!strcmp(position->message,"Marker"));
 char stale[32];snprintf(stale,sizeof(stale),"%u",ContextIdentity(position->s.number));int oldSlot=position->s.number;G_FreeEntity(position);position=ControllerAt("target_position",0,900);assert(position->s.number==oldSlot);ControllerSelect(position);ContextRaw(0,"controller_text_426164",stale);assert(!strcmp(position->message,"Sandbox destination"));
 Command(0,"jar_property","controller_text_4d61726b6572");
 // Every setting, pending delay and current counter state survives copy and JRW8.
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES],old[WORLD_BYTES];assert(WorldCapture(&saved));int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 CopyAssembly(p,print->s.number);assert(copies[0].count==4&&CopyControllersValid(&copies[0]));
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){int kind=ControllerKind(n);gentity_t*e=&g_entities[n];if(kind==1)assert(!strcmp(e->message,"Hello there!"));else if(kind==2)assert(e->spawnflags==1&&(e->flags&FL_INACTIVE));else if(kind==3)assert(e->wait==2.5f&&e->random==.5f&&e->spawnflags==1&&e->nextthink>level.time);else if(kind==4)assert(e->count==2&&e->genericValue1==3&&e->bounceCount==-1);else if(kind==5)assert(!strcmp(e->message,"Marker"));}
 assert(WorldClear()&&PlaceCopy(p,&copies[0],(float[3]){1000,0,100},NULL,NULL));for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(ControllerKind(n)==1)assert(!strcmp(g_entities[n].message,"Hello there!"));assert(WorldClear());
 // JRW12 uses the same node layout and remains readable.
 memcpy(old,bytes,WORLD_HEADER);int legacyAt=0;WorldPut(old,&legacyAt,0x4357524au);legacyAt=WORLD_HEADER;for(int i=0;i<saved.copy.count;++i){memcpy(old+legacyAt,bytes+WORLD_HEADER+i*WORLD_NODE,272);legacyAt+=272;}for(int r=0;r<saved.copy.ropes;++r){memcpy(old+legacyAt,bytes+WORLD_HEADER+saved.copy.count*WORLD_NODE+r*WORLD_ROPE,72);legacyAt+=72;}legacyAt+=4;WorldTestHash(old,legacyAt);assert(WorldDecode(old,legacyAt,&decoded)&&decoded.copy.count==saved.copy.count);int legacy13=0;WorldPut(old,&legacy13,0x4457524au);WorldTestHash(old,legacyAt);assert(WorldDecode(old,legacyAt,&decoded)&&decoded.copy.count==saved.copy.count);
 // JRW7 reconstructs exact prior defaults, retaining its count, graph and timer.
 memcpy(old,bytes,WORLD_HEADER);int at=0;WorldPut(old,&at,0x3757524au);at=WORLD_HEADER;
 for(int i=0;i<saved.copy.count;++i){memcpy(old+at,bytes+WORLD_HEADER+i*WORLD_NODE,180);at+=180;}
 assert(!saved.copy.ropes);at+=4;WorldTestHash(old,at);assert(WorldDecode(old,at,&decoded));
 for(int i=0;i<decoded.copy.count;++i){int index;const jarMPEntry_t*e=Entry(decoded.copy.node[i].id,&index);int kind=ControllerClass(e->name);const jarControllerState_t*s=&decoded.copy.node[i].controller;if(kind==1)assert(!strcmp(s->text,"Sandbox target activated"));if(kind==3)assert(s->waitMs==1000&&!s->randomMs&&!s->flags);if(kind==4)assert(s->count==2&&s->initial==3&&!s->repeats);}
 // Rehashed corrupt text and invalid settings are rejected before allocation.
 const int offsets[]={180,184,188,192,196};const unsigned invalid[]={0x80000000u,0x80000000u,129,0x80000000u,0x22222222};
 for(int k=0;k<5;++k){memcpy(old,bytes,size);at=WORLD_HEADER+offsets[k];WorldPut(old,&at,invalid[k]);WorldTestHash(old,size);decoded.generation=42;assert(!WorldDecode(old,size,&decoded)&&decoded.generation==42);}
 imports.SendServerCommand=Send;PhysicsReset();puts("MP controller properties: native callbacks, text safety, number and state validation, Undo conflicts, identity/ownership, immutable copy, JRW8 and JRW7 defaults passed.");
}
static void ContextSelectionTrace(trace_t*t,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask,int capsule,int flags,int lod){Trace(t,start,mins,maxs,end,pass,mask,capsule,flags,lod);if(mask==MASK_SOLID&&!blocked)t->fraction=1;}
static void ContextWorldCases(void){
 RigidFixture();imports.Trace=ContextSelectionTrace;gentity_t*e=CopySpawn(1,0,100);int n=e->s.number;aimed=n;
 Command(0,"jar_select","ray_0_0_0_1_0_0");assert(players[0].selected==n);
 const char*invalid[]={"ray_0_0_0_nan_0_0","ray_0_0_0_1_0_0bad","ray_0_0_0_0_0_0","ray_9999_0_0_1_0_0","ray_0_0_0_2_0_0",NULL};
 for(const char**s=invalid;*s;++s){players[0].selected=ENTITYNUM_NONE;Command(0,"jar_select",*s);assert(players[0].selected==ENTITYNUM_NONE);}
 blocked=1;Command(0,"jar_select","ray_0_0_0_1_0_0");assert(players[0].selected==ENTITYNUM_NONE);blocked=0;
 ControllerSelect(e);int tool=players[0].tool;Command(0,"jar_property","thruster");assert(objects[n].thrustMounted&&objects[n].thruster&&players[0].tool==tool);
 Command(0,"jar_property","detach");assert(!objects[n].thrustMounted);
 Command(0,"jar_property","rope");assert(players[0].first==n&&players[0].firstTool==4);
 gentity_t*b=CopySpawn(1,0,500);ControllerSelect(b);Command(0,"jar_property","weld");assert(players[0].first==b->s.number&&players[0].firstTool==5);
 ControllerSelect(e);Command(0,"jar_property","weld");assert(players[0].first==ENTITYNUM_NONE);Command(0,"jar_property","detach");G_FreeEntity(b);Command(0,"jar_property","copy");assert(copies[0].count==1);Command(0,"jar_property","paste");assert(players[0].tool==tool);
 players[0].noTools=1;Command(0,"jar_select","self");assert(players[0].selected==0&&!(ContextCaps(&g_entities[0],0)&JAR_CONTEXT_FREEZE));Command(0,"jar_property","health_321");assert(g_entities[0].health==321);Command(0,"jar_property","property_undo");assert(g_entities[0].health==100);
 Command(0,"jar_property","freeze");assert(!actors[0].active);Command(0,"jar_property","remove");assert(g_entities[0].inuse);
 imports.Trace=Trace;PhysicsReset();puts("MP context selection: bounded rays, obstruction, same tool mode, origin thruster, copy/paste, self fields and protected self body passed.");
}
