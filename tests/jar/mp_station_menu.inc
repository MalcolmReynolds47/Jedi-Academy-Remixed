static void StationMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 for(int kind=24;kind<=28;++kind){char number[16];snprintf(number,sizeof(number),"%d",kind);args[1]="99";args[2]=number;args[3]="100";args[4]=args[5]=args[6]=args[9]=args[11]=args[12]=args[13]=args[14]="0";args[7]="80";args[8]="200";args[10]="-";CG_JAR_Controller();assert(contextControllerKind==kind&&contextController.count==80&&contextController.initial==200);contextPage=1;CG_JAR_Draw();
  Click(450,154);assert(contextEditing==203);Q_strncpyz(contextInput,"300",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_count_300 99"));
  Click(450,182);assert(contextEditing==210);Q_strncpyz(contextInput,"50",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_charge_50 99"));
  Click(450,210);assert(contextEditing==201);Q_strncpyz(contextInput,"500",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_delay_500 99"));
  Click(450,238);assert(!strcmp(request,"jar_property controller_nodrain 99"));Click(450,266);assert(!strcmp(request,"jar_property controller_reset 99"));Click(450,294);assert(!strcmp(request,"jar_property controller_enabled 99"));
  args[7]="201";CG_JAR_Controller();assert(contextController.count==80);args[7]="80";args[3]="0";CG_JAR_Controller();assert(contextController.waitMs==100);args[3]="100";args[5]="2";CG_JAR_Controller();assert(!contextController.flags);
 }
 CG_JAR_Key(A_ESCAPE,qtrue);puts("MP station UI: native family echoes, capacity/charge/recharge input, unlimited/refill/enabled actions and server-value bounds passed.");
}
