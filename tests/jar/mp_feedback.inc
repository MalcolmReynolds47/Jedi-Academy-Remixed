// Owner feedback regressions: visible mounts, deletion, persistence and respawn handoff.
static void FeedbackCases(void){
 RigidFixture();imports.SendServerCommand=Send;
 gentity_t*body=CopySpawn(1,0,100);int n=body->s.number;
 vec3_t mount={4,0,8},normal={0,0,1};AttachAim(body,mount,normal);players[0].tool=2;Tool(&g_entities[0],qfalse);ForceRopeVisuals();
 int visual=thrusterVisual[n]-1;assert(visual>=MAX_CLIENTS&&g_entities[visual].inuse&&g_entities[visual].s.generic1==254&&g_entities[visual].s.otherEntityNum==n);
 vec3_t point,axis;ThrusterFrame(n,point,axis);assert(VectorCompare(point,g_entities[visual].r.currentOrigin)&&VectorCompare(axis,g_entities[visual].s.origin2));
 static jarWorld_t world;assert(WorldCapture(&world)&&world.copy.count==1); // cosmetic body is never saved/copied as a second prop
 Tool(&g_entities[0],qfalse);ForceRopeVisuals();assert(thrusterVisual[n]==visual+1&&!g_entities[visual].s.time2); // off still has a housing
 vec3_t angle={0,90,0};assert(PhysicsSetAngles(body,angle));ForceRopeVisuals();ThrusterFrame(n,point,axis);assert(VectorCompare(point,g_entities[visual].r.currentOrigin));
 players[0].tool=12;players[0].linkOutput=1;players[0].ropeSlack=1.8f;players[0].thrustForce=9000;
 // Native ClientSpawn owns health/inventory reset; this Sandbox hook must keep
 // the build and tool preferences while removing transient player control.
 g_entities[0].health=0;JAR_MP_ClientReset(0);
 assert(body->inuse&&players[0].tool==12&&players[0].linkOutput==1&&players[0].ropeSlack==1.8f&&players[0].thrustForce==9000);
 g_entities[0].health=100;clients[0].ps.stats[STAT_HEALTH]=100;clients[0].ps.stats[STAT_WEAPONS]=1<<WP_MELEE;ActorTick();
 assert((clients[0].ps.stats[STAT_WEAPONS]&((1<<WP_PHYSGUN)|(1<<WP_TOOLGUN)))==((1<<WP_PHYSGUN)|(1<<WP_TOOLGUN))&&body->inuse);
 int feedback=undoSounds;
 Undo(&g_entities[0],qfalse);ForceRopeVisuals();assert(body->inuse&&!objects[n].thrustMounted&&!thrusterVisual[n]&&undoSounds==feedback+1);
 Undo(&g_entities[0],qfalse);assert(!body->inuse&&undoSounds==feedback+2);
 Undo(&g_entities[0],qfalse);assert(undoSounds==feedback+2); // empty Undo only reports status
 // Undo and Despawn All remove the builder's mount from another player's prop.
 gentity_t*shared=CopySpawn(1,1,100);int sharedNum=shared->s.number;
 gentity_t*other=CopySpawn(1,1,200);int otherNum=other->s.number;
 sharedEditing.integer=1;
 players[0].tool=players[1].tool=2;
 AttachAim(other,mount,normal);Tool(&g_entities[1],qfalse);
 AttachAim(shared,mount,normal);Tool(&g_entities[0],qfalse);
 assert(objects[otherNum].thrustMounted&&objects[sharedNum].thrustMounted);
 sharedEditing.integer=0;
 Undo(&g_entities[0],qfalse);
 assert(shared->inuse&&!objects[sharedNum].thrustMounted&&objects[otherNum].thrustMounted&&undoSounds==feedback+3);
 sharedEditing.integer=1;
 AttachAim(shared,mount,normal);Tool(&g_entities[0],qfalse);
 gentity_t*vehicle=CopySpawn(3,0,300);assert(vehicle);CopyReady(vehicle);
 vehicle->m_pVehicle->m_pPilot=(bgEntity_t*)&g_entities[0];
 Undo(&g_entities[0],qtrue);
 assert(vehicle->inuse&&objects[sharedNum].thrustMounted&&objects[otherNum].thrustMounted);
 vehicle->m_pVehicle->m_pPilot=NULL;
 G_FreeEntity(vehicle);
 Undo(&g_entities[0],qtrue);
 assert(shared->inuse&&other->inuse&&!objects[sharedNum].thrustMounted&&objects[otherNum].thrustMounted&&undoSounds==feedback+3);
 // The requested defaults allow a second builder, retain departing objects,
 // and protect them when the host chooses owned-only/world editing off.
 body=CopySpawn(1,0,100);n=body->s.number;sharedEditing.integer=worldEditing.integer=1;cleanupDisconnect.integer=0;
 assert(Edit(&g_entities[1],n));JAR_MP_Disconnect(0);assert(body->inuse&&objects[n].owner==MAX_CLIENTS+1&&Edit(&g_entities[1],n));
 sharedEditing.integer=worldEditing.integer=0;assert(!Edit(&g_entities[1],n));PhysicsReset();
 puts("Owner feedback: thruster housing lifecycle, Undo acknowledgement, tool/prop retention and server editing defaults passed.");
}
