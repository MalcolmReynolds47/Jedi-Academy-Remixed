// Server-console world persistence. Fixed little-endian records, no pointers,
// native scripts, client slots or executable paths. GPL-2.0-only.
typedef struct {
 jarCopy_t copy;
 vec3_t velocity[MAX_GENTITIES],angular[MAX_GENTITIES];
 jarSectionStates_t sections;
 unsigned generation,checksum;
 char map[MAX_QPATH];
} jarWorld_t;
enum {
 WORLD_HEADER=84, WORLD_NODE=284, WORLD_ROPE=172, WORLD_JRW18=0x4957524a, WORLD_JRW19=0x4a57524a,
 WORLD_JRW20=0x4b57524a, WORLD_JRW21=0x4c57524a, WORLD_JRW22=0x4d57524a, WORLD_JRW23=0x4e57524a,
 WORLD_JRW24=0x4f57524a, WORLD_JRW25=0x5057524a,
 WORLD_ACTOR_MOVEMENT_V24_BYTES=5, WORLD_ACTOR_MOVEMENT_BYTES=6,
 WORLD_BYTES=WORLD_HEADER+MAX_GENTITIES*(WORLD_NODE+JAR_MP_CITY_DAMAGE_BYTES)+JAR_MP_ATTACHMENT_LIMIT*WORLD_ROPE+
             sizeof(jarSectionStates_t)+MAX_GENTITIES*WORLD_ACTOR_MOVEMENT_BYTES+4
};
typedef struct {int parked,transient;} jarAutoCapture_t;
static qboolean worldTried,worldAutoBlocked,worldAutoReplace;
static unsigned worldAutoFailedMask;
static unsigned worldLastAttempt;
static int worldInterval,worldLastSize;
static byte worldLastData[WORLD_BYTES];
static char worldAutoSlot[33],worldAutoStatus[96];
static void WorldReset(void){worldTried=worldAutoBlocked=worldAutoReplace=qfalse;worldAutoFailedMask=0;worldLastAttempt=0;worldInterval=worldLastSize=0;worldAutoSlot[0]=0;Q_strncpyz(worldAutoStatus,"waiting for map",sizeof(worldAutoStatus));}
static void WorldRestoreFailed(void){worldAutoBlocked=qtrue;Q_strncpyz(worldAutoStatus,"blocked: restore failed; load or clear explicitly",sizeof(worldAutoStatus));}
static int WorldInterval(void){return worldAutoSave.integer<=0?0:worldAutoSave.integer<30?30:worldAutoSave.integer>3600?3600:worldAutoSave.integer;}
static unsigned WorldHash(const byte*p,int size){unsigned h=2166136261u;for(int i=0;i<size;++i)h=(h^p[i])*16777619u;return h;}
static unsigned WorldGet(const byte*p,int*at){unsigned n=(unsigned)p[*at]|((unsigned)p[*at+1]<<8)|((unsigned)p[*at+2]<<16)|((unsigned)p[*at+3]<<24);*at+=4;return n;}
static void WorldPut(byte*p,int*at,unsigned n){for(int i=0;i<4;++i)p[(*at)++]=(byte)(n>>(i*8));}
static float WorldFloat(const byte*p,int*at){unsigned bits=WorldGet(p,at);float f;memcpy(&f,&bits,4);return f;}
static void WorldPutFloat(byte*p,int*at,float f){unsigned bits;memcpy(&bits,&f,4);WorldPut(p,at,bits);}
static unsigned WorldChecksum(void){
 char text[32];trap->Cvar_VariableStringBuffer("sv_mapChecksum",text,sizeof(text));
 const unsigned checksum=(unsigned)strtol(text,NULL,10);
 // Construct 1.161 adds only a non-solid ladder volume. Keep the original
 // world journal and autosave slot for this verified BSP revision.
 if(checksum==0x85fcb9a9u&&!strcmp(level.mapname,"maps/gm_construct.bsp"))return 0x7ee6cc66u;
 return checksum;
}
static qboolean WorldSlot(const char*s){if(!s||!*s||strlen(s)>32)return qfalse;for(;*s;++s)if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='_'||*s=='-'))return qfalse;return qtrue;}
static void WorldPath(const char*slot,int version,char*out,int size){Com_sprintf(out,size,"jar/worlds/%s.%d.jrw",slot,version);}
static qboolean WorldValid(const jarWorld_t*w){
 const jarCopy_t*c=&w->copy;if(!w->generation||!w->map[0]||!memchr(w->map,0,sizeof(w->map))||c->count<0||c->count>MAX_GENTITIES||c->ropes<0||c->ropes>JAR_MP_ATTACHMENT_LIMIT)return qfalse;
 int sentinel=0,shaft=0,rift=0,korriban=0,yavin=0;
 for(int i=0;i<c->count;++i){const jarCopyNode_t*n=&c->node[i];int index;const jarMPEntry_t*entry=Entry(n->id,&index);if(!entry)return qfalse;
  if(n->id==SECTION_SENTINEL_ID){
   if(sentinel>=SENTINEL_WORLD_LIMIT)return qfalse;
   const jarSentinelState_t*s=&w->sections.sentinels[sentinel++];
   if(!SectionTimersValid(s->lifts,SENTINEL_LIFTS,s->pads,SENTINEL_PADS,s->pickups,SENTINEL_PICKUPS))return qfalse;
  }else if(n->id==SECTION_SHAFT_ID){
   if(shaft>=SHAFT_WORLD_LIMIT)return qfalse;
   const jarShaftState_t*s=&w->sections.shafts[shaft++];
   if(!SectionTimersValid(s->lifts,SHAFT_LIFTS,s->pads,SHAFT_PADS,NULL,0))return qfalse;
  }else if(n->id==SECTION_RIFT_ID){
   if(rift>=RIFT_WORLD_LIMIT||!SectionTimersValid(NULL,0,NULL,0,w->sections.rifts[rift++].pickups,RIFT_PICKUPS))return qfalse;
  }else if(n->id==SECTION_KORRIBAN_ID){
   if(korriban>=KORRIBAN_WORLD_LIMIT||!SectionTimersValid(NULL,0,NULL,0,w->sections.korriban[korriban++],JAR_MP_KORRIBAN_PICKUPS))return qfalse;
  }else if(n->id==SECTION_YAVIN_ID){
   if(yavin>=YAVIN_WORLD_LIMIT||!SectionYavinStateValid(w->sections.yavin[yavin++]))return qfalse;
  }
  if(n->poseFrame<0||n->poseFrame>65535||(!CatalogBody(entry)&&n->poseFrame)||!CopyThrustValid(n)||n->frozen<0||n->frozen>1||n->thruster<0||n->thruster>1||n->noGravity<0||n->noGravity>1||n->count<0||n->health<0||n->health>100000||!JAR_ContextMassValid(n->mass))return qfalse;
  if(SectionCard(entry)&&(!VectorCompare(w->velocity[i],vec3_origin)||!VectorCompare(w->angular[i],vec3_origin)))return qfalse;
  if(CopyHiddenBeacon(n)&&(!VectorCompare(w->velocity[i],vec3_origin)||!VectorCompare(w->angular[i],vec3_origin)))return qfalse;
  if((entry->kind==2||entry->kind==3)&&n->health<1)return qfalse;
  const int boundsLimit=SectionCard(entry)||entry->kind==5?WORLD_SIZE:8192;
  if(!CopyMotionValid(n,entry->kind)||!CopyVector(n->offset,MAX_WORLD_COORD)||!CopyVector(n->angles,100000)||!CopyVector(n->mins,boundsLimit)||!CopyVector(n->maxs,boundsLimit)||!CopyVector(w->velocity[i],131072)||!CopyVector(w->angular[i],10000))return qfalse;
  for(int a=0;a<3;++a)if(n->mins[a]>n->maxs[a])return qfalse;
 }
 for(int i=0;i<c->ropes;++i)if(!CopyLinkValid(&c->rope[i],c->count)||!CopyLinkEndpoints(c,&c->rope[i]))return qfalse;
 if(!CopyControllersValid(c))return qfalse;
 return qtrue;
}
static int WorldEncode(const jarWorld_t*w,byte*data){
 if(!WorldValid(w))return 0;
 unsigned magic=WORLD_JRW18;
 for(int i=0;i<w->copy.count;++i){
  const jarCopyNode_t*n=&w->copy.node[i];const unsigned id=n->id;
  if(n->actorGravity>JAR_MOVEMENT_MAX_GRAVITY+1||n->actorFriction>JAR_MOVEMENT_MAX_FRICTION+1){magic=WORLD_JRW25;break;}
  if(n->actorGravity||n->actorFriction)magic=WORLD_JRW24;
  if(id==SECTION_YAVIN_ID&&magic<WORLD_JRW23)magic=WORLD_JRW23;
  if(id==SECTION_KORRIBAN_ID&&magic<WORLD_JRW22)magic=WORLD_JRW22;
  if((id==SECTION_SHAFT_ID||id==SECTION_RIFT_ID)&&magic<WORLD_JRW21)magic=WORLD_JRW21;
  if(id==SECTION_SENTINEL_ID&&magic<WORLD_JRW20)magic=WORLD_JRW20;
  if(id==SECTION_CITY_ID&&magic<WORLD_JRW19)magic=WORLD_JRW19;
 }
 int at=0;WorldPut(data,&at,magic);WorldPut(data,&at,w->generation);WorldPut(data,&at,w->checksum);WorldPut(data,&at,w->copy.count);WorldPut(data,&at,w->copy.ropes);
 memset(data+at,0,MAX_QPATH);Q_strncpyz((char*)data+at,w->map,MAX_QPATH);at+=MAX_QPATH;
 for(int i=0;i<w->copy.count;++i){const jarCopyNode_t*n=&w->copy.node[i];WorldPut(data,&at,n->id);WorldPut(data,&at,n->frozen);WorldPut(data,&at,n->thruster);WorldPut(data,&at,n->noGravity);WorldPut(data,&at,n->health);WorldPut(data,&at,n->count);WorldPutFloat(data,&at,n->mass);
  for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->offset[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->angles[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->mins[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->maxs[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,w->velocity[i][a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,w->angular[i][a]);WorldPut(data,&at,n->thrustMounted);WorldPutFloat(data,&at,n->thrustForce);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->thrustAnchor[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->thrustAxis[a]);WorldPut(data,&at,n->poseFrame);WorldPut(data,&at,n->vehicleMotion);for(int a=0;a<3;++a)WorldPutFloat(data,&at,n->vehicleDrift[a]);WorldPutFloat(data,&at,n->vehicleSpeed);WorldPut(data,&at,n->output[0]);WorldPut(data,&at,n->output[1]);WorldPut(data,&at,n->controllerCount);WorldPut(data,&at,n->controllerInitial);WorldPut(data,&at,n->controllerRemaining);WorldPut(data,&at,n->controllerInactive);
  WorldPut(data,&at,n->controller.waitMs);WorldPut(data,&at,n->controller.randomMs);WorldPut(data,&at,n->controller.flags);WorldPut(data,&at,n->controller.repeats);memset(data+at,0,64);Q_strncpyz((char*)data+at,n->controller.text,64);at+=64;WorldPut(data,&at,n->controller.speed);WorldPut(data,&at,n->controller.active);WorldPut(data,&at,n->controller.damage);WorldPut(data,&at,n->itemConsumed);WorldPut(data,&at,n->controller.pushSpeed);WorldPut(data,&at,n->stationUseRemaining);
 }
 for(int i=0;i<w->copy.ropes;++i){const jarCopyRope_t*r=&w->copy.rope[i];WorldPut(data,&at,(unsigned)r->first);WorldPut(data,&at,(unsigned)r->second);for(int a=0;a<3;++a)WorldPutFloat(data,&at,r->a[a]);for(int a=0;a<3;++a)WorldPutFloat(data,&at,r->b[a]);WorldPutFloat(data,&at,r->length);WorldPut(data,&at,r->kind);for(int a=0;a<4;++a)WorldPutFloat(data,&at,r->ra[a]);for(int a=0;a<4;++a)WorldPutFloat(data,&at,r->rb[a]);WorldPut(data,&at,(unsigned)r->path.count);for(int p=0;p<JAR_ROPE_PATH_BENDS;++p)for(int a=0;a<3;++a)WorldPutFloat(data,&at,r->path.bend[p][a]);}
 // Section state follows the base records in catalog-node order.
 int sentinel=0,shaft=0,rift=0,korriban=0,yavin=0;
 for(int i=0;i<w->copy.count;++i){
  const jarCopyNode_t*n=&w->copy.node[i];
  if(n->id==SECTION_CITY_ID){memcpy(data+at,n->cityDamage,JAR_MP_CITY_DAMAGE_BYTES);at+=JAR_MP_CITY_DAMAGE_BYTES;}
  if(n->id==SECTION_SENTINEL_ID){memcpy(data+at,&w->sections.sentinels[sentinel++],sizeof(jarSentinelState_t));at+=sizeof(jarSentinelState_t);}
  if(n->id==SECTION_SHAFT_ID){memcpy(data+at,&w->sections.shafts[shaft++],sizeof(jarShaftState_t));at+=sizeof(jarShaftState_t);}
  if(n->id==SECTION_RIFT_ID){memcpy(data+at,&w->sections.rifts[rift++],sizeof(jarRiftState_t));at+=sizeof(jarRiftState_t);}
  if(n->id==SECTION_KORRIBAN_ID){memcpy(data+at,w->sections.korriban[korriban++],sizeof(w->sections.korriban[0]));at+=sizeof(w->sections.korriban[0]);}
  if(n->id==SECTION_YAVIN_ID){memcpy(data+at,w->sections.yavin[yavin++],YAVIN_STATE_BYTES);at+=YAVIN_STATE_BYTES;}
 }
 if(magic>=WORLD_JRW24)for(int i=0;i<w->copy.count;++i){
  const jarCopyNode_t*n=&w->copy.node[i];if(!n->actorGravity&&!n->actorFriction)continue;
  data[at++]=(byte)i;data[at++]=(byte)(i>>8);
  data[at++]=(byte)n->actorGravity;data[at++]=(byte)(n->actorGravity>>8);data[at++]=(byte)n->actorFriction;
  if(magic==WORLD_JRW25)data[at++]=(byte)(n->actorFriction>>8);
 }
 unsigned hash=WorldHash(data,at);WorldPut(data,&at,hash);return at;
}
static qboolean WorldDecode(const byte*data,int size,jarWorld_t*out){
 static jarWorld_t w;if(size<WORLD_HEADER+4||size>WORLD_BYTES)return qfalse;int at=0;unsigned magic=WorldGet(data,&at);
 const unsigned revision=magic>>24;
 if((magic&0xffffffu)!=0x57524au||!((revision>='1'&&revision<='9')||(revision>='A'&&revision<='P')))return qfalse;
 const int index=revision<='9'?revision-'1':9+revision-'A';
 static const int nodeSizes[]={88,100,132,136,136,156,180,260,264,268,268,272,272,276,276,280,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE,WORLD_NODE};
 const int nodeSize=nodeSizes[index];
 const int linkSize=magic>=WORLD_JRW18?WORLD_ROPE:magic>=0x3557524au?72:36;
 memset(&w,0,sizeof(w));w.generation=WorldGet(data,&at);w.checksum=WorldGet(data,&at);unsigned count=WorldGet(data,&at),ropes=WorldGet(data,&at);
 if(count>MAX_GENTITIES||ropes>JAR_MP_ATTACHMENT_LIMIT)return qfalse;
 const int baseSize=WORLD_HEADER+(int)count*nodeSize+(int)ropes*linkSize+4;
 if(magic==WORLD_JRW19){
  if(size<baseSize+JAR_MP_CITY_DAMAGE_BYTES||(size-baseSize)%JAR_MP_CITY_DAMAGE_BYTES||
     (size-baseSize)/JAR_MP_CITY_DAMAGE_BYTES>(int)count)return qfalse;
 }else if(magic==WORLD_JRW20){
  if(size<baseSize+(int)sizeof(jarSentinelState_t))return qfalse;
 }else if(magic==WORLD_JRW21){
  if(size<baseSize+(int)sizeof(jarShaftState_t))return qfalse;
 }else if(magic==WORLD_JRW22){
  if(size<baseSize+(int)sizeof(w.sections.korriban[0]))return qfalse;
 }else if(magic>=WORLD_JRW24){
  if(size<baseSize+(magic==WORLD_JRW25?WORLD_ACTOR_MOVEMENT_BYTES:WORLD_ACTOR_MOVEMENT_V24_BYTES))return qfalse;
 }else if(magic==WORLD_JRW23){
  if(size<baseSize+YAVIN_STATE_BYTES)return qfalse;
 }else if(size!=baseSize)return qfalse;
 const int payloadEnd=size-4;
 int hashOffset=payloadEnd;unsigned expected=WorldHash(data,payloadEnd);if(expected!=WorldGet(data,&hashOffset))return qfalse;w.copy.count=count;w.copy.ropes=ropes;memcpy(w.map,data+at,MAX_QPATH);at+=MAX_QPATH;
 for(int i=0;i<w.copy.count;++i){jarCopyNode_t*n=&w.copy.node[i];n->id=WorldGet(data,&at);n->frozen=(int)WorldGet(data,&at);n->thruster=(int)WorldGet(data,&at);n->noGravity=(int)WorldGet(data,&at);n->health=(int)WorldGet(data,&at);n->count=(int)WorldGet(data,&at);n->mass=WorldFloat(data,&at);
  for(int a=0;a<3;++a)n->offset[a]=WorldFloat(data,&at);for(int a=0;a<3;++a)n->angles[a]=WorldFloat(data,&at);for(int a=0;a<3;++a)n->mins[a]=WorldFloat(data,&at);for(int a=0;a<3;++a)n->maxs[a]=WorldFloat(data,&at);for(int a=0;a<3;++a)w.velocity[i][a]=WorldFloat(data,&at);if(nodeSize>=100)for(int a=0;a<3;++a)w.angular[i][a]=WorldFloat(data,&at);
   n->thrustForce=3000;VectorSet(n->thrustAxis,0,0,1);if(nodeSize>=132){n->thrustMounted=(int)WorldGet(data,&at);n->thrustForce=WorldFloat(data,&at);for(int a=0;a<3;++a)n->thrustAnchor[a]=WorldFloat(data,&at);for(int a=0;a<3;++a)n->thrustAxis[a]=WorldFloat(data,&at);}if(nodeSize>=136)n->poseFrame=(int)WorldGet(data,&at);
   if(nodeSize>=156){n->vehicleMotion=(int)WorldGet(data,&at);for(int a=0;a<3;++a)n->vehicleDrift[a]=WorldFloat(data,&at);n->vehicleSpeed=WorldFloat(data,&at);}
   if(nodeSize>=180){n->output[0]=(int)WorldGet(data,&at);n->output[1]=(int)WorldGet(data,&at);n->controllerCount=(int)WorldGet(data,&at);n->controllerInitial=(int)WorldGet(data,&at);n->controllerRemaining=(int)WorldGet(data,&at);n->controllerInactive=(int)WorldGet(data,&at);}
   if(nodeSize>=260){n->controller.waitMs=(int)WorldGet(data,&at);n->controller.randomMs=(int)WorldGet(data,&at);n->controller.flags=(int)WorldGet(data,&at);n->controller.repeats=(int)WorldGet(data,&at);memcpy(n->controller.text,data+at,64);at+=64;}
   else {int index;const jarMPEntry_t*entry=Entry(n->id,&index);int kind=entry&&entry->kind==5?ControllerClass(entry->name):0;
    if(kind==3)n->controller.waitMs=1000;if(kind==1||kind==5)Q_strncpyz(n->controller.text,kind==5?"Sandbox destination":"Sandbox target activated",64);}
   if(nodeSize>=264)n->controller.speed=(int)WorldGet(data,&at);if(nodeSize>=268)n->controller.active=(int)WorldGet(data,&at);if(nodeSize>=272)n->controller.damage=(int)WorldGet(data,&at);if(nodeSize>=276)n->itemConsumed=(int)WorldGet(data,&at);if(nodeSize>=280)n->controller.pushSpeed=(int)WorldGet(data,&at);if(nodeSize>=284)n->stationUseRemaining=(int)WorldGet(data,&at);
   if(n->id!=JAR_MP_DISTANCE_ID){n->controller.count=n->controllerCount;n->controller.initial=n->controllerInitial;}
   n->controller.inactive=n->controllerInactive;
 }
 for(int i=0;i<w.copy.ropes;++i){jarCopyRope_t*r=&w.copy.rope[i];unsigned a=WorldGet(data,&at),b=WorldGet(data,&at);r->first=a==UINT_MAX?-1:(int)a;r->second=b==UINT_MAX?-1:(int)b;for(int j=0;j<3;++j)r->a[j]=WorldFloat(data,&at);for(int j=0;j<3;++j)r->b[j]=WorldFloat(data,&at);r->length=WorldFloat(data,&at);if(linkSize>=72){r->kind=(int)WorldGet(data,&at);for(int j=0;j<4;++j)r->ra[j]=WorldFloat(data,&at);for(int j=0;j<4;++j)r->rb[j]=WorldFloat(data,&at);}if(linkSize==WORLD_ROPE){unsigned bends=WorldGet(data,&at);if(bends>JAR_ROPE_PATH_BENDS)return qfalse;r->path.count=(int)bends;for(int p=0;p<JAR_ROPE_PATH_BENDS;++p)for(int j=0;j<3;++j)r->path.bend[p][j]=WorldFloat(data,&at);}}
 int sentinel=0,shaft=0,rift=0,korriban=0,yavin=0;
 if(magic>=WORLD_JRW19)for(int i=0;i<w.copy.count;++i){
  jarCopyNode_t*n=&w.copy.node[i];
  if(n->id==SECTION_CITY_ID){
   if(at>payloadEnd-JAR_MP_CITY_DAMAGE_BYTES)return qfalse;
   memcpy(n->cityDamage,data+at,JAR_MP_CITY_DAMAGE_BYTES);at+=JAR_MP_CITY_DAMAGE_BYTES;
  }
  if(magic>=WORLD_JRW20&&n->id==SECTION_SENTINEL_ID){
   if(sentinel>=SENTINEL_WORLD_LIMIT||at>payloadEnd-(int)sizeof(jarSentinelState_t))return qfalse;
   memcpy(&w.sections.sentinels[sentinel++],data+at,sizeof(jarSentinelState_t));at+=sizeof(jarSentinelState_t);
  }
  if(magic>=WORLD_JRW21&&n->id==SECTION_SHAFT_ID){
   if(shaft>=SHAFT_WORLD_LIMIT||at>payloadEnd-(int)sizeof(jarShaftState_t))return qfalse;
   memcpy(&w.sections.shafts[shaft++],data+at,sizeof(jarShaftState_t));at+=sizeof(jarShaftState_t);
  }
  if(magic>=WORLD_JRW21&&n->id==SECTION_RIFT_ID){
   if(rift>=RIFT_WORLD_LIMIT||at>payloadEnd-(int)sizeof(jarRiftState_t))return qfalse;
   memcpy(&w.sections.rifts[rift++],data+at,sizeof(jarRiftState_t));at+=sizeof(jarRiftState_t);
  }
  if(magic>=WORLD_JRW22&&n->id==SECTION_KORRIBAN_ID){
   const int bytes=sizeof(w.sections.korriban[0]);
   if(korriban>=KORRIBAN_WORLD_LIMIT||at>payloadEnd-bytes)return qfalse;
   memcpy(w.sections.korriban[korriban++],data+at,bytes);at+=bytes;
  }
  if(magic>=WORLD_JRW23&&n->id==SECTION_YAVIN_ID){
   if(yavin>=YAVIN_WORLD_LIMIT||at>payloadEnd-YAVIN_STATE_BYTES)return qfalse;
   memcpy(w.sections.yavin[yavin++],data+at,YAVIN_STATE_BYTES);at+=YAVIN_STATE_BYTES;
  }
 }
 if(magic>=WORLD_JRW24){
  const int stride=magic==WORLD_JRW25?WORLD_ACTOR_MOVEMENT_BYTES:WORLD_ACTOR_MOVEMENT_V24_BYTES;
  const int remaining=payloadEnd-at;
  if(remaining<stride||remaining%stride)return qfalse;
  int previous=-1;
  for(;at<payloadEnd;at+=stride){
   const int node=data[at]|(data[at+1]<<8);
   if(node<=previous||node>=w.copy.count)return qfalse;
   previous=node;
   jarCopyNode_t*n=&w.copy.node[node];
   n->actorGravity=data[at+2]|(data[at+3]<<8);n->actorFriction=data[at+4];
   if(magic==WORLD_JRW25)n->actorFriction|=data[at+5]<<8;
   else if(n->actorGravity>JAR_MOVEMENT_MAX_GRAVITY+1||n->actorFriction>JAR_MOVEMENT_MAX_FRICTION+1)return qfalse;
   if(!n->actorGravity&&!n->actorFriction)return qfalse;
  }
 }
 if(at!=payloadEnd||(magic==WORLD_JRW20&&!sentinel)||(magic==WORLD_JRW21&&!shaft&&!rift)||(magic==WORLD_JRW22&&!korriban)||(magic==WORLD_JRW23&&!yavin))return qfalse;
 if(magic<0x4257524au)for(int i=0;i<w.copy.count;++i)if(!CopyVector(w.copy.node[i].vehicleDrift,2048))return qfalse;
 if(!WorldValid(&w))return qfalse;*out=w;return qtrue;
}
// 0 absent, -1 unreadable/invalid, 1 complete validated revision.
static int WorldRead(const char*path,jarWorld_t*w){
 static byte data[WORLD_BYTES];fileHandle_t file=0;int size=trap->FS_Open(path,&file,FS_READ);if(!file)return 0;
 if(size<WORLD_HEADER+4||size>WORLD_BYTES){trap->FS_Close(file);return -1;}
 int read=trap->FS_Read(data,size,file);trap->FS_Close(file);return read==size&&WorldDecode(data,size,w)?1:-1;
}
static qboolean WorldCaptureMode(jarWorld_t*w,jarAutoCapture_t*automatic){
 int indices[MAX_GENTITIES];memset(w,0,sizeof(*w));for(int n=0;n<MAX_GENTITIES;++n)indices[n]=-1;
 int sentinel=0,shaft=0,rift=0,korriban=0,yavin=0;
 w->generation=1;w->checksum=WorldChecksum();Q_strncpyz(w->map,level.mapname,sizeof(w->map));
 for(int n=MAX_CLIENTS;n<level.num_entities;++n){gentity_t*e=&g_entities[n];jarObject_t*o=&objects[n];if(!e->inuse||!o->owner)continue;
  // BSP furniture is instantiated on map load. Journal only player-created
  // objects, including any furniture copied with the Duplicate tool.
  if(o->transientPickup||NativeMapProp(n))continue;
  if(o->copyPending||(!automatic&&(o->heldBy||CopyOccupied(e)))){Tell(NULL,"Release held objects, dismount and wait for NPC initialization before saving.");return qfalse;}
  if(!o->catalog){if(e->s.generic1==251)continue;Tell(NULL,"An owned entity has no persistent catalog definition; nothing saved.");return qfalse;}
  if(o->catalog<1||o->catalog>JAR_MP_CATALOG_COUNT||(e->client&&(!e->NPC||e->health<=0||(e->s.eFlags&EF_NODRAW)))){Tell(NULL,"A catalog member is unavailable, dead or not initialized; nothing saved.");return qfalse;}
  int i=w->copy.count++;indices[n]=i;jarCopyNode_t*node=&w->copy.node[i];node->poseFrame=PhysicsProp(n)?e->s.frame:0;node->id=jarMPCatalog[o->catalog-1].id;node->frozen=o->frozen;node->thruster=o->thruster;node->noGravity=o->noGravity;node->mass=o->mass;CopyThrustCapture(node,o);node->health=e->client?e->health:0;node->count=e->item?e->count:0;node->itemConsumed=o->itemConsumed;CopyControllerCapture(node,e);
  if(JAR_MP_WelderRootState(&e->s))node->controllerRemaining=WelderRemaining(e);
  if(node->id==SECTION_CITY_ID)SectionCityCapture(n,node->cityDamage);
  if(node->id==SECTION_SENTINEL_ID){
   if(sentinel>=SENTINEL_WORLD_LIMIT)return qfalse;
   jarSentinelState_t*s=&w->sections.sentinels[sentinel++];
   SectionTimersCapture(n,s->lifts,SENTINEL_LIFTS,s->pads,SENTINEL_PADS,s->pickups,SENTINEL_PICKUPS);
  }else if(node->id==SECTION_SHAFT_ID){
   if(shaft>=SHAFT_WORLD_LIMIT)return qfalse;
   jarShaftState_t*s=&w->sections.shafts[shaft++];
   SectionTimersCapture(n,s->lifts,SHAFT_LIFTS,s->pads,SHAFT_PADS,NULL,0);
  }else if(node->id==SECTION_RIFT_ID){
   if(rift>=RIFT_WORLD_LIMIT)return qfalse;
   SectionTimersCapture(n,NULL,0,NULL,0,w->sections.rifts[rift++].pickups,RIFT_PICKUPS);
  }else if(node->id==SECTION_KORRIBAN_ID){
   if(korriban>=KORRIBAN_WORLD_LIMIT)return qfalse;
   SectionTimersCapture(n,NULL,0,NULL,0,w->sections.korriban[korriban++],JAR_MP_KORRIBAN_PICKUPS);
  }else if(node->id==SECTION_YAVIN_ID){
   if(yavin>=YAVIN_WORLD_LIMIT)return qfalse;
   SectionYavinCapture(n,w->sections.yavin[yavin++]);
  }
  if(musicRank[n]){node->controller.active=1;node->controllerRemaining=musicRank[n];}
  VectorCopy(e->r.currentOrigin,node->offset);VectorCopy(e->r.currentAngles,node->angles);VectorCopy(e->r.mins,node->mins);VectorCopy(e->r.maxs,node->maxs);VectorCopy(e->client?e->client->ps.velocity:o->velocity,w->velocity[i]);if(PhysicsProp(n))VectorCopy(o->angular,w->angular[i]);
  if(e->client&&e->m_pVehicle){node->vehicleMotion=1;if(!node->frozen){VectorCopy(e->client->ps.jarVehicleDrift,node->vehicleDrift);node->vehicleSpeed=e->client->ps.speed;}}
  // Client slots, pilots and live grips are not persistent. Only the snapshot
  // parks these members; the running world and occupants remain untouched.
  if(automatic&&(o->heldBy||CopyOccupied(e))){node->frozen=1;VectorClear(w->velocity[i]);VectorClear(w->angular[i]);VectorClear(node->vehicleDrift);node->vehicleSpeed=0;++automatic->parked;}
 }
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(indices[n]>=0){
  for(int o=0;o<2;++o){int target=objects[n].output[o]-1;
   if(target>=0){if(target>=ENTITYNUM_WORLD||indices[target]<0){Tell(NULL,"A controller target has no persistent definition; nothing saved.");return qfalse;}w->copy.node[indices[n]].output[o]=indices[target]+1;}
  }
  CopyDistanceCapture(&w->copy.node[indices[n]],n,indices);
 }
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner&&g_entities[n].s.generic1==251){jarObject_t*o=&objects[n];int a=o->first-1,b=o->second-1;
  if(w->copy.ropes==JAR_MP_ATTACHMENT_LIMIT||a<0||b<0||a>ENTITYNUM_WORLD||b>ENTITYNUM_WORLD){Tell(NULL,"An attachment endpoint is invalid; nothing saved.");return qfalse;}
  if((a!=ENTITYNUM_WORLD&&indices[a]<0)||(b!=ENTITYNUM_WORLD&&indices[b]<0)){
   if(automatic){++automatic->transient;continue;}
   Tell(NULL,"An attachment endpoint has no persistent definition; nothing saved.");return qfalse;
  }
  jarRopePath_t path=o->ropePath;
  if(!RopeRoute(o,&path,NULL)){Tell(NULL,"A rope route is obstructed; nothing saved. Move the obstacle or remove the rope.");return qfalse;}
  jarCopyRope_t*r=&w->copy.rope[w->copy.ropes++];r->first=a==ENTITYNUM_WORLD?-1:indices[a];r->second=b==ENTITYNUM_WORLD?-1:indices[b];CopyLinkCapture(r,o);
  r->path=path;
 }
 if(!WorldValid(w)){Tell(NULL,"World contains unsupported properties; nothing saved.");return qfalse;}return qtrue;
}
static qboolean WorldCapture(jarWorld_t*w){return WorldCaptureMode(w,NULL);}
static qboolean WorldWrite(const char*slot,jarWorld_t*world){
 static jarWorld_t previous[2],check;static byte data[WORLD_BYTES],readback[WORLD_BYTES];char paths[2][MAX_QPATH];int valid[2];
 if(!WorldSlot(slot)){Tell(NULL,"Slot names use 1-32 letters, digits, underscores or hyphens.");return qfalse;}
 for(int i=0;i<2;++i){WorldPath(slot,i,paths[i],sizeof(paths[i]));valid[i]=WorldRead(paths[i],&previous[i]);}
 int newest=valid[0]==1?0:-1;if(valid[1]==1&&(newest<0||previous[1].generation>previous[0].generation))newest=1;
 const qboolean automatic=!strcmp(slot,worldAutoSlot);
 unsigned unknown=0;for(int i=0;i<2;++i)if(valid[i]<0&&!(worldAutoFailedMask&(1u<<i)))unknown|=1u<<i;
 if(newest<0&&(valid[0]<0||valid[1]<0)&&(!automatic||(!worldAutoReplace&&unknown))){Tell(NULL,"Existing revisions are unreadable; choose a new slot to preserve them.");return qfalse;}
 if(newest>=0&&previous[newest].generation==UINT_MAX){Tell(NULL,"Revision counter exhausted; use a new slot.");return qfalse;}
 int next=newest<0?0:1-newest;world->generation=newest<0?1:previous[newest].generation+1;int size=WorldEncode(world,data);if(!size)return qfalse;fileHandle_t file=0;
 trap->FS_Open(paths[next],&file,FS_WRITE);if(!file){Tell(NULL,"Could not open the next save revision. Previous revision retained.");return qfalse;}
 int wrote=trap->FS_Write(data,size,file);trap->FS_Close(file);
 if(wrote!=size||WorldRead(paths[next],&check)!=1||WorldEncode(&check,readback)!=size||memcmp(data,readback,size)){if(automatic)worldAutoFailedMask|=1u<<next;Tell(NULL,"Save verification failed. Previous revision retained.");return qfalse;}
 if(automatic){worldAutoReplace=qfalse;worldAutoFailedMask=0;}
 Tell(NULL,va("Saved %s revision %u: %d objects, %d attachments. Previous revision retained.",slot,world->generation,world->copy.count,world->copy.ropes));return qtrue;
}
static qboolean WorldSave(const char*slot){
 static jarWorld_t world;if(!WorldSlot(slot)){Tell(NULL,"Slot names use 1-32 letters, digits, underscores or hyphens.");return qfalse;}
 return WorldCapture(&world)&&WorldWrite(slot,&world);
}
static qboolean WorldClear(void){
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner&&RemovalOccupied(&g_entities[n])){Tell(NULL,"Dismount or leave supported sections first. World unchanged.");return qfalse;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner&&!NativeMapProp(n))G_FreeEntity(&g_entities[n]);
 worldAutoBlocked=qfalse;worldAutoReplace=qtrue;worldLastSize=0;Q_strncpyz(worldAutoStatus,"clear requested; save pending",sizeof(worldAutoStatus));
 Tell(NULL,"Spawned Sandbox world cleared. Players and map-authored entities preserved.");return qtrue;
}
static qboolean WorldLoad(const char*slot){
 static jarWorld_t revisions[2];int valid[2];char path[MAX_QPATH];
 if(!WorldSlot(slot)){Tell(NULL,"Slot names use 1-32 letters, digits, underscores or hyphens.");return qfalse;}
 for(int i=0;i<2;++i){WorldPath(slot,i,path,sizeof(path));valid[i]=WorldRead(path,&revisions[i]);}
 int newest=valid[0]==1?0:-1;if(valid[1]==1&&(newest<0||revisions[1].generation>revisions[0].generation))newest=1;
 if(newest<0){Tell(NULL,"No valid world revision found. Existing world unchanged.");return qfalse;}
 const jarWorld_t*w=&revisions[newest];if(strcmp(w->map,level.mapname)||w->checksum!=WorldChecksum()){Tell(NULL,"Saved map or BSP checksum differs. Load the original map first.");return qfalse;}
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner&&!NativeMapProp(n)){Tell(NULL,"Save/clear the existing Sandbox world first with jar_world_save / jar_world_clear. Nothing loaded.");return qfalse;}
 vec3_t origin={0,0,0};if(w->copy.count&&!PlaceCopyState(NULL,&w->copy,origin,w->velocity,w->angular,&w->sections))return qfalse;
 worldAutoBlocked=qfalse;worldAutoReplace=qtrue;worldLastSize=0;Q_strncpyz(worldAutoStatus,"loaded; save pending",sizeof(worldAutoStatus));
 Tell(NULL,va("Loaded %s revision %u: %d objects, %d attachments as server-owned. jar_worldEditing controls player edits.%s",slot,w->generation,w->copy.count,w->copy.ropes,valid[1-newest]<0?" Invalid other revision ignored.":""));return qtrue;
}
static qboolean WorldAutomaticSave(void){
 static jarWorld_t world;static byte encoded[WORLD_BYTES];jarAutoCapture_t info={0,0};
 if(worldAutoBlocked){Tell(NULL,worldAutoStatus);return qfalse;}
 if(!WorldCaptureMode(&world,&info)){Q_strncpyz(worldAutoStatus,"deferred: world not ready; previous revision retained",sizeof(worldAutoStatus));return qfalse;}
 int size=WorldEncode(&world,encoded);if(!size)return qfalse;
 if(size==worldLastSize&&!memcmp(encoded,worldLastData,size)){Com_sprintf(worldAutoStatus,sizeof(worldAutoStatus),"unchanged; %d parked; %d transient attachments omitted",info.parked,info.transient);return qtrue;}
 if(!WorldWrite(worldAutoSlot,&world)){Q_strncpyz(worldAutoStatus,"write failed; previous revision retained",sizeof(worldAutoStatus));return qfalse;}
 memcpy(worldLastData,encoded,size);worldLastSize=size;
 Com_sprintf(worldAutoStatus,sizeof(worldAutoStatus),"verified revision %u; %d parked; %d transient attachments omitted",world.generation,info.parked,info.transient);
 Tell(NULL,va("Autosave %s: %s.",worldAutoSlot,worldAutoStatus));return qtrue;
}
static void WorldFrame(void){
 trap->Cvar_Update(&worldAutoSave);trap->Cvar_Update(&worldAutoLoad);
 if(!level.mapname[0]||!sandbox.integer)return;
 if(!worldTried){
  static jarWorld_t existing;char path[MAX_QPATH];qboolean present=qfalse;worldTried=qtrue;
  Com_sprintf(worldAutoSlot,sizeof(worldAutoSlot),"auto_%08x_%08x",WorldChecksum(),WorldHash((const byte*)level.mapname,(int)strlen(level.mapname)));
  worldInterval=WorldInterval();worldLastAttempt=(unsigned)level.time;
  trap->Cvar_Update(&worldLoad);
  if(worldLoad.string[0]){if(!WorldLoad(worldLoad.string))WorldRestoreFailed();}
  else {
   for(int i=0;i<2;++i){WorldPath(worldAutoSlot,i,path,sizeof(path));if(WorldRead(path,&existing)!=0)present=qtrue;}
   if(present){if(!worldAutoLoad.integer||!WorldLoad(worldAutoSlot))WorldRestoreFailed();}
   else Q_strncpyz(worldAutoStatus,"new map; save pending",sizeof(worldAutoStatus));
  }
  return;
 }
 int interval=WorldInterval();
 if(interval!=worldInterval){worldInterval=interval;worldLastAttempt=(unsigned)level.time;}
 if(!interval)return;
 if((unsigned)level.time-worldLastAttempt>=(unsigned)interval*1000){worldLastAttempt=(unsigned)level.time;WorldAutomaticSave();}
}
void JAR_MP_Shutdown(void){
 trap->Cvar_Update(&sandbox);trap->Cvar_Update(&worldAutoSave);
 if(worldTried&&sandbox.integer&&WorldInterval()>0&&level.mapname[0])WorldAutomaticSave();
 PhysicsReset();
}
#include "g_jar_actor_info.inc"
#include "g_jar_vehicle_rig.inc"
qboolean JAR_MP_ServerCommand(const char*command){
 if(ActorInfo(command)||VehicleRigInfo(command))return qtrue;
 if(!Q_stricmp(command,"jar_model_info")){
  char idText[32],frameText[32],*end;int index;trap->Argv(1,idText,sizeof(idText));trap->Argv(2,frameText,sizeof(frameText));unsigned long id=strtoul(idText,&end,10);
  if(trap->Argc()!=3||!*idText||*end||id>UINT_MAX||!Entry((unsigned)id,&index)){Tell(NULL,"Server console: jar_model_info CATALOG_ID FRAME");return qtrue;}
  long frame=strtol(frameText,&end,10);if(!*frameText||*end||frame<0||frame>65535){Tell(NULL,"Frame must be 0..65535.");return qtrue;}
  vec3_t low,high;qboolean valid=PhysicsModelBounds(index+1,(int)frame,low,high);
  if(valid)Tell(NULL,va("Model %u frame %d of %d: %.6f %.6f %.6f %.6f %.6f %.6f",(unsigned)id,(int)frame,PhysicsFrameCount(index+1),low[0],low[1],low[2],high[0],high[1],high[2]));
  else Tell(NULL,va("Model %u frame %d of %d: unavailable",(unsigned)id,(int)frame,PhysicsFrameCount(index+1)));return qtrue;
 }
 if(!Q_stricmp(command,"jar_physics_status")){PhysicsStatus();return qtrue;}
 qboolean save=!Q_stricmp(command,"jar_world_save"),load=!Q_stricmp(command,"jar_world_load"),clear=!Q_stricmp(command,"jar_world_clear"),status=!Q_stricmp(command,"jar_world_status");
 if(!save&&!load&&!clear&&!status)return qfalse;
 if(status){int count=0,ropes=0,pending=0;for(int n=MAX_CLIENTS;n<level.num_entities;++n)if(g_entities[n].inuse&&objects[n].owner){if(g_entities[n].s.generic1==251)++ropes;else ++count;if(objects[n].copyPending)++pending;}
  Tell(NULL,va("World: %d objects, %d attachments, %d pending native members; map %s. Autosave %s (%ds): %s.",count,ropes,pending,level.mapname,worldAutoSlot,WorldInterval(),worldAutoStatus));return qtrue;}
 if(trap->Argc()!=(clear?1:2)){Tell(NULL,"Server console: jar_world_save SLOT | jar_world_load SLOT | jar_world_clear | jar_world_status");return qtrue;}
 if(!sandbox.integer){Tell(NULL,"Enable jar_sandbox before saving, loading or clearing worlds.");return qtrue;}
 if(clear)WorldClear();else{char slot[MAX_TOKEN_CHARS];trap->Argv(1,slot,sizeof(slot));if(save)WorldSave(slot);else WorldLoad(slot);}return qtrue;
}
