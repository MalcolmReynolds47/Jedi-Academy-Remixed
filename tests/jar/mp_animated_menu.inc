static void AnimatedMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]="0";args[3]="0";args[4]="99";args[5]="1";args[6]="92740395";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="44";args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]="0";
 args[10]="-";args[11]=args[12]=args[13]=args[14]="0";
 CG_JAR_Controller();assert(contextControllerKind==JAR_BOMB_CONTROLLER&&!contextController.active);
 contextPage=1;CG_JAR_Draw();
 Click(450,182);assert(!strcmp(request,"jar_property activate 99"));
 Click(450,210);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[12]="1";CG_JAR_Controller();assert(contextController.active==JAR_MP_BOMB_DISARMING);
 int sent=requests;Click(450,182);assert(requests==sent);
 args[12]="2";CG_JAR_Controller();assert(contextController.active==JAR_MP_BOMB_DISARMED);
 args[12]="3";CG_JAR_Controller();assert(contextController.active==JAR_MP_BOMB_DISARMED);
 args[2]="45";args[12]="0";CG_JAR_Controller();assert(contextControllerKind==JAR_BEACON_CONTROLLER&&!contextController.active);
 CG_JAR_Draw();Click(450,182);assert(!strcmp(request,"jar_property activate 99"));
 args[12]="1";CG_JAR_Controller();assert(contextController.active==JAR_MP_BEACON_DEPLOYING);
 args[12]="2";CG_JAR_Controller();assert(contextController.active==JAR_MP_BEACON_SIGNALLING);
 args[13]="1";args[9]="1";CG_JAR_Controller();assert(!contextController.inactive);
 args[13]="0";CG_JAR_Controller();assert(contextController.inactive);
 sent=requests;Click(450,182);assert(requests==sent);
 Click(450,210);assert(!strcmp(request,"jar_property controller_enabled 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP animated controller UI: typed bomb and beacon phases, activation and Enabled passed.");
}
