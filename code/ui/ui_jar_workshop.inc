// Native inventory/Sandbox menus. Included after AddItem. GPL-2.0-only.
static char workshopStatus[240],workshopCount[128],workshopPage[128],workshopQ[180];
static char inventoryRows[8][240];
static int inventoryPage;
static void WorkshopResume(){Menus_CloseAll();ui.Key_SetCatcher(0);Cvar_Set("cl_paused","0");}
static menuDef_t *WorkshopMenu(const char*name,const char*title){
 auto*m=Menu_CreateJAR(name);if(!m)return nullptr;m->fullScreen=qtrue;m->window.style=WINDOW_STYLE_FILLED;m->window.flags|=WINDOW_BACKCOLORSET;
 VectorSet4(m->window.backColor,0.025f,0.04f,0.065f,1);VectorSet4(m->focusColor,0.35f,0.85f,1,1);
 m->onESC="uiScript jarWorkshopClose ;";AddItem(m,"title",title,24,24,570,nullptr);
 auto*s=AddItem(m,"status",workshopStatus,24,391,590,nullptr);if(s)s->textscale=.60f;
 AddItem(m,"resume","Resume",24,446,155,"uiScript jarWorkshopClose ;");return m;
}
static void WorkshopMenus(){
 if(!Menus_FindByName("jarInventory")){if(auto*m=WorkshopMenu("jarInventory","JA:R / CARRIED WEAPONS")){
  auto*i=AddItem(m,"page",workshopPage,24,60,560,nullptr);if(i)i->textscale=.65f;
  for(int row=0;row<8;++row){auto*b=AddItem(m,va("choice%d",row),inventoryRows[row],32,99.0f+row*31,575,String_Alloc(va("uiScript jarWorkshopEquip %d ;",row)));if(b)b->textscale=.7f;}
  AddItem(m,"previous","Previous",32,363,160,"uiScript jarWorkshopPage -1 ;");
  AddItem(m,"next","Next",215,363,155,"uiScript jarWorkshopPage 1 ;");
  AddItem(m,"sandbox","Sandbox",428,446,170,"uiScript jarWorkshopOpen sandbox ;");Menu_PostParse(m);
 }}
 if(!Menus_FindByName("jarSandbox")){if(auto*m=WorkshopMenu("jarSandbox","JA:R / SANDBOX")){
  auto*hint=AddItem(m,"hint","Optional mission changes. Off cleans owned objects; carried choices remain.",24,60,590,nullptr);if(hint)hint->textscale=.59f;
  AddItem(m,"enable","Enable / claim Q",24,91,265,"uiScript jarWorkshopCommand on ;");
  AddItem(m,"disable","Off / restore Q",330,91,275,"uiScript jarWorkshopCommand off ;");
  auto*q=AddItem(m,"binding",workshopQ,24,123,590,nullptr);if(q)q->textscale=.55f;
  auto*browse=AddItem(m,"browse","Enable Sandbox, resume, then hold Q for icon cards and tools.",24,177,590,nullptr);browse->textscale=.64f;
  auto*bar=AddItem(m,"weaponbar","Physgun and Toolgun appear in the normal bottom weapon bar.",24,216,590,nullptr);bar->textscale=.62f;
  auto*aim=AddItem(m,"aim","Cards spawn at your crosshair. Move while browsing; release Q to close.",24,253,590,nullptr);aim->textscale=.59f;
  auto*risk=AddItem(m,"risk","Map edits can break missions. Removed map entities require reloading.",24,298,590,nullptr);risk->textscale=.59f;
  auto*undo=AddItem(m,"undo","Undo removes the last spawn or Toolgun creation.",24,335,590,nullptr);undo->textscale=.57f;
  Menu_PostParse(m);
 }}
 if(!Menus_FindByName("jarSandboxTools")){if(auto*m=WorkshopMenu("jarSandboxTools","JA:R / LEGACY TOOL COMMANDS")){
  auto*h=AddItem(m,"hint","Use held Q and the weapon bar. Eligible map models can also be edited.",24,67,590,nullptr);if(h)h->textscale=.65f;
  const char*labels[]={"Select aimed object","Grab / move with aim","Rotate selected 15 degrees","Freeze selected","Release with gravity","Remove selected"};
  const char*actions[]={"select","grab","rotate","freeze","release","remove"};
  AddItem(m,"physics","Carry: Physics gun",24,106,280,"uiScript jarWorkshopGrant physics_gun ;");
   AddItem(m,"toolgun","Carry: Toolgun",330,106,270,"uiScript jarWorkshopGrant toolgun ;");
   auto*p=AddItem(m,"equipHint","Select tools in the weapon bar. Primary acts; secondary drops / changes mode.",24,141,590,nullptr);if(p)p->textscale=.55f;
   for(int i=0;i<6;++i)AddItem(m,va("tool%d",i),labels[i],32,181.0f+i*27,560,String_Alloc(va("uiScript jarWorkshopTool %s ;",actions[i])));
  auto*l=AddItem(m,"limits","Weld/Hinge support native entities and props. Box3D adds prop collisions.",24,354,590,nullptr);if(l)l->textscale=.60f;
  AddItem(m,"sandbox","Sandbox",428,446,170,"uiScript jarWorkshopOpen sandbox ;");Menu_PostParse(m);
 }}
 if(!Menus_FindByName("jarSandboxActors")){if(auto*m=WorkshopMenu("jarSandboxActors","JA:R / NPCs AND VEHICLES")){
  auto*h=AddItem(m,"hint","Native AI and allegiance. Eight actors maximum; use clear, level ground.",24,67,590,nullptr);if(h)h->textscale=.6f;
  const char*labels[]={"Stormtrooper / enemy","Rebel / ally","R2 unit / neutral","Swoop / controllable speeder","Tauntaun / controllable mount"};
  const int ids[]={3001,3002,3003,4001,4002};
  for(int i=0;i<5;++i)AddItem(m,va("actor%d",i),labels[i],32,112.0f+i*37,560,String_Alloc(va("uiScript jarWorkshopSpawn %d ;",ids[i])));
  auto*l=AddItem(m,"limits","Aim at a vehicle and Use to board. Jump to leave a swoop.",24,330,590,nullptr);if(l)l->textscale=.6f;
  auto*b=AddItem(m,"unsupported","Aircraft / bosses: controller and space requirements not yet qualified.",24,355,590,nullptr);if(b)b->textscale=.55f;
  AddItem(m,"sandbox","Sandbox",428,446,170,"uiScript jarWorkshopOpen sandbox ;");Menu_PostParse(m);
 }}
 if(!Menus_FindByName("jarSandboxPickups")){if(auto*m=WorkshopMenu("jarSandboxPickups","JA:R / PICKUPS AND SUPPLIES")){
  auto*h=AddItem(m,"hint","Native pickup amounts and 30-second expiry. Inventory grants can alter missions.",24,67,590,nullptr);if(h)h->textscale=.58f;
  const char*labels[]={"Blaster pistol","E-11 blaster","Disruptor","Bowcaster","Repeater","DEMP2","Flechette","Rocket launcher","Thermal detonator","Trip mine","Det pack","Concussion rifle","Health pack","Small shield","Battery","Large shield"};
  for(int i=0;i<16;++i)AddItem(m,va("pickup%d",i),labels[i],24.0f+(i%2)*306,104.0f+(i/2)*31,275,String_Alloc(va("uiScript jarWorkshopSpawn %d ;",i<12?2002+i:2501+i-12)));
  AddItem(m,"sandbox","Sandbox",428,446,170,"uiScript jarWorkshopOpen sandbox ;");Menu_PostParse(m);
 }}

}
void UI_JAR_WorkshopUpdate(){
 SandboxOverlayUpdate();
 Q_strncpyz(workshopStatus,Cvar_VariableString("sv_running")[0]=='1'?Cvar_VariableString("jar_workshopStatus"):"Start a mission to use carried inventory and Sandbox actions.",sizeof(workshopStatus));
 Q_strncpyz(workshopCount,Cvar_VariableString("jar_sandboxCount"),sizeof(workshopCount));
 Q_strncpyz(workshopPage,Cvar_VariableString("jar_inventoryPage"),sizeof(workshopPage));
 Q_strncpyz(workshopQ,Cvar_VariableString("jar_sandboxQStatus"),sizeof(workshopQ));
 for(int i=0;i<8;++i){Q_strncpyz(inventoryRows[i],Cvar_VariableString(va("jar_inventoryLabel%d",i)),sizeof(inventoryRows[i]));
  if(auto*m=Menus_FindByName("jarInventory"))if(auto*item=Menu_FindItemByName(m,va("choice%d",i))){
   if(atoi(Cvar_VariableString(va("jar_inventoryId%d",i)))>0)item->window.flags|=WINDOW_VISIBLE;
   else item->window.flags&=~(WINDOW_VISIBLE|WINDOW_HASFOCUS);
  }
 }
}
void UI_JAR_WorkshopOpen(const char*page){
 if(Q_stricmp(page,"inventory") && !std::strcmp(Cvar_VariableString("sv_running"),"1") && !std::strcmp(Cvar_VariableString("jar_sandbox"),"1")){UI_JAR_OverlayHeld(true);return;}
 UI_JAR_OverlayClose();
 WorkshopMenus();if(atoi(Cvar_VariableString("sv_running"))){ui.Cmd_ExecuteText(EXEC_NOW,va("jar_inventory page %d\n",inventoryPage));ui.Cmd_ExecuteText(EXEC_NOW,"jar_sb status\n");inventoryPage=atoi(Cvar_VariableString("jar_inventoryPageIndex"));}
 UI_JAR_WorkshopUpdate();Menus_CloseAll();Cvar_Set("cl_paused","1");ui.Key_SetCatcher(KEYCATCH_UI);
 Menus_ActivateByName(!Q_stricmp(page,"inventory")?"jarInventory":!Q_stricmp(page,"tools")?"jarSandboxTools":!Q_stricmp(page,"actors")?"jarSandboxActors":!Q_stricmp(page,"pickups")?"jarSandboxPickups":"jarSandbox");
}
qboolean UI_JAR_WorkshopScript(const char*name,const char**args){
 if(SandboxOverlayScript(name,args))return qtrue;
 if(!Q_stricmp(name,"jarWorkshopClose")){WorkshopResume();return qtrue;}
 const char*value;
 if(!Q_stricmp(name,"jarWorkshopOpen")){if(String_Parse(args,&value))UI_JAR_WorkshopOpen(value);return qtrue;}
 if(!Q_strncmp(name,"jarWorkshop",11)){
  if(!String_Parse(args,&value))return qtrue;
  if(!Q_stricmp(name,"jarWorkshopPage")){inventoryPage=std::max(0,std::min(atoi(Cvar_VariableString("jar_inventoryLastPage")),inventoryPage+atoi(value)));UI_JAR_WorkshopOpen("inventory");return qtrue;}
  char command[160]={};
  if(!Q_stricmp(name,"jarWorkshopEquip")){int row=atoi(value);if(row<0||row>=8)return qtrue;Com_sprintf(command,sizeof(command),"jar_inventory equip %d\n",atoi(Cvar_VariableString(va("jar_inventoryId%d",row))));}
  else if(!Q_stricmp(name,"jarWorkshopSpawn"))Com_sprintf(command,sizeof(command),"jar_sb spawn %d\n",atoi(value));
  else if(!Q_stricmp(name,"jarWorkshopGrant"))Com_sprintf(command,sizeof(command),"jar_sb give %s\n",value);
  else if(!Q_stricmp(name,"jarWorkshopTool"))Com_sprintf(command,sizeof(command),"jar_sb tool %s\n",value);
  else if(!Q_stricmp(name,"jarWorkshopCommand"))Com_sprintf(command,sizeof(command),"jar_sb %s\n",value);
  else return qfalse;
  WorkshopResume();ui.Cmd_ExecuteText(EXEC_APPEND,command);return qtrue;
 }
 return qfalse;
}
