static char welderMenuRows[6][64];
static void WelderMenuFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 if(x==402&&y>=148&&y<=288&&(y-148)%28==0){
  const int row=(y-148)/28;
  Q_strncpyz(welderMenuRows[row],s,sizeof(welderMenuRows[row]));
 }
}

static void WelderMenuCases(void){
 const char *queries[]={"misc_model_welder","Welder"};
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 for(int i=0;i<ARRAY_LEN(queries);++i){
  Click(248,115);Click(60,115);ContextType(queries[i]);CG_JAR_Key(A_ENTER,qtrue);
  Cards();assert(total==1&&cardCount==1&&jarMPCatalog[cards[0]].id==JAR_MP_WELDER_ID);
  Click(30,140);assert(!strcmp(request,"jar_spawn 89954407"));
 }
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="89954407";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==JAR_MP_WELDER_ID);
 args[1]="99";args[2]="50";args[3]="200";for(int i=4;i<=9;++i)args[i]="0";
 args[10]="-";args[11]="15000";args[12]="1";args[13]="10";args[14]="45000";
 CG_JAR_Controller();assert(contextControllerKind==JAR_WELDER_CONTROLLER&&contextController.active&&!contextController.inactive);
 const jarControllerState_t defaults=contextController;
 assert(defaults.waitMs==JAR_MP_WELDER_DEFAULT_INTERVAL&&defaults.speed==JAR_MP_WELDER_DEFAULT_REACH&&
        defaults.pushSpeed==JAR_MP_WELDER_DEFAULT_RADIUS&&defaults.damage==JAR_MP_WELDER_DEFAULT_DAMAGE);
 contextPage=1;imports.R_Font_DrawString=WelderMenuFontDraw;CG_JAR_Draw();
 const char *defaultRows[]={"Reach: 15.000","Damage radius: 45.000","Interval: 0.200",
                            "Damage: 10","On - turn off","Enabled"};
 for(int i=0;i<ARRAY_LEN(defaultRows);++i)assert(!strcmp(welderMenuRows[i],defaultRows[i]));

 const int fields[]={CONTEXT_WELDER_REACH,CONTEXT_WELDER_RADIUS,CONTEXT_WELDER_INTERVAL,206};
 const char *low[]={".","-","1e",""},*high[]={"1e999","1e999","2147484","2147483648"};
 const char *valid[]={"31.625","255.875","0.125","0"};
 const char *expected[]={"jar_property controller_welder_reach_31625 99","jar_property controller_welder_radius_255875 99",
                         "jar_property controller_welder_interval_125 99","jar_property controller_damage_0 99"};
 for(int i=0;i<ARRAY_LEN(fields);++i){
  Click(450,154+28*i);assert(contextEditing==fields[i]);
  const int sent=requests;
  ContextType(low[i]);CG_JAR_Key(A_ENTER,qtrue);assert(requests==sent&&contextEditing==fields[i]);
  contextInput[0]=0;ContextType(high[i]);CG_JAR_Key(A_ENTER,qtrue);assert(requests==sent&&contextEditing==fields[i]);
  contextInput[0]=0;ContextType(valid[i]);CG_JAR_Key(A_ENTER,qtrue);assert(!contextEditing&&!strcmp(request,expected[i]));
 }
 assert(!memcmp(&contextController,&defaults,sizeof(defaults)));

 args[3]="125";args[11]="31625";args[13]="0";args[14]="255875";CG_JAR_Controller();
 const jarControllerState_t edited=contextController;
 assert(edited.waitMs==125&&edited.speed==31625&&edited.pushSpeed==255875&&!edited.damage);
 const int positions[]={3,11,12,13,14};
 const char *invalid[]={"0","0","2","32768","131072001"};
 for(int i=0;i<ARRAY_LEN(positions);++i){
  const int pos=positions[i];const char *value=args[pos];args[pos]=invalid[i];
  CG_JAR_Controller();assert(!memcmp(&contextController,&edited,sizeof(edited)));args[pos]=value;
 }

 Click(450,266);assert(!strcmp(request,"jar_property activate 99"));
 args[12]="0";CG_JAR_Controller();CG_JAR_Draw();assert(!strcmp(welderMenuRows[4],"Off - turn on"));
 Click(450,266);assert(!strcmp(request,"jar_property activate 99"));
 args[12]="1";CG_JAR_Controller();
 Click(450,294);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[9]="1";CG_JAR_Controller();CG_JAR_Draw();
 assert(!strcmp(welderMenuRows[4],"Paused")&&!strcmp(welderMenuRows[5],"Disabled"));
 const jarContextRect_t activate=JAR_ContextRect(JC_ACTION,2);
 const int sent=requests;Click(450,266);Click((int)activate.x+6,(int)activate.y+6);assert(requests==sent);
 Click(450,294);assert(!strcmp(request,"jar_property controller_enabled 99"));
 imports.R_Font_DrawString=FontDraw;CG_JAR_Key(A_ESCAPE,qtrue);
 for(int i=13;i<=18;++i)args[i]="0";
 puts("MP welder menu: exact spawn card, bounded decimal reach/radius and interval, damage, and authoritative on/disabled state passed.");
}
