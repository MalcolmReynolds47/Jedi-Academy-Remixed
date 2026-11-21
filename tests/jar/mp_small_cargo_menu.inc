enum {
 CARGO_HEADING=1,CARGO_HEAVY_ONLY=2,CARGO_MEDPACK=4,CARGO_PRIMARY=8,
 CARGO_ACTIVATE=16,CARGO_PENDING=32
};
static int smallCargoDraw;
static void SmallCargoFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 if(!strcmp(s,"Small cargo crate"))smallCargoDraw|=CARGO_HEADING;
 if(!strcmp(s,"Heavy weapons only"))smallCargoDraw|=CARGO_HEAVY_ONLY;
 if(!strcmp(s,"Drops one instant medpack."))smallCargoDraw|=CARGO_MEDPACK;
 if(!strcmp(s,"Link Primary to run on destruction."))smallCargoDraw|=CARGO_PRIMARY;
 if(!strcmp(s,"Activate")||!strcmp(s,"Detonate"))smallCargoDraw|=CARGO_ACTIVATE;
 if(!strcmp(s,"Destruction pending"))smallCargoDraw|=CARGO_PENDING;
}

static void SmallCargoMenuCases(void){
 const char *queries[]={"MISC_MODEL_CARGO_SMALL","Small cargo crate"};
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 for(int i=0;i<ARRAY_LEN(queries);++i){
  Click(248,115);Click(60,115);ContextType(queries[i]);CG_JAR_Key(A_ENTER,qtrue);
  Cards();assert(total==1&&cardCount==1&&jarMPCatalog[cards[0]].id==JAR_MP_SMALL_CARGO_ID);
  Click(30,140);assert(!strcmp(request,"jar_spawn 87961440"));
 }
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="87961440";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==JAR_MP_SMALL_CARGO_ID&&!contextControllerKind);
 const jarContextRect_t activate=JAR_ContextRect(JC_ACTION,2);
 const int activateX=(int)activate.x+6,activateY=(int)activate.y+6;
 int sent=requests;Click(activateX,activateY);assert(requests==sent);

 args[1]="99";args[2]="46";args[3]=args[4]=args[5]=args[6]=args[9]=args[14]="0";
 args[7]=args[8]="40";args[10]="-";args[11]="96";args[12]=args[13]="1";
 CG_JAR_Controller();
 assert(contextControllerKind==JAR_SMALL_CARGO_CONTROLLER&&contextController.active==1&&contextController.count==40);
 assert(contextController.initial==40&&contextController.damage==1&&contextController.speed==96&&!contextController.inactive);
 imports.R_Font_DrawString=SmallCargoFontDraw;smallCargoDraw=0;
 contextPage=1;CG_JAR_Draw();
 assert(smallCargoDraw==(CARGO_HEADING|CARGO_HEAVY_ONLY|CARGO_MEDPACK|CARGO_PRIMARY));

 const int y[]={154,182,210},fields[]={203,206,212};
 const char *settings[]={"jar_property controller_count_125 99","jar_property controller_damage_125 99","jar_property controller_radius_125 99"};
 for(int i=0;i<ARRAY_LEN(fields);++i){
  Click(450,y[i]);assert(contextEditing==fields[i]);ContextType("125");CG_JAR_Key(A_ENTER,qtrue);
  assert(!contextEditing&&!strcmp(request,settings[i]));
 }
 sent=requests;Click(450,238);Click(activateX,activateY);assert(requests==sent);
 Click(450,266);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[9]="1";CG_JAR_Controller();assert(contextController.inactive);
 args[5]="1";args[9]="0";CG_JAR_Controller();assert(contextController.inactive&&!contextController.flags);
 args[5]="0";args[7]="0";args[12]="2";CG_JAR_Controller();assert(contextController.active==2);
 smallCargoDraw=0;CG_JAR_Draw();assert((smallCargoDraw&CARGO_PENDING)&&!(smallCargoDraw&CARGO_ACTIVATE));
 sent=requests;Click(450,154);Click(450,238);Click(activateX,activateY);assert(requests==sent&&!contextEditing);
 imports.R_Font_DrawString=FontDraw;CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP small cargo menu: catalog label and class search, terminal fields, Primary help and no Activate passed.");
}
