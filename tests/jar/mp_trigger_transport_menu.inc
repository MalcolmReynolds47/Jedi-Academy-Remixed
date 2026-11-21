static qboolean teleporterFilterDrawn,teleporterLinkDrawn;
static void TeleporterCardFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 if(x==402&&y==176&&!strcmp(s,"Accept: actors and spectators"))teleporterFilterDrawn=qtrue;
 if(x==402&&y==204&&!strcmp(s,"Link Primary to a destination marker."))teleporterLinkDrawn=qtrue;
}

static void MiscTeleporterMenuCase(void){
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 Click(248,115);Click(60,115);ContextType("misc_teleporter");CG_JAR_Key(A_ENTER,qtrue);Cards();
 int slot=-1;
 for(int i=0;i<cardCount;++i)if(!strcmp(jarMPCatalog[cards[i]].name,"misc_teleporter")){
  assert(slot==-1&&jarMPCatalog[cards[i]].id==86167446u);slot=i;
 }
 assert(slot>=0);Click(30+(slot%3)*80,140+(slot/3)*57);assert(!strcmp(request,"jar_spawn 86167446"));
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="86167446";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==86167446u);
 args[1]="99";args[2]="23";for(int i=3;i<=9;++i)args[i]="0";
 args[10]="-";args[11]="64";args[12]="1";args[13]=args[14]="0";
 CG_JAR_Controller();assert(contextControllerKind==23&&contextController.active&&contextController.speed==64);
 contextPage=1;imports.R_Font_DrawString=TeleporterCardFontDraw;CG_JAR_Draw();
 assert(teleporterFilterDrawn&&teleporterLinkDrawn);
 imports.R_Font_DrawString=FontDraw;CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP misc_teleporter menu: exact class card, stable spawn command and teleport-volume Context passed.");
}

static void TriggerTransportMenuCases(void){
 args[1]="81";CG_JAR_ContextDown();args[1]="64";args[2]="0";args[3]="1";args[4]="99";args[5]="1";args[6]="0";args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";CG_JAR_Selection();
 args[1]="99";args[2]="22";args[3]="500";args[4]=args[6]=args[7]=args[8]=args[9]=args[13]="0";args[5]="4";args[10]="-";args[11]="64";args[12]="1";args[14]="1000";CG_JAR_Controller();assert(contextControllerKind==22&&contextController.pushSpeed==1000&&contextController.waitMs==500);contextPage=1;CG_JAR_Draw();
 Click(450,182);assert(contextEditing==209);Q_strncpyz(contextInput,"2147483648",sizeof(contextInput));int sent=requests;CG_JAR_Key(A_ENTER,qtrue);assert(requests==sent&&contextEditing==209);Q_strncpyz(contextInput,"1500",sizeof(contextInput));CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_pushspeed_1500 99")&&!contextEditing);
 Click(450,210);assert(!strcmp(request,"jar_property controller_pushmode 99"));Click(450,238);assert(contextEditing==201);CG_JAR_Key('-'|K_CHAR_FLAG,qtrue);CG_JAR_Key('1'|K_CHAR_FLAG,qtrue);CG_JAR_Key(A_ENTER,qtrue);assert(!strcmp(request,"jar_property controller_delay_-1 99"));
 Click(450,266);assert(!strcmp(request,"jar_property controller_multiple 99"));Click(450,294);assert(!strcmp(request,"jar_property controller_playing 99"));Click(450,322);assert(!strcmp(request,"jar_property controller_enabled 99"));
 for(int once=0;once<2;++once){args[3]=once?"-1":"0";CG_JAR_Controller();CG_JAR_Draw();sent=requests;Click(450,266);assert(requests==sent);}
 args[3]="500";CG_JAR_Controller();
 args[14]="4001";CG_JAR_Controller();assert(contextController.pushSpeed==4001);args[14]="1000";args[5]="16";CG_JAR_Controller();assert(contextController.flags==4);args[5]="0";CG_JAR_Controller();assert(contextController.flags==4);args[3]="0";CG_JAR_Controller();assert(contextController.flags==0);CG_JAR_Draw();sent=requests;Click(450,238);assert(requests==sent&&!contextEditing);
 args[2]="23";args[3]=args[5]=args[14]="0";CG_JAR_Controller();assert(contextControllerKind==23&&!contextController.pushSpeed);CG_JAR_Draw();Click(450,182);assert(!strcmp(request,"jar_property controller_spectators 99"));args[5]="1";CG_JAR_Controller();CG_JAR_Draw();assert(contextController.flags==1);args[5]="2";CG_JAR_Controller();assert(contextController.flags==1);Click(450,294);assert(!strcmp(request,"jar_property controller_playing 99"));CG_JAR_Key(A_ESCAPE,qtrue);
 puts("MP transport trigger UI: speed/wait input, native direction modes, same-frame and spectator filters, armed/enabled and echoed bounds passed.");
 MiscTeleporterMenuCase();
}
