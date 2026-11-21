typedef struct {
 const char *classname,*title;
 unsigned id;
 int kind,slots;
 const char *items[5];
} rackMenuFixture_t;
static const rackMenuFixture_t rackMenuFixtures[]={
 {"misc_model_gun_rack","Gun rack",JAR_MP_GUN_RACK_ID,JAR_GUN_RACK_CONTROLLER,3,
  {"E-11 blaster","Heavy repeater","Rocket launcher"}},
 {"misc_model_ammo_rack","Ammo rack",JAR_MP_AMMO_RACK_ID,JAR_AMMO_RACK_CONTROLLER,5,
  {"Blaster ammo","Metallic bolts","Power cell","E-11 blaster","Instant medpack"}}
};
static const rackMenuFixture_t *rackMenuFixture;
static int rackStatusLines,rackCollectedMask,rackHeading,rackControllerButtons;
static void RackMenuFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 for(int i=0;i<rackMenuFixture->slots;++i){
  const char *name=rackMenuFixture->items[i];
  const size_t length=strlen(name);
  if(strncmp(s,name,length))continue;
  if(!strcmp(s+length," - Collected"))rackCollectedMask|=1<<i;
  else assert(!strcmp(s+length," - Stocked"));
  rackStatusLines|=1<<i;
 }
 if(!strcmp(s,rackMenuFixture->title))++rackHeading;
 if(!strcmp(s,"Activate")||!strcmp(s,"Enabled")||!strcmp(s,"Disabled"))++rackControllerButtons;
}

static void RackMenuCase(const rackMenuFixture_t *fixture){
 char id[16],kind[16],spawn[32],flag[16];
 const int allSlots=(1<<fixture->slots)-1;
 snprintf(id,sizeof(id),"%u",fixture->id);
 snprintf(kind,sizeof(kind),"%d",fixture->kind);
 snprintf(spawn,sizeof(spawn),"jar_spawn %u",fixture->id);
 const char *queries[]={fixture->classname,fixture->title};
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 for(int i=0;i<ARRAY_LEN(queries);++i){
  Click(248,115);Click(60,115);ContextType(queries[i]);CG_JAR_Key(A_ENTER,qtrue);
  Cards();assert(total==1&&cardCount==1&&jarMPCatalog[cards[0]].id==fixture->id);
  Click(30,140);assert(!strcmp(request,spawn));
 }
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]=id;
 args[7]="6783";args[8]="1";args[9]="0";args[10]="80";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==fixture->id&&!contextControllerKind);
 const jarContextRect_t activate=JAR_ContextRect(JC_ACTION,2);
 int sent=requests;Click((int)activate.x+6,(int)activate.y+6);assert(requests==sent);

 args[1]="99";args[2]=kind;for(int i=3;i<=14;++i)args[i]="0";args[10]="-";
 rackMenuFixture=fixture;contextPage=1;imports.R_Font_DrawString=RackMenuFontDraw;
 for(int mask=0;mask<=allSlots;++mask){
  snprintf(flag,sizeof(flag),"%d",mask);args[5]=flag;
  CG_JAR_Controller();assert(contextControllerKind==fixture->kind&&contextController.flags==mask);
  rackStatusLines=rackCollectedMask=rackHeading=rackControllerButtons=0;
  CG_JAR_Draw();assert(rackStatusLines==allSlots&&rackCollectedMask==mask&&rackHeading==1&&!rackControllerButtons);
 }
 imports.R_Font_DrawString=FontDraw;
 snprintf(flag,sizeof(flag),"%d",allSlots+1);CG_JAR_Controller();assert(contextController.flags==allSlots);
 args[5]="0";args[9]="1";CG_JAR_Controller();assert(contextController.flags==allSlots);
 args[9]="0";
 for(int i=12;i<=14;++i){args[i]="1";CG_JAR_Controller();assert(contextController.flags==allSlots);args[i]="0";}
 sent=requests;
 for(int i=0;i<fixture->slots;++i)Click(450,154+28*i);
 Click(450,322);Click((int)activate.x+6,(int)activate.y+6);
 assert(requests==sent&&!contextEditing);
 Click(450,352);assert(!strcmp(request,"jar_property gravity 99"));
 const jarContextRect_t freeze=JAR_ContextRect(JC_ACTION,0);
 Click((int)freeze.x+6,(int)freeze.y+6);assert(!strcmp(request,"jar_property freeze 99"));
 CG_JAR_Key(A_ESCAPE,qtrue);
 args[2]="0";
}

static void RackMenuCases(void){
 for(int i=0;i<ARRAY_LEN(rackMenuFixtures);++i)RackMenuCase(&rackMenuFixtures[i]);
 puts("MP rack menus: native cards, gun and ammo shelf states, read-only Context, and ordinary prop actions passed.");
}
