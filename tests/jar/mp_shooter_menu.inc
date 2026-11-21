static void ShooterMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="86744182";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 const int kinds[]={JAR_PLASMA_SHOOTER_CONTROLLER,JAR_GRENADE_SHOOTER_CONTROLLER,
                    JAR_ROCKET_SHOOTER_CONTROLLER,JAR_BLASTER_SHOOTER_CONTROLLER};
 for(int i=0;i<ARRAY_LEN(kinds);++i){
  char kind[16];Com_sprintf(kind,sizeof(kind),"%d",kinds[i]);
  args[1]="99";args[2]=kind;args[3]="500";args[4]="200";args[5]="0";
  args[6]=args[7]=args[8]=args[9]="0";args[10]="-";args[11]=args[12]=args[13]=args[14]="0";
  CG_JAR_Controller();assert(contextControllerKind==kinds[i]&&contextController.waitMs==500);
  contextPage=1;CG_JAR_Draw();
  Click(450,154);assert(contextEditing==201);
  Q_strncpyz(contextInput,"2147483648",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(contextEditing==201);
  Q_strncpyz(contextInput,".",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(contextEditing==201);
  Q_strncpyz(contextInput,"120000",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_delay_120000 99"));
  Click(450,182);assert(contextEditing==202);
  Q_strncpyz(contextInput,"2147483648",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(contextEditing==202);
  Q_strncpyz(contextInput,"95000",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_random_95000 99"));
  Click(450,210);assert(!strcmp(request,"jar_property controller_shooter_alt 99"));
  Click(450,238);assert(!strcmp(request,"jar_property controller_shooter_repeat 99"));
  Click(450,266);assert(!strcmp(request,"jar_property activate 99"));
  Click(450,294);assert(!strcmp(request,"jar_property controller_enabled 99"));
  args[12]="1";CG_JAR_Controller();assert(!contextController.active);
  args[5]="3";CG_JAR_Controller();assert(contextController.active&&contextController.flags==3);
  args[9]="1";CG_JAR_Controller();assert(contextController.active&&!contextController.inactive);
  args[12]="0";args[13]="1";CG_JAR_Controller();assert(contextController.active&&!contextController.inactive);
  args[13]="0";CG_JAR_Controller();assert(!contextController.active&&contextController.inactive);
  int sent=requests;Click(450,266);assert(requests==sent);
 }
 args[2]="";CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP shooter UI: numeric interval and jitter, typed fire modes, activation and authoritative disabled state passed.");
}
