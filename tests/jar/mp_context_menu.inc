static void ContextType(const char*text){for(;*text;++text)CG_JAR_Key(*text|K_CHAR_FLAG,qtrue);}
static jarPortablePortalView_t portalView;
static int portalDraws;
static void PortalViewCapture(const jarPortablePortalView_t* view){portalView=*view;++portalDraws;}
static void PortalMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="88142876";
 args[7]="4096";args[8]=args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]=va("%d",JAR_PORTAL_SURFACE_CONTROLLER);for(int i=3;i<=9;++i)args[i]="0";
 args[10]="-";args[11]="128";args[12]="1";args[13]="0";args[14]="192";
 CG_JAR_Controller();assert(contextControllerKind==JAR_PORTAL_SURFACE_CONTROLLER&&contextController.pushSpeed==192);
 Click(480,55);Click(460,178);ContextType("256");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_portal_height_256 99"));
 Click(460,209);assert(!strcmp(request,"jar_property controller_portal_mode 99"));
 Click(460,237);assert(!strcmp(request,"jar_property controller_portal_power 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);args[14]="0";
 imports.R_AddPortalToScene=PortalViewCapture;
 centity_t cent={0};
 cent.currentState.eType=ET_GENERAL;cent.currentState.generic1=250;cent.currentState.bolt2=JAR_MP_PORTAL_SURFACE_TAG;
 cent.currentState.speed=128;cent.currentState.eventParm=192;cent.currentState.time=JAR_PORTAL_VIEW_CAMERA;
 VectorSet(cent.lerpOrigin,100,200,300);VectorSet(cent.currentState.origin,500,600,700);
 assert(CG_JAR_Entity(&cent)&&portalDraws==1&&!portalView.mirror&&portalView.height==192);
 assert(portalView.origin[0]==100&&portalView.cameraOrigin[0]==500);
 cent.currentState.time=JAR_PORTAL_VIEW_MIRROR;
 assert(CG_JAR_Entity(&cent)&&portalDraws==2&&portalView.mirror&&portalView.cameraOrigin[0]==100);
 imports.R_AddPortalToScene=NULL;
}
static void SearchlightMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="95681750";
 args[7]="4096";args[8]=args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]=va("%d",JAR_SEARCHLIGHT_CONTROLLER);for(int i=3;i<=9;++i)args[i]="0";
 args[5]="33554431";args[10]="-";args[11]="8192";args[12]="1";args[13]="8";args[14]="225";
 CG_JAR_Controller();assert(contextControllerKind==JAR_SEARCHLIGHT_CONTROLLER&&contextController.speed==8192);
 Click(480,55);Click(460,234);ContextType("80c0ff");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_searchlight_color_80c0ff 99"));
 Click(460,182);ContextType("30");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_searchlight_angle_30 99"));
 Click(460,266);assert(!strcmp(request,"jar_property controller_searchlight_track 99"));
 Click(460,292);assert(!strcmp(request,"jar_property controller_searchlight_power 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);args[13]=args[14]="0";
}
static void DirectionalMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="89652619";
 args[7]="4096";args[8]=args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]=va("%d",JAR_DIRECTIONAL_CONTROLLER);for(int i=3;i<=9;++i)args[i]="0";
 args[3]="3600000";args[7]=args[8]="100000";args[10]="-";args[11]="64";args[12]="1";
 CG_JAR_Controller();assert(contextControllerKind==JAR_DIRECTIONAL_CONTROLLER&&contextController.waitMs==3600000);
 Click(480,55);Click(460,182);ContextType("-1");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_directional_wait_-1 99"));
 Click(460,210);ContextType("100001");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_directional_uses_100001 99"));
 Click(460,210);ContextType("10");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_directional_uses_10 99"));
 Click(460,238);assert(!strcmp(request,"jar_property controller_directional_players 99"));
 Click(460,266);assert(!strcmp(request,"jar_property controller_playing 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);
}
static void MovementMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="2";args[6]="0";
 args[7]="128";args[8]=args[9]="0";args[10]="80";args[11]="100";args[12]="0";args[19]="125";args[20]="-1";
 CG_JAR_Selection();assert(contextActorGravity==125&&contextActorFriction==-1);
 Click(480,55);Click(460,154);ContextType("3200");CG_JAR_Selection();assert(contextEditing==CONTEXT_ACTOR_GRAVITY);
 CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property actor_gravity_3200 99"));
 Click(460,180);ContextType("0");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property actor_friction_0 99"));
 Click(460,210);assert(!strcmp(request,"jar_property actor_map_gravity 99"));
 Click(460,239);assert(!strcmp(request,"jar_property actor_map_friction 99"));
 args[4]="100";args[5]="1";args[6]="92295820";args[7]="4096";CG_JAR_Selection();
 args[1]="100";args[2]=va("%d",JAR_FRICTION_CONTROLLER);for(int i=3;i<=9;++i)args[i]="0";
 args[10]="-";args[11]="2";CG_JAR_Controller();assert(contextControllerKind==JAR_FRICTION_CONTROLLER&&contextController.speed==2);
 Click(480,55);Click(460,154);ContextType("10");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property controller_movement_value_10 100"));
 Click(460,180);assert(!strcmp(request,"jar_property controller_enabled 100"));
 Click(460,210);assert(!strcmp(request,"jar_property activate 100"));
 CG_JAR_Key(A_ESCAPE,qtrue);
 args[19]=args[20]="";
}
static void ContextMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="1";args[3]="3";args[4]="99";args[5]="1";args[6]="0";args[7]="639";args[8]="0";args[9]="0";args[10]="80.25";args[11]="0";args[12]="0";
 CG_JAR_Selection();assert(ContextField()==JAR_PROPERTY_MASS&&contextMass==80.25f);
 Click(480,328);assert(contextEditing==JAR_PROPERTY_MASS);keyHeld=0;CG_JAR_Key(81,qfalse);CG_JAR_MenuUp();CG_JAR_Draw();assert(menu&&contextEditing&&menuPinned);keyHeld=1;ContextType("1e-");int count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count&&contextEditing);
 Click(480,328);ContextType("2.5.0x;0");CG_JAR_Key(A_ENTER,qtrue);
 assert(!strcmp(request,"jar_property mass_2.5 99"));
 Click(480,328);ContextType("2.5e4");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property mass_25000 99"));
 Click(480,328);ContextType("432.5");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property mass_432.5 99")&&!contextEditing&&contextMass==80.25f);
 args[10]="432.5";args[7]="1663";CG_JAR_Selection();assert(contextMass==432.5f);Click(480,55);Click(480,155);Click(410,55);assert(!strcmp(request,"jar_property gravity 99"));
 Click(440,452);assert(!strcmp(request,"jar_property property_undo 99"));
 // The identity token, rather than merely the reused slot number, cancels drafts.
 Click(480,328);ContextType("200");args[4]="100";CG_JAR_Selection();assert(!contextEditing);count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count);
 // Context refreshes on the same entity do not steal a numeric/pose draft.
 Click(480,328);ContextType("200");CG_JAR_Selection();assert(contextEditing&&contextInput[0]=='2');Click(560,55);Click(450,155);assert(poseEditing&&!contextEditing);Click(410,55);Click(480,328);assert(contextEditing&&!poseEditing);
 args[5]="2";args[7]="195";args[2]="0";args[3]="0";args[11]="333";CG_JAR_Selection();assert(!contextEditing&&ContextField()==JAR_PROPERTY_HEALTH);
 Click(480,328);ContextType("2147483648");count=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==count&&contextEditing);
 Click(480,328);ContextType("173");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property health_173 100"));CG_JAR_Draw();
 args[5]="5";args[7]="352";args[11]="0";args[12]="17";CG_JAR_Selection();assert(ContextField()==JAR_PROPERTY_QUANTITY);
 Click(480,328);ContextType("0");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property quantity_0 100"));
 Click(580,380);assert(!strcmp(request,"jar_property remove 100")); // First applicable pickup action, no actor/prop controls.
 args[7]="96";CG_JAR_Selection();count=requests;Click(480,328);assert(!contextEditing&&requests==count); // native holdable/armor: no unsupported amount editor
 args[5]="4";args[7]="195";args[11]="100";args[12]="0";CG_JAR_Selection();assert(!(contextCaps&JAR_CONTEXT_REMOVE));
 args[10]="nan";CG_JAR_Selection();assert(contextKind==JAR_CONTEXT_PLAYER&&contextMass==432.5f);args[10]="80";
 args[4]="0";CG_JAR_Selection();assert(contextIdentity==100);args[4]="100";args[7]="8192";CG_JAR_Selection();assert(contextCaps==195);
 args[1]="0";args[7]="194";args[21]="27";CG_JAR_Selection();assert((contextCaps&JAR_CONTEXT_ARMOR)&&contextArmor==27);
 Click(420,328);ContextType("80");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property health_80 100"));
 Click(565,328);assert(contextEditing==JAR_PROPERTY_ARMOR);ContextType("33");CG_JAR_ContextDown();CG_JAR_Selection();
 assert(contextEditing==JAR_PROPERTY_ARMOR&&!strcmp(contextInput,"33"));Click(460,350);assert(!strcmp(request,"jar_property armor_33 100"));
 count=requests;Click(410,380);assert(requests==count); // Self Freeze is unavailable.
 args[1]="1";args[4]="101";args[7]="195";CG_JAR_Selection();
 Click(565,328);ContextType("41");CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property armor_41 101"));
 Click(565,328);ContextType("99");args[21]="";CG_JAR_Selection();assert(!contextEditing&&!(contextCaps&JAR_CONTEXT_ARMOR));
 args[1]="-1";args[2]=args[3]=args[4]=args[5]=args[6]=args[7]=args[8]=args[9]=args[10]=args[11]=args[12]="0";
 CG_JAR_Selection();assert(selectedEntity==-1&&!contextIdentity&&!ContextField());count=requests;Click(480,328);assert(requests==count);
 CG_JAR_Key(A_ESCAPE,qtrue);assert(!strcmp(request,"jar_select close"));
 puts("MP context UI: type-specific actions, valid numeric input, server-confirmed values, token commands, stale-draft cancellation and close passed.");
}
