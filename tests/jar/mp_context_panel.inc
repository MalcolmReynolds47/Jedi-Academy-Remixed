static void TransformMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="1";args[3]="3";args[4]="99";args[5]="1";args[6]="0";args[7]="2687";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 args[13]="100";args[14]="200";args[15]="300";args[16]="0";args[17]="90";args[18]="0";CG_JAR_Selection();
 assert(!strcmp(contextTransformDraft[0],"100"));int count=requests;
 Click(460,155);assert(contextEditing==100);ContextType("-123.25");CG_JAR_Key(A_ENTER,qtrue);assert(requests==count&&!contextEditing);
 Click(460,175);ContextType("24.75");args[13]="500";CG_JAR_Selection();assert(contextEditing==101&&!strcmp(contextInput,"24.75")&&!strcmp(contextTransformDraft[0],"-123.25"));
 Click(460,215);assert(!strcmp(request,"jar_property transform_-123.25_24.75_300_0_90_0 99")&&!contextEditing&&contextTransform[0]==500);
 Click(550,125);assert(!strcmp(contextTransformDraft[0],"500")&&!strcmp(contextTransformDraft[1],"200"));
 Click(460,195);ContextType("1e999");count=requests;Click(460,215);assert(requests==count&&contextEditing==102);
 Click(100,425);assert(!contextEditing&&!strcmp(request,"jar_release"));
 Click(460,195);ContextType("-");Click(220,450);assert(!contextEditing&&!strcmp(request,"jar_allowTools 0"));
 Click(460,260);ContextType("3601");count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(!contextEditing&&requests==count&&!strcmp(contextTransformDraft[4],"3601"));Click(550,125);assert(!contextEditing);
 args[16]="nan";CG_JAR_Selection();assert(contextTransform[3]==0);args[16]="0";
 Click(460,155);ContextType("42");args[4]="100";CG_JAR_Selection();assert(!contextEditing&&!contextTransformPage);count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count);
 Click(460,155);ContextType("12");args[7]="639";CG_JAR_Selection();assert(!contextEditing);args[7]="2687";CG_JAR_Selection();Click(550,125);
 for(int i=3;i<6;++i){Click(460,240+20*(i-3));ContextType("-3600");CG_JAR_Key(A_ENTER,qtrue);}
 Click(460,303);assert(!strcmp(request,"jar_property transform_500_200_300_-3600_-3600_-3600 100"));
 for(int i=0;i<11;++i){jarContextRect_t r=JAR_ContextRect(JC_ACTION,i);Click((int)r.x+2,(int)r.y+2);const char*actions[]={"freeze","release","activate","remove","copy","paste","rope","thruster","weld","hinge","detach"};assert(!strcmp(request,va("jar_property %s 100",actions[i])));}
 Click(410,125);assert(!strcmp(request,"jar_select self"));
 VectorSet(cg.refdef.vieworg,1,2,3);AxisClear(cg.refdef.viewaxis);cg.refdef.fov_x=90;cg.refdef.fov_y=60;cg.refdef.width=1280;cg.refdef.height=720;cgs.screenXScale=2;cgs.screenYScale=1.5f;Click(320,240);assert(!strcmp(request,"jar_select ray_1.000_2.000_3.000_1.000000_0.000000_0.000000"));
 cg.refdef.x=100;cg.refdef.y=50;cg.refdef.width=1080;cg.refdef.height=620;Click(320,240);assert(!strcmp(request,"jar_select ray_1.000_2.000_3.000_1.000000_0.000000_0.000000"));int before=requests;Click(8,35);assert(requests==before);cg.refdef.x=cg.refdef.y=0;cg.refdef.width=1280;cg.refdef.height=720;
 Click(460,328);assert(contextEditing==JAR_PROPERTY_MASS);CG_JAR_MenuUp();assert(menu&&contextEditing);CG_JAR_Key(A_ESCAPE,qtrue);assert(!contextEditing&&!contextTransformPage);
 puts("MP SP-layout context: transform drafts, separate apply groups, bounds, actions, cursor ray, stale identity and release/consent escape passed.");
}
static void ControllerMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 const char*saved[19];for(int i=0;i<19;++i)saved[i]=args[i];
 args[1]="99";args[2]="3";args[3]="1000";args[4]=args[5]=args[6]=args[7]=args[8]=args[9]="0";args[10]="-";CG_JAR_Controller();assert(contextControllerKind==3&&contextController.waitMs==1000);
 Click(490,55);Click(450,155);assert(contextEditing==201);ContextType("2500");keyHeld=0;CG_JAR_MenuUp();CG_JAR_Draw();assert(menu&&menuPinned&&contextEditing);CG_JAR_Controller();assert(!strcmp(contextInput,"2500"));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_delay_2500 99"));
 Click(450,182);ContextType("250");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_random_250 99"));
 Click(450,210);assert(!strcmp(request,"jar_property controller_flags_1 99"));
 args[1]="98";args[3]="2000";CG_JAR_Controller();assert(contextController.waitMs==1000);args[1]="99";args[3]="30";CG_JAR_Controller();assert(contextController.waitMs==30);
 args[2]="1";args[3]="0";args[10]="48656c6c6f";CG_JAR_Controller();assert(contextControllerKind==1&&!strcmp(contextController.text,"Hello"));
 Click(450,155);ContextType("Hello there!");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_text_48656c6c6f20746865726521 99"));
 Click(450,155);ContextType("a\";\\b");assert(!strcmp(contextInput,"ab"));for(int i=0;i<80;++i)ContextType("x");assert(strlen(contextInput)==63);CG_JAR_Key(A_ENTER,qtrue);assert(strlen(request)<sizeof(request)-1);
 Click(450,155);ContextType("draft");for(int i=0;i<19;++i)args[i]=saved[i];args[4]="100";CG_JAR_Selection();assert(!contextEditing&&!contextControllerKind);
 CG_JAR_Key(A_ESCAPE,qtrue);keyHeld=1;puts("MP controller menu: validated identity echoes, draft retention, latch typing, numeric/text requests, input filtering and bounds passed.");
}
