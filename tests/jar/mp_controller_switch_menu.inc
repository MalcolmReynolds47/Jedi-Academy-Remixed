static void ControllerSwitchMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 for(int kind=11;kind<=12;++kind){
  args[1]="99";args[2]=kind==11?"11":"12";args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]=args[11]=args[12]="0";args[10]="-";CG_JAR_Controller();assert(contextControllerKind==kind);
  contextPage=1;CG_JAR_Draw();int count=requests;Click(450,155);Click(450,182);assert(!contextEditing&&requests==count);Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
  args[9]="1";CG_JAR_Controller();assert(contextController.inactive);CG_JAR_Draw();
  args[5]="1";CG_JAR_Controller();assert(contextController.flags==0);args[5]="0";args[10]="41";CG_JAR_Controller();assert(!contextController.text[0]);args[10]="-";args[2]="33";CG_JAR_Controller();assert(contextControllerKind==kind);
 }
 CG_JAR_Key(A_ESCAPE,qtrue);puts("MP controller switch UI: typed state, Enabled command, no media fields and malformed echo rejection passed.");
}
