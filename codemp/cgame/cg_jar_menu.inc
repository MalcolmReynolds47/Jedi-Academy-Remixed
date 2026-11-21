// SP layout and tool descriptions, with multiplayer request/acknowledgement adapters.
static void SearchEditing(qboolean editing){searchEditing=editing;if(editing)menuPinned=qtrue;trap->Cvar_Set("jar_mpMenuTyping",editing?"1":"0");}
static void MenuStatus(const char*message){Q_strncpyz(menuStatus,message,sizeof(menuStatus));menuStatusUntil=cg.time+6000;}
void CG_JAR_Status(void){MenuStatus(CG_Argv(1));}
void CG_JAR_Stats(void){
 int owned,props,physics,actors=0;
 if(!SelectionNumber(CG_Argv(1),0,MAX_GENTITIES,&owned)||!SelectionNumber(CG_Argv(2),0,MAX_GENTITIES,&props)||!SelectionNumber(CG_Argv(3),0,1,&physics))return;
 const char*actorCount=CG_Argv(4);
 if(*actorCount&&!SelectionNumber(actorCount,0,owned,&actors))return;
 menuOwned=owned;menuProps=props;menuPhysics=physics;menuActors=actors;
}
void CG_JAR_Tool(void){int mode,equip;if(!SelectionNumber(CG_Argv(1),0,12,&mode)||!SelectionNumber(CG_Argv(2),0,1,&equip))return;if(tool!=mode)toolEditing=0;tool=mode;if(equip){CG_RegisterWeapon(WP_TOOLGUN);cg.weaponSelect=WP_TOOLGUN;cg.weaponSelectTime=cg.time;}}
static void SelectTool(int mode){trap->SendClientCommand(va("jar_tool %d",mode));}
static void MenuWeapon(int weapon){for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(jarMPCatalog[i].tab==2&&jarMPCatalog[i].kind==4&&!strcmp(jarMPCatalog[i].name,weapon==WP_PHYSGUN?"weapon_physgun":"weapon_toolgun")){trap->SendClientCommand(va("jar_equip %u",jarMPCatalog[i].id));return;}}
static void Widget(jarMenuPart_t part,int index,const char*text,qboolean focused){
 jarMenuRect_t r=JAR_MenuRect(part,index);vec4_t panel={JAR_MENU_PANEL_COLOR},card={JAR_MENU_CARD_COLOR},status={JAR_MENU_STATUS_COLOR},normal={JAR_MENU_TEXT_COLOR},focus={JAR_MENU_FOCUS_COLOR};
 if(r.fill)CG_FillRect(r.x,r.y,r.w,r.h,r.fill==1?panel:r.fill==2?card:status);
 if(text)trap->R_Font_DrawString((int)(r.x+r.tx),(int)(r.y+r.ty),text,focused?focus:normal,jarMenuFont|STYLE_DROPSHADOW,-1,r.scale);
}
static qboolean Hit(jarMenuPart_t part,int index){return JAR_MenuHit(part,index,cursorX,cursorY)?qtrue:qfalse;}
static qboolean CardMatches(const jarMPEntry_t*entry){
 // The catalog keeps map spawner IDs for older worlds and console commands.
 if(entry->tab!=tab||!JAR_MPCatalogVisible(entry))return qfalse;
 return JAR_MenuMatches(JAR_MPCatalogName(entry),searchText)||JAR_MenuMatches(entry->name,searchText)||
        (entry->kind==4&&JAR_MenuMatches(JAR_MPCatalogPickupClass(entry),searchText));
}
static void Cards(void){
 total=cardCount=0;
 for(unsigned i=0;i<JAR_MP_CATALOG_COUNT;++i)if(CardMatches(&jarMPCatalog[i]))++total;
 if(page<0)page=0;if(page>(total?total-1:0)/JAR_MENU_CARDS)page=(total?total-1:0)/JAR_MENU_CARDS;
 int at=0;for(unsigned order=0;order<JAR_MP_CATALOG_COUNT;++order){int i=cardOrder[order];if(CardMatches(&jarMPCatalog[i])){if(at++>=page*JAR_MENU_CARDS&&cardCount<JAR_MENU_CARDS)cards[cardCount++]=i;}}
}
static void SearchChanged(void){page=0;memset(pages,0,sizeof(pages));Cards();}
static void MenuCard(qboolean alternate){Cards();for(int i=0;i<cardCount;++i)if(Hit(JM_CARD,i)){if(!alternate||tab==2)trap->SendClientCommand(va("%s %u",JAR_CardEquip(tab,alternate)?"jar_equip":"jar_spawn",jarMPCatalog[cards[i]].id));return;}}
static qboolean SearchKey(int key){
 if(!searchEditing)return qfalse;
 if(key&K_CHAR_FLAG){int c=key&~K_CHAR_FLAG;size_t len=strlen(searchText);if(c>=32&&c<127&&len<sizeof(searchText)-1){searchText[len]=(char)c;searchText[len+1]=0;SearchChanged();}return qtrue;}
 if(key==A_BACKSPACE){size_t len=strlen(searchText);if(len){searchText[len-1]=0;SearchChanged();}return qtrue;}
 if(key==A_ENTER||key==A_KP_ENTER){SearchEditing(qfalse);return qtrue;}
 return key!=A_MOUSE1&&key!=A_MOUSE2?qtrue:qfalse;
}
static qboolean ToolInputKey(int key){
 if(!toolEditing)return qfalse;
 if(key&K_CHAR_FLAG){JAR_NumberAppend(toolInput,sizeof(toolInput),key&~K_CHAR_FLAG,1);return qtrue;}
 if(key==A_BACKSPACE){size_t n=strlen(toolInput);if(n)toolInput[n-1]=0;return qtrue;}
 if(key==A_ENTER||key==A_KP_ENTER){
  float value;
  if(!JAR_ParseFloat(toolInput,&value)){MenuStatus("Enter a valid number.");return qtrue;}
  trap->SendClientCommand(va("jar_setting %s_%.9g",toolEditing==4?"slack":toolEditing==2?"force":"rotation",value));
  toolEditing=0;return qtrue;
 }
 return qfalse;
}
static void MenuClick(void){
 toolEditing=0;
 if(Hit(JM_SEARCH,0)){SearchEditing(qtrue);return;}SearchEditing(qfalse);
 if(Hit(JM_CLEAR,0)){searchText[0]=0;SearchChanged();return;}
 for(int t=0;t<JAR_MENU_TABS;++t)if(Hit(JM_TAB,t)){pages[tab]=page;tab=t;page=pages[t];Cards();return;}
 for(int row=0;row<JAR_MENU_TOOLS+2;++row)if(Hit(JM_TOOL,row)){
  if(row<2)MenuWeapon(row?WP_TOOLGUN:WP_PHYSGUN);
  else if(jarMenuTools[row-2].mp>=0)SelectTool(jarMenuTools[row-2].mp);
  else MenuStatus("This tool's multiplayer entity controllers are still being ported.");return;
 }
 if(Hit(JM_UNDO,0)){CG_JAR_Undo();return;}if(Hit(JM_CLEANUP,0)){trap->SendClientCommand("jar_cleanup");return;}
 if(Hit(JM_PHYSICS,0)){MenuStatus("The host controls Box3D for the shared server world.");return;}
 if(tool==2&&Hit(JM_THRUST_DIRECTION,0)){trap->SendClientCommand(thrustReverse?"jar_setting thrust_in":"jar_setting thrust_out");return;}
 if((tool==2||tool==4||tool==8)&&Hit(JM_VALUE,0)){toolEditing=tool;toolInput[0]=0;return;}
 if(tool==12&&(Hit(JM_LESS,0)||Hit(JM_MORE,0))){trap->SendClientCommand(Hit(JM_MORE,0)?"jar_setting output_secondary":"jar_setting output_primary");return;}
 if((tool==4||tool==2)&&(Hit(JM_LESS,0)||Hit(JM_MORE,0))){trap->SendClientCommand(va("jar_setting %s_%s",tool==4?"slack":"force",Hit(JM_MORE,0)?"more":"less"));return;}
 Cards();if(Hit(JM_PREVIOUS,0)){if(page>0)--page;return;}if(Hit(JM_NEXT,0)){if((page+1)*JAR_MENU_CARDS<total)++page;return;}
 MenuCard(qfalse);
}
static void DrawSandbox(void){
 const int mode=JAR_MenuToolIndex(tool);const jarMenuTool_t*spec=&jarMenuTools[mode];const char*hover=NULL;
 Widget(JM_PANEL,0,NULL,qfalse);Widget(JM_TITLE,0,"SANDBOX",qfalse);Widget(JM_HOLD,0,menuPinned?"Typing unlocked / Esc to close":"Hold Q to browse / release to play",qfalse);
 for(int t=0;t<JAR_MENU_TABS;++t)Widget(JM_TAB,t,jarMenuTabs[t],tab==t);
 // Tail scrolling keeps the caret visible without rendering over the tools.
 size_t len=strlen(searchText);Widget(JM_SEARCH,0,va("Search: %s%s",searchText+(len>23?len-23:0),searchEditing?"_":""),searchEditing);Widget(JM_CLEAR,0,"X",qfalse);
 Widget(JM_TOOLS,0,"TOOLS",qfalse);
 Cards();for(int i=0;i<cardCount;++i){const jarMPEntry_t*entry=&jarMPCatalog[cards[i]];char label[64];JAR_MenuLabel(JAR_MPCatalogName(entry),label,sizeof(label));Widget(JM_CARD,i,label,Hit(JM_CARD,i));jarMenuRect_t r=JAR_MenuRect(JM_ICON,i);CG_DrawPic(r.x,r.y,r.w,r.h,trap->R_RegisterShaderNoMip(entry->icon));if(Hit(JM_CARD,i))hover=JAR_MPCatalogName(entry);}
 if(page>0)Widget(JM_PREVIOUS,0,"< Previous",Hit(JM_PREVIOUS,0));Widget(JM_PAGE,0,va("%d / %d (%d)",page+1,(total?total-1:0)/JAR_MENU_CARDS+1,total),qfalse);if((page+1)*JAR_MENU_CARDS<total)Widget(JM_NEXT,0,"Next >",Hit(JM_NEXT,0));
 for(int row=0;row<JAR_MENU_TOOLS+2;++row)Widget(JM_TOOL,row,row<2?(row?"Toolgun":"Physgun"):jarMenuTools[row-2].name,row==mode+2||Hit(JM_TOOL,row));
 Widget(JM_PROPERTIES,0,spec->name,qfalse);Widget(JM_DETAIL,0,va("Left: %s",spec->left),qfalse);Widget(JM_DETAIL,1,va("Right: %s",spec->right),qfalse);
 Widget(JM_DETAIL,2,mode==10?"Aim at a face to mount":mode==5?"Keeps the current relative pose":mode==6?"Pivot / axis: first aimed face":"Physgun: hold left to move",qfalse);
 Widget(JM_DETAIL,3,mode==10?"Direction applies to new mounts":mode==5||mode==6?"Frozen / movers / world: anchors":"Wheel: distance / Right: freeze",qfalse);
 Widget(JM_DETAIL,4,mode==10?"Visible jet points opposite thrust":mode==5||mode==6?"Player tools require consent":"Release left to fling",qfalse);
 if(tool==2||tool==4||tool==8){
  const char *label=tool==2?"Thrust (N)":tool==4?"Rope slack":"Rotation (degrees)";
  const float value=tool==2?thrustForce:tool==4?ropeSlack:rotationStep;
  Widget(JM_VALUE,0,toolEditing?va("%s: %s_",label,toolInput):va("%s: %.9g",label,value),Hit(JM_VALUE,0));
 }else if(tool==12)Widget(JM_VALUE,0,linkOutput==JAR_DISTANCE_SUBJECT?"Distance input: NPC":linkOutput==JAR_DISTANCE_REFERENCE?"Distance input: Reference":linkOutput?"Secondary output":"Primary output",qfalse);
 if(tool==4||tool==2||tool==12){Widget(JM_LESS,0,"-",Hit(JM_LESS,0));Widget(JM_MORE,0,"+",Hit(JM_MORE,0));}
 if(tool==2)Widget(JM_THRUST_DIRECTION,0,thrustReverse?JAR_THRUSTER_DIRECTION_OUT:JAR_THRUSTER_DIRECTION_IN,Hit(JM_THRUST_DIRECTION,0));
 Widget(JM_UNDO,0,"Undo (Z)",Hit(JM_UNDO,0));Widget(JM_CLEANUP,0,"Despawn All",Hit(JM_CLEANUP,0));Widget(JM_PHYSICS,0,menuPhysics?"Box3D: On":"Box3D: Off",Hit(JM_PHYSICS,0));
 Widget(JM_COUNT,0,va("Your spawns: %d | NPCs/vehicles: %d | Server props: %d / %d",menuOwned,menuActors,menuProps,JAR_MP_PROP_LIMIT),qfalse);
 if(hover)Widget(JM_STATUS,0,tab==2?va("%s | Left click: equip in hands | Right click: spawn on ground",hover):
  !strcmp(hover,"trigger_visible")?"trigger_visible | Fires Primary when seen. Activate to rearm.":
  !strcmp(hover,"trigger_entdist")?"trigger_entdist | Activate checks range once. C edits distance and links.":
  !strcmp(hover,"func_wall")?"func_wall | Activate toggles the wall. An off wall leaves a gizmo.":
  !strcmp(hover,"func_door")?"func_door | Activate opens or closes. C edits travel, direction and auto-close.":hover,qfalse);
 else Widget(JM_STATUS,0,menuStatusUntil>cg.time?menuStatus:"Choose a card to spawn. Hold Q to browse / release to play.",qfalse);
 DrawCursor();
}
