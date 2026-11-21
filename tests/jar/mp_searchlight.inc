static unsigned searchlightNotices;
static int searchlightBlocked;

static void SearchlightSend(int c,const char*message){
 if(c>=0&&c<MAX_CLIENTS&&!strcmp(message,"cp \"Searchlight detected you\""))searchlightNotices|=1u<<c;
}

static void SearchlightTrace(trace_t*trace,const vec3_t start,const vec3_t mins,const vec3_t maxs,
                             const vec3_t end,int pass,int mask,int capsule,int flags,int lod){
 Trace(trace,start,mins,maxs,end,pass,mask,capsule,flags,lod);
 if(mask!=MASK_SHOT)return;
 trace->fraction=1;
 if(searchlightBlocked&&start[0]<150&&end[0]>150){
  vec3_t delta;VectorSubtract(end,start,delta);trace->fraction=(150-start[0])/delta[0];
  VectorMA(start,trace->fraction,delta,trace->endpos);trace->entityNum=ENTITYNUM_WORLD;
 }
}

static void SearchlightCases(void){
 RigidFixture();level.maxclients=3;imports.SendServerCommand=SearchlightSend;
 gentity_t*p=&g_entities[0];vec3_t point={100,0,80};
 gentity_t*root=SpawnWithController(p,JAR_MP_SEARCHLIGHT_ID,point,vec3_origin);assert(root);
 const int n=root->s.number,aim=objects[n].output[0]-1,notice=objects[n].output[1]-1;
 assert(JAR_MP_Searchlight(&root->s)&&root->r.contents==CONTENTS_ITEM&&g_entities[notice].spawnflags==4);
 VectorSet(g_entities[1].r.currentOrigin,300,0,80);VectorSet(g_entities[2].r.currentOrigin,320,0,80);
 searchlightNotices=0;searchlightBlocked=1;imports.Trace=SearchlightTrace;
 SearchlightFrame(root);assert(!searchlightNotices&&root->s.origin[0]==150&&root->s.time);
 searchlightBlocked=0;level.time+=50;SearchlightFrame(root);assert(searchlightNotices==6&&root->s.origin[0]>2100);
 searchlightNotices=0;level.time+=1001;SearchlightFrame(root);assert(!searchlightNotices);
 SearchlightClearClient(1);players[1].noTools=1;level.time+=50;SearchlightFrame(root);assert(!searchlightNotices);
 players[1].noTools=0;level.time+=50;SearchlightFrame(root);assert(searchlightNotices==2);
 Move(&g_entities[aim],(float[3]){100,128,80});level.time+=50;SearchlightFrame(root);
 assert(fabsf(root->s.origin[1]-2060)<.01f&&fabsf(root->s.origin[0]-100)<.01f);
 imports.Trace=Trace;
 ControllerSelect(root);Command(0,"jar_property","controller_searchlight_range_131073");assert(root->genericValue1==2048);
 Command(0,"jar_property","controller_searchlight_range_1600");assert(root->genericValue1==1600);
 Command(0,"jar_property","property_undo");assert(root->genericValue1==2048);
 Command(0,"jar_property","controller_searchlight_color_80c0ff");assert((root->spawnflags&JAR_SEARCHLIGHT_WHITE)==0x80c0ff);
 Command(0,"jar_property","controller_searchlight_track");assert(!(root->spawnflags&JAR_SEARCHLIGHT_TRACK));
 static jarWorld_t saved,decoded;static byte data[WORLD_BYTES];assert(WorldCapture(&saved));
 int size=WorldEncode(&saved,data);assert(size&&WorldDecode(data,size,&decoded));
 assert(decoded.copy.count==3&&decoded.copy.node[0].controller.flags==0x80c0ff&&decoded.copy.node[0].controllerRemaining>0);
 CopyAssembly(p,n);assert(copies[0].count==3&&PlaceCopy(p,&copies[0],(float[3]){600,0,80},NULL,NULL));
 const int copy=players[0].selected;assert(copy!=n&&JAR_MP_Searchlight(&g_entities[copy].s));
 assert(objects[copy].output[0]!=objects[n].output[0]&&objects[copy].output[1]!=objects[n].output[1]);
 Undo(p,qfalse);assert(!g_entities[copy].inuse);
 assert(WorldClear()&&PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 int restored=0;
 for(int i=MAX_CLIENTS;i<level.num_entities;++i)if(g_entities[i].inuse&&JAR_MP_Searchlight(&g_entities[i].s)){
  ++restored;
  assert(g_entities[i].spawnflags==0x80c0ff&&g_entities[i].genericValue5==0&&g_entities[i].painDebounceTime>level.time);
  assert(objects[i].output[0]&&objects[i].output[1]);
 }
 assert(restored==1&&WorldClear());imports.SendServerCommand=Send;
 puts("MP searchlight: native clipping, per-player entry and consent, tracking, typed Context, copy and journal passed.");
}
