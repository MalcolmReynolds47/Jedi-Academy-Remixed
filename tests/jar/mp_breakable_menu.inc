static void BreakableMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 for(int k=30;k<=32;++k){
  const char*kinds[]={"30","31","32"};args[1]="99";args[2]=kinds[k-30];args[3]="1000";args[4]="500";args[5]=args[6]=args[9]=args[11]=args[13]="0";args[7]="40";args[8]="100";args[10]="-";args[12]="1";args[14]="100";CG_JAR_Controller();assert(contextControllerKind==k&&contextController.count==40);contextPage=1;CG_JAR_Draw();
  Click(450,154);assert(contextEditing==203);Q_strncpyz(contextInput,"200",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_count_200 99"));
  Click(450,210);assert(!strcmp(request,"jar_property controller_damagepolicy 99"));Click(450,238);assert(!strcmp(request,"jar_property controller_playeruse 99"));Click(550,238);assert(!strcmp(request,"jar_property controller_usemode 99"));Click(450,266);assert(!strcmp(request,"jar_property controller_repair 99"));Click(550,266);assert(!strcmp(request,"jar_property controller_enabled 99"));
  Click(450,322);assert(breakPage);CG_JAR_Draw();Click(450,154);assert(!strcmp(request,"jar_property controller_material 99"));
  int y[]={182,210,238,266},fields[]={211,206,212,202};const char*requests[]={"jar_property controller_chunks_200 99","jar_property controller_damage_200 99","jar_property controller_radius_200 99","jar_property controller_random_200 99"};
  for(int i=0;i<4;++i){Click(450,y[i]);assert(contextEditing==fields[i]);Q_strncpyz(contextInput,"200",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,requests[i]));}
  Click(450,294);assert(!strcmp(request,"jar_property controller_explosion 99"));args[12]="2";args[7]="0";CG_JAR_Controller();assert(contextController.active==2);args[12]="3";CG_JAR_Controller();assert(contextController.active==2);args[12]="2";args[6]="17";CG_JAR_Controller();assert(contextController.repeats==0);
 }
 args[1]="99";args[2]="36";args[3]=args[4]=args[5]=args[6]=args[9]=args[14]="0";
 args[7]=args[8]="40";args[10]="-";args[11]="200";args[12]="1";args[13]="100";
 CG_JAR_Controller();assert(contextControllerKind==JAR_CARGO_BARREL_CONTROLLER&&contextController.count==40);
 CG_JAR_Draw();
 const int cargoY[]={154,182,210},cargoFields[]={203,206,212};
 const char* cargoRequests[]={"jar_property controller_count_125 99","jar_property controller_damage_125 99","jar_property controller_radius_125 99"};
 for(int i=0;i<3;++i){Click(450,cargoY[i]);assert(contextEditing==cargoFields[i]);Q_strncpyz(contextInput,"125",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,cargoRequests[i]));}
 Click(450,238);assert(!strcmp(request,"jar_property activate 99"));
 Click(450,266);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[12]="2";args[7]="0";CG_JAR_Controller();assert(contextController.active==2);
 Click(450,154);assert(!contextEditing);
 args[12]="0";CG_JAR_Controller();assert(contextController.active==2);
 args[12]="2";args[4]="1";CG_JAR_Controller();assert(!contextController.randomMs);
 args[2]="37";args[4]="0";args[7]=args[8]="800";args[11]="60";args[12]="1";args[13]="0";
 CG_JAR_Controller();assert(contextControllerKind==JAR_EMPLACED_CONTROLLER&&contextController.count==800&&contextController.speed==60);
 CG_JAR_Draw();
 Click(450,182);assert(contextEditing==213);Q_strncpyz(contextInput,"2147483648",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(contextEditing==213);
 Q_strncpyz(contextInput,"90",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_yaw_90 99"));
 Click(450,210);assert(!strcmp(request,"jar_property activate 99"));
 Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[7]="0";args[12]="2";CG_JAR_Controller();assert(contextController.active==2);
 args[12]="0";CG_JAR_Controller();assert(!contextController.active);
 Click(450,210);assert(!strcmp(request,"jar_property controller_repair 99"));
 args[2]="38";args[7]=args[8]="250";args[11]="180";args[12]="1";args[14]="999";
 CG_JAR_Controller();assert(contextControllerKind==JAR_EWEB_CONTROLLER&&contextController.count==250&&contextController.pushSpeed==999);
 CG_JAR_Draw();
 Click(450,210);assert(contextEditing==214);Q_strncpyz(contextInput,"2147483648",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(contextEditing==214);
 Q_strncpyz(contextInput,"123",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_ammo_123 99"));
 Click(450,238);assert(!strcmp(request,"jar_property activate 99"));
 Click(450,266);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[14]="1500";CG_JAR_Controller();assert(contextController.pushSpeed==1500);
 const char* explosiveKinds[]={"39","40"};
 const char* explosiveHealth[]={"40","20"},* explosiveRadius[]={"128","48"},* explosiveDamage[]={"50","32"};
 for(int i=0;i<2;++i){
  const int kind=i?JAR_GAS_TANK_CONTROLLER:JAR_EXPLODING_CRATE_CONTROLLER;
  args[2]=explosiveKinds[i];args[5]=i?"2":"0";args[7]=args[8]=explosiveHealth[i];
  args[11]=explosiveRadius[i];args[12]="1";args[13]=explosiveDamage[i];args[14]="0";
  CG_JAR_Controller();assert(contextControllerKind==kind&&contextController.active==1);
  assert(contextController.flags==(i?JAR_GAS_TANK_PULSE_SPENT:0));
  CG_JAR_Draw();
  for(int j=0;j<3;++j){
   Click(450,cargoY[j]);assert(contextEditing==cargoFields[j]);
   Q_strncpyz(contextInput,"125",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,cargoRequests[j]));
  }
  Click(450,238);assert(!strcmp(request,"jar_property activate 99"));
  Click(450,266);assert(!strcmp(request,"jar_property controller_enabled 99"));
  args[5]=i?"1":"2";args[7]="0";args[12]="2";
  CG_JAR_Controller();assert(contextController.active==1);
  args[5]="0";CG_JAR_Controller();assert(contextController.active==2);
  CG_JAR_Draw();
  Click(450,154);assert(!contextEditing);
  Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 }
 CG_JAR_Key(A_ESCAPE,qtrue);puts("MP breakable UI: typed health, policy, repair, effects and damage controls with authoritative state validation passed.");
}
