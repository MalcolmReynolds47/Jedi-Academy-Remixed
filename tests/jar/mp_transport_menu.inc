static void TransportMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="6";args[3]=args[4]="0";args[5]="2";args[6]=args[7]=args[8]=args[9]="0";args[10]="-";args[11]="600";CG_JAR_Controller();assert(contextControllerKind==6&&contextController.speed==600);
 Click(490,55);Click(450,155);assert(contextEditing==205);ContextType("1234");CG_JAR_MenuUp();CG_JAR_Controller();assert(menu&&contextEditing&&!strcmp(contextInput,"1234"));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_speed_1234 99"));
 Click(450,182);assert(!strcmp(request,"jar_property controller_flags_0 99"));Click(450,210);assert(!strcmp(request,"jar_property controller_flags_3 99"));
 Click(450,155);ContextType("2147483648");int count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count&&contextEditing);
 args[11]="49";CG_JAR_Controller();assert(contextController.speed==49);args[11]="4001";CG_JAR_Controller();assert(contextController.speed==4001);args[11]="50";CG_JAR_Controller();assert(contextController.speed==50&&contextEditing);CG_JAR_Draw();
 args[2]="7";args[5]=args[11]="0";CG_JAR_Controller();assert(!contextEditing&&contextControllerKind==7);CG_JAR_Draw();Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[2]="8";args[10]="4d61726b6572";CG_JAR_Controller();assert(contextControllerKind==8&&!strcmp(contextController.text,"Marker"));CG_JAR_Draw();Click(450,155);ContextType("Exit");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_text_45786974 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);puts("MP transport properties UI: push speed bounds, mode/sound flags, teleport and destination labels, retained drafts and input lifetime passed.");
}
