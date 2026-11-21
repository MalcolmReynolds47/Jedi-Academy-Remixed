static char locationConfig[MAX_LOCATIONS][64];
static qboolean locationVisible;

static void LocationConfigSet(int index,const char *value){
 assert(index>CS_LOCATIONS&&index<CS_LOCATIONS+MAX_LOCATIONS);
 Q_strncpyz(locationConfig[index-CS_LOCATIONS],value,sizeof(locationConfig[0]));
}

static qboolean LocationPVS(const vec3_t from,const vec3_t to){return locationVisible;}

static locationData_t *LocationAt(const vec3_t origin, locationData_t *authored, float distance){
 gentity_t subject={0};
 VectorCopy(origin,subject.r.currentOrigin);
 return JAR_MP_Location(&subject,authored,distance);
}

static void LocationCases(void){
 RigidFixture();
 imports.SetConfigstring=LocationConfigSet;imports.InPVS=LocationPVS;
 level.locations.num=1;
 locationData_t *authored=&level.locations.data[0];authored->cs_index=1;
 Q_strncpyz(locationConfig[1],"Authored",sizeof(locationConfig[1]));
 gentity_t *owner=&g_entities[0];
 gentity_t *marker=SpawnWithController(owner,95482230u,(vec3_t){300,0,80},vec3_origin);
 gentity_t *volume=SpawnWithController(owner,93292771u,(vec3_t){600,0,80},(vec3_t){0,45,0});
 assert(marker&&volume&&LocationSlot(marker->s.number)==2&&LocationSlot(volume->s.number)==3);
 assert(!strcmp(locationConfig[1],"Authored")&&!strcmp(locationConfig[2],"Sandbox location"));
 assert(!objects[marker->s.number].output[0]&&!objects[volume->s.number].output[0]);
 locationVisible=qtrue;
 assert(LocationAt(marker->r.currentOrigin,authored,2000)->cs_index==2);
 assert(LocationAt((vec3_t){250,0,80},authored,100)->cs_index==1);
 assert(LocationAt((vec3_t){615,20,80},authored,100)->cs_index==3);
 assert(LocationAt((vec3_t){710,110,80},authored,100)->cs_index==1); // Inside AABB, outside rotated volume.
 gentity_t subject={0};
 VectorSet(subject.r.mins,-15,-15,-24);
 VectorSet(subject.r.maxs,15,15,32);
 VectorSet(subject.r.currentOrigin,700,100,80);
 assert(LocationAt(subject.r.currentOrigin,authored,100)==authored);
 assert(JAR_MP_Location(&subject,authored,100)->cs_index==3);
 VectorSet(subject.r.currentOrigin,720,120,80);
 assert(JAR_MP_Location(&subject,authored,100)==authored);
 locationVisible=qfalse;
 assert(LocationAt(marker->r.currentOrigin,authored,2000)==authored);
 assert(LocationAt(marker->r.currentOrigin,NULL,0)->cs_index==2);
 assert(LocationAt(volume->r.currentOrigin,NULL,0)->cs_index==3);
 sandbox.integer=0;assert(LocationAt(volume->r.currentOrigin,authored,2000)==authored);sandbox.integer=1;
 objects[volume->s.number].copyPending=1;
 assert(LocationAt(volume->r.currentOrigin,authored,2000)==authored);
 objects[volume->s.number].copyPending=0;

 ControllerSelect(volume);
 Command(0,"jar_property","controller_text_48616E676172");
 assert(!strcmp(locationConfig[3],"Hangar"));
 Command(0,"jar_property","controller_location_color");
 assert(portableLocations[3].count==1);
 Command(0,"jar_property","controller_extent_0");assert(volume->speed==128);
 Command(0,"jar_property","controller_extent_64");assert(volume->speed==64);
 assert(LocationAt((vec3_t){660,60,80},authored,2000)==authored);
 Command(0,"jar_property","property_undo");assert(volume->speed==128);
 Command(0,"jar_property","controller_extent_64");
 Command(0,"jar_property","controller_enabled");
 assert(LocationAt(volume->r.currentOrigin,authored,2000)==authored);
 Command(0,"jar_property","property_undo");
 assert(LocationAt(volume->r.currentOrigin,authored,2000)->cs_index==3);

 static jarWorld_t saved,decoded;
 static byte encoded[WORLD_BYTES];
 assert(WorldCapture(&saved));
 const int size=WorldEncode(&saved,encoded);
 assert(size>0&&WorldDecode(encoded,size,&decoded));
 assert(WorldClear()&&!locationConfig[2][0]&&!locationConfig[3][0]);
 assert(PlaceCopy(NULL,&decoded.copy,vec3_origin,decoded.velocity,decoded.angular));
 volume=NULL;
 for(int n=MAX_CLIENTS;n<level.num_entities;++n)
  if(ControllerKind(n)==JAR_LOCATION_VOLUME_CONTROLLER)volume=&g_entities[n];
 assert(volume&&volume->speed==64);
 locationData_t *restored=LocationAt(volume->r.currentOrigin,authored,2000);
 assert(restored&&restored->count==1&&!strcmp(restored->message,"Hangar"));
 assert(WorldClear()&&!strcmp(locationConfig[1],"Authored"));

 level.locations.num=MAX_LOCATIONS-2;
 marker=SpawnWithController(owner,95482230u,(vec3_t){300,0,80},vec3_origin);
 assert(marker&&LocationSlot(marker->s.number)==MAX_LOCATIONS-1);
 assert(!SpawnWithController(owner,93292771u,(vec3_t){600,0,80},vec3_origin));
 G_FreeEntity(marker);
 volume=SpawnWithController(owner,93292771u,(vec3_t){600,0,80},vec3_origin);
 assert(volume&&LocationSlot(volume->s.number)==MAX_LOCATIONS-1);
 assert(WorldClear());
 imports.SetConfigstring=NULL;imports.InPVS=NULL;
 puts("MP locations: authored indices, native labels, oriented volumes, edits, Undo, save/restore and bounded index reuse passed.");
}
