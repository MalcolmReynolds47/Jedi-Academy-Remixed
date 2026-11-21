static char doorCameraAction[32],doorCameraStatus[32],doorCameraExtension[48];
static void DoorCameraFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 if(x==402&&y==204)Q_strncpyz(doorCameraAction,s,sizeof(doorCameraAction));
 if(x==402&&y==260)Q_strncpyz(doorCameraStatus,s,sizeof(doorCameraStatus));
 if(x==402&&y==288)Q_strncpyz(doorCameraExtension,s,sizeof(doorCameraExtension));
}

static void DoorCameraMenuDraw(const char*action,const char*status,const char*extension){
 doorCameraAction[0]=doorCameraStatus[0]=doorCameraExtension[0]=0;
 CG_JAR_Draw();
 assert(!strcmp(doorCameraAction,action)&&!strcmp(doorCameraStatus,status)&&!strcmp(doorCameraExtension,extension));
}

static void DoorCameraMenuCases(void){
 const char*queries[]={"misc_model_jabba_cam","Jabba door camera"};
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 for(int i=0;i<ARRAY_LEN(queries);++i){
  Click(248,115);Click(60,115);ContextType(queries[i]);CG_JAR_Key(A_ENTER,qtrue);
  Cards();assert(total==1&&cardCount==1&&jarMPCatalog[cards[0]].id==JAR_MP_DOOR_CAMERA_ID);
  Click(30,140);assert(!strcmp(request,"jar_spawn 86873624"));
 }
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="86873624";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="20";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==JAR_MP_DOOR_CAMERA_ID);
 args[1]="99";args[2]="49";for(int i=3;i<=9;++i)args[i]="0";
 args[10]="-";args[11]="120000";args[12]="0";args[13]="48000";args[14]="0";
 CG_JAR_Controller();
 assert(contextControllerKind==JAR_DOOR_CAMERA_CONTROLLER&&contextController.damage==48000&&contextController.speed==120000);
 contextPage=1;imports.R_Font_DrawString=DoorCameraFontDraw;
 DoorCameraMenuDraw("Extend","Retracted","Extension 0.000 units");

 Click(450,154);assert(contextEditing==CONTEXT_CAMERA_TRAVEL);
 int sent=requests;ContextType("1e-");CG_JAR_Key(A_ENTER,qtrue);
 assert(requests==sent&&contextEditing==CONTEXT_CAMERA_TRAVEL);
 contextInput[0]=0;ContextType("256.001");CG_JAR_Key(A_ENTER,qtrue);
 assert(requests==sent+1&&!strcmp(request,"jar_property controller_camera_travel_256001 99"));
 Click(450,154);
 contextInput[0]=0;ContextType("48.125");CG_JAR_Key(A_ENTER,qtrue);
 assert(!contextEditing&&!strcmp(request,"jar_property controller_camera_travel_48125 99")&&contextController.damage==48000);

 Click(450,182);assert(contextEditing==CONTEXT_CAMERA_SPEED);
 sent=requests;ContextType(".");CG_JAR_Key(A_ENTER,qtrue);
 assert(requests==sent&&contextEditing==CONTEXT_CAMERA_SPEED);
 contextInput[0]=0;ContextType("512.001");CG_JAR_Key(A_ENTER,qtrue);
 assert(requests==sent+1&&!strcmp(request,"jar_property controller_camera_speed_512001 99"));
 Click(450,182);
 contextInput[0]=0;ContextType("511.875");CG_JAR_Key(A_ENTER,qtrue);
 assert(!contextEditing&&!strcmp(request,"jar_property controller_camera_speed_511875 99")&&contextController.speed==120000);

 Click(450,210);assert(!strcmp(request,"jar_property activate 99"));
 args[11]="511875";args[12]="1";args[13]="48125";args[14]="15250";
 CG_JAR_Controller();assert(contextController.damage==48125&&contextController.speed==511875&&contextController.pushSpeed==15250);
 DoorCameraMenuDraw("Retract","Extending","Extension 15.250 units");
 centity_t*root=&cg_entities[64];memset(root,0,sizeof(*root));
 root->currentValid=qtrue;root->currentState.number=64;root->currentState.eType=ET_GENERAL;
 root->currentState.generic1=250;root->currentState.bolt2=JAR_MP_DOOR_CAMERA_ROOT_TAG;
 root->currentState.modelindex=8;root->currentState.time2=700;root->currentState.time=JAR_MP_DOOR_CAMERA_BLOCKED;
 DoorCameraMenuDraw("Retract","Blocked","Extension 15.250 units");
 root->currentState.bolt2=0;DoorCameraMenuDraw("Retract","Extending","Extension 15.250 units");
 memset(root,0,sizeof(*root));

 args[14]="48126";CG_JAR_Controller();assert(contextController.pushSpeed==15250);
 args[14]="48125";CG_JAR_Controller();DoorCameraMenuDraw("Retract","Extended","Extension 48.125 units");
 args[12]="0";args[14]="15250";CG_JAR_Controller();DoorCameraMenuDraw("Extend","Retracting","Extension 15.250 units");
 Click(450,238);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[9]="1";CG_JAR_Controller();DoorCameraMenuDraw("","Paused","Extension 15.250 units");
 const jarContextRect_t activate=JAR_ContextRect(JC_ACTION,2);
 sent=requests;Click(450,210);Click((int)activate.x+6,(int)activate.y+6);assert(requests==sent);

 imports.R_Font_DrawString=FontDraw;CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP door camera menu: catalog, fractional numeric settings, native activation, enable and authoritative extension status passed.");
}
