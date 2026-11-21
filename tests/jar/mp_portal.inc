static void PortalCases(void){
 RigidFixture();
 gentity_t* player=&g_entities[0];
 vec3_t point={400,0,180},angle={0,180,0};
 gentity_t* panel=SpawnWithController(player,JAR_MP_PORTAL_SURFACE_ID,point,angle);
 assert(panel&&JAR_MP_PortalSurface(&panel->s)&&panel->s.time==JAR_PORTAL_VIEW_CAMERA);
 const int n=panel->s.number,c=objects[n].output[0]-1;
 gentity_t* camera=&g_entities[c];
 assert(camera->s.bolt2==JAR_MP_PORTAL_CAMERA_TAG&&panel->r.svFlags&SVF_PORTAL);
 assert(VectorCompare(panel->s.origin,camera->r.currentOrigin)&&panel->r.contents==CONTENTS_ITEM);
 assert(fabsf(panel->r.maxs[1]-64)<.01f&&panel->r.maxs[2]==96);
 gentity_t* aim=ControllerAt("target_position",0,700);
 assert(aim);
 Move(aim,(float[3]){camera->r.currentOrigin[0],160,180});
 ControllerLink(player,n,qfalse);ControllerLink(player,aim->s.number,qfalse);
 assert(objects[n].output[0]==c+1);
 ControllerLink(player,c,qfalse);ControllerLink(player,aim->s.number,qfalse);
 assert(objects[c].output[0]==aim->s.number+1);
 PortalSync(panel);
 assert(fabsf(panel->s.angles[YAW]-90)<.01f);
 ControllerSelect(panel);
 Command(0,"jar_property","controller_portal_width_1025");assert(panel->genericValue1==1025);
 Command(0,"jar_property","property_undo");assert(panel->genericValue1==128);
 Command(0,"jar_property","controller_portal_width_256");assert(panel->genericValue1==256&&fabsf(panel->r.maxs[1]-128)<.01f);
 Command(0,"jar_property","property_undo");assert(panel->genericValue1==128);
 Command(0,"jar_property","controller_portal_height_320");assert(panel->genericValue2==320&&panel->r.maxs[2]==160);
 Command(0,"jar_property","controller_portal_mode");
 assert(panel->s.time==JAR_PORTAL_VIEW_MIRROR&&!(panel->r.svFlags&SVF_PORTAL));
 Command(0,"jar_property","property_undo");assert(panel->s.time==JAR_PORTAL_VIEW_CAMERA);
 PortalUse(camera,player,player);PortalSync(panel);
 assert(panel->s.time==JAR_PORTAL_VIEW_OFF&&!(panel->r.svFlags&SVF_PORTAL));
 PortalUse(camera,player,player);PortalSync(panel);assert(panel->s.time==JAR_PORTAL_VIEW_CAMERA);
 static jarWorld_t saved,decoded;static byte bytes[WORLD_BYTES];
 assert(WorldCapture(&saved));
 int size=WorldEncode(&saved,bytes);assert(size&&WorldDecode(bytes,size,&decoded));
 assert(decoded.copy.count==3&&decoded.copy.node[0].controller.pushSpeed==320);
 CopyAssembly(player,n);
 assert(copies[0].count==3&&PlaceCopy(player,&copies[0],(float[3]){1000,0,180},NULL,NULL));
 const int copy=players[0].selected;
 assert(copy!=n&&g_entities[copy].s.time==JAR_PORTAL_VIEW_CAMERA&&objects[copy].output[0]!=c+1);
 Undo(player,qfalse);assert(!g_entities[copy].inuse);
 G_FreeEntity(camera);PortalSync(panel);
 assert(!objects[n].output[0]&&panel->s.time==JAR_PORTAL_VIEW_OFF);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 int restored=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&JAR_MP_PortalSurface(&g_entities[i].s)){
  ++restored;assert(g_entities[i].s.eventParm==320&&g_entities[i].s.time==JAR_PORTAL_VIEW_CAMERA);
  assert(ControllerKind(objects[i].output[0]-1)==JAR_PORTAL_CAMERA_CONTROLLER);
 }
 assert(restored==1&&WorldClear());
 gentity_t* cameraRoot=SpawnWithController(player,JAR_MP_PORTAL_CAMERA_ID,point,vec3_origin);
 assert(cameraRoot);
 int cameraPanel=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&JAR_MP_PortalSurface(&g_entities[i].s))cameraPanel=i;
 assert(cameraPanel&&objects[cameraPanel].output[0]==cameraRoot->s.number+1);
 // The fixture obstruction is halfway along the point trace. Leave the same
 // 16-unit clearance as the real placement path.
 assert(g_entities[cameraPanel].s.time==JAR_PORTAL_VIEW_CAMERA&&g_entities[cameraPanel].r.currentOrigin[0]==point[0]-48);
 assert(fabsf(fabsf(g_entities[cameraPanel].r.currentAngles[YAW])-180)<.01f&&WorldClear());
 puts("MP portals: typed links, modes, Context, native camera pose, duplication and world journal passed.");
}
