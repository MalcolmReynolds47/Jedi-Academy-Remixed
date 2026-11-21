static int probeCount;
static jarProbe_t submittedProbe;
static int SubmitProbes(const jarProbe_t* probes,int count){
 probeCount=count;
 if(count)submittedProbe=probes[0];
 return count;
}
static void ReflectionMenuCases(void){
 imports.JAR_SetReflectionProbes=SubmitProbes;
 snapshot.numEntities=2;
 snapshot.entities[0]=(entityState_t){.eType=ET_GENERAL,.generic1=250,.bolt2=JAR_MP_PROBE_TAG,.time=3,.time2=401,.speed=512};
 VectorSet(snapshot.entities[0].pos.trBase,80,-100,72);
 snapshot.entities[1]=snapshot.entities[0];snapshot.entities[1].time=0;
 CG_JAR_ReflectionProbes();assert(probeCount==1&&submittedProbe.radius==512&&submittedProbe.revision==3&&submittedProbe.origin[2]==72);
 const unsigned first=submittedProbe.owner;
 CG_JAR_ClearReflectionProbes();assert(!probeCount);
 CG_JAR_ReflectionProbes();assert(probeCount==1&&submittedProbe.owner!=first);
 snapshot.numEntities=0;++snapshot.serverTime;CG_JAR_ReflectionProbes();assert(!probeCount);
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="98286831";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 for(int i=13;i<19;++i)args[i]="0";CG_JAR_Selection();
 char kind[8];Com_sprintf(kind,sizeof(kind),"%d",JAR_REFLECTION_CONTROLLER);
 args[1]="99";args[2]=kind;args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]=args[13]=args[14]="0";args[12]="1";args[10]="-";args[11]="32768";
 CG_JAR_Controller();assert(contextControllerKind==JAR_REFLECTION_CONTROLLER&&contextController.speed==32768);
 contextPage=1;CG_JAR_Draw();
 Click(450,155);assert(contextEditing==CONTEXT_PROBE_RADIUS);
 CG_JAR_Key('6'|K_CHAR_FLAG,qtrue);CG_JAR_Key('4'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_probe_radius_64 99"));
 Click(450,182);assert(!strcmp(request,"jar_property controller_probe_active 99"));
 Click(450,212);assert(!strcmp(request,"jar_property controller_probe_refresh 99"));
 Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);imports.JAR_SetReflectionProbes=NULL;
 puts("MP reflection UI: typed radius, toggle, recapture, full snapshot and map identity passed.");
}
static void LogicControllerMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="14";args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]=args[11]=args[13]="0";args[12]="1";args[10]="-";CG_JAR_Controller();assert(contextControllerKind==14&&contextController.active);contextPage=1;CG_JAR_Draw();
 Click(450,155);assert(!strcmp(request,"jar_property controller_flags_1 99"));Click(450,182);assert(!strcmp(request,"jar_property controller_playing 99"));Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[2]="15";args[3]="1000";args[12]="0";CG_JAR_Controller();assert(contextControllerKind==15&&contextController.waitMs==1000&&!contextController.active);CG_JAR_Draw();
 Click(450,155);assert(contextEditing==201);CG_JAR_Key('5'|K_CHAR_FLAG,qtrue);CG_JAR_Key('0'|K_CHAR_FLAG,qtrue);CG_JAR_Key('0'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_delay_500 99"));
 Click(450,182);assert(contextEditing==202);CG_JAR_Key('0'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_random_0 99"));
 Click(450,210);assert(!strcmp(request,"jar_property controller_playing 99"));Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[4]="1001";CG_JAR_Controller();assert(contextController.randomMs==0);args[4]="0";args[2]="33";CG_JAR_Controller();assert(contextControllerKind==15);
 CG_JAR_Key(A_ESCAPE,qtrue);puts("MP logic UI: Timer interval/random/running and Random use-once/armed controls, typed echo validation and Enabled passed.");
}
