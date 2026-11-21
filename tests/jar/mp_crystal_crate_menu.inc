static char crystalMenuRows[9][64];
static void CrystalMenuFontDraw(int x,int y,const char*s,const float*c,int font,int limit,float scale){
 FontDraw(x,y,s,c,font,limit,scale);
 int row=-1;
 if(x==402&&y>=148&&y<=304&&(y-148)%26==0)row=(y-148)/26;
 else if(x==514&&y==304)row=7;
 else if(x==402&&y==329)row=8;
 if(row>=0)Q_strncpyz(crystalMenuRows[row],s,sizeof(crystalMenuRows[row]));
}

static void CrystalCrateMenuCases(void){
 const char*queries[]={"misc_crystal_crate","Crystal crate"};
 args[1]="81";CG_JAR_MenuDown();Click(172,85);assert(tab==3);
 for(int i=0;i<ARRAY_LEN(queries);++i){
  Click(248,115);Click(60,115);ContextType(queries[i]);CG_JAR_Key(A_ENTER,qtrue);
  Cards();assert(total==1&&cardCount==1&&jarMPCatalog[cards[0]].id==JAR_MP_CRYSTAL_CRATE_ID);
  Click(30,140);assert(!strcmp(request,"jar_spawn 93844316"));
 }
 Click(248,115);CG_JAR_MenuUp();

 args[1]="81";CG_JAR_ContextDown();
 args[1]="64";args[2]=args[3]="0";args[4]="99";args[5]="1";args[6]="93844316";
 args[7]="6783";args[8]="1";args[9]="0";args[10]="40";args[11]=args[12]="0";
 for(int i=13;i<=18;++i)args[i]="0";
 CG_JAR_Selection();assert(contextCatalog==JAR_MP_CRYSTAL_CRATE_ID);
 args[1]="99";args[2]="51";args[3]=args[4]=args[5]="0";
 args[6]="16";args[7]=args[8]="80";args[9]="0";args[10]="-";
 args[11]="80";args[12]="1";args[13]="40";args[14]="32000";
 CG_JAR_Controller();assert(contextControllerKind==JAR_CRYSTAL_CRATE_CONTROLLER);
 const jarControllerState_t defaults=contextController;
 assert(defaults.active==1&&!defaults.inactive&&!defaults.flags&&defaults.count==80&&defaults.initial==80&&
        defaults.repeats==16&&defaults.pushSpeed==32000&&defaults.damage==40&&defaults.speed==80);
 contextPage=1;imports.R_Font_DrawString=CrystalMenuFontDraw;CG_JAR_Draw();
 const char*rows[]={"Maximum health: 80","Hit damage: 16","Hit radius: 32.000",
                    "Blast damage: 40","Blast radius: 80","Collision: Solid","Enabled","Detonate","Health 80 / 80"};
 for(int i=0;i<ARRAY_LEN(rows);++i)assert(!strcmp(crystalMenuRows[i],rows[i]));

 const int fields[]={CONTEXT_CRYSTAL_HEALTH,CONTEXT_CRYSTAL_HIT_DAMAGE,CONTEXT_CRYSTAL_HIT_RADIUS,
                     CONTEXT_CRYSTAL_BLAST_DAMAGE,CONTEXT_CRYSTAL_BLAST_RADIUS};
 const char*invalid[]={"+","2147483648","1e999","-","2147483648"};
 const char*values[]={"90","0","1023.875","0","1024"};
 const char*commands[]={"controller_count_90","controller_crystal_hit_damage_0","controller_crystal_hit_radius_1023875",
                       "controller_damage_0","controller_crystal_blast_radius_1024"};
 for(int i=0;i<ARRAY_LEN(fields);++i){
  Click(450,154+26*i);assert(contextEditing==fields[i]);
  const int sent=requests;
  ContextType(invalid[i]);CG_JAR_Key(A_ENTER,qtrue);assert(requests==sent&&contextEditing==fields[i]);
  contextInput[0]=0;ContextType(values[i]);CG_JAR_Key(A_ENTER,qtrue);
  assert(!contextEditing&&!strcmp(request,va("jar_property %s 99",commands[i])));
 }
 assert(!memcmp(&contextController,&defaults,sizeof(defaults)));

 Click(450,284);assert(!strcmp(request,"jar_property controller_crystal_solid 99"));
 args[5]="1";args[6]="0";args[7]="50";args[8]="90";
 args[11]="1024";args[13]="0";args[14]="1023875";
 CG_JAR_Controller();CG_JAR_Draw();
 const jarControllerState_t edited=contextController;
 assert(edited.flags==JAR_MP_CRYSTAL_CRATE_SHOT_ONLY&&!edited.repeats&&!edited.damage&&edited.count==50&&edited.initial==90&&
        edited.pushSpeed==1023875&&edited.speed==1024);
 assert(!strcmp(crystalMenuRows[5],"Collision: Shot only")&&!strcmp(crystalMenuRows[8],"Health 50 / 90"));
 const int positions[]={5,6,7,8,11,12,13,14};
 const char*badEcho[]={"2","32768","91","0","131073","2","32768","131072001"};
 for(int i=0;i<ARRAY_LEN(positions);++i){
  const int pos=positions[i];const char*original=args[pos];args[pos]=badEcho[i];
  CG_JAR_Controller();assert(!memcmp(&contextController,&edited,sizeof(edited)));args[pos]=original;
 }

 Click(550,310);assert(!strcmp(request,"jar_property activate 99"));
 Click(450,310);assert(!strcmp(request,"jar_property controller_enabled 99"));
 args[9]="1";CG_JAR_Controller();memset(crystalMenuRows,0,sizeof(crystalMenuRows));CG_JAR_Draw();
 assert(!strcmp(crystalMenuRows[6],"Disabled")&&!crystalMenuRows[7][0]);
 const jarContextRect_t activate=JAR_ContextRect(JC_ACTION,2);
 const int sent=requests;Click(550,310);Click((int)activate.x+6,(int)activate.y+6);assert(requests==sent);

 imports.R_Font_DrawString=FontDraw;CG_JAR_Key(A_ESCAPE,qtrue);
 for(int i=13;i<=18;++i)args[i]="0";
 puts("MP crystal crate menu: stock card, health, typed hit/blast properties, solidity and guarded detonation passed.");
}
