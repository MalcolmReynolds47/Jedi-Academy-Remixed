static void ToolClickTrace(trace_t* hit,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,
 int pass,int mask,int capsule,int flags,int lod){
 RigidTrace(hit,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mins)return;
 assert(flags==JAR_TRACE_PHYS_GUN&&mask==JAR_PHYSGUN_MP_AIM_MASK);
 VectorSet(hit->endpos,92,4,100);
 VectorSet(hit->plane.normal,-1,0,0);
}

static void ToolParityCases(void){
 RigidFixture();gentity_t*p=&g_entities[0],*a=CopySpawn(1,0,100);int n=a->s.number;aimed=n;
 imports.Trace=ToolClickTrace;
 assert(players[0].tool==7);Tool(p,qfalse);assert(players[0].selected==n);
 players[1].tool=7;Tool(&g_entities[1],qtrue);assert(players[1].selected==ENTITYNUM_NONE);
 gentity_t* newer=CopySpawn(1,0,400);
 vec3_t oldOrigin,contact={92,4,100};
 VectorCopy(a->r.currentOrigin,oldOrigin);
 jarMPGrip_t pivot;
 assert(JAR_GripBegin(&pivot,oldOrigin,a->r.currentAngles,contact,0));
 players[0].tool=8;float yaw=a->r.currentAngles[YAW];Tool(p,qtrue);assert(fabsf(AngleSubtract(a->r.currentAngles[YAW],yaw)+15)<.01f);
 vec3_t clicked;
 JAR_GripPoint(&pivot,a->r.currentOrigin,a->r.currentAngles,clicked);
 assert(Distance(clicked,contact)<.01f&&Distance(oldOrigin,a->r.currentOrigin)>1);
 Command(0,"jar_undo","");assert(!newer->inuse&&a->inuse&&fabsf(AngleSubtract(a->r.currentAngles[YAW],yaw)+15)<.01f);
 Tool(p,qfalse);assert(fabsf(AngleSubtract(a->r.currentAngles[YAW],yaw))<.01f&&Distance(oldOrigin,a->r.currentOrigin)<.01f);
 blocked=1;imports.Trace=Trace;Tool(p,qfalse);assert(fabsf(AngleSubtract(a->r.currentAngles[YAW],yaw))<.01f);blocked=0;imports.Trace=RigidTrace;
 newer=CopySpawn(1,0,400);
 players[0].tool=9;objects[n].frozen=objects[n].noGravity=1;Tool(p,qfalse);assert(!objects[n].frozen&&!objects[n].noGravity);Tool(p,qtrue);assert(objects[n].frozen);
 Command(0,"jar_undo","");assert(!newer->inuse&&a->inuse&&objects[n].frozen&&!objects[n].noGravity);
 players[0].selected=n;Command(0,"jar_property","mass_200");assert(objects[n].mass==200);
 const int undoFeedbackBeforeRevert=undoSounds;
 Command(0,"jar_property","property_undo");assert(objects[n].mass!=200&&undoSounds==undoFeedbackBeforeRevert);
 // Detach removes links/thrust but keeps both entities alive and owned.
 gentity_t*r=ForceLink(0,n,ENTITYNUM_WORLD,50);int link=r->s.number;objects[n].thruster=objects[n].thrustMounted=1;
 players[0].tool=10;Tool(p,qtrue);assert(g_entities[link].inuse&&players[0].selected==n);Tool(p,qfalse);assert(!g_entities[link].inuse&&a->inuse&&!objects[n].thruster&&!objects[n].thrustMounted);
 // Mounts are creations. Toggles/removal do not add entries or recreate mounts.
 players[0].tool=2;Tool(p,qfalse);assert(objects[n].thrustMounted&&objects[n].thruster);
 newer=CopySpawn(1,0,400);Tool(p,qfalse);assert(objects[n].thrustMounted&&!objects[n].thruster);
 Command(0,"jar_undo","");assert(!newer->inuse&&objects[n].thrustMounted&&!objects[n].thruster);
 objects[n].frozen=1;Command(0,"jar_undo","");assert(a->inuse&&!objects[n].thrustMounted&&objects[n].frozen);
 Tool(p,qfalse);newer=CopySpawn(1,0,400);Tool(p,qtrue);assert(!objects[n].thrustMounted);
 Command(0,"jar_undo","");assert(!newer->inuse&&a->inuse&&!objects[n].thrustMounted);
 // Controller rewiring keeps its new destination when undoing an unrelated spawn.
 gentity_t*relay=ControllerAt("target_relay",1,500),*notice=ControllerAt("target_print",1,700);
 sharedEditing.integer=1;newer=CopySpawn(1,0,400);players[0].first=ENTITYNUM_NONE;
 ControllerLink(p,relay->s.number,qfalse);ControllerLink(p,notice->s.number,qfalse);
 Command(0,"jar_undo","");assert(!newer->inuse&&objects[relay->s.number].output[0]==notice->s.number+1);
 sharedEditing.integer=0;
 // Primary Duplicate first copies; further primary shots paste. Alternate recaptures.
 players[0].tool=3;assert(!copies[0].count);unsigned before=order;Tool(p,qfalse);assert(copies[0].count==1&&order==before);
 // Gameplay drops and authored entities cannot displace a physical creation.
 ForceTool(0,n,4,qfalse);ForceTool(0,ENTITYNUM_WORLD,4,qfalse);
 const int rope=Last(0);assert(g_entities[rope].s.generic1==251);
 gentity_t*authored=G_Spawn();
 gentity_t*drop=G_Spawn();TrackTemporaryPickup(drop,1,order+1);
 Command(0,"jar_undo","");assert(!g_entities[rope].inuse&&a->inuse&&authored->inuse&&drop->inuse);
 players[0].tool=1;aimed=ENTITYNUM_WORLD;Tool(p,qtrue);assert(!a->inuse); // undo does not require an aimed entity
 Command(0,"jar_undo","");assert(authored->inuse&&drop->inuse);
 // Native consenting actors rotate without freezing or damaging them.
 RigidFixture();gentity_t*actor=CopySpawn(2,0,100);CopyReady(actor);int health=actor->health;aimed=actor->s.number;players[0].tool=8;
 imports.Trace=ToolClickTrace;
 yaw=actor->r.currentAngles[YAW];Tool(p,qfalse);assert(fabsf(AngleSubtract(actor->r.currentAngles[YAW],yaw)-15)<.01f&&actor->health==health&&!JAR_MP_ActorControlled(actor));
 // Tool selection must not bypass the normal weapon-disable rules.
 bg_numItems=2;bg_itemlist[1].classname="weapon_toolgun";bg_itemlist[1].giType=IT_WEAPON;bg_itemlist[1].giTag=WP_TOOLGUN;
 g_weaponDisable.integer=1<<WP_TOOLGUN;players[0].tool=7;Command(0,"jar_tool","8");assert(players[0].tool==7);
 g_weaponDisable.integer=0;Command(0,"jar_tool","8");assert(players[0].tool==8&&clients[0].ps.weapon==WP_TOOLGUN);
 Command(0,"jar_tool","13");assert(players[0].tool==8);Command(0,"jar_tool","8;quit");assert(players[0].tool==8);
 puts("MP tool parity: Select, bidirectional Rotate/collision refusal, Gravity, Unlink, first-copy Duplicate, aimed-independent undo and server-gated selection passed.");
}
